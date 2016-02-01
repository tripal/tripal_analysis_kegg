<?php 
/**
 *
 *
 * @ingroup tripal_analysis_kegg
 */
function tripal_analysis_kegg_preprocess_tripal_analysis_kegg_report(&$variables) {
    $analysis = $variables['node']->analysis;
    $report = tripal_analysis_kegg_full_report($analysis->analysis_id);
    
    if (!is_object($analysis->tripal_analysis_kegg)) {
      $analysis->tripal_analysis_kegg = new stdClass;  
    }
    $analysis->tripal_analysis_kegg->kegg_report = $report;
}
/**
 *
 * @ingroup tripal_analysis_kegg
 */
function tripal_analysis_kegg_preprocess_tripal_organism_kegg_summary(&$variables) {
  $node = $variables['node'];
  $organism = $node->organism;
  if (!property_exists($organism, 'tripal_analysis_kegg') || !is_object($organism->tripal_analysis_kegg)){
    $organism->tripal_analysis_kegg = new stdClass;  
  }
  $organism->tripal_analysis_kegg->select_form = tripal_analysis_kegg_load_organism_kegg_summary($node);
}

/**
 * 
 * @param $node
 * 
 */
function tripal_analysis_kegg_load_organism_kegg_summary($node) {
  $organism = $node->organism;

  // find analyses that have KEGG terms
  $sql = "
    SELECT analysis_id
    FROM {kegg_by_organism} KBO
    WHERE organism_id = :organism_id
    ORDER BY analysis_id DESC
  ";

  $results = chado_query($sql, array(':organism_id' => $organism->organism_id));


  # check to ensure we have access to at least one of the KEGG analyses
  $has_results = 0;
  while ($analysis = $results->fetchObject()) {
    $nid = chado_get_nid_from_id('analysis', $analysis->analysis_id);
    $anode = node_load($nid);
    if (node_access("view", $anode)) {
      $has_results = 1;
    }
  }
  return array(
    'has_results' => $has_results,
    'form' => drupal_get_form('tripal_analysis_kegg_select_form', $node),
  );
}

/**
*
* @param $form
* @param $form_state
* @param $node
*
*/
function tripal_analysis_kegg_select_form($form, $form_state, $node) {

  $form = array();
  
  // find analyses that have KEGG terms and their organisms.  this is kept
  // in the kegg_by_organism materialized view
  $sql = "
    SELECT *
    FROM {kegg_by_organism} KBO
    WHERE organism_id = :organism_id
    ORDER BY analysis_id DESC
  ";
  $results = chado_query($sql, array(':organism_id' => $node->organism->organism_id));
  
  // now build the list of analyses to use in the drop down element
  $analyses = array();
  $analyses[''] = '';
  while ($analysis = $results->fetchObject()) {
    # check to see if the user has permission to see this analysis
    $nid = chado_get_nid_from_id('analysis', $analysis->analysis_id);
    $anode = node_load($nid);
    if (node_access("view", $anode)) {
      $analyses[$analysis->analysis_id] = "$analysis->analysis_name";
    }
  }

  // now generate the select box
  $form['tripal_analysis_kegg_select'] = array(
    '#title' => t('Select a KEGG report to view'),
    '#description' => t('Any analysis with KEGG results related to this organism are available for viewing. For further information, see the analysis information page.'),
    '#type'  => 'select',
    '#options' => $analyses,
    '#ajax' => array(
      'callback' => "tripal_analysis_kegg_org_report",
      'wrapper' => 'tripal-analyis-kegg-report',
      'effect'   => 'fade',
      'method'   => 'replace',
    ),
    '#suffix' => '<div id="tripal-analyis-kegg-report"></div>'
  );
  return $form;
}

/**
 * 
 * @param $analysis_id
 */
function tripal_analysis_kegg_org_report($form, $form_state) {
  
  $analysis_id = $form_state['values']['tripal_analysis_kegg_select'];
  $content = tripal_analysis_kegg_full_report($analysis_id);
  return '<div id="tripal-analyis-kegg-report">' . $content . '</div>';
}
/**
 *
 * @param  $analysis_id
 *
 */
