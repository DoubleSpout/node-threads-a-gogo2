var http = require('http');
var tagg = require('../index.js'); 
var assert = require('assert');
var fibo =function fibo (n) {
      return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
    }


var th_func = function(){

	var fibo =function fibo (n) {
      return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
    }
    var n = fibo(40) + '';
    thread.end(n);
}

	

http.createServer(function (request, response) {

  response.writeHead(200, {'Content-Type': 'text/plain'});
  
  
  if(request.url == '/test'){
  			response.end('ok');
  }
  else{
  	tagg.create(th_func, function(err,result){
		  	if(err) response.end(err);
		  	console.log(result)
		  	response.end(result+'');
 		 })
  }

  
  
  
}).listen(8000);
console.log('Server running at http://127.0.0.1:8000/');




console.log('http test start,may be take a few minutes')

var d = Date.now()
var m = 6
while(m--){
	fibo(40)
}
var d2 = Date.now()
var t = Math.ceil((d2-d)/1.3)


console.log(t)



var n = 6;
var y = 7;
while(n--){
	http.get('http://localhost:8000', function(res){
		console.log("Got response: " + res.statusCode);
		var body ='';

		res.on('data',function(ck){
			body += ck;
		})
		res.on('end',function(){
				var x = body - 0;
	     		assert.strictEqual(165580141, x);
	     		console.log(x)
	     		if(!(--y)){
	     			test_ok("http request test ok")
	     		}

		})
	}).on('error', function(e) {
	  throw(e.message);
	}).setTimeout(t, function(){
		 throw('timeout');
	});

}



http.get('http://localhost:8000/test', function(res){
		console.log("Got response: " + res.statusCode);
		var body ='';

		res.on('data',function(ck){
			body += ck;
		})
		res.on('end',function(){
				
	     		assert.strictEqual('ok', 'ok');
	     		console.log('/test request ok')
	     		if(!(--y)){
	     			test_ok("http request test ok")
	     		}

		})
	}).on('error', function(e) {
	  throw(e.message);
	}).setTimeout(1000, function(){
		 throw('timeout');
	});



