<?php

class Term {
  public $name;
  public $id;
  public $description;
  public $parents = array();
}

if ($argc < 2) echo "Please specify input files for parsing.\n";
else {
  $terms = array();
  $db_name = "KEGG:";
  $output_file = "kegg.obo";
  $file_contents = "";

  // Iterate through file and get all terms
  foreach($argv as $file) {
    if ($file == $argv[0]) continue;
    $input = file_get_contents($file);
    $data = json_decode($input);
    $previous_objects[] = $data;
    // Create first term
    $term = new Term();
    $term->id = $data->name;
    $term->name = $data->name;
    $terms[$term->id] = $term;

    get_terms($data->children, $terms, $previous_objects);
  }

  // Output headers
  $output = "format-version: 1.2\ndefault-namespace: kegg ontology\n\n";

  // Print out all the terms
  foreach ($terms as $key => $value) {
    $output .= "[Term]\n";
    $output .= "id: $db_name$value->id\n";
    $output .= "name: $value->name\n";
    if ($value->description) $output .= "def: $value->description\n";
    foreach ($value->parents as $parent) {
      // If parent name matches term id, ignore it
      if ($parent == $value->id) continue;
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
 */
function get_terms($children, &$terms, $previous_objects) {
  // We have reached the leaves here
  if (is_array($children))
  {
    foreach($children as $child)
    {
      get_terms($child, $terms, $previous_objects);
    }
  }
  // Add term and proceed one layer deeper
  if (is_object($children) && property_exists($children, "children")) {
    $term = new Term();
    $separated_name = explode("  ", $children->name);
    $term->id = $separated_name[0];
    if (count($separated_name) > 1) {
      $term->name = $separated_name[1];
    } else {
      $term->name = $term->id;
    }
    $term->parents[] = end($previous_objects)->name;
    $previous_objects[] = $children;
    if (isset($terms[$term->id])) {
      $terms[$term->id]->parents[] = end($previous_objects)->name;
    } else {
      $terms[$term->id] = $term;
    }

    return get_terms($children->children, $terms, $previous_objects);
  }
  if (is_object($children)) {
    $term = new Term();
    $access_name = explode("  ", $children->name);
    $term->id = $access_name[0];
    if (count($access_name) > 1) {
      $desc_name = explode("; ", $access_name[1]);
      $term->name = $desc_name[0];
      if (count($desc_name) > 1) {
        $term->description = $desc_name[1];
      }
    } else {
      $term->name = $term->id;
    }
    $term->parents[] = end($previous_objects)->name;

    if (isset($terms[$term->id])) {
      // Duplicate relationship protection
      if (!in_array(end($previous_objects)->name, $terms[$term->id]->parents))
        $terms[$term->id]->parents[] = end($previous_objects)->name;
    } else {
      $terms[$term->id] = $term;
    }

    update_object_array($children, $previous_objects);
  }
}

function update_object_array($current_object, &$object_array) {
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