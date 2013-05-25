//加载tagg2的模块
//require TAGG2 lib
var tagg = require('../index.js'); 

//创建一个buffer,做为参数传给子线程
//create a buffer string as parameter passed to the thread
var buf = new Buffer('tagg2 buffer');

//子线程工作函数
//the function which working in the trhead
var th_func = function(){
	//加载一个外部js脚本
	//require a javascript file
	require('req_ex.js');
	//这个会异步执行
	//async call back
	thread.nextTick(function(){
		console.log('next tick');
		//回调给主线程,执行一次
		//mean return the string result to main thread,best do once
		thread.end("thread.end"); 
	})
	console.log('TAGG2');
	//打印当前文件目录路径
	//current file path
	console.log(__dirname);
	//打印传递过来的buffer字符串信息
	//must be tagg2 buffer
	console.log(thread.buffer.toString());
	//调用外部加载的函数
	//call the function defined in req_ex.js
	req_func();

	
}


var n = 5
var thread_cb = function(err, res){//线程回调函数
	if(err) throw new(err);
	console.log(res);//thread.end
	if(!(--n)){
		thread.destroy();//摧毁线程
	}
	console.log(n)
}



//创建子线程,并且注册回调
//create a thread and regist the calback
var thread = tagg.create(3);
thread.dirname = __dirname;
//or var thread = tagg.create(th_func,{buffer:buf,poolsize:3},thread_cb);
thread.pool(th_func,buf,thread_cb);
thread.pool(th_func,buf,thread_cb);
thread.pool(th_func,buf,thread_cb);
thread.pool(th_func,buf,thread_cb);
thread.pool(th_func,buf,thread_cb);

console.log('thread.totalThreads: '+thread.totalThreads())
console.log('thread.idleThreads: '+thread.idleThreads())
console.log('thread.pendingJobs: '+thread.pendingJobs())