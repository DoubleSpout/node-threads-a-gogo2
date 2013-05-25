var blank_func = function(){};

module.filename = global._thread_filename;
module.paths.unshift(global._thread_dirname);
__dirname =  global._thread_dirname;

var _object_toString = function (object) {
    if('string' === typeof object) return object;
    if('number' === typeof object) return object + '';
	if('boolean' === typeof object) return object ? 'true' : 'false';
    if('function' === typeof object || 'array' === typeof object || Buffer.isBuffer(object)) return object.toString();
    if('object' === typeof object) {
        try{
          var obj_str = JSON.stringify(object);
        }
        catch(e){
         return "";
        }
        return obj_str;
    }
  }

var has_thread_end = false;
var callback;
global.thread={}; 
thread.id = process.pid;


thread.end = function(buf){//当进程执行结束
	if(has_thread_end) return;
	has_thread_end = true;
	if('undefined' ===  typeof buf) buf = 'undefined'
	process.send({_TAGG2_END: _object_toString(buf)});
}



process.on('uncaughtException', function(err) { //遇到意外error
	try{
			if(err) process.send({_TAGG2_ERR: JSON.stringify(err)});
			else process.send({_TAGG2_END: ''});
		}
	catch(e){

	}

	process.exit(0);
});
	
process.on('exit', function() { //当进程推出执行回掉
	try{
		process.send({_TAGG2_END: ''});
	}
	catch(e){
		
	}
	process.nextTick(function(){
		process.exit(0);
	})
});

process.on('message', function(m) {

  if('undefined' !== typeof m._TAGG2_BUFFER){		
		thread.buffer = new Buffer(m._TAGG2_BUFFER);	
		callback && callback();
  }	

  if('undefined' !== typeof m._TAGG2_JOB){
  		has_thread_end = false;
	  	eval(m._TAGG2_JOB); //执行任务
  }

});

thread.nextTick = process.nextTick;

process.nextTick(function(){
	process.send({_TAGG2_READY:1}); //当进程准备完毕之后，通知主进程准备接受buffer
})

module.exports = function(cb){//接受回掉参数
	callback = cb || blank_func;
}
	
if(process.argv[3] === 'true'){ //如果是进程池，为了不让child自动退出，重用进程

	setInterval(function(){

	},1000*60*60)
}