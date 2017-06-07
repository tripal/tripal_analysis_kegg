<?php

class KeggImporter extends TripalImporter {

 /**
   * The name of this loader.  This name will be presented to the site
   * user.
   */
  public static $name = 'Chado KEGG Hier results loader';

  /**
   * The machine name for this loader. This name will be used to construct
   * the URL for the loader.
   */
  public static $machine_name = 'kegg_loader';

  /**
   * A brief description for this loader.  This description will be
   * presented to the site user.
   */
  public static $description = 'Import a KEGG Hier file into Chado';

  /**
   * An array containing the extensions of allowed file types.
   */
  public static $file_types = array('keg');


  /**
   * Provides information to the user about the file upload.  Typically this
   * may include a description of the file types allowed.
   */
  public static $upload_description = 'Please provide the Hier file.';

  /**
   * The title that should appear above the upload button.
   */
  public static $upload_title = 'Hier File';
  /**
   * @see TripalImporter::form()
   */
  public function form($form, &$form_state) {
    // set the fedaults
    $query_re = '';
    $query_type = '';
    $query_uniquename = '';
    $organism_id = NULL;

    // if we are re constructing the form from a failed validation or ajax callback
    // then use the $form_state['values'] values
    if (array_key_exists('values', $form_state)) {
        $query_re           = $form_state['values']['query_re'];
        $query_type         = $form_state['values']['query_type'];
        $query_uniquename   = $form_state['values']['query_uniquename'];
        $organism_id = $form_state['values']['organism_id'];
    }

    $form['query_re'] = array(
        '#title' => t('Query Name RE'),
        '#type' => 'textfield',
        '#description' => t('Enter the regular expression that will extract the '.
            'feature name from the query line in the interpro results. This option '.
            'is only required when the query does not identically match a feature '.
            'in the database. For example: ^.*id=(.*?).*$'),
        '#default_value' => $query_re,
    );

    $form['query_uniquename'] = array(
        '#title' => t('Use Unique Name'),
        '#type' => 'checkbox',
        '#description' => t('Select this checkbox if the query name in the results file matches the unique name of the feature. '),
        '#default_value' => $query_uniquename,
    );

    $cv = tripal_get_cv(array('name' => 'sequence'));
    $cv_id = $cv->cv_id;
    $form['query_type'] = array(
        '#title' => t('Query Type'),
        '#type' => 'textfield',
        '#description' => t('Please enter the Sequence Ontology term (e.g. contig, polypeptide, mRNA) that describes '.
            'the query sequences in the InterProScan XML results file(s).  This is only necessary if two '.
            'or more sequences have the same name.'),
        '#default_value' => $query_type,
        '#autocomplete_path' => "admin/tripal/storage/chado/auto_name/cvterm/$cv_id",
    );

    $sql = "
    SELECT organism_id, genus, species
    FROM {organism}
    ORDER BY genus,species
  ";
    $orgs = chado_query($sql);
    $organisms = array();
    $organisms[] = '';
    while($organism = $orgs->fetchObject()) {
      $organisms[$organism->organism_id] = $organism->genus . " " . $organism->species;
    }
    $form['kegg']['organism_id'] = array(
      '#type' => 'select',
      '#title' => t('Organism'),
      '#description' => t('Select the organism for these KEGG results. The
        feature names in the KAAS .hier files must exist for the specified organism.'),
      '#options' => $organisms,
      '#default_value' => $organism_id
    );
    return $form;
  }
  /**
   * @see TripalImporter::formSubmit()
   */
  public function formSubmit($form, &$form_state) {
    global $user;



    return '';
  }
  /**
   * @see TripalImporter::formValidate()
   */
  public function formValidate($form, &$form_state) {
    // trim character fields
    $form_state['values']['query_uniquename']  = trim($form_state['values']['query_uniquename']);
    $form_state['values']['query_type']        = trim($form_state['values']['query_type']);
    $form_state['values']['query_re']          = trim($form_state['values']['query_re']);


    // check the regular expression to make sure it is valid
    set_error_handler(function() {}, E_WARNING);
    $result = preg_match("/" . $form_state['values']['query_re'] . "/", null);
    restore_error_handler();
    if ($result === FALSE) {
        form_set_error('query_re', 'Invalid regular expression.');
    }
  }

  /**
   * @see TripalImporter::run()
   */
  public function run() {
    $arguments = $this->arguments['run_args'];
    $analysis_id = $arguments['analysis_id'];
    $hierfile= trim($this->arguments['file']['file_path']);
    $query_re = $arguments['query_re'];
    $query_type = $arguments['query_type'];
    $query_uniquename =$arguments['query_uniquename'];
    $organism_id = $arguments['organism_id'];

    $this->parseHierFile($analysis_id, $hierfile, $base_path,
      $query_re, $query_type, $query_uniquename, $organism_id, $job_id);
  }


  /**
   * @file
   * Parser and loader for KEGG/KAAS analysis result
   */

  function parseHierFile($analysis_id, $hierfile, $base_path,
      $query_re, $query_type, $query_uniquename, $organism_id, $job_id) {

        $transaction = db_transaction();
        try {

          // If user input a file (e.g. hier.tar.gz), decompress it first
          if (is_file($hierfile)) {
            $this->logMessage(t("Unpacking heir file...\n"));
            // generate a unique directory name for extracting and parsing the file
            $data_dir = sys_get_temp_dir() . "/" . uniqid();
            mkdir($data_dir);
            $stderr = shell_exec('cd  ' . escapeshellarg($data_dir) . '; tar -zxf ' . escapeshellarg($hierfile));
            if ($stderr) {
              watchdog('trp_kegg',"ERROR: %err", array('%err' => $stderr), 'error');
              exit;
            }
            // now get the uncompressed directory name.  There should only be one
            // other directory in our temp data directory so get that
            $dhandle = @opendir($data_dir);
            while ($dir = readdir($dhandle)) {
              if (strcmp($dir, '.')==0 or strcmp($dir, '..')==0) {
                continue;
              }
              $hierdir = $data_dir . "/$dir";
            }
          }
          // Otherwise, treat it as a directory
          else {
            $hierdir = $hierfile;
          }
          $this->logMessage(t("Counting Files...\n"));
          $dir_handle = @opendir($hierdir) or die("Unable to open unpacked KEGG heir directory: $hierdir\n");
          $total_files = count(glob($hierdir . '/*.keg'));


          // Remove the analysis features for this analysis
          // we will rebuild them from just this parsing
          $select =  array('analysis_id' => $analysis_id);
          if (!chado_delete_record('analysisfeature', $select)) {
            watchdog('trp_kegg',"ERROR: Unable to clear previous results", array(), 'error');
            exit;
          }

          // remove the analysis BRITE_TERMS from the analysisprop table
          $this->logMessage(t("Preparing database...\n"));
          $sql = "
      DELETE FROM {analysisprop}
      WHERE analysis_id = :analysis_id and
        type_id IN (
          SELECT cvterm_id
          FROM {cvterm} CVT
            INNER JOIN {cv} CV on CVT.cv_id = CV.cv_id
          WHERE CV.name='KEGG_BRITE')";
          $success = chado_query($sql, array(':analysis_id' => $analysis_id));
          if (!$success) {
            watchdog('trp_kegg',"ERROR: Unable to clear previous analysisprop results.", array(), 'error');
            exit;
          }

          // iterate through the files in the directory
          while ($file = readdir($dir_handle)) {

            if (preg_match("/^.*\.keg/", $file)) {
              $no_file++;

              # $hierarchy variable will be set in $this->parseKeggFile()
              $results = $this->parseKeggFile("$hierdir/$file", $hierarchy,
                  $analysis_id, $organism_id, $base_path, $query_re, $query_type,
                  $query_uniquename, $no_file, $total_files);

              # add the item to the database
              if (count($results) > 0) {

                // Get type_id for the BRITE term
                $values = array(
                  'cv_id' => array(
                    'name' => 'KEGG_BRITE'
                  ),
                  'name' => $hierarchy,
                );
                $options = array('statement_name' => 'sel_cvterm_cvna');
                $brite_term = chado_select_record('cvterm', array('cvterm_id'), $values, $options);
                if (!$brite_term or count($brite_term) == 0) {
                  watchdog('trp_kegg', "ERROR: Cannot find hierarchy cvterm: '%heirarchy'",
                      array('%heirarchy' => $hierarchy), 'error');
                  exit;
                }
                $brite_cvterm_id = $brite_term[0]->cvterm_id;

                // KEGG results are returned in an array format, however
                // to make display a bit easier on the web using CSS, the array
                // will be reformated within a <ul> HTML heirarchy and stored
                // in the analysisprop table.
                $content = '<ul>';
                $this->arrayToList($results, $content, $i);
                $content .= '</ul>';

                // Replace all single quote as HTML code before insert
                $content = preg_replace("/\'/", "&#39;", $content);

                // Insert the value
                $values = array(
                  'analysis_id' => $analysis_id,
                  'type_id' => $brite_cvterm_id,
                  'value' => $content,
                );
                $options = array('statement_name' => 'ins_analysisprop_antyva');
                $success = chado_insert_record('analysisprop', $values, $options);
                if (!$success) {
                  watchdog('trp_kegg',"ERROR: Cannot add '%heirarchy' results to the database.",
                      array('%heirarchy' => $hierarchy), 'error');
                  exit;
                }
              }
            }
          }

          closedir($dir_handle);

          // If user input a file, remove decompressed files after parsing
          if (is_file($hierfile)) {
            $stderr = shell_exec('rm -r ' . escapeshellarg($data_dir));
            if($stderr) {
              watchdog('trp_kegg', "ERROR: %err", array('%err' => $stderr), 'error');
            }
          }
        }
        catch (Exception $e) {
          $this->logMessage(t("\n")); // make sure we start errors on new line
          watchdog_exception('trp_kegg', $e);
          $transaction->rollback();
          $this->logMessage(t("FAILED: Rolling back database changes...\n"));
        }
        $this->logMessage(t("\nDone\n"));
        return;
  }
  /**
   *
   */
  function arrayToList($array, &$content, &$i) {
    foreach ($array as $index => $item) {
      if (is_array($item)) {
        if (is_numeric($index)) {
          $this->arrayToList($item, $content, $i);
        }
        else {
          $content .= "<li id=\"term_$i\"><a></a>$index\n<ul>";
          $i++;
          $this->arrayToList($item, $content, $i);
          $content .= "</ul>\n</li>\n";
        }
      }
      else {
        $content .= "<li id=\"term_$i\"><a></a>$item</li>\n";
        $i++;
      }
    }
  }
  /**
   * Parse *.keg files.
   * Each file has a definition line. BRITE term is extracted * from this line
   * and added to chado as a cvterm. Tree structure for this cvterm is then
   * generated and saved to analysisfeature and analysisfeatureprop tables.
   */
  function parseKeggFile($file, &$hierarchy, $analysis_id, $organism_id,
      $base_path, $query_re, $query_type, $query_uniquename, $no_file, $total_files) {

        $filepos = 0;

        // iterate through the lines of the file and recurse through the various levels
        $handle = fopen($file, 'r');
        $hierarchy = '';
        $hierarchy_id = '';
        $filesize = filesize($file);
        $intv_read = 0;
        $matches = array();
        $filename = preg_replace('/^.*\/(.*)$/','\1', $file);

        while ($line = fgets($handle)) {
          $filepos += strlen($line);
          $intv_read += strlen($line);
          $line = trim($line);

          $this->parseHeirUpdateJob($filepos, $filesize, $intv_read, $no_file, $total_files, $filename);

          // the .keg files represtend the KEGG BRITE Functional heirarchies.  These
          // heirarchies begin with a ko
          if (preg_match("/#.*nbsp;\s(.*)<\/h2>$/", $line, $matches)) {
            $hierarchy = $matches[1];
          }
          // KEGG BRITE heirarchy terms have a prefix of 'ko' and are found in the
          // header portion of each file.
          if (preg_match("/#ENTRY\s+(ko.*)$/", $line, $matches)) {
            $hierarchy_id = $matches[1];
          }
          // once we have the term ID and name we can parse the rest of the file
          if ($hierarchy and $hierarchy_id) {
            // there is a mistake in the q01007.keg file which incorrectly lists the
            // BRITE Id as ko01006.  We'll correct it here
            if ($hierarchy_id == 'ko01006' and $hierarchy == 'Amino acid related enzymes') {
              $hierarchy_id = 'ko01007';
            }

            // add the cvterm
            $brite_term =

            $cvterm = tripal_insert_cvterm(array(
              'id' => "$hierarchy_id",
              'name' => $hierarchy,
              'def' => $hierarchy,
              'namespace' => 'KEGG_BRITE',
              'is_obsolete' => 0,
              'cv_name' => 'KEGG_BRITE',
              'db_name' => 'KEGG_BRITE'
            ));
            if (!$cvterm) {
              watchdog('trp_kegg', "Failed.  Cannot add cvterm '%heirarchy'.",
                  array('%heirarchy', $hierarchy), 'error');
              return;
            }

            // now that we have the file type we can recurse
            $next = $this->getNextLine($handle, $filepos, $filesize,
                $intv_read, $no_file, $total_files, $filename);

            $results = $this->recurseHeir($handle, $next, $query_re,
                $query_type, $query_uniquename, $base_path, $analysis_id, $organism_id, $hierarchy,
                $filepos, $filesize, $intv_read, $no_file, $total_files, $filename);
          }
        }
        fclose($handle);
        return $results;
  }

  /**
   *
   */
  function recurseHeir($handle, $line, $query_re,
      $query_type, $query_uniquename, $base_path, $analysis_id, $organism_id,
      $hierarchy, &$filepos, $filesize, &$intv_read, $no_file, $total_files, $file) {
        $results = array();

        // get the current level and the value
        $level = $line[0];
        $value = $line[1];
        $pathway = $line[2];

        // now get the next line to see what is coming next.  If the
        // next level is greater then recurse immediately.
        $prevpos = $filepos;
        while ($next = $this->getNextLine($handle, $filepos, $filesize, $intv_read, $no_file, $total_files, $file)) {
          $next_level = $next[0];
          $next_value = $next[1];
          $next_pathway = $next[2];

          // if we have a pathway from our current line and our next line does
          // not then that means we are traveling down the heirarcical tree in the
          // q00001.keg file. We need to make sure the pathway gets set for
          // children of the pathway
          if (count($next_pathway) == 0 and count($pathway) > 0) {
            $next_pathway['id'] = $pathway['id'];
            $next_pathway['name'] = $pathway['name'];
            $next_pathway['type_id'] = $pathway['type_id'];
            $next[2] = $next_pathway;
          }

          // check this line to see if it has a feature we need to keep
          $ret = $this->checkLineHandleFeature($query_re,
              $query_type, $query_uniquename, $base_path, $analysis_id, $organism_id,
              $hierarchy, $value, $pathway);
          if ($ret) {
            $results[] = $ret;
          }

          // if we're going up a level then recurse immediately and add results to our array
          if (ord($next_level) > ord($level)) {
            // now recurse
            $ret = $this->recurseHeir($handle, $next,
                $query_re, $query_type, $query_uniquename, $base_path, $analysis_id, $organism_id,
                $hierarchy, $filepos, $filesize, $intv_read, $no_file, $total_files, $file);
            if (count($ret) > 0) {
              $results[][$value] = $ret;
            }
          }

          // if we go down a level on the next line then reset the
          // filepointer and return
          elseif (ord($next_level) < ord($level)) {
            fseek($handle, $prevpos);
            $filepos = $prevpos;
            return $results;
          }
          else {
            $line = $next;
            $level = $next[0];
            $value = $next[1];
          }
          $prevpos = $filepos;
        }

        return $results;
  }
  /**
   *
   */

  function getNextLine($handle, &$filepos, $filesize, &$intv_read, $no_file, $total_files, $filename) {
    $good = 0;
    $level = '';
    $value = '';
    $pathway = array();

    // get the next line in the file
    $line = fgets($handle);
    $filepos += strlen($line);
    $intv_read += strlen($line);
    $this->parseHeirUpdateJob($filepos, $filesize, $intv_read, $no_file, $total_files, $filename);

    // we hit the end of the file, so exit with a NULL
    if (!$line) {
      return NULL;
    }
    while (!$good) {
      $line = trim($line);
      preg_match("/^([ABCDEFGHIJKLMNOP])\s*(.*)/", $line, $matches);
      $level = $matches[1];
      $value = $matches[2];

      // skip lines that aren't data or are empty
      if ($level and $value) {

        // check to see if this is a line identifying a KEGG pathway
        //print "$value\n";
        if (preg_match("/^(\d+)\s+(.*?)\s+\[PATH:.*>(.*)<\/a>]$/", $value, $pathmatch) > 0) {
          $pathway['id'] = $pathmatch[3];
          $pathway['name'] = $pathmatch[2];
          $pathway['def'] = $pathmatch[2];

          // add the pathway as a cvterm
          $pathway_term = array(
            'id' => $pathway['id'],
            'name' => $pathway['name'],
            'def' => $pathway['def'],
            'namespace' => 'KEGG_PATHWAY',
            'is_obsolete' => 0,
            'cv_name' => 'KEGG_PATHWAY',
            'db_name' => 'KEGG_PATHWAY',
          );
          $cvterm = tripal_insert_cvterm($pathway_term);
          if (!$cvterm) {
            watchdog('trp_kegg', "ERROR: Failure to add KEGG PATHWAY term to cvterm table: %table %name.",
                array('%table' => $pathway['id'], '%name' => $pathway['name']), 'error');
          }
          else {
            $pathway_id = $cvterm->cvterm_id;
            $pathway['type_id'] = $pathway_id;
          }
        }

        // change all relative paths to absolute paths pointing to KEGG (www.genome.jp)
        // add id to <a> tags so we can link kegg.gif to it in tripal_analysis_kegg.css
        $value = preg_replace("/<a href=\"\//i", "<a href=\"http://www.genome.jp/", $value);
        $value = preg_replace("/<a href=\"/i", "<a id=\"tripal_kegg_brite_links\" target=\"_blank\" href=\"", $value);

        // this line is good so let's exit out
        $good = 1;
      }
      else {
        $line = fgets($handle);
        $filepos += strlen($line);
        $intv_read += strlen($line);
        $this->parseHeirUpdateJob($filepos, $filesize, $intv_read, $no_file, $total_files, $filename);
        // we hit the end of the file, so exit with a NULL
        if (!$line) {
          return NULL;
        }
      }
    }
    return array($level, $value, $pathway);
  }
  /**
   *
   */
  function checkLineHandleFeature($query_re,
      $query_type, $query_uniquename, $base_path, $analysis_id, $organism_id,
      $hierarchy, $value, $pathway) {

        // extract the features that have been mapped to the KEGG IDs
        if (preg_match("/^(.*?);\s*(\<a.+)/", $value, $matches)) {
          $has_feature = 1;
          $fname = $matches[1];
          $keggterm = $matches[2];

          // get the KEGG ortholog and add it as a term
          $ko_type_id = '';
          $ko_def = '';
          $ko_id = '';
          $ko_cvname = '';
          $ko_dbname = '';

          if (preg_match("/^<a.*>(K.*?)<\/a>\s+(.*)$/", $keggterm, $matchparts)) {
            $ko_id = $matchparts[1];
            $ko_def = $matchparts[2];
            $ko_name = $ko_def;
            $ko_cvname = 'KEGG_ORTHOLOGY';
            $ko_dbname = 'KEGG_ORTHOLOGY';
            $ko_term = array(
              'id' => $ko_id,
              'name' => $ko_name,
              'def' => $ko_def,
              'namespace' => $ko_cvname,
              'is_obsolete' => 0,
              'cv_name' => $ko_cvname,
              'db_name' => $ko_dbname,
            );
            $cvterm = tripal_insert_cvterm($ko_term);

            if (!$cvterm) {
              watchdog('trp_kegg', "Failure to add KEGG ORTHOLOGY term to cvterm table: %ko_id %ko_def. Cannot add term to feature",
                  array('%ko_id' => $ko_id, '%ko_def' => $ko_def), WATCHDOG_ERROR);
              return;
            }
            else {
              $ko_type_id = $cvterm->cvterm_id;
            }
          }
          if (preg_match("/^<a.*>(M.*?)<\/a>\s+(.*)$/", $keggterm, $matchparts)) {
            $ko_id = $matchparts[1];
            $ko_def = $matchparts[2];
            $ko_name = $ko_def;
            $ko_cvname = 'KEGG_MODULE';
            $ko_dbname = 'KEGG_MODULE';
            $ko_term = array(
              'id' => $ko_id,
              'name' => $ko_name,
              'def' => $ko_def,
              'namespace' => $ko_cvname,
              'is_obsolete' => 0,
              'cv_name' => $ko_cvname,
              'db_name' => $ko_dbname,
            );
            $cvterm = tripal_insert_cvterm($ko_term);
            if (!$cvterm) {
              watchdog('trp_kegg', "Failure to add KEGG MODULE term to cvterm table: %ko_id %ko_def. Cannot add term to feature",
                  array('%ko_id' => $ko_id, '%ko_def' => $ko_def), WATCHDOG_ERROR);
              return;
            }
            else {
              $ko_type_id = $cvterm->cvterm_id;
            }
          }


          // get the feature name using the user's regular expression
          if ($query_re and preg_match("/$query_re/", $fname, $matches)) {
            $feature = $matches[1];
          }
          // If not in above format then pull up to the first space
          else {
            if (preg_match('/^(.*?)\s.*$/', $fname, $matches)) {
              $feature = $matches[1];
            }
            // if no match up to the first space then just use the entire string
            else {
              $feature = $fname;
            }
          }

          // now find the feature in chado
          $select = array();
          if ($query_uniquename) {
            $select['uniquename'] = $feature;
          }
          else {
            $select['name'] = $feature;
          }
          if ($organism_id) {
            $select['organism_id'] = $organism_id;
          }
          if ($query_type) {
            $select['type_id'] = array(
              'cv_id' => array(
                'name' => 'sequence'
              ),
              'name' => $query_type,
            );
          }
          $feature_arr = chado_select_record('feature', array('feature_id'), $select);

          if (count($feature_arr) > 1) {
            watchdog('trp_kegg', "Failed (Ambiguous): '%feature' matches more than one feature and is being skipped.",
                array('%feature' => $feature), 'error');
            return;
          }
          if (count($feature_arr) == 0) {
            watchdog('trp_kegg', "Failed: '%feature' cannot find a matching feature in the databasef.  RE: %query_re; LINE: %fname.",
                array('%feature' => $feature, '%query_re' => $query_re, '%fname' => $fname), 'error');
            return;
          }
          $feature_id = $feature_arr[0]->feature_id;

          // if we match with a feature then add in all the properties.
          if ($feature_id) {

            // add the analysisfeature record if it doesn't already exist.
            $columns = array('analysisfeature_id');
            $values = array(
              'feature_id' => $feature_id,
              'analysis_id' => $analysis_id
            );
            $options = array('statement_name' => 'sel_analysisfeature_fean');
            $analysisfeature_arr = chado_select_record('analysisfeature', $columns, $values, $options);
            if (count($analysisfeature_arr) == 0) {
              if (chado_insert_record('analysisfeature', $values)) {
                $options = array('statement_name' => 'ins_analysisfeature_fean');
                $analysisfeature_arr = chado_select_record('analysisfeature',
                    array('analysisfeature_id'), $values, $options);
              }
              else {
                watchdog('trp_kegg', "Failed: Cannot add analysis feature.  Feature Id: %feature_id",
                    array('%feature_id' => $feature_id), 'error');
                return 0;
              }
            }
            $analysisfeature_id = $analysisfeature_arr[0]->analysisfeature_id;

            // add this term to the analysis feature properties
            $success = chado_insert_property(
                array('table' => 'analysisfeature', 'id' => $analysisfeature_id),
                array('type_name' => $ko_name, 'cv_name' => $ko_cvname, 'value' => $ko_def)
                );
            if (!$success) {
              watchdog('trp_kegg', "Failed: Could not add orthlog to feature.  Feature Id: %feature_id, Ortholog: '%ortholog'",
                  array('%feature_id' => $feature_id, '%ortholog' => $ko_id), 'warning');
            }

            // Insert term into the feature_cvterm table. if the cvterm already
            // exists then just skip adding it
            $values = array(
              'feature_id' => $feature_id,
              'cvterm_id' => $cvterm->cvterm_id,
              'pub_id' => array(
                'uniquename' => 'null',
              ),
            );
            $columns = array('*');
            $fcvterm = chado_select_record('feature_cvterm', $columns, $values);
            if (count($fcvterm) == 0) {
              $success = chado_insert_record('feature_cvterm', $values);
              if (!$success) {
                watchdog('tpr_kegg', "Failed: Cannot add cvterm: %name '%value'",
                    array('%name' => $cvterm->name, '%value' =>  $cvterm->def), WATCHDOG_ERROR);
                return;
              }
            }

            // associate this pathway with the feature
            if (count($pathway) > 0) {
              $success = chado_insert_property(
                  array('table' => 'analysisfeature', 'id' => $analysisfeature_id),
                  array('type_name' => $pathway['name'], 'cv_name' => 'KEGG_PATHWAY', 'value' => $pathway['def'])
                  );
              if (!$success) {
                watchdog('trp_kegg', "Failed: Could not add pathway to feature.  Feature Id: %feature_id, Pathway: '%pathway'",
                    array('%feature_id' => $feature_id, '%pathway' => $pathway['name']), 'warning');
              }

              // Insert pathway into the feature_cvterm table. if the cvterm already
              // exists then just skip adding it
              $values = array(
                'feature_id' => $feature_id,
                'cvterm_id' => $pathway['type_id'],
                'pub_id' => 1,
              );
              $columns = array('*');
              $fcvterm = chado_select_record('feature_cvterm', $columns, $values);
              if (count($fcvterm) == 0) {
                $success = chado_insert_record('feature_cvterm', $values);
                if (!$success) {
                  watchdog('tpr_kegg', "Failed: Cannot add cvterm: %name '%value'",
                      array('%name' => $cvterm->name, '%value' =>  $cvterm->def), WATCHDOG_ERROR);
                  return;
                }
              }
            }

            // Get the node ID of the feature if one exists if the legacy
            // module is enabled.
            if (module_exists('tripal_feature')) {
              $nid = chado_get_nid_from_id('feature', $feature_id);

              // Add link to each matched feature
              if ($nid) {
                // TODO: remove the domain from the base path to make the link relative
                $value = preg_replace("/^(.*?)(;\s*\<a)/", "<a id=\"tripal_kegg_feature_links\" target=\"_blank\" href=\"$base_path/node/$nid\">"."$1"."</a>"."$2", $value);
              }
            }
            // if we have a feature match then add this to our results array
            return $value;
          }
        }
        return NULL;
  }

  /*
   *
   */
  function parseHeirUpdateJob($filepos, $filesize, &$intv_read, $no_file, $total_files, $filename){
    $this->setTotalItems($total_files);
    $this->setItemsHandled($no_file);
  }
}
