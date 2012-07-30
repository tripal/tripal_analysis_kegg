<?php

/********************************************************************************
 */
function tripal_analysis_kegg_parseHierFile ($analysis_id, $hierfile, $base_path, 
   $query_re,$query_type,$query_uniquename,$job_id) {
 
 	// Prepare log
	$filename = preg_replace("/.*\/(.*)/", "$1", $hierfile);
	$logfile = file_directory_path() . "/tripal/tripal_analysis_kegg/load_$filename.log";
   $logfile = tempnam(sys_get_temp_dir(),"tripal_analysis_kegg_import");
	$log = fopen($logfile, 'a'); // append parsing results to log file
   if(!$log){
     print "ERROR: cannot open log file: $logfile\n";
     exit;
   }
 
   // If user input a file (e.g. hier.tar.gz), decompress it first
   if (is_file($hierfile)) {
      // generate a unique directory name for extracting and parsing the file
      $data_dir = sys_get_temp_dir() . "/" . uniqid();
      mkdir($data_dir);
      $stderr = shell_exec("cd $data_dir; tar -zxf $hierfile;");
      print "$stderr\n";
      // now get the uncompressed directory name.  There should only be one
      // other directory in our temp data directory so get that
      $dhandle = @opendir($data_dir);
      while ($dir = readdir($dhandle)) {
         if(strcmp($dir,'.')==0 or strcmp($dir,'..')==0){
            continue;
         } 
         $hierdir = $data_dir . "/$dir";
      }      
   } 
   // Otherwise, treat it as a directory
   else {
      $hierdir = $hierfile;
   }
   $dir_handle = @opendir($hierdir) or die("Unable to open unpacked KEGG heir directory: $hierdir\n");
   $total_files = count(glob($hierdir . '/*.*'));
   fwrite($log,"There are $total_files keg file(s).\n");
   $interval = intval($total_files * 0.01);
   if($interval < 1){
      $interval = 1;
   }
   $no_file = 0;

   // Remove the analysis features for this analysis
   // we will rebuild them from just this parsing
   $select =  array('analysis_id' => $analysis_id);
   if(!tripal_core_chado_delete('analysisfeature',$select)){
      print "ERROR: Unable to clear previous results.\n";
      exit;
   }
   
   // remove the analysis BRITE_TERMS from the analysisprop table
   $sql = "DELETE FROM analysisprop
           WHERE analysis_id = %d and
                 type_id IN (SELECT cvterm_id 
                            FROM cvterm CVT INNER JOIN cv CV on CVT.cv_id = CV.cv_id
                            WHERE CV.name='KEGG_BRITE')";
   $previous_db = tripal_db_set_active('chado'); // Use chado database
   $success = db_query($sql,$analysis_id);
   tripal_db_set_active($previous_db); // Use drupal database   
   if(!$success){
      print "ERROR: Unable to clear previous analysisprop results.\n";
      exit;
   }  

   // iterate through the files in the directory
   while ($file = readdir($dir_handle)) {

      if(preg_match("/^.*\.keg/",$file)){
         // Update the progress
         if ($no_file % $interval == 0) {
            $percentage = (int) ($no_file / $total_files * 100);
            tripal_job_set_progress($job_id, $percentage);
            print $percentage."%\r";
         }
         $no_file ++;
         
         # $heirarchy variable will be set in tripal_analysis_kegg_parse_kegg_file()
         $results = tripal_analysis_kegg_parse_kegg_file("$hierdir/$file",$heirarchy,
            $analysis_id, $base_path, $query_re,$query_type,$query_uniquename,$log);

         # add the item to the database
         if(count($results) > 0){
            print "\nSaving results for '$heirarchy'\n";
          

         	// Get type_id for the BRITE term            	
         	$previous_db = tripal_db_set_active('chado'); // Use chado database
         	$sql = "SELECT cvterm_id 
                    FROM {cvterm} CVT 
                    INNER JOIN CV ON CVT.cv_id = CV.cv_id
                    WHERE CV.name = 'KEGG_BRITE' AND CVT.name = '%s'";
         	$brite_cvterm_id = db_result(db_query($sql, $heirarchy)); 
            if(!$brite_cvterm_id){
               print "ERROR: Cannot find cvterm for '$heirarchy'.\n";
               exit; 
            }

            // KEGG results are returned in an array format, however
            // to make display a bit easier on the web using CSS, the array
            // will be reformated within a <ul> HTML heirarchy and stored
            // in the analysisprop table. 
            $i = 0;
            $content = '<ul>';
            tripal_analysis_kegg_array2list($results,$content,$i);
            $content .= '</ul>';
            
            // Replace all single quote as HTML code before insert
         	$content = preg_replace("/\'/", "&#39;", $content);
         	
         	// Insert the value
            $sql = "INSERT INTO {analysisprop} (analysis_id, type_id, value) 
                    VALUES (%d, %d,'%s')";   
            
            if(!db_query($sql, $analysis_id, $brite_cvterm_id,$content)){
               print "ERROR: Cannot add '$heirarchy' results to the database.\n";
               exit;
            }
            tripal_db_set_active($previous_db); // Use drupal database

            
         }
      }
   }

   tripal_job_set_progress($job_id,100);
   closedir($dir_handle);

   // If user input a file, remove decompressed files after parsing
   if (is_file($hierfile)) {
      $stderr = shell_exec("rm -r $data_dir;");
      print "$stderr\n";
   }
   
   print "Done.\nSuccessful and failed entries have been saved in the log file:\n $logfile\n";
	fwrite($log, "\n");
	fclose($log);
   return;
}
/**
*
*/
function tripal_analysis_kegg_array2list($array,&$content,&$i){
   foreach($array as $index => $item){
      if(is_array($item)){
         if(is_numeric($index)){
            tripal_analysis_kegg_array2list($item,$content,$i);
         } else {
            $content .= "<li id=\"term_$i\"><a></a>$index\n<ul>";
            $i++;
            tripal_analysis_kegg_array2list($item,$content,$i);
            $content .= "</ul>\n</li>\n";
         }
      } else {
         $content .= "<li id=\"term_$i\"><a></a>$item</li>\n";
         $i++;
      }
   }   
}
/**
 * Parse *.keg files. 
 * Each file has a definition line. BRITE term is extracted * from this line 
 * and added to chado as a cvterm. Tree structure for this cvterm is then 
 * generated and saved to analysisfeature and analysisfeatureprop tables.
 */
