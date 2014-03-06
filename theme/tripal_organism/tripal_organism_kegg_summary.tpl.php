<?php
$organism = $node->organism;
$form = $organism->tripal_analysis_kegg->select_form['form'];
$has_results = $organism->tripal_analysis_kegg->select_form['has_results'];

if($has_results) {
  print drupal_render($form); ?>
  <div id="tripal_analysis_kegg_org_report"></div>
  </div> <?php
}
  
print tripal_set_message("Administrators, if there are no KEGG reports available, you must:
    <ul>
      <li>Create a <a href=\"" . url('node/add/chado-analysis-kegg') . "\" target=\"_blank\">KEGG analysis page</a>.</li>
      <li>Populate the <a href=\"" . url('admin/tripal/schema/mviews'); . "\" target=\"_blank\">kegg_by_organism</a> materialized view</li>
      <li>Ensure the user <a href=\"" . url('admin/people/permissions'). "\"> has permission</a> to view the KEGG analysis content</li>
      <li>Refresh this page</li>
    </ul> 
    </p>
    This page will not appear to site visitors unless the KEGG data is present.", TRIPAL_INFO, array('return_html' => 1)); 





