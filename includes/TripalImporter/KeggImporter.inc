<?php

class KeggImporter extends TripalImporter {

  /**
   * The name of this loader.  This name will be presented to the site
   * user.
   */
  public static $name = 'Chado KEGG Loader';

  /**
   * The machine name for this loader. This name will be used to construct
   * the URL for the loader.
   */
  public static $machine_name = 'chado_kegg_loader';

  /**
   * A brief description for this loader.  This description will be
   * presented to the site user.
   */
  public static $description = 'Import a KEGG file into Chado';

  /**
   * An array containing the extensions of allowed file types.
   */
  public static $file_types = ['txt'];


  /**
   * Provides information to the user about the file upload.  Typically this
   * may include a description of the file types allowed.
   */
  public static $upload_description = 'Please provide the KEGG file.';

  /**
   * The title that should appear above the upload button.
   */
  public static $upload_title = 'KEGG File';

  /**
   * If the loader should require an analysis record.  To maintain provenance
   * we should always indiate where the data we are uploading comes from.
   * The method that Tripal attempts to use for this by associating upload files
   * with an analysis record.  The analysis record provides the details for
   * how the file was created or obtained. Set this to FALSE if the loader
   * should not require an analysis when loading. if $use_analysis is set to
   * true then the form values will have an 'analysis_id' key in the $form_state
   * array on submitted forms.
   */
  public static $use_analysis = TRUE;

  /**
   * If the $use_analysis value is set above then this value indicates if the
   * analysis should be required.
   */
  public static $require_analysis = TRUE;

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Import KEGG File';

  /**
   * Indicates the methods that the file uploader will support.
   */
  public static $methods = [
    // Allow the user to upload a file to the server.
    'file_upload' => TRUE,
    // Allow the user to provide the path on the Tripal server for the file.
    'file_local' => TRUE,
    // Allow the user to provide a remote URL for the file.
    'file_remote' => TRUE,
  ];

  /**
   * Indicates if the file must be provided.  An example when it may not be
   * necessary to require that the user provide a file for uploading if the
   * loader keeps track of previous files and makes those available for
   * selection.
   */
  public static $file_required = TRUE;


  /**
   * The array of arguments used for this loader.  Each argument should
   * be a separate array containing a machine_name, name, and description
   * keys.  This information is used to build the help text for the loader.
   */
  public static $argument_list = [];


  /**
   * Indicates how many files are allowed to be uploaded.  By default this is
   * set to allow only one file.  Change to any positive number. A value of
   * zero indicates an unlimited number of uploaded files are allowed.
   */
  public static $cardinality = 1;


  /**
   * Be default, all loaders are automaticlly added to the Admin >
   * Tripal > Data Laders menu.  However, if this loader should be
   * made available via a different menu path, then set it here.  If the
   * value is empty then the path will be the default.
   */
  public static $menu_path = '';

  public function form($form, &$form_state) {

    $query_re = '';
    $query_uniquename = '';
    $query_type = '';

    $form['query_re'] = [
      '#title' => t('Query Name RE'),
      '#type' => 'textfield',
      '#description' => t('Enter the regular expression that will extract the ' .
        'feature name from the query line in the interpro results. This option ' .
        'is only required when the query does not identically match a feature ' .
        'in the database. For example: ^.*id=(.*?).*$'),
      '#default_value' => $query_re,
    ];

    $form['query_uniquename'] = [
      '#title' => t('Use Unique Name'),
      '#type' => 'checkbox',
      '#description' => t('Select this checkbox if the query name in the results file matches the unique name of the feature.'),
      '#default_value' => $query_uniquename,
    ];

    $cv = tripal_get_cv(['name' => 'sequence']);
    $cv_id = $cv->cv_id;
    $form['query_type'] = [
      '#title' => t('Query Type'),
      '#type' => 'textfield',
      '#description' => t('Please enter the Sequence Ontology term (e.g. contig, polypeptide, mRNA) that describes ' .
        'the query sequences in the InterProScan XML results file(s).  This is only necessary if two ' .
        'or more sequences have the same name.'),
      '#default_value' => $query_type,
      '#autocomplete_path' => "admin/tripal/storage/chado/auto_name/cvterm/$cv_id",
    ];

    return $form;
  }

  /**
   * Handles submission of the form elements.
   *
   * The form elements provided in the implementation of the form() function
   * can be used for special submit if needed.
   */
  public function formSubmit($form, &$form_state) {

  }