function tripal_analysis_kegg_parse_kegg_file ($file, &$heirarchy, $analysis_id, 
   $base_path, $query_re, $query_type, $query_uniquename,$log)
{

   print "Parsing $file\n";
   fwrite($log,"Parsing $file\n");   

   $filepos = 0;

   // iterate through the lines of the file and recurse through the various levels
   $handle = fopen($file,'r');
   $heirarchy = '';
   $heirarchy_id = '';
   while($line = fgets($handle)){
      $filepos += strlen($line);
      $line = trim($line);

      // the .keg files represtend the KEGG BRITE Functional heirarchies.  These
      // heirarchies begin with a ko
      if(preg_match("/#.*nbsp;\s(.*)<\/h2>$/",$line,$matches)){
         $heirarchy = $matches[1];
      }
      // KEGG BRITE heirarchy terms have a prefix of 'ko' and are found in the
      // header portion of each file.
      if(preg_match("/#ENTRY\s+(ko.*)$/",$line,$matches)){
         $heirarchy_id = $matches[1];
      }
      // once we have the term ID and name we can parse the rest of the file
      if($heirarchy and $heirarchy_id){
         // there is a mistake in the q01007.keg file which incorrectly lists the
         // BRITE Id as ko01006.  We'll correct it here
         if($heirarchy_id == 'ko01006' and $heirarchy == 'Amino acid related enzymes'){
            $heirarchy_id = 'ko01007';
         }
         
         // add the cvterm
         $brite_term = array(
            'id' => "$heirarchy_id",
            'name' => $heirarchy,
            'def' => $heirarchy,
            'namespace' => 'KEGG_BRITE',
            'is_obsolete' => 0,
         );
         fwrite($log,"Adding term: '$heirarchy' ($heirarchy_id)\n");
         $cvterm = tripal_cv_add_cvterm($brite_term,'KEGG_BRITE',0,0,'KEGG_BRITE');
         if(!$cvterm){
            fwrite($log,"Failed.  Cannot add cvterm '$heirarchy'\n");
            return;
         }

         // now that we have the file type we can recurse
         $next = tripal_analysis_kegg_get_next_line($handle,$filepos,$log);
         $results = tripal_analysis_kegg_recurse_heir($handle,$next,$query_re,
            $query_type,$query_uniquename,$base_path,$analysis_id,$heirarchy,
            $filepos,$log);
      }
   }
   fclose($handle);
   return $results;
}

