<?php
$analysis = $node->analysis;
$report = $analysis->tripal_analysis_kegg->kegg_report; ?>

<div class="tripal_analysis_kegg-info-box-desc tripal-info-box-desc"><?php print $analysis->name ?></div> <?php 
print $report ?>
