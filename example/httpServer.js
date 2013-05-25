var http = require('http');
var tagg = require('../index.js'); 
var assert = require('assert');



var th_func = function(){

	var fibo =function fibo (n) {
      return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
    }
    var n = fibo(45) + '';
    thread.end(n);
}

	

http.createServer(function (request, response) {

  response.writeHead(200, {'Content-Type': 'text/plain'});
  
  
  if(request.url == '/test'){
  			response.end('ok');
  }
  else{
  	tagg.create(th_func,function(err,result){
		  	if(err) response.end(err);
		  	console.log(result)
		  	response.end(result);
 		 })
  }

  
  
  
}).listen(8000);
console.log('Server running at http://127.0.0.1:8000/');




