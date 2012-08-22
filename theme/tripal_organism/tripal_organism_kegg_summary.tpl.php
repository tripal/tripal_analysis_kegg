<?php
$organism = $node->organism;
$form = $organism->tripal_analysis_kegg->select_form['form'];
$has_results = $organism->tripal_analysis_kegg->select_form['has_results'];  

if ($has_results) { ?>
  <div id="tripal_organism-kegg_summary-box" class="tripal_organism-info-box tripal-info-box">
    <div  class="tripal_organism-info-box-title tripal-info-box-title">KEGG Analysis Reports</div> 
    <?php print $form; ?>
    <div id="tripal_analysis_kegg_org_report"></div>
    <div id="tripal_ajaxLoading" style="display:none">
      <div id="loadingText">Loading...</div>
    </div>   
  </div> <?php
} 
else {
  // show a message to the site administrator instructing how to enable 
  // a KEGG report.  Otherwise, if the user is not an administrator and 
  // there is no content then nothing get's shown.
  if (user_access('access administration pages')) { ?>
    <div id="tripal_organism-kegg_summary-box" class="tripal_organism-info-box tripal-info-box">
      <div  class="tripal_organism-info-box-title tripal-info-box-title">KEGG Analysis Reports</div> 
      <div class="tripal-no-results">
        There are no KEGG reports available
        <p><br>Administrators, to view a KEGG report you must:
        <ul>
          <li>Create a <a href="<?php print url('node/add/chado-analysis-kegg'); ?>" target="_blank">KEGG analysis page</a>.</li>
          <li>Populate the <a href="<?php print url('admin/tripal/mviews'); ?>" target="_blank">kegg_by_organism</a> materialized view</li>
          <li>Ensure the user <a href="<?php print url('admin/user/permissions'); ?>"> has permission</a> to view the KEGG analysis content</li>
          <li>Refresh this page</li>
        </ul> 
        </p>
        This page will not appear to site visitors unless the KEGG data is present. 
      </div>
    </div><?php 
  }
}





