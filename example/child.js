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
	require('./blank.js');
	//打印TAGG2系想你
	thread.nextTick(function(){//这个会异步执行
		console.log('next tick');
		thread.end("thread.end");
	})
	console.log('TAGG2');
	//打印当前文件目录路径
	console.log(__dirname);
	//打印传递过来的buffer字符串信息
	console.log(thread.buffer.toString());//tagg2 buffer
	//调用外部加载的函数
	req_func();
	//异步异步函数

	
}

//创建子线程,并且注册回调
//create a thread and regist the calback
var thread = tagg.create(th_func,{buffer:buf,fastthread:false,dirname:__dirname},function(err, res){
	if(err) throw new(err);
	console.log(res);//thread.end
	thread.destroy();//摧毁线程
});