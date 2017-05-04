<?php

class data__kegg_object_identifier_widget extends ChadoFieldWidget {

  // The default lable for this field.
  public static $default_label = 'KEGG object identifier';

  // The list of field types for which this formatter is appropriate.
  public static $field_types = array('data__kegg_object_identifier');


  /**
   * @see TripalFieldWidget::form()
   */
  public function form(&$widget, &$form, &$form_state, $langcode, $items, $delta, $element) {
    parent::form($widget, $form, $form_state, $langcode, $items, $delta, $element);
  }

  /**
   * @see TripalFieldWidget::validate()
   */
  public function validate($element, $form, &$form_state, $langcode, $delta) {

  }
}
