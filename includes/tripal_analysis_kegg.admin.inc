<?php
/**
 * Implementation of hook_settings_form for creating the 
 * administrative settings page
 * 
 * @param $form
 * @param $form_state
 * 
 * @return
 */
function tripal_analysis_kegg_settings_form($form, &$form_state) {
  $form = array();
  
  // Add fields for setting the node titles
  $details = array(
    'module' => 'tripal_analysis_kegg',       // the name of the MODULE implementing the content type
    'content_type' => 'chado_analysis_kegg',   // the name of the content type
    'options' => array(
      '[analysis.name]' => 'Analysis Name Only',
      '[analysis.name] ([analysis.sourcename]) [analysis.program] version [analysis.programversion]' => 'Unique Contraint: Includes the name, source and program name/version'
    ),
    'unique_option' => '[analysis.name] ([analysis.sourcename]) [analysis.program] version [analysis.programversion]'
  );
  chado_add_admin_form_set_title($form, $form_state, $details);
  
  return $form;
}
