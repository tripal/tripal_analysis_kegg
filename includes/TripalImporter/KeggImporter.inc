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

    // get the list of analyses
    $sql = "SELECT * FROM {analysis} ORDER BY name";
    $org_rset = chado_query($sql);
    $analyses = array();
    $analyses[''] = '';
    while ($analysis = $org_rset->fetchObject()) {
        $analyses[$analysis->analysis_id] = "$analysis->name ($analysis->program $analysis->programversion, $analysis->sourcename)";
    }
    $form['analysis_id'] = array(
        '#title'       => t('Analysis'),
        '#type'        => t('select'),
        '#description' => t("Choose the analysis to which these features are associated.
        Why specify an analysis for a data load?  All data comes
        from some place, even if downloaded from Genbank. By specifying
        analysis details for all data imports it allows an end user to reproduce the
        data set, but at least indicates the source of the data."),
        '#required'    => TRUE,
        '#options'     => $analyses,
    );
    // if we are re constructing the form from a failed validation or ajax callback
    // then use the $form_state['values'] values
    if (array_key_exists('values', $form_state)) {
        $query_re           = $form_state['values']['query_re'];
        $query_type         = $form_state['values']['query_type'];
        $query_uniquename   = $form_state['values']['query_uniquename'];
        $organism_id = $form_state['values']['organism_id'];
    }
    // if we are re building the form from after submission (from ajax call) then
    // the values are in the $form_state['input'] array
    if (array_key_exists('input', $form_state) and !empty($form_state['input'])) {
        $query_re           = $form_state['input']['query_re'];
        $query_type         = $form_state['input']['query_type'];
        $query_uniquename   = $form_state['input']['query_uniquename'];
        $organism_id = $form_state['input']['organism_id'];
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
  public function run($details, $job_id = NULL) {
    $arguments = $details->arguments;
    $analysis_id = $arguments['analysis_id'];
    $hierfile= $arguments['file_path'];
    $query_re = $arguments['query_re'];
    $query_type = $arguments['query_type'];
    $query_uniquename =$arguments['query_uniquename'];
    $organism_id = $arguments['organism_id'];
    
    tripal_analysis_kegg_parseHierFile($analysis_id, $hierfile, $base_path,
      $query_re, $query_type, $query_uniquename, $organism_id, $job_id);
  }

}
