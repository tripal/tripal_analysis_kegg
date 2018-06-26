<?php

class Term {
  public $name;
  public $id;
  public $description;
  public $parents = array();

  // The number of times this term occurs
  // Allows duplicates to sometimes occur, which we want
  public $count = 0;
  public $subtype = '';
  public $has_duplicate = false;
}

if ($argc < 2) echo "Please specify input files for parsing.\n";
else {
  $terms = [];
  $db_name = "KEGG:";
  $output_file = "kegg.obo";
  $file_contents = "";

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

    get_terms($data->children, $terms, $previous_objects);
  }

  // Output headers
  $output = "format-version: 1.2\ndefault-namespace: kegg ontology\n\n";

  // Print out all the terms
  foreach ($terms as $term) {
    // Separate the ID from its numerical identifier if it has duplicates
    $term_id = explode('__', $term->id)[0];
    // Append a unique identifier so the OBO doesn't break on duplicates
    if ($term->has_duplicate) $term_id .= " ($term->subtype)";

    $output .= "[Term]\n";
    $output .= "id: $db_name$term_id\n";
    $output .= "name: $term->name\n";
    if ($term->description) $output .= "def: $term->description\n";
    foreach ($term->parents as $parent) {
      // If parent name matches term id, ignore it
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
    $term->name = count($separated_name) > 1 ? $separated_name[1] : $term->id;
    $term->parents[] = end($previous_objects)->name;

    if (isset($terms[$term->id])) {
      $found = false;
      $subtype = $previous_objects[0]->name;
      // Try to see if the term already exists under the same subtype
      // If so, IDs will be the same and OBO will break
      // So if it's found, instead just add relationships to the other term
      // todo: dry
      if ($subtype == $terms[$term->id]->subtype) {
        $terms[$term->id]->parents[] = end($previous_objects)->name;
        $found = true;
      }

      for ($i = 0; $i < $terms[$term->id]->count; $i++) {
        // Re-create ID with suffix to each known duplicate term
        $tid = $term->id . '__' . ($i + 1);

        if ($subtype == $terms[$tid]->subtype && !in_array(end($previous_objects)->name, $terms[$tid]->parents)) {
          $terms[$tid]->parents[] = end($previous_objects)->name;
          $found = true;
          break;
        }
      }
      if (!$found) {
        $terms[$term->id]->count++;
        // Create a unique identifier so we can find the term later
        $term_id = $term->id . '__' . $terms[$term->id]->count;
        $terms[$term_id] = $term;
        $terms[$term_id]->subtype = $previous_objects[0]->name;
        $terms[$term_id]->has_duplicate = true;
        $terms[$term->id]->has_duplicate = true;
      }

    } else {
      $terms[$term->id] = $term;
      $terms[$term->id]->subtype = $previous_objects[0]->name;
    }

    $previous_objects[] = $children;

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

/**
 * @param $current_object
 * @param $object_array
 *
 *  Checks to see if we have reached the end of a branch and backs out of the
 *  branch if necessary.
 */
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