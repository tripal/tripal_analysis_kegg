<?php

class Term
{
  public $name;
  public $id;
  public $description;
  public $parents = array();
}

if ($argc < 2) echo "Please specify input files for parsing.\n";
else
{
  $terms = array();
  $names = array();
  $db_name = "KEGG:";
  $output_file = "kegg.obo";
  $file_contents = "";

  foreach($argv as $file) {
    if ($file == $argv[0]) continue;
    $input = file_get_contents($file);
    $data = json_decode($input);

    $file_contents .= $data;
  }

    $previous_object = $data->name;
    get_terms($data->children, $terms, $names, $previous_object);
    // Output headers
    $output = "format-version: 1.2\ndefault-namespace: kegg ontology\n\n";
    // Output first object
    $output .= "[Term]\n";
    $output .= "id: $db_name:$data->name\n";
    $output .= "name: $data->name\n\n";

    // Loop through all terms
    foreach ($terms as $term) {
      $output .= "[Term]\n";
      $output .= "id: $db_name$term->id\n";
      $output .= "name: $term->name\n";
      if ($term->description) {
        $output .= "def: $term->description\n";
      }
      foreach ($term->parents as $parent) {
        $output .= "is_a: $db_name$parent\n";
      }
      $output .= "\n";
  }
  // Output the contents of the file
  file_put_contents($output_file, $output);
}

function get_terms($children, &$terms, &$names, &$previous_object)
{
  // We have reached the leaves here
  if (is_array($children))
  {
    foreach($children as $child)
    {
      get_terms($child, $terms, $names, $previous_object);
    }
  }
  // Add term and proceed one layer deeper
  if (is_object($children) && property_exists($children, "children"))
  {
    if (!in_array($children->name, $names)) {
      $term = new Term();
      $separated_name = explode("  ", $children->name);
      $term->id = $separated_name[0];
      if (count($separated_name) > 1) {
        $term->name = $separated_name[1];
      }
      else {
        $term->name = $term->id;
      }
      $term->parents[] = $previous_object;
      $previous_object = $term->id;
      $terms[] = $term;
      $names[] = $children->name;
    }


    return get_terms($children->children, $terms, $names, $previous_object);
  }

  // This is an object that does not have children
  if (is_object($children))
  {
    if (!in_array($children->name, $names))
    {
      // This is a leaf; create new term and fill
      $term = new Term();
      $access_name = explode("  ", $children->name);
      $term->id = $access_name[0];
      if (count($access_name) > 1) {
        $desc_name = explode("; ", $access_name[1]);
        $term->name = $desc_name[0];
        if (count($desc_name) > 1) $term->description = $desc_name[1];
      }
      else {
        $term->name = $term->id;
      }
      $term->parents[] = $previous_object;
      $terms[] = $term;

      $names[] = $children->name;
      // If the term already exists, add the previous object as a relationship
    } else {
      $search_name = explode("  ", $children->name);
      foreach($terms as $term)
      {
        if ($term->id == $search_name[0])
        {
          $term->parents[] = $previous_object;

          break;
        }
      }
    }
  }
}