/**
*
*/
function tripal_analysis_kegg_recurse_heir($handle, $line,$query_re,
   $query_type, $query_uniquename, $base_path, $analysis_id, 
   $heirarchy,&$filepos,$log)
{
   $results = array(); 

   // get the current level and the value
   $level = $line[0];
   $value = $line[1];
   $pathway = $line[2];

   // now get the next line to see what is coming next.  If the 
   // next level is greater then recurse immediately. 
   $prevpos = $filepos;
   while($next = tripal_analysis_kegg_get_next_line($handle,$filepos,$log)){
      $next_level = $next[0];
      $next_value = $next[1];
      $next_pathway = $next[2];
      print ".";
      
      // if we have a pathway from our current line and our next line does 
      // not then that means we are traveling down the heirarcical tree in the 
      // q00001.keg file. We need to make sure the pathway gets set for 
      // children of the pathway
      if(count($next_pathway) == 0 and count($pathway) > 0){
         $next_pathway['id'] = $pathway['id'];  
         $next_pathway['name'] = $pathway['name'];  
         $next_pathway['type_id'] = $pathway['type_id'];    
         $next[2] = $next_pathway;       
      } 

      // check this line to see if it has a feature we need to keep
      $ret = tripal_analysis_kegg_check_line_handle_feature($query_re,
         $query_type, $query_uniquename, $base_path, $analysis_id, 
         $heirarchy,$value,$log,$pathway);
      if($ret){
         $results[] = $ret;
      }

      // if we're going up a level then recurse immediately and add results to our array
      if(ord($next_level) > ord($level)){
         // now recurse
         $ret = tripal_analysis_kegg_recurse_heir($handle,$next, 
            $query_re,$query_type, $query_uniquename, $base_path, $analysis_id,
            $heirarchy,$filepos,$log);
         if(count($ret) > 0){
            $results[][$value] = $ret; 
         }
      }

      // if we go down a level on the next line then reset the 
      // filepointer and return
      elseif(ord($next_level) < ord($level)){
          fseek($handle,$prevpos);
          $filepos = $prevpos;
          return $results;
      }
      else {
         $line = $next;
         $level = $next[0];
         $value = $next[1];
      }
      $prevpos = $filepos; 
   }

   return $results;
}
/**
*
*/

