var i = 0
//加载tagg2的模块
//require TAGG2 lib
var tagg = require('../index.js');
var buf = new Buffer("childe pool");

var th_func1 = function(){
	require('./blank.js');

	var n = 0;
	
	require('../test/require_blk.js');

	//fast thread don't have nodejs module
	var fs = require('fs');
	if(!fs) throw('fs not found');

	if(!console.log){
		throw("not have console.log")
	}

	thread.nextTick(function(){
		if(n!==1) throw("thread.nextTick not work")

		thread.end(__dirname); 
	})

	if(thread.buffer.toString() && thread.buffer.toString() !== "childe pool") throw("buffer is not transfer in thread")

	if(global.req_string2 !== "req_func") throw("req_string2 incorrect")

	global.req_func();
	
	if(global.req_string1 !== 'req_func') throw("req_string1 incorrect")

	n++;

}


var thread_cb = function(err, res){
	if(err) throw(err);
	console.log(++i);
	console.log(res);
}



//创建进程池，里面含有5个进程
var threadp1 = tagg.create({poolsize:5, fastthread:false, dirname:__dirname});
threadp1.pool(th_func1);
threadp1.pool(th_func1, buf);
threadp1.pool(th_func1, thread_cb);
threadp1.pool(th_func1, buf, thread_cb);
threadp1.pool(th_func1, buf, thread_cb);


