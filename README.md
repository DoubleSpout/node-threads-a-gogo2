# TAGG2(Threads A GoGo2) for Node.js [![Build Status](https://travis-ci.org/DoubleSpout/node-threads-a-gogo2.png?branch=master)](https://travis-ci.org/DoubleSpout/node-threads-a-gogo2)

A native module for Node.js that provides an asynchronous, evented and/or continuation passing style API for moving blocking/longish CPU-bound tasks out of Node's event loop to JavaScript threads that run in parallel in the background and that use all the available CPU cores automatically; all from within a single Node process.

TAGG2 is thread_a_gogo module's fork,it require nodejs version>=0.8.x, thanks for Jorge Chamorro Bieling.

TAGG2 support windows, linux, mac.

## Installing the module

With [npm](http://npmjs.org/):

Threads A GoGo2 module is supported windows, linux, mac.

Make sure, node-gyp has installed.

     npm install tagg2

From source:

     git clone https://github.com/DoubleSpout/node-threads-a-gogo2.git
     cd node-threads-a-gogo2
     node-gyp rebuild

To include the module in your project:

     var tagg2 = require('tagg2');

## Quick example

     var tagg2 = require('tagg2'); //require the module

     var th_func = function(){

    	   console.log('i am in thread,my file path is :' + __dirname); 	
    	   //in thread you can do some cpu hard work,such as fibo.
    	
    	   thread.end("thread over"); 
    	   //when thread over, the string "thread over" will transfer to main nodejs thread.
      
         }
       
         var thread = tagg.create(th_func, function(err, res){
         //var thread = tagg.create(th_func, {buffer:buf}, callback); may transfer buffer to thread.
         
         if(err) throw(err);//thread occur some errors

         console.log(res);//this will be print "thread over"

       	 thread.destroy();//make suer to destory the thread which you have created

     });


## Thread pool

 tagg2 module support thread pool and process pool, that's will be created first,and do any other number of hard works.

 when you have too many threads, your program may be out of memory,so pool will protect you from that.

     var tagg2 = require('tagg2'); //require the module

     var buf = new Buffer('tagg2 buffer'); //create the buffer to transfer to thread

     var th_func = function(){
	
	   console.log(thread.buffer.toString()); 	
	   //this will print 'tagg2 buffer'.
	
	   thread.end("thread over"); 
	   //when thread over, the string "thread over" will transfer to main nodejs thread.
  
     }
    
     var thread = tagg2.create({poolsize:10,dirname:__dirname});//create a pool which size of 10.
   
         thread.pool(th_func, buf, function(err, res){
         //thread.pool(th_func,callback); may not transfer buffer to thread.

    	   if(err) throw(err);//thread occur some errors

    	   console.log(res);//this will be print "thread over"

    	   thread.destroy();//destory the whole thread pool

     });
   

##API

##tagg2

  object, you can get tagg2 object from require('tagg2');

##tagg2-thread and child process

tagg2 thread using pthread to create a worker thread or create a thread pool.To set options.fastthread=false, switch to child process model,you can using any nodejs api in it,make sure when the thread or child-process work finished, call the function thread.end([buf]),then the callback will be called,and the thread will return into the pool if you set a thread pool.

###tagg2.create(...)

  return a thread object, the thread object has some methods and properties follow. see below.

     ex1. tagg2.create(thread_function)     

     ex2. tagg2.create(thread_function, callback)

     ex3. tagg2.create(thread_function, options, callback)

     ex4. tagg2.create(options) //to create a thread pool
    
###options
      
  buffer:the buffer which you want to transfer to thread,default is false,create pool may not set it.

  poolsize:thread poolsize, set false not use thread pool. default is false;

  fastthread:to set false, using nodejs child process, you can use all nodejs module and api.To set true, using quick thread, you only can use js function in it, but more faster.default is true. 

  dirname:if you want to use require in thread, make sure set the correct dirname.you can also set it by created thread pool like this:

     var threadp1 = tagg.create({poolsize:10,dirname:__dirname});//the thread in pool are all using the same __dirname

  notice if fastthread is false,you have the real another node.js process,so you can do anything in it.


###callback(err,res) 
  
  every callback has the same parameter

  the first argument is error, if some error occur in the thread, the parameter will be not null;

  the second argument is result,when thread.end(result) run in the thread, the result will transfer to main thread and execute the callback; thread.end method see below;
   
###thread

  object, you can get thread object from execute tagg2.create(...).

  if you set poolsize from tagg2.create(...),like tagg2.create({poolsize:10}),return thread pool object.

###thread.id  --not support thread pool
      
  the number mark the thread or child process.

###thread.destory([true]])

  destory the thread or thread pool.make sure to execute it when you don't need the thread or pool any more.

     ex1. thread.destory();//kill the thread smooth

     ex2. thread.destory([true]); //kill the thread or child process immediately,like kill pid

  notice: if using pool, once thread.destory called,the pool will destory but not the thread, thread when work finished the thread will auto return to the pool.
	       
###thread.pool(...)   --pool only

  put some job to the idle thread in the thread pool,if all the thread is working, the job will wait.
	
	   ex1. thread.pool(thread_func)
  
	   ex2. thread.pool(thread_func, buffer)

	   ex3. thread.pool(thread_func, callback)

	   ex4. thread.pool(thread_func, buffer, callback)
      
###thread.totalThreads() --pool only
        
 returns the number of threads in this pool

     ex1. thread.totalThreads();
	
###thread.idleThreads() --pool only
      
  returns the number of threads in this pool that are currently idle (sleeping)

     ex1. thread.idleThreads();

###thread.pendingJobs() --pool only
          
  returns the number of jobs pending

     ex1. thread.pendingJobs()

###in the thread or child-process

  in every thread or child function,you can use some global objects below;

###thread.id

  the number mark the thread or child process.
  
###thread.buffer   --buffer object
        
 object,save the buffer object which has sent from main thread.

###thread.buffer.toString()

 return string; to get the buffer to utf-8 string

     ex1. thread.buffer.toString();

###thread.nextTick(function)

 Asynchronous do a job

     ex1. thread.nextTick(function(){

      console.log("Asynchronous")

      });

###thread.end([prarm])

     ex1.thread.end(); return undefined result to main thread callback;

     ex2.thread.end(reslut); return array or object or string to main thread callback;
      
###console.log(param)
      
  print the array or object or string or number etc. to stdio

     ex1. console.log("tagg2")       

###require(filepath) 

 load a js file,you can use global object to read or write the Variable in the require file

     ex1. require("./tagg2_require.js"); //make sure tagg2_require.js has in the same dir with __dirname,
 
 support "../../" , "./user", "/user".

 notice if fastthread is true, the required file not support "moudle.export", please use global.xxxx to share the function or object.

 notice. in the fast thread, there is not a really node.js runtime env,you can't require node.js module,so don't do that 'var fs = require("fs");'

###global    --in thread's global object

  save the global object in the thread like Browser's window.you can set or get some value from it.every thread has it's own global.

###__dirname
        
  the nodejs dir path which filedirname you called create the thread.the pool used the same dirname when you create the pool.
  
###user fastthread:false

  set fastthread to false will use the slow thread, it fact is a real nodejs process,tagg2 use of child.fork() to achieve
      
  all in the thread1's object and functionally
      
  when you set fastthread false, you can use all the api of node.js,There is no limit,tagg2 also provided a process pool.
 
##more
	
  see /example , /benchmark and /test for more useful code. do test please run node ./test/main_test.js

##future
  
  TAGG2 module has already in experiment,so you may not use it in production.TAGG2 will more stronger and feature-richer.

