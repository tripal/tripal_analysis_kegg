USAGE:

	json_to_obo is a command line php script that takes multiple input files.

	the script is primarily designed to convert a .json file into a .obo file readable by tripal's obo loader.

	to run json_to_obo, run the following command in your terminal:

	php json_to_obo <file1.json> <file2.json> <file3.json> 

	the .json files given will all be converted into one .obo file under the name kegg.obo

	NOTE: if a kegg.obo already exists in the same directory, it will be overwritten!
