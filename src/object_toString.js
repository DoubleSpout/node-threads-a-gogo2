(function () {
  'use strict';
  
  //2013-5 write thread.end function
	
  var _object_toString = function (object) {
    if('string' === typeof object) return object;
    if('number' === typeof object) return object + '';
	if('boolean' === typeof object) return object ? 'true' : 'false';
    if('function' === typeof object || 'array' === typeof object) return object.toString();
	if('undefined' === typeof object) return 'undefined';

    if('object' === typeof object) {
        try{
          var obj_str = JSON.stringify(object);
        }
        catch(e){
         return "";
        }
        return obj_str;
    }
  }
  return _object_toString;
})()
