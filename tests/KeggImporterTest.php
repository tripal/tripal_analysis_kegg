<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

class KeggImporterTest extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  public function testKeggImporterBasic() {
    $importer = $this->runKEGGImporterDevSeed();

    $query = db_select('chado.feature', 'f');
      $query->join('chado.feature_cvterm', 'fc', 'f.feature_id = fc.feature_id');
      $query->join('chado.cvterm', 'c', 'c.cvterm_id = fc.cvterm_id');
      $query->condition('f.name', 'FRAEX38873_v2_000000060.1');
      $query->fields('c', ['name']);
      $name = $query->execute()->fetchField();
    //mnat1 === name for kegg K10842.
    //see https://github.com/statonlab/tripal_dev_seed/blob/master/Fexcel_mini/kegg/f_excelsior_ko.txt
      $this->assertEquals('MNAT1', $name);

  }

  private function runKEGGImporterDevSeed() {

    $organism = factory('chado.organism')->create();

    //load sequences first
    $mRNA_file = ['file_remote' => 'https://raw.githubusercontent.com/statonlab/tripal_dev_seed/master/Fexcel_mini/sequences/mrna_mini.fasta'];
    $analysis = factory('chado.analysis')->create(['name' => 'KEGG_test_mrna']);
    $run_args = [
      'organism_id' => $organism->organism_id,
      'analysis_id' => $analysis->analysis_id,
      'seqtype' => 'mRNA',
      'method' => 2, //default insert and update
      'match_type' => 1, //unique name default
      //optional
      're_name' => NULL,
      're_uname' => NULL,
      're_accession' => NULL,
      'db_id' => NULL,
      'rel_type' => NULL,
      're_subject' => NULL,
      'parent_type' => NULL,
    ];

    module_load_include('inc', 'tripal_chado', 'includes/TripalImporter/FASTAImporter');

    $importer = new \FASTAImporter();
    $importer->create($run_args, $mRNA_file);
    $importer->prepareFiles();
    $importer->run();


    //FRAEX38873_v2_000000010.2

    $file = ['file_remote' => 'https://raw.githubusercontent.com/statonlab/tripal_dev_seed/master/Fexcel_mini/kegg/f_excelsior_ko.txt'];

    $analysis = factory('chado.analysis')->create(['name' => 'KEGG_test']);

    $run_args = [
      'analysis_id' => $analysis->analysis_id,
      //no other args should be necessary
    ];

    module_load_include('inc', 'tripal_analysis_kegg', 'includes/TripalImporter/KeggImporter');

    $importer = new \KeggImporter();
    $importer->create($run_args, $file);
    $importer->prepareFiles();
    $importer->run();
    return $importer;
  }

}
