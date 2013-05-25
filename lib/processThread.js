var child_process = require('child_process');
var generic_pool = require('generic-pool');//加载连接池
var string_to_object = require('./string_to_object');
var node_cmd = process.execPath;
var path = require('path');
var fs = require('fs');
var os = require('os');
var dirname = __dirname;
var tmp_path = path.join(__dirname, '..', 'tmp')
var NO = 1;

var iswin32 = os.platform() === 'win32' ? true : false;

var blank_func = function(){};


var CHOMD_ERROR = 'Make sure the path : '+ tmp_path + ', you have 0777 chomd!'
var POOL_ERROR = 'thread.pool arguments error, ex:thread.pool(func [,buffer] [,callback])'





var path_replace = function(paths){
	if(iswin32){
		paths = paths.replace(/\\/g,'\\\\'); 
	}
	return paths;
}

var init = function(){
	
	if(!fs.existsSync(tmp_path)){
		fs.mkdirSync(tmp_path, '0777');
	}

	try{
		fs.chmodSync(tmp_path, '0777');
		var file_ary = fs.readdirSync(tmp_path);
		file_ary.forEach(function(v){
			if(v !== '.tmp'){
					fs.unlinkSync(tmp_path+path.sep+v);
			}
		})
	}
	catch(e){
		console.log(CHOMD_ERROR);
		console.log(e)
	}


	return arguments.callee;
}()


var create_child = function(func, opt, cb){

	var filename = gen_js_file(func, opt, cb);//

	if(!filename) return false;

	var ispool = opt.poolsize>0 ? true : false;

	return fork_child(filename, opt, cb, ispool);

}


var gen_js_file = function(func, opt, cb){
	var require_path = path_replace(dirname+path.sep+'processRequire.js');
	var require_dirname = path_replace(opt.dirname);
	var require_filename = path_replace(require_dirname + path.sep + 'tmp.js');

	var func = 'global._thread_dirname = __dirname = "'+require_dirname+'";'+
			   'global._thread_filename = module.filename = "' + require_filename + '/";'+
			   'module.paths.unshift("'+require_dirname+'");'+          //增加require的路径，否则会找不到模块
			   'var _TAGG_READY = require("'+require_path+'");'+    //加载预设的process模块		   
			   '_TAGG_READY('+func+');';//注册回调
    

    var filename = tmp_path+path.sep+(Date.now()+ process.pid + (NO++))+'.js'; //拼接临时子进程文件名

	try{
			fs.writeFileSync(filename, func); //为了兼容node V0.8.x
			fs.chmodSync(filename, '0777');
		
	}
	catch(e){
		 cb(JSON.stringify(e));
		 return false;
	}

    return filename;


}

var fork_child = function(filename, opt, cb, ispool){
	try{
		var child = child_process.fork(filename,['-ispool',ispool]);
	}
	catch(e){
		cb(e);
		return false;
	}
	
	child.filename = filename;

	child.cb = cb;

	child.hasCalled = false;

	child.id = child.pid;

	child.on('message', function(m) {
		var that = this;
		
		if(m._TAGG2_READY === 1){
			
			that.send({_TAGG2_BUFFER:opt.buffer.toString(), _TAGG2_DIRNAME:opt.dirname});

			ispool && that._ready();//当使用pool时

		}
		else if('undefined' !== typeof m._TAGG2_END){
			if(that.hasCalled) return;
			that.hasCalled = true;

			that.cb(null, string_to_object(m._TAGG2_END));
		}

		else if('undefined' !== typeof m._TAGG2_ERR){
			if(that.hasCalled) return;
			that.hasCalled = true;
			that.cb(m._TAGG2_ERR, null);
		}

	});
	
	child.destroy = function(bool){
		var that = this;
		var pid = that.id;

		that.removeAllListeners(); //移除所有监听器

		if(bool){ //如果是强制关闭
				if(iswin32){
					child_process.spawn('tskill', [pid]);
				}
				else{
					child_process.spawn('kill', ['-9',pid]);
				}
		}
		else{//平滑关闭

			process.nextTick(function(){
				that.disconnect()
			})

		}
		fs.unlink(that.filename, function(err){
			//console.log(that.filename +' deleted')
		})

	};
	

	return child;

}






module.exports = function(func, opt, cb){

	if(opt.poolsize >0){ //如果使用线程池

		var pool = generic_pool.Pool({
		    name     : 'child_pool_'+(NO++),
		    create   : function(callback) {
		        var child = create_child(func, opt, cb);
		        child._ready = function(err){
		        	callback(err, child);		
		        }
		        
		    },
		    destroy  : function(child) { child.destroy(true); },
		    max      : opt.poolsize,
		    // optional. if you set this, make sure to drain() (see step 3)
		    min      : 0, 
		    // specifies how long a resource can stay idle in pool before being removed
		    idleTimeoutMillis : 1000*10,
		     // if true, logs via console.log - can also be a function
		    log 	 : false 

		});

		var thread_pool_obj = {}; //定义返回的thread pool 对象
		thread_pool_obj._poolobj = pool;

		thread_pool_obj.pool = function(func, buffer, callback){

				var that = this;

    			if(arguments.length === 1){
						if('function' !== typeof func) throw(POOL_ERROR);
						func = '('+func.toString()+')()'; //组装在线程中运行的函数
						buffer = new Buffer(0);
						callback = blank_func;
				}
				else if(arguments.length === 2){
					if('function' !== typeof func) throw(POOL_ERROR);
					if('function' === typeof buffer){
						func = '('+func.toString()+')()'; //组装在线程中运行的函数
						callback = buffer;
						buffer = new Buffer(0);
					}
					else if(Buffer.isBuffer(buffer)){
						func = '('+func.toString()+')()'; //组装在线程中运行的函数
						buffer = buffer;
						callback = blank_func

					}
					else throw(POOL_ERROR);
				}
				else{
					if('function' !== typeof func) throw(POOL_ERROR);
					if(!Buffer.isBuffer(buffer)) throw(POOL_ERROR);
					if('function' !== typeof callback) throw(POOL_ERROR);
					func = '('+func.toString()+')()'; //组装在线程中运行的函数
				}

			that._poolobj.acquire(function(err, cobj){
				if(err){
					 return callback(err);
				}

				cobj.cb = function(err,res){	
					that._poolobj.release(cobj);		
					callback(err,res)
								
				};

				cobj.hasCalled = false;

				cobj.send({_TAGG2_BUFFER:buffer.toString(), _TAGG2_JOB: func});

			})


			return that;
		}

		
		thread_pool_obj.totalThreads = function(){
			return this._poolobj.getPoolSize();
		}
		thread_pool_obj.idleThreads = function(){
			return this._poolobj.availableObjectsCount();
		}
		thread_pool_obj.pendingJobs = function(){
			return this.totalThreads() - this.idleThreads();
		}

		thread_pool_obj.destroy = function(){
			var that = this;
			that._poolobj.drain(function() {
			    that._poolobj.destroyAllNow();
			});
		}
		thread_pool_obj.id = thread_pool_obj._poolobj.getName();
		return thread_pool_obj;

	}
	else{//如果不使用线程池

		return create_child(func, opt, cb);

	}






}
