
## Tripal Analysis KEGG

This module allows you to import KEGG annotations created by the KEGG KOALA annotation tool into a Tripal v3 site. 

### Annotating data

To annotate your features with KEGG terms, use the [BlastKOALA tool](http://www.kegg.jp/blastkoala/).


### Loading data
The loader expects a tab delimited file with the first column being your feature names and the second column the KEGG IDs.  This is the the file available if you Download the annotation data from the BlastKOALA report page.


## OBO builder

The KEGG Ontology is not available as an OBO publicly.  We therefore build a rudimentary OBO file ourselves from the JSON files available online.

### OBO builder instructions

The OBO builder is available in `kegg_OBO_builder/json_to_obo.php`.  Simply run `json_to_obo.php [input files]`.
