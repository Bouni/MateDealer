/*
$(document).ready(function(){
    $.ajax({
        url: "/status",
        data: "",
        success: function(ret) {
           if(ret.locked == true){
            $("#vend_button").hide();
            $("#locked_msg").html("Der MateDealer ist momentan belegt"); 
           } else {
            $("#vend_button").show(); 
            $("#locked_msg").html(""); 
            }  
        }
    });
    
});/*
