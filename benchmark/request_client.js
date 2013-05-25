var cluster = require('cluster');
var http = require('http');
var tagg = require('../index.js'); 
var assert = require('assert');




var n = 16;
var y = 17;
var x = 16;

console.log('start test '+n+' times fibo(40)!enjoy it!')

console.time('all')
console.time('fibo')

while(n--){


	http.get('http://localhost:8000/fibo/40', function(res){
		var body ='';
		res.on('data',function(ck){
			body += ck;
		})

		res.on('end',function(){
			
	     		assert.strictEqual(165580141, body - 0);
				
	     		if(!(--x)){
					console.timeEnd('fibo')
				}
	     		if(!(--y)){
					console.timeEnd('all')
	     			//process.exit(1)
	     		}

		})
	}).on('error', function(e) {
	  throw(e.message);
	})

}




console.time('/test response ok')

http.get('http://localhost:8000/test', function(res){
		
		var body ='';

		res.on('data',function(ck){
			body += ck;
		})

		res.on('end',function(){
	
	     		assert.strictEqual('ok', body);
	     		console.timeEnd('/test response ok')

	     		if(!(--y)){
					console.timeEnd('all')
	     			//process.exit(1)
	     		}

		})
	}).on('error', function(e) {
	  throw(e.message);
	})






