<?php

class KEGGImporter extends TripalImporter
{
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
  public static $file_types = array('txt');


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
  public static $methods = array(
    // Allow the user to upload a file to the server.
    'file_upload' => TRUE,
    // Allow the user to provide the path on the Tripal server for the file.
    'file_local' => TRUE,
    // Allow the user to provide a remote URL for the file.
    'file_remote' => TRUE,
  );

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
  public static $argument_list = array();


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

  }

  /**
   * Performs the import.
   */
  public function run() {

    // fetch the kegg term that goes along with the id
    $brite_term = chado_select_record('cvterm', array('cvterm_id', 'definition'), [
      'cv_id' => [
        'name' => 'KEGG'
      ],
      'dbxref_id' => [
        'accession' => $ko_id
      ]
    ]);

    $arguments = $this->arguments['run_args'];
    $kegg_file = trim($this->arguments['files'][0]['file_path']);
    $organism_id = $arguments['organism_id'];

    $this->parse_kegg_file($arguments, $kegg_file, $organism_id);
  }

  public function parse_kegg_file($arguments, $kegg_file, $organism_id) {
    $feature_name = "";
    $ko_id = "";

    // open the file
    $handle = file($kegg_file, FILE_IGNORE_NEW_LINES);

    // read the file
    foreach($handle as $line) {
      // separate into feature name and kegg id
      $line = explode("  ", $line);
      $feature_name = $line[0];
      $ko_id = $line[1];
    }
    
    // fetch the kegg term that goes along with the id
    $brite_term = chado_select_record('cvterm', array('cvterm_id', 'definition'), [
      'cv_id' => [
        'name' => 'KEGG'
      ],
      'dbxref_id' => [
        'accession' => $ko_id
      ]
    ]);

    var_dump($brite_term);

    // annotate the feature with the kegg term

  }
}