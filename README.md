
## Tripal Analysis KEGG

This module allows you to import KEGG annotations created by the KEGG KOALA annotation tool into a Tripal v3 site. 

### Loading the KEGG Vocabulary into Chado

There is no OBO file available for loading in to Tripal: this module creates one from the online KEGG API (see OBO builder below).

Before you can run the loader, you must import the OBO file.  To do so, visit `admin -> Tripal -> Data loaders -> Chado Vocabularies ->  OBO Vocabulary Loader`, select **KEGG** from the dropdown, and press the **Import OBO File** button.  Copy and execute the Drush command to run the import job.  For example (note your root might be different):

```bash
  drush trp-run-jobs --username=administrator --root=/var/www/html

```

### Annotating data

To annotate your features with KEGG terms, use the [BlastKOALA tool](http://www.kegg.jp/blastkoala/).


### Loading data
The loader expects a tab delimited file with the first column being your feature names and the second column the KEGG IDs.  This is the the file available if you Download the annotation data from the BlastKOALA report page.


## OBO builder

The KEGG Ontology is not available as an OBO publicly.  We therefore build a rudimentary OBO file ourselves from the JSON files available online.

### OBO builder instructions

The OBO builder is available in `kegg_OBO_builder/json_to_obo.php`.  Simply run `json_to_obo.php [input files]`.


