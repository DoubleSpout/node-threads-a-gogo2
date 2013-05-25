var cluster = require('cluster');
var http = require('http');
var tagg = require('../index.js'); 
var assert = require('assert');
var numCPUs = require('os').cpus().length;



if (cluster.isMaster) {
  // Fork workers.
  for (var i = 0; i < numCPUs; i++) {
    cluster.fork();
  }

  cluster.on('exit', function(worker, code, signal) {
    console.log('worker ' + worker.process.pid + ' died');
  });

} else {


var fibo =function fibo (n) {
      return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
    }

http.createServer(function (request, response) {
  response.writeHead(200, {'Content-Type': 'text/plain'});
  
   var req_ary = request.url.split('/')

  if(req_ary[1] === 'fibo'){
		var n = req_ary[2] || 1;
		response.end(fibo(n)+'');
  }
  else{
	response.end('ok');
  }
   
  
}).listen(8000);
console.log('Server running at http://127.0.0.1:8000/');



}


