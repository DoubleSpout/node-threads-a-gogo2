var http = require('http');
var tagg = require('../index.js'); 
var assert = require('assert');


var th_func = function(){

  var fibo =function fibo (n) {
      return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
    }
    var n = fibo(thread.buffer.toString()-0) + '';
    thread.end(n);
}

  

http.createServer(function (request, response) {
  response.writeHead(200, {'Content-Type': 'text/plain'});
  


  var req_ary = request.url.split('/')

  if(req_ary[1] === 'fibo'){
		var n = req_ary[2] || 1;
		tagg.create(th_func,{buffer:new Buffer(n+''),fastthread:false},function(err,result){
				if(err) response.end(err);
				response.end(result+'');
		})

  }
  else{
	response.end('ok');
  }





  if(request.url == '/test'){
        response.end('ok');
  }
  else{
    
  }

  
  
  
}).listen(8000);
console.log('Server running at http://127.0.0.1:8000/');




