<?php

class data__kegg_object_identifier_formatter extends ChadoFieldFormatter {

  // The default lable for this field.
  public static $default_label = 'KEGG object identifier';

  // The list of field types for which this formatter is appropriate.
  public static $field_types = array('data__kegg_object_identifier');

  /**
   * @see TripalFieldFormatter::view()
   */
  public function view(&$element, $entity_type, $entity, $langcode, $items, $display) {

    // Get the settings
    $settings = $display['settings'];
    $feature = $entity->chado_record;

    $i = 0;
    $list_items = '';
    foreach ($items as $delta => $item) {
      $element[0] = array(
        '#type' => 'markup',
        '#markup' =>$item['value'],
      );
    }
  }
}
