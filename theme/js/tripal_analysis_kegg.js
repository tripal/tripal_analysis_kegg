(function($) {
  Drupal.behaviors.tripal_kegg_analysisBehavior = {
    attach: function (context, settings){ 
      // code for toggling trees below borrowed from here: http://homework.nwsnet.de/releases/ea21/
    
      // Find list items representing folders and
      // style them accordingly.  Also, turn them
      // into links that can expand/collapse the
      // tree leaf.
      $('#heirarchy_terms_list li > ul').each(function(i) {
        // Find this list's parent list item.
        var parent_li = $(this).parent('li');

        // Style the list item as folder.
        parent_li.addClass('folder');

        // Temporarily remove the list from the
        // parent list item, wrap the remaining
        // text in an anchor, then reattach it.
        var sub_ul = $(this).remove();
        parent_li.wrapInner('<a/>').find('a').click(function() {
          // Make the anchor toggle the leaf display.
          sub_ul.toggle();
        });
        parent_li.append(sub_ul);
      });

      // Hide all lists except the outermost.
      $('#heirarchy_terms_list ul ul').hide();
    }
  };
})(jQuery);

if (Drupal.jsEnabled) {
   $(document).ready(function() {
     

  
       // Select default KEGG analysis when available
       var selectbox = $('#edit-tripal-analysis-kegg-select');
       if(selectbox.length > 0){ 
         var option = document.getElementById("analysis_id_for_kegg_report");
         if (option) {
           var options = document.getElementsByTagName('option');
           var index = 0;
           for (index = 0; index < options.length; index ++) {
             if (options[index].value == option.value) {
               break;
             }
           }
           selectbox[0].selectedIndex = index;
           var baseurl = tripal_get_base_url();
           tripal_analysis_kegg_org_report(option.value, baseurl);
        // Otherwise, show the first option by default
         } else {
           selectbox[0].selectedIndex = 1;
           selectbox.change();
         }
       }

   });

   //------------------------------------------------------------
   function tripal_analysis_kegg_org_report(item,baseurl,themedir){
      if(!item){
         $("#tripal_analysis_kegg_org_report").html('');
         return false;
      }
      // Form the link for the following ajax call  
      var link = baseurl + "/";
      if(!isClean){
         link += "?q=";
      }
      link += 'tripal_analysis_kegg_org_report/' + item;

      tripal_startAjax();
      $.ajax({
           url: link,
           dataType: 'json',
           type: 'POST',
           success: function(data){
             $("#tripal_analysis_kegg_org_report").html(data[0]);
             $(".tripal_kegg_brite_tree").attr("id", function(){
                init_kegg_tree($(this).attr("id"));    
             });
             tripal_stopAjax();
           }
      });
      return false;
   }
   
   //------------------------------------------------------------
   // Update the BRITE hierarchy based on the user selection
   function tripal_update_brite(link,type_id){
      tripal_startAjax();
      $.ajax({
         url: link.href,
         dataType: 'json',
         type: 'POST',
         success: function(data){
            $("#tripal_kegg_brite_hierarchy").html(data.update);
            $("#tripal_kegg_brite_header").html(data.brite_term);
            tripal_stopAjax();
            init_kegg_tree(data.id);
         }
      });
      return false;
   }
}
