var assert = require('assert');


//加载tagg2的模块
//require TAGG2 lib
var tagg = require('../index.js'); 

var allback =function(){
	if(!(--x)){
		threadp2.destroy()
		threadp1.destroy()
		console.log(x)
		test_ok("pool test ok")
	}
}

var buf = new Buffer('test quick');
var buf2 = new Buffer('test quick2');
var buf3 = new Buffer('test quick3');

var th_func1 = function(){
	var n = 0;

	require('./require_blk.js');

	//fast thread don't have nodejs module
	//var fs = require('fs');
	//if(!fs) throw('fs not found');

	if(!console.log){
		throw("not have console.log")
	}

	thread.nextTick(function(){
		if(n!==1) throw("thread.nextTick not work")

		thread.end(__dirname); 
	})

	if(thread.buffer.toString() && thread.buffer.toString() !== "test quick") throw("buffer is not transfer in thread")

	if(global.req_string2 !== "req_func") throw("req_string2 incorrect")

	global.req_func();
	
	if(global.req_string1 !== 'req_func') throw("req_string1 incorrect")

	n++;
}

th_func2 = function(){
	if(thread.buffer.toString() && thread.buffer.toString() !== "test quick2") throw("buffer is not transfer in thread")

	var obj = {key:1}
	thread.end(obj)
}

th_func3 = function(){
	if(thread.buffer.toString() && thread.buffer.toString() !== "test quick3") throw("buffer is not transfer in thread")
	

	var obj = [1,2]
	thread.end(obj)
}

th_func4 = function(){
	var obj = false
	thread.nextTick(function(){
		
	})
	thread.nextTick(function(){
		
	})
	thread.nextTick(function(){
		require('./require_blk.js');
	})
	thread.nextTick(function(){
		thread.end(obj);
	})
	
 
}

th_func5 = function(){
	var obj = 5
	
	thread.nextTick(function(){
		require('require_blk.js');
		thread.nextTick(function(){
			require('require_blk.js');
			thread.nextTick(function(){
				require('require_blk.js');
				thread.nextTick(function(){
					thread.end(obj)
				})
			})
			
		})
		
	})

	
	require('require_blk.js');
	thread.end(obj)
}

th_func6 = function(){
	
	thread.end(0)
}

th_func7 = function(){
	var obj = thread.id
	thread.end()
}

var x= 8;


var thread_cb = function(err, res){
	if(err) throw(err);
	assert.ok(threadp1.totalThreads());
	if('number' !== typeof threadp1.idleThreads()){
		throw('error on threadp1.idleThreads() test')
	}
	if('number' !== typeof threadp1.pendingJobs()){
		throw('error on  threadp1.pendingJobs() test')
	}
	assert.strictEqual(res, __dirname);
	allback()
}


var thread_cb2 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('object', typeof res);
	assert.strictEqual(JSON.stringify(res), '{\"key\":1}');
	allback()
}

var thread_cb3 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('object', typeof res);
	assert.strictEqual(JSON.stringify(res), '[1,2]');
	allback()
}

var thread_cb4 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('boolean', typeof res);
	assert.strictEqual(res, false);

	allback()
}

var thread_cb5 = function(err, res){
	if(err) throw(err);
	assert.strictEqual('number', typeof res);
	assert.strictEqual(res, 5);
	allback()
}

var thread_cb6 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('number', typeof res);
	assert.strictEqual(res, 0);

	allback()
}

var thread_cb7 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('undefined', typeof res);
	assert.ok(threadp2.totalThreads());
	if('number' !== typeof threadp2.idleThreads()){
		throw('error on threadp2.idleThreads() test')
	}
	if('number' !== typeof threadp2.pendingJobs()){
		throw('error on  threadp2.pendingJobs() test')
	}
	allback()
	
}

var threadp1 = tagg.create({poolsize:5,fastthread:true});
var threadp2 = tagg.create({poolsize:3,fastthread:true});


setTimeout(function(){
	threadp1.pool(th_func1,thread_cb);
	threadp1.pool(th_func1,buf,thread_cb);
	
	 

},2000)


setTimeout(function(){
	threadp2.pool(th_func2,buf2,thread_cb2);
	threadp2.pool(th_func3,buf3,thread_cb3);
	threadp2.pool(th_func4,buf,thread_cb4);
	threadp2.pool(th_func5,buf,thread_cb5);
	threadp2.pool(th_func6,buf,thread_cb6);
	
},5000)

setTimeout(function(){
	 
	 threadp2.pool = tagg.create(th_func7,buf,thread_cb7);
},8000)