function tripal_analysis_kegg_get_next_line($handle,&$filepos,$log){
   $good = 0;
   $level = '';
   $value = '';
   $pathway = array();

   // get the next line in the file
   $line = fgets($handle);
   $filepos += strlen($line);

   // we hit the end of the file, so exit with a null
   if(!$line){
      return null;
   }  
   while(!$good){
      $line = trim($line);
      preg_match("/^([ABCDEFGHIJKLMNOP])\s*(.*)/",$line,$matches);
      $level = $matches[1];
      $value = $matches[2];

      // skip lines that aren't data or are empty
      if($level and $value) {

         // check to see if this is a line identifying a KEGG pathway
         if(preg_match("/^(\d+)\s+(.*?)\s+\[PATH:.*$/",$value,$pathmatch) > 0){
            $pathway['id'] = $pathmatch[1];
            $pathway['name'] = $pathmatch[2];  
            
            // add the pathway as a cvterm
            $pathway_term = array(
               'id' => $pathway['id'],
               'name' => $pathway['name'],
               'def' => $pathway['name'],
               'namespace' => 'KEGG_PATHWAY',
               'is_obsolete' => 0,
            );  
            $cvterm = tripal_cv_add_cvterm($pathway_term,'KEGG_PATHWAY',0,0,'KEGG_PATHWAY');
            if(!$cvterm){
               fwrite($log,"Failure to add KEGG PATHWAY term to cvterm table: ".$pathway['id'] ." " . $pathway['name'] .".\n");
            } else {
               $pathway_id = $cvterm->cvterm_id;
               $pathway['type_id'] = $pathway_id;
            }     
         }

         // change all relative paths to absolute paths pointing to KEGG (www.genome.jp)
         // add id to <a> tags so we can link kegg.gif to it in tripal_analysis_kegg.css
         $value = preg_replace("/<a href=\"\//i","<a href=\"http://www.genome.jp/",$value);     
         $value = preg_replace("/<a href=\"/i","<a id=\"tripal_kegg_brite_links\" target=\"_blank\" href=\"",$value);

         // this line is good so let's exit out
         $good = 1;
      } else {
         $line = fgets($handle);
         $filepos += strlen($line);
         // we hit the end of the file, so exit with a null
         if(!$line){
            return null;
         }         
      }
   }
   return array($level,$value,$pathway);
}
/**
*
*/
function tripal_analysis_kegg_check_line_handle_feature($query_re,
   $query_type, $query_uniquename, $base_path, $analysis_id, 
   $heirarchy,$value,$log,$pathway)
{

   // extract the features that have been mapped to the KEGG IDs
   if(preg_match("/^(.*?);\s*(\<a.+)/",$value,$matches)){
      $has_feature = 1;
      $fname = $matches[1];
      $keggterm = $matches[2];
      
      // get the KEGG ortholog and add it as a term
      $ko_type_id = '';
      $ko_name = '';
      $ko_id = '';

      if(preg_match("/^<a.*>(K.*?)<\/a>\s+(.*)$/",$keggterm,$matchparts)){
         $ko_id = $matchparts[1];
         $ko_name = $matchparts[2];
         $ko_term = array(
            'id' => $ko_id,
            'name' => $ko_id,
            'def' => $ko_name,
            'namespace' => 'KEGG_ORTHOLOGY',
            'is_obsolete' => 0,
         );  
         $cvterm = tripal_cv_add_cvterm($ko_term,'KEGG_ORTHOLOGY',0,0,'KEGG_ORTHOLOGY');
         if(!$cvterm){
            fwrite($log,"Failure to add KEGG ORTHOLOGY term to cvterm table: $ko_id $ko_name. Cannot add term to feature.\n");
            return;
         } else {
            $ko_type_id = $cvterm->cvterm_id;
         }
      }
      
		// get the feature name using the user's regular expression
		if ($query_re and preg_match("/$query_re/", $fname, $matches)) {
			$feature = $matches[1];
		} 
      // If not in above format then pull up to the first space
      else {
			if (preg_match('/^(.*?)\s.*$/', $fname, $matches)) {
				$feature = $matches[1];
         } 
         // if no match up to the first space then just use the entire string
         else {
            $feature = $fname;
         }
      } 

      // now find the feature in chado
      $select = array();
      if($query_uniquename){
         $select['uniquename'] = $feature;
      } else {
         $select['name'] = $feature;
      }
      if($query_type){
         $select['type_id'] = array(
           'cv_id' => array(
              'name' => 'sequence'
           ),
           'name' => $query_type,
         );
      }

      $feature_arr = tripal_core_chado_select('feature',array('feature_id'),$select);

      if(count($feature_arr) > 1){
		   fwrite($log,"Ambiguous: '$feature' matches more than one feature and is being skipped.\n");
			return;
      }
      if(count($feature_arr) == 0){
			fwrite($log,"Failed: '$feature' cannot find a matching feature in the databasef.  RE: $query_re; LINE: $fname\n");
         return;
      }
      $feature_id = $feature_arr[0]->feature_id;

      // if we match with a feature then add in all the properties.
      if($feature_id){
         
         // add the analysisfeature record if it doesn't already exist.
         $columns = array('analysisfeature_id');
         $values = array(
            'feature_id' => $feature_id,
            'analysis_id' => $analysis_id
         );
         $analysisfeature_arr = tripal_core_chado_select('analysisfeature',$columns,$values);
         if(count($analysisfeature_arr) == 0){
            if(tripal_core_chado_insert('analysisfeature',$values)){
               $analysisfeature_arr = tripal_core_chado_select('analysisfeature',
                  array('analysisfeature_id'),$values);
            } 
            else {
               fwrite($log,"Failed: Cannot add analysis feature.  Feature Id: $feature_id\n");
               return 0;
            }
         }
         $analysisfeature_id = $analysisfeature_arr[0]->analysisfeature_id;

         // add this term to the analysis feature properties
         //print "Adding KEGG term '$ko_id' from $heirarchy to $feature\n";
         fwrite($log,"Adding KEGG term '$ko_id' from $heirarchy to $feature. Term: '$ko_name'\n");                  

         tripal_feature_analysis_insert_property($analysis_id, $feature_id,
            NULL, $ko_id, $ko_name, 0, 'KEGG_ORTHOLOGY');
         
         // add it as a feature property
         tripal_feature_insert_property($feature_id,$ko_id, $ko_name, 0, 'KEGG_ORTHOLOGY');
            
         // associate this pathway with the feature
         if(count($pathway) > 0){
            //print "Adding KEGG term '".$pathway['name']."' from KEGG_PATHWAY to $feature\n";
            fwrite($log,"Adding KEGG term '".$pathway['name']."' from KEGG_PATHWAY to $feature\n"); 
            tripal_feature_analysis_insert_property($analysis_id, $feature_id,
               NULL, $pathway['name'], $pathway['name'], 0, 'KEGG_PATHWAY');
             
            // add it as a feature property
            tripal_feature_insert_property($feature_id,$pathway['name'], $pathway['name'], 0, 'KEGG_PATHWAY');
         }

         // get the node ID of the feature if one exists
         $sql = "SELECT nid FROM {chado_feature} WHERE feature_id = %d";
         $nid = db_result(db_query($sql, $feature_id)); 

         // Add link to each matched feature
         if($nid){
            $value = preg_replace("/^(.*?)(;\s*\<a)/","<a id=\"tripal_kegg_feature_links\" target=\"_blank\" href=\"$base_path/node/$nid\">"."$1"."</a>"."$2",$value);
         }
         // if we have a feature match then add this to our results array
         return $value;  
      }       
   }
   return null;
}