  /**
   * Handles validation of the form elements.
   *
   * The form elements provided in the implementation of the form() function
   * should be validated using this function.
   */
  public function formValidate($form, &$form_state) {
    // trim character fields
    $form_state['values']['query_uniquename'] = trim($form_state['values']['query_uniquename']);
    $form_state['values']['query_type'] = trim($form_state['values']['query_type']);
    $form_state['values']['query_re'] = trim($form_state['values']['query_re']);
    // check the regular expression to make sure it is valid
    set_error_handler(function () {
    }, E_WARNING);
    $result = preg_match("/" . $form_state['values']['query_re'] . "/", NULL);
    restore_error_handler();
    if ($result === FALSE) {
      form_set_error('query_re', 'Invalid regular expression.');
    }
  }

  /**
   * Performs the import.
   */
  public function run() {

    $arguments = $this->arguments['run_args'];
    $kegg_file = trim($this->arguments['files'][0]['file_path']);
    $organism_id = $arguments['organism_id'];
    $query_re = $arguments['query_re'];
    $query_uniquename = $arguments['query_uniquename'];
    $query_type = $arguments['query_type'];

    $this->parse_kegg_file($kegg_file, $organism_id, $query_re,
      $query_uniquename, $query_type);
  }

  public function parse_kegg_file($kegg_file, $organism_id,
                                  $query_re, $query_uniquename, $query_type) {

    $terms = [];

    // open the file
    $handle = fopen($kegg_file, "r");

    // read the file

    while (($line = fgets($handle)) !== FALSE) {
      // the variables that will hold info on our term
      $def = "";
      $cvterm_id = "";

      // separate into feature name and kegg id
      $line = explode("\t", $line);
      $feature_name = $line[0];
      if (!isset($line[1])) {
        //KEGG returns features without annotations: skip them.
        continue;
      }
      $ko_id = $line[1];

      $ko_id = str_replace(["\r", "\n"], '', $ko_id);

      // find the definition from the database
      $sql = "SELECT CV.definition, CV.cvterm_id
      FROM {chado.cvterm} CV
      INNER JOIN {chado.dbxref} DB ON CV.dbxref_id = DB.dbxref_id
      WHERE DB.accession = :ko_id";

      $result = db_query($sql, [':ko_id' => $ko_id]);

      if (!$result) {
        watchdog('trp_kegg', "ERROR: Unable to find KEGG term:  '%ko_id'",
          ['%ko_id' => $ko_id], 'error');
        exit;
      }

      // if record exists, pull out the definition from the record
      foreach ($result as $record) {
        $def = $record->definition;
        $cvterm_id = $record->cvterm_id;
      }


      $term = [
        "feature_name" => $feature_name,
        "accession" => $ko_id,
        "definition" => $def,
        "cvterm_id" => $cvterm_id,
      ];

      // we're only worried about features that have a kegg term
      if ($ko_id) {
        $terms[] = $term;
      }
    }

    foreach ($terms as $term) {
      // get the feature name using the user's regular expression
      if ($query_re and preg_match("/$query_re/", $term['feature_name'], $matches)) {
        $feature = $matches[1];
      }
      // If not in above format then pull up to the first space
      else {
        if (preg_match('/^(.*?)\s.*$/', $term['feature_name'], $matches)) {
          $feature = $matches[1];
        }
        // if no match up to the first space then just use the entire string
        else {
          $feature = $term['feature_name'];
        }
      }

      // now find the feature in chado
      $select = [];
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
        $select['type_id'] = [
          'cv_id' => [
            'name' => 'sequence',
          ],
          'name' => $query_type,
        ];
      }

      $f_id = chado_select_record('feature', ['feature_id'], $select);

      foreach ($f_id as $id) {
      }
      $feature_id = $id->feature_id;

      // find the publication id with which to associate

      $sql = "SELECT P.pub_id
        FROM {chado.pub} P
        WHERE P.uniquename = 'null'";

      $result = db_query($sql);
      foreach ($result as $record) {
        $pub_id = $record->pub_id;
      }
      if (!$pub_id) {
        watchdog('trp_kegg', "ERROR: Unable to find publication ID", [], 'error');
        return;
      }

      // Ensure everything is valid before insertion
      if (!$term['cvterm_id']) {
        $this->LogMessage("Unable to find a Chado cvterm record for the KEGG term '!name'", ['!name' => $ko_id]);
        print ("No cvterm for $feature_name");
        continue;
      }
      if (!$feature_id) {
        $this->logMessage("Unable to identify the feature for  '!name'",
          ['!name' => $feature_name, TRIPAL_WARNING]);
        continue;

      }

      // Annotate the feature
      $sql = "INSERT INTO chado.feature_cvterm (cvterm_id, feature_id, pub_id) 
      VALUES (:cvterm_id, :feature_id, :pub_id)";

      chado_query($sql, [
        ':cvterm_id' => $term['cvterm_id'],
        ':feature_id' => $feature_id,
        ':pub_id' => $pub_id,
      ]);
    }
  }
}