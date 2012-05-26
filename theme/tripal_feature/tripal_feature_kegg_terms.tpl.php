<?php

$feature  = $variables['node']->feature;
$results = $feature->tripal_analysis_kegg->results;

// The way KEGG results are stored in the database has changed. Now the results
// array contains two fields: 'KO' for kegg orthologs and 'PATH' for kegg 
// pathways. Previously only orthologs were stored.  However, we want the
// template to be backwards compatible with the old way, so we check the
// $results array for the 'KO' key and handle the results differently
if(!array_key_exists('KO',$results)){
  include('tripal_feature_kegg_terms_0.3.tpl_php');
}
else {
   $pathways = $results['PATH'];
   $orthologs = $results['KO'];
   if(!is_array($pathways)){
      $pathways = array($pathways);
   }
   if(!is_array($orthologs)){
      $orthologs = array($orthologs);
   }
   $i = 0;
   ?>
   <div id="tripal_feature-kegg_results_<?php print $i?>-box" class="tripal_analysis_kegg-box tripal-info-box">
      <div class="tripal_feature-info-box-title tripal-info-box-title">KEGG Assignments</div>
      <div class="tripal_feature-info-box-desc tripal-info-box-desc">
         <div class="tripal_feature-kegg_results_subtitle"></div>           
            <strong>Assigned KEGG Pathways</strong>
            <?php
            $header = array('KEGG Pathway','Name');
            $rows = array();
            foreach($pathways as $prop){ 
              $urlprefix = $prop->type_id->dbxref_id->db_id->urlprefix;
              $accession = $prop->type_id->dbxref_id->accession;
              $cvname = $prop->type_id->name;
              if($urlprefix){
                 $accession = "<a href=\"$urlprefix$accession\" target=\"_blank\">$accession</a>";
              }
              $rows[] = array(
                 $accession,
                 $cvname
              );
            }
            print theme('table', $header, $rows); 
            ?>
            <strong>Assigned KEGG Orthologs</strong>
            <?php
            $header = array('KEGG Ortholog','Name');
            $rows = array();
            foreach($orthologs as $prop){ 
              $urlprefix = $prop->type_id->dbxref_id->db_id->urlprefix;
              $accession = $prop->type_id->dbxref_id->accession;
              $definition = $prop->type_id->definition;
              if($urlprefix){
                 $accession = "<a href=\"$urlprefix$accession\" target=\"_blank\">$accession</a>";
              }
              $rows[] = array(
                 $accession,
                 $definition
              );
            }
            print theme('table', $header, $rows); 
            ?>
         </div>
      </div>
   </div>
<?php } ?>
