var fastThread = require('./fastThread');
var processThread = require('./processThread');
var path = require('path');
var thread_obj={};
var blank_func = function(){};

ERROR_ARG_1 = 'thread_obj.create first argument must be a run thread function or a  {poolsize:100, fastthread:false}';
ERROR_ARG_2 = 'thread_obj.create second argument must be a callback or an option object';
ERROR_ARG_3 = 'thread_obj.create third argument must be a callback function';


ERROR_POOLSIZE = 'option\'s poolsize must be an unsign int';


/*
create a thread or a pool
var thread_func = function(){
	//do something
}
thread_obj.create(thread_func, [{buffer:buf}, function(err,cb]){

})
*/
thread_obj.create = function(func, opt, cb){
	if(arguments.length === 1){
		if('number' === typeof func){
			opt = {poolsize:func}
			cb = func = blank_func;
		}
		else if('function' === typeof func){			
			opt = {};
			cb = blank_func
		}
		else if('object' === typeof func){ //如果是多进程进程池
			if(!isFinite(func.poolsize)) throw(ERROR_ARG_1);
			if('boolean' !== typeof func.fastthread) throw(ERROR_ARG_1);
			opt = {buffer:new Buffer(0), poolsize:func.poolsize, fastthread:func.fastthread}
			cb = func = blank_func;
		}
		else{
			throw(ERROR_ARG_1);
		}
	}

	if(!func || 'function' !== typeof func){
		throw(ERROR_ARG_1);
	}
	if(opt && 'function' !== typeof opt && 'object' !== typeof opt ){
		throw(ERROR_ARG_2);
	}
	if(cb && 'function' !== typeof cb){
		throw(ERROR_ARG_3);
	}
	if('function' === typeof opt){
		var cb = opt;
		opt = {};
	}
	
	opt = opt || {};
	cb = cb || blank_func;
	
	opt.buffer = opt.buffer || new Buffer(0); //默认线程间传递的buffer值为0buffer
	if(opt.poolsize && (!isFinite(opt.poolsize) || opt.poolsize<=0)){ //如果线程池大小不正确
		throw(ERROR_POOLSIZE);
	}

	opt.poolsize = opt.poolsize || false;
	if('undefined' === typeof opt.fastthread){
		opt.fastthread = true;
	}


	var default_dirname = path.dirname(module.parent.parent.filename); //默认dirname,为加载tagg2模块的路径

	opt.dirname = opt.dirname || default_dirname;//默认的文件路径


	if(opt.fastthread){//如果是使用快速线程模型

		func = '('+func.toString()+')()'; //组装在线程中运行的函数
		return fastThread(func, opt, cb);
	
	}
	else{ //如果使用多进程模型
		
		return processThread(func, opt, cb);
	}
	
}





module.exports = thread_obj;
