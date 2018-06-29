<?php

class Term {
  public $name;
  public $id;
  public $description;
  public $parents = [];
}

if ($argc < 2) echo "Please specify input files for parsing.\n";
else {
  $terms = [];
  $db_name = 'KEGG:';
  $output_file = 'kegg.obo';
  $file_contents = '';

  // Iterate through file and get all terms
  foreach($argv as $file) {
    if ($file == $argv[0]) continue;
    $input = file_get_contents($file);
    $data = json_decode($input);
    $previous_objects[0] = $data;
    // Create first term
    $term = new Term();
    $term->id = $data->name;
    $term->name = $data->name;
    $terms[$term->id] = $term;

    tripal_analysis_kegg_get_terms($data->children, $terms, $previous_objects);
  }

  // Output headers
  $output = "format-version: 1.2\ndefault-namespace: kegg ontology\n\n";

  // Print out all the terms
  foreach ($terms as $term) {
    $output .= "[Term]\n";
    $output .= "id: $db_name$term->id\n";
    $output .= "name: $term->name\n";
    if ($term->description) $output .= "def: $term->description\n";
    foreach ($term->parents as $parent) {
      // Safety check
      if ($parent == $term->id) continue;
      $output .= "is_a: $db_name$parent\n";
    }
    $output .= "\n";
  }

  file_put_contents($output_file, $output);
}
/**
 * @param $children
 * @param $terms
 * @param $previous_objects
 *
 * @return mixed
 *
 *  Iterates through each object in the JSON file and returns an array of terms.
 */
function tripal_analysis_kegg_get_terms($children, &$terms, $previous_objects) {

  // We have reached the leaves here
  if (is_array($children))
  {
    foreach($children as $child)
    {
      tripal_analysis_kegg_get_terms($child, $terms, $previous_objects);
    }
  }

  // Add term and proceed one layer deeper
  if (is_object($children) && property_exists($children, "children")) {
    $term = new Term();
    $subtype = $previous_objects[0]->name;
    $separated_name = explode('  ', $children->name);
    $term->id = $separated_name[0] . " ($subtype)";
    $term->name = count($separated_name) > 1 ? $separated_name[1] : $term->id;

    if (isset($terms[$term->id])) {
      if (!in_array(end($previous_objects), $terms[$term->id]->parents))
        $terms[$term->id]->parents[] = tripal_analysis_kegg_get_ID(end($previous_objects)->name, $subtype, end($previous_objects)->name);
    } else {
      $term->parents[] = tripal_analysis_kegg_get_ID(end($previous_objects)->name, $subtype, end($previous_objects)->name);
      $terms[$term->id] = $term;
    }

    $previous_objects[] = $children;

    return tripal_analysis_kegg_get_terms($children->children, $terms, $previous_objects);
  }

  if (is_object($children)) {
    $term = new Term();
    $access_name = explode('  ', $children->name);
    $term->id = $access_name[0];
    if (count($access_name) > 1) {
      $desc_name = explode('; ', $access_name[1]);
      $term->name = $desc_name[0];
      if (count($desc_name) > 1) {
        $term->description = $desc_name[1];
      }
    } else {
      $term->name = $term->id;
    }
    $subtype = $previous_objects[0]->name;
    $term->parents[] = tripal_analysis_kegg_get_ID(end($previous_objects)->name, $subtype, end($previous_objects)->name);

    if (isset($terms[$term->id])) {
      // Duplicate relationship protection
      if (!in_array(end($previous_objects)->name, $terms[$term->id]->parents))
        $terms[$term->id]->parents[] = tripal_analysis_kegg_get_ID(end($previous_objects)->name, $subtype, end($previous_objects)->name);
    } else {
      $terms[$term->id] = $term;
    }

    tripal_analysis_kegg_update_object_array($children, $previous_objects);
  }
}

/**
 * @param $current_object
 * @param $object_array
 *
 *  Checks to see if we have reached the end of a branch and backs out of the
 *  branch if necessary.
 */
function tripal_analysis_kegg_update_object_array($current_object, &$object_array) {
  $reverse_object_array = array_reverse($object_array);

  foreach($reverse_object_array as $object) {
    $size = count($object->children) - 1;

    if ($current_object->name == $object->children[$size]->name) {
      array_pop($object_array);
      $current_object = $object;
      continue;
    }

    break;
  }
}

/**
 * @param $string
 * @param $subtype
 *  Optional string; appends the subtype to the ID
 * @param $subtype_condition
 *  Optional string; if condition is met subtype will not be applied
 *
 * @return $id
 *
 *  Returns the ID given by a name in the JSON file.
 */
function tripal_analysis_kegg_get_ID($string, $subtype = '', $subtype_condition = '') {
  $id = explode('__', $string);
  $id = $id[0];
  if ($subtype != $subtype_condition)
    $id .= " ($subtype)";

  return $id;
}