function tripal_analysis_kegg_full_report($analysis_id) {
  $has_results = 0;
   
  // The cvterm for KEGG Brite heirarchies has changed, therefore, we
  // have two queries, one for the new method and one for backwards
  // compatiblity.  The data, however, is still stored in the same way.
  // the SQL below will pull both forms:
  $sql = "
    SELECT CVT.name, CVT.cvterm_id
    FROM {cvterm} CVT
      INNER JOIN {analysisprop} AP ON CVT.cvterm_id = AP.type_id
      INNER JOIN {cv} CV on CVT.cv_id = CV.cv_id
    WHERE AP.analysis_id = :analysis_id
      AND (CVT.definition LIKE 'KEGG BRITE term: %' or CV.name = 'KEGG_BRITE')
    ORDER BY CVT.cvterm_id
  ";
  $result = chado_query($sql, array(':analysis_id' => $analysis_id));

  if ($result->fetchObject()) {
    return $content = tripal_analysis_kegg_brite($analysis_id, 0, 0);
  }
   
  return "<i>Note:</i> No results available.";
}

/*******************************************************************************
 */
function tripal_analysis_kegg_brite($analysis_id, $type_id = 0, $ajax, $is_v0_3=0) {
  
  drupal_add_library('system', 'drupal.ajax');
  
  $headers = array(
    'KEGG BRITE Heirarchy Name',
    'Heirarchy Terms'
  );
  
  $rows = array();

  // List all BRITE terms on the left. The SQL statement below can use
  // the older method for CVTerms with BRITE hierarchies or the newer method
  $sql = "
    SELECT DISTINCT CVT.name, CVT.cvterm_id
    FROM {cvterm} CVT
      INNER JOIN {analysisprop} AP ON CVT.cvterm_id = AP.type_id
      INNER JOIN {cv} CV on CV.cv_id = CVT.cv_id
    WHERE AP.analysis_id = :analysis_id AND
      (CVT.definition LIKE 'KEGG BRITE term: %' or CV.name = 'KEGG_BRITE')
    ORDER BY CVT.cvterm_id";

  $result = chado_query($sql, array(':analysis_id' => $analysis_id));

  $brite_list = '<ul>';
  while ($brite_term = $result->fetchObject()) {
    $link = l($brite_term->name, "tripal_analysis_kegg/report/nojs/$analysis_id/$brite_term->cvterm_id",
        array('attributes' => array('class' => 'use-ajax', 'onclick' => 'javascript:void(0);')));
    $brite_list .= '<li>' . $link . '</li>';
  }
  $brite_list .= '</ul>';

  // If called by ajax, generate tree structure
  $rows[] = array(
    array(
      'data' => $brite_list,
      'valign' => 'top',
      'width' => '25%',
    ),
    array(
      'data' => '<div id="heirarchy_terms_list">' . t('<i>Note:</i> Click a BRITE heirarchy name to browse term assignments.') . '<div>',
      'valign' => 'top',
    ),
  );
  
  // the $table array contains the headers and rows array as well as other
  // options for controlling the display of the table.  Additional
  // documentation can be found here:
  // https://api.drupal.org/api/drupal/includes%21theme.inc/function/theme_table/7
  $table = array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal_analysis_kegg-table-report',
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => '',
  );
  
  $content = theme_table($table);
  $content .= "<div id='ajax-response-goes-here'></div>";
  return $content;

}
/**
 * 
 */
function tripal_analysis_kegg_brite_ajax_update($analysis_id, $cvterm_id) {
 // Get BRITE term from cvterm table

  $sql = 'SELECT name FROM {cvterm} WHERE cvterm_id = :cvterm_id';
  $brite_term = chado_query($sql, array(':cvterm_id' => $cvterm_id))->fetchField();

  // Get BRITE hierarchy tree for this analysis
  $sql = "
    SELECT value
    FROM {analysisprop} AP
      INNER JOIN {CVterm} CVT on AP.type_id = CVT.cvterm_id
      INNER JOIN {CV} on CVT.cv_id = CV.cv_id
    WHERE (CV.name = 'tripal' or CV.name = 'KEGG_BRITE') and CVT.name = :name
     AND AP.analysis_id = :analysis_id
  ";
  $result = chado_query($sql, array(':name' => $brite_term, 'analysis_id' => $analysis_id))->fetchObject();

  $content = "<div id=\"heirarchy_terms_list\"><b>$brite_term</b>" . $result->value . '</div>';
  
  $commands = array();
  $commands[] = ajax_command_replace('#heirarchy_terms_list', $content);
  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

