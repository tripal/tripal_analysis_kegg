<?php

$feature  = $variables['node']->feature;
$results = $feature->tripal_analysis_kegg->results;
$results_v0_3 = $feature->tripal_analysis_kegg->results_v0_3;

// don't show this block on the page if there are no KEGG results
if ($results[KO] or $results[PATH] or $results[MODULE] or $results_v0_3) {  

  // The way KEGG results are stored in the database has changed. Now the results
  // array contains two fields: 'KO' for kegg orthologs and 'PATH' for kegg 
  // pathways. Previously only orthologs were stored.  However, we want the
  // template to be backwards compatible with the old way, so we check the
  // $results array for the 'KO' key and handle the results differently
  if (count($results_v0_3)>0) {
    $results = $feature->tripal_analysis_kegg->results_v0_3;
    if ($feature->cvname != 'gene' and count($results) > 0) { 
        $i = 0;
        foreach ($results as $analysis_id => $analysisprops) { 
          $analysis = $analysisprops['analysis'];
          $terms = $analysisprops['terms']; 
          ?>
          <div id="tripal_feature-kegg_results_<?php print $i?>-box" class="tripal_feature_kegg-box tripal-info-box">
             <div class="tripal_feature-info-box-title tripal-info-box-title">KEGG Report <?php print preg_replace("/^(\d+-\d+-\d+) .*/","$1",$analysis->timeexecuted); ?></div>
             <div class="tripal_feature-info-box-desc tripal-info-box-desc"><?php 
                 if($analysis->nid){ ?>
                    Analysis name: <a href="<?php print url('node/'.$analysis->nid) ?>"><?php print $analysis->name?></a><?php
                 } else { ?>
                    Analysis name: <?php print $analysis->name;
                 } ?><br>
                 Date Performed: <?php print preg_replace("/^(\d+-\d+-\d+) .*/","$1",$analysis->timeexecuted); ?>
             </div>
  
          <div class="tripal_feature-kegg_results_subtitle">Annotated Terms</div>
          <table id="tripal_feature-kegg_summary-<?php $i ?>-table" class="tripal_analysis_kegg-summary-table tripal-table tripal-table-horz">
          <?php 
          $j=0;
          foreach ($terms as $term) { 
            $ipr_id = $term[0];
            $ipr_name = $term[1];
            $class = 'tripal_feature-table-odd-row tripal-table-odd-row';
            if($j % 2 == 0 ){
              $class = 'tripal_feature-table-even-row tripal-table-even-row';
            }?>
            <tr class="<?php print $class ?>">
              <td><?php print $term ?></td>
            </tr>
            <?php
            $j++;
          } ?>
          </table>     
          </div> <?php
          $i++;
        } // end for each analysis 
     } // end if
  }
  else {
     $pathways = $results['PATH'];
     $orthologs = $results['KO'];
     $modules = $results['MODULE'];
     if (!is_array($pathways)) {
        $pathways = array($pathways);
     }
     if (!is_array($orthologs)) {
        $orthologs = array($orthologs);
     }
     if (!is_array($modules)) {
        $modules = array($modules);
     }
     $i = 0; ?>
     <div id="tripal_feature-kegg-box" class="tripal_feature-box tripal-info-box">
       <div class="tripal_feature-info-box-title tripal-info-box-title">KEGG Terms</div>
       <div class="tripal_feature-info-box-desc tripal-info-box-desc"></div>
       <div class="tripal_feature-kegg_results_subtitle">Assigned KEGG Pathways</div><?php
       if($results['PATH']){
          $header = array('KEGG Pathway','Name');
          $rows = array();
          foreach($pathways as $term){ 
             $urlprefix = $term->cvterm_id->dbxref_id->db_id->urlprefix;
             $accession = $term->cvterm_id->dbxref_id->accession;
             $cvname = $term->cvterm_id->name;
             if($urlprefix){
                $accession = "<a href=\"$urlprefix$accession\" target=\"_blank\">$accession</a>";
             }
             $rows[] = array(
                $accession,
                $cvname
             );
          }
          print theme('table', $header, $rows); 
       } 
       else { 
          print "<div class=\"tripal-no-results\">There are no KEGG pathways for this feature</div>";
       } ?>
       <br><br>
       <div class="tripal_feature-kegg_results_subtitle">Assigned KEGG Orthologs</div><?php
       if($results['KO']){
          $header = array('KEGG Ortholog','Name');
          $rows = array();
          foreach($orthologs as $term){ 
            // add in the definition (it's a text column);
            $urlprefix = $term->cvterm_id->dbxref_id->db_id->urlprefix;
            $accession = $term->cvterm_id->dbxref_id->accession;
            $cvname = $term->cvterm_id->name;
            if($urlprefix){
               $accession = "<a href=\"$urlprefix$accession\" target=\"_blank\">$accession</a>";
            }
            $rows[] = array(
               $accession,
               $cvname
            );
          }
          print theme('table', $header, $rows); 
       } 
       else { 
          print "<div class=\"tripal-no-results\">There are no KEGG orthologs for this feature</div>";
       } ?>
       <br><br>
       <div class="tripal_feature-kegg_results_subtitle">Assigned KEGG Modules</div><?php
       if($results['MODULE']){
          $header = array('KEGG Module','Name');
          $rows = array();
          foreach($modules as $term){ 
            // add in the definition (it's a text column);
            $urlprefix = $term->cvterm_id->dbxref_id->db_id->urlprefix;
            $accession = $term->cvterm_id->dbxref_id->accession;
            $cvname = $term->cvterm_id->name;
            if($urlprefix){
               $accession = "<a href=\"$urlprefix$accession\" target=\"_blank\">$accession</a>";
            }
            $rows[] = array(
               $accession,
               $cvname
            );
          }
          print theme('table', $header, $rows); 
       } 
       else { 
          print "<div class=\"tripal-no-results\">There are no KEGG modules for this feature</div>";
       } ?>
    </div> <?php 
  } 
}
?>
