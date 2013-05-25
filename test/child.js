var assert = require('assert');


//加载tagg2的模块
//require TAGG2 lib
var tagg = require('../index.js'); 

var allback =function(){
	if(!(--x)){
		console.log(x)
		test_ok("child test ok")
	}
}

var buf = new Buffer('test quick');
var buf2 = new Buffer('test quick2');
var buf3 = new Buffer('test quick3');

var th_func1 = function(){
	var n = 0;

	require('require_blk.js');
	var fs = require('fs');
	
	if(!fs) throw('fs not found');

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
		require('require_blk.js');
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

}

th_func6 = function(){
	var obj = thread.id
	thread.end(obj)
}

th_func7 = function(){
	var obj = thread.id
	thread.end()
}

var x= 8;


var thread_cb = function(err, res){
	if(err) throw(err);

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
	assert.strictEqual(res, thread6.id);

	allback()
}

var thread_cb7 = function(err, res){
	if(err) throw(err);

	assert.strictEqual('undefined', typeof res);

	allback()
}


var thread = tagg.create(th_func1,{fastthread:false});

setTimeout(function(){
	 thread = tagg.create(th_func1,{fastthread:false},thread_cb);
	 thread = tagg.create(th_func1,{buffer:buf,fastthread:false},thread_cb);
	
	 

},2000)


setTimeout(function(){
	 thread2 = tagg.create(th_func2,{buffer:buf2,fastthread:false},thread_cb2);
	 thread3 = tagg.create(th_func3,{buffer:buf3,fastthread:false},thread_cb3);
	 thread4 = tagg.create(th_func4,{buffer:buf,fastthread:false},thread_cb4);
	 thread5 = tagg.create(th_func5,{buffer:buf,fastthread:false},thread_cb5);
	 thread6 = tagg.create(th_func6,{buffer:buf,fastthread:false},thread_cb6);
	
},5000)

setTimeout(function(){
	 
	 thread7 = tagg.create(th_func7,{buffer:buf,fastthread:false},thread_cb7);
},8000)






