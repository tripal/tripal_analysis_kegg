<?php

/**
 * Implements hook_bundle_create_fields().
 *
 * This is a Tripal defined hook that supports integration with the
 * TripalEntity field.
 */
function tripal_analysis_kegg_bundle_fields_info($entity_type, $bundle) {
  $info = array();
  
  $term = tripal_load_term_entity(array('term_id' => $bundle->term_id));
  $vocab = $term->vocab;
  if ($vocab->vocabulary == 'local' && $term->accession == 'kegg_results') {
    $field_name = 'local__kegg_results';
    $field_type = 'local__kegg_results';
    $info[$field_name] = array(
      'field_name' => $field_name,
      'type' => $field_type,
      'cardinality' => 1,
      'locked' => FALSE,
      'storage' => array(
        'type' => 'field_chado_storage'
      ),
    );
  }
  return $info;
}

/**
 * Impelments hook_create_tripalfield_instance().
 *
 * This is a Tripal defined hook that supports integration with the
 * TripalEntity field.
 */
function tripal_analysis_kegg_bundle_instances_info($entity_type, $bundle) {
  $info = array();
  
  $term = tripal_load_term_entity(array('term_id' => $bundle->term_id));
  $vocab = $term->vocab;

  if ($vocab->vocabulary == 'local' && $term->accession == 'kegg_results') {
    $field_name = 'local__kegg_results';
    $info[$field_name] = array(
      'field_name' => $field_name,
      'entity_type' => $entity_type,
      'bundle' => $bundle->name,
      'label' => 'kegg results',
      'description' => 'Results format for searches of the kegg database.',
      'required' => FALSE,
      'settings' => array(
        'auto_attach' => FALSE,
        'chado_table' => 'analysis_feature',
        'chado_column' => '',
        'base_table' => 'feature',
      ),
      'widget' => array(
        'type' => 'local__kegg_results_widget',
      ),
      'display' => array(
        'default' => array(
          'label' => 'inline',
          'type' => 'local__kegg_results_formatter',
        ),
      ),
    );
  }
  return $info;
}
