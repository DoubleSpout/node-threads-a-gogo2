//2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
//threads_a_gogo.cc

#include <v8.h>
#include <node.h>
#include <node_version.h>
#include <node_buffer.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>

#endif

#include <string.h>
#include <string>
#include <stdio.h>

#ifdef WIN32
  #include "pthreads.2\pthread.h"
#else
  #include <pthread.h>
#endif

#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>


#include "queues_a_gogo.cpp" // queues_a_gogo 主要实现了线程池的队列


//using namespace node;
using namespace v8;

static Persistent<String> id_symbol; //全局的静态变量string，代表线程的id号
static Persistent<ObjectTemplate> threadTemplate; //对js提供接口的 线程 类，这里使用 Persistent<T>
static bool useLocker;  //用户锁

static typeQueue* freeJobsQueue= NULL;  //空闲任务队列
static typeQueue* freeThreadsQueue= NULL; //空闲线程队列

#define kThreadMagicCookie 0x99c0ffee //线程加上标示





/*
关于使用到的libuv 的 uv_async_t说明
文档地址： https://github.com/joyent/libuv/blob/master/include/uv.h


 * uv_async_t is a subclass of uv_handle_t.
 *
 * uv_async_send wakes up the event loop and calls the async handle's callback.
 * There is no guarantee that every uv_async_send call leads to exactly one
 * invocation of the callback; the only guarantee is that the callback function
 * is called at least once after the call to async_send. Unlike all other
 * libuv functions, uv_async_send can be called from another thread.
 */
//struct uv_async_s {
//  UV_HANDLE_FIELDS
//  UV_ASYNC_PRIVATE_FIELDS
//};

//UV_EXTERN int uv_async_init(uv_loop_t*, uv_async_t* async,
//    uv_async_cb async_cb);

/*
 * This can be called from other threads to wake up a libuv thread.
 *
 * libuv is single threaded at the moment.
 */
// UV_EXTERN int uv_async_send(uv_async_t* async);






typedef struct {  //一个线程结构体的定义 typeThread

  uv_async_t async_watcher; //MUST be the first one 使用 libuv

  
  long int id;     //线程id
  pthread_t thread; //线程标示
  volatile int sigkill; //volatile声明表示总是从内存中去读取变量
  
  typeQueue inQueue;  //Jobs to run
  typeQueue outQueue; //Jobs done
  
  volatile int IDLE; //闲置
  pthread_cond_t IDLE_cv;      //线程状态
  pthread_mutex_t IDLE_mutex;  //线程锁
  
  Isolate* isolate;   //V8的isolate类
  Persistent<Context> context;  //上下文
  Persistent<Object> JSObject;  //js对象
  Persistent<Object> threadJSObject;  //线程对象
  Persistent<Object> dispatchEvents;  //派遣事件
  
  unsigned long threadMagicCookie;  //标识线程是TAGG创建
} typeThread; 

enum jobTypes { //任务的类型
  kJobTypeEval, //0
  kJobTypeEvent  //1
};

typedef struct {
  int jobType;
  Persistent<Object> cb; //任务的回调函数
  char *buf_ptr; //传入的缓存buffer指针，modify by snoopy
  String::Utf8Value* __dirname; //当前执行地址	
  int buf_len;
  union {
//任务结构体分2种

    struct { //第1种是事件任务
      int length; 
      String::Utf8Value* eventName; //事件名称
      String::Utf8Value** argumentos; //事件的参数
    } typeEvent;

    struct { //第2种是任务执行
      int error;  //标识是否出错
      int tiene_callBack; //是否有回调
      int useStringObject; //是否使用字符串对象
      String::Utf8Value* resultado; //返回值字符串
      union {
        char* scriptText_CharPtr; //字符串
        String::Utf8Value* scriptText_StringObject; //字符串对象
      };
    } typeEval;

  };
} typeJob; //任务的结构体

/*

cd deps/minifier/src
gcc minify.c -o minify
cat ../../../src/events.js | ./minify kEvents_js > ../../../src/kEvents_js
cat ../../../src/load.js | ./minify kLoad_js > ../../../src/kLoad_js
cat ../../../src/createPool.js | ./minify kCreatePool_js > ../../../src/kCreatePool_js
cat ../../../src/thread_nextTick.js | ./minify kThread_nextTick_js > ../../../src/kThread_nextTick_js

*/

#include "events.js.c"
//#include "load.js.c"
#include "createPool.js.c"
#include "thread_nextTick.js.c"
#include "object_toString.js.c"
#include "path_resolve.js.c"
//#include "JASON.js.c"



static char* readFile (Handle<String> path);

static Handle<Value> require_file(const Arguments &args) //读取js文件
{
	HandleScope scope;
	
		if(!args[0]->IsString()){
		return ThrowException(Exception::TypeError(String::New("require(filepath): filapath must needed")));
	}

	Local<Object> Gobj = args.This();

	String::Utf8Value* dirname_p = (String::Utf8Value*) Gobj->GetPointerFromInternalField(0);
	std::string dirname = **dirname_p;


	//c++利用js函数加工处理参数，这里让c++利用 _object_toString
	
	Local<Object> path_resolve_obj = Script::Compile(String::New(path_resolve_js))->Run()->ToObject();
#ifdef WIN32
	Local<Object> path_resolve_func = path_resolve_obj->Get(String::New("resolve_win"))->ToObject();
#else
	Local<Object> path_resolve_func = path_resolve_obj->Get(String::New("resolve_linux"))->ToObject();
#endif   
	Local<Value> argv[2];
    argv[0] = String::New(dirname.c_str());
	argv[1] = args[0];

	Local<Value> path_str_value = path_resolve_func->CallAsFunction(Object::New(), 2, argv);


	String::Utf8Value utf8_value(path_str_value->ToString());
	std::string path_str = *utf8_value;


	char *file = readFile(String::New(path_str.c_str()));
	
	TryCatch onError;
	Script::Compile(String::New(file))->Run();

	if(onError.HasCaught()){
		std::string err_str= "require has error";
		
		err_str += path_str;
		return ThrowException(Exception::TypeError(String::New(err_str.c_str())));
	}
	
	return scope.Close(Gobj);

}




static typeQueueItem* nuJobQueueItem (void) { //从闲置任务队列出列一项
  typeQueueItem* qitem= queue_pull(freeJobsQueue);
  if (!qitem) { //如果闲置队列没有内容，则创建一个项，调用queues_a_gogo.cc里面的 nuItem 方法
    qitem= nuItem(kItemTypePointer, calloc(1, sizeof(typeJob)));
  }
  return qitem; //返回一个队列项，并且asptr属性指向 typeJob 类型
}






static typeThread* isAThread (Handle<Object> receiver) { //接受 Handle<Object> 判断这个对象是不是线程对象
  typeThread* thread;
  
  if (receiver->IsObject()) { 
    if (receiver->InternalFieldCount() == 1) { //如果内部的属性为1个
      thread= (typeThread*) receiver->GetPointerFromInternalField(0); //获得内部的属性，0表示第一个
      if (thread && (thread->threadMagicCookie == kThreadMagicCookie)) { //如果thread存在，并且thread的 threadMagicCookie 属性正确，则返回thread指针
        return thread;
      }
    }
  }
  
  return NULL;
}






static void pushToInQueue (typeQueueItem* qitem, typeThread* thread) { //将一个线程入队列
  pthread_mutex_lock(&thread->IDLE_mutex); //加锁
  queue_push(qitem, &thread->inQueue); //入队列
  if (thread->IDLE) { //如果线程闲置
    pthread_cond_signal(&thread->IDLE_cv); //唤醒线程
  }
  pthread_mutex_unlock(&thread->IDLE_mutex); //解锁
}





 //这个方法是打印信息用的，将会打印所有的js参数
static Handle<Value> Puts (const Arguments &args) {
  //fprintf(stdout, "*** Puts BEGIN\n");
  
  HandleScope scope;
  
  Local<Value> argv[1];
  argv[0] = args[0];
 
  //c++利用js函数加工处理参数，这里让c++利用 _object_toString
  Local<Object> object_toString_func = Script::Compile(String::New(object_toString_js))->Run()->ToObject();

  Local<Value> put_str = object_toString_func->CallAsFunction(Object::New(), 1, argv);
  
  String::Utf8Value c_str(put_str);

  std::cout<<*c_str<<std::endl;
  std::cout.flush();

  //fprintf(stdout, "*** Puts END\n");
  return Undefined();
}


 //这个方法是打印信息用的，将会打印所有的js参数
static Handle<Value> Puts_Error (const Arguments &args) {
  
  HandleScope scope;
  
  if(args.Length()){
    return ThrowException(Exception::TypeError(args[0]->ToString()));
  }


  return Undefined();
}


//模拟node官方的toString方法
static Handle<Value> Buffer_toString (const Arguments &args) {
  
  HandleScope scope;

  Local<Object> buf = args.This();

  int has_buf = buf->GetHiddenValue(String::New("isBuffer"))->Int32Value();
 
  if(has_buf){
    return scope.Close(buf->GetHiddenValue(String::New("buffer"))->ToString());
  }
  else{
	return scope.Close(Undefined());
  }
  
}

//模拟node官方的toString方法
static Handle<Value> threadEnd (const Arguments &args) {
  
  HandleScope scope;
  
  Local<Object> th_obj = args.This();

  Local<Value> argv[1];

  argv[0] = args[0];
 
  //c++利用js函数加工处理参数，这里让c++利用 _object_toString
  Local<Object> object_toString_func = Script::Compile(String::New(object_toString_js))->Run()->ToObject();

  Local<Value> put_str = object_toString_func->CallAsFunction(Object::New(), 1, argv); 
  
  if(put_str->IsString()){
    th_obj->Set(String::New("_TAGG_RES"), put_str->ToString());
  }
  else{
    th_obj->Set(String::New("_TAGG_RES"), String::New(""));
  } 


  return scope.Close(Undefined());
    
}





static void eventLoop (typeThread* thread); //函数原型

// A background thread
//创建线程的回调 pthread_create 的回调
static void* aThread (void* arg) { 
  
  int dummy;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
  
  typeThread* thread= (typeThread*) arg; //将参数arg转变为 typeThread 指针

  //static Isolate *  New ()
  //创建一个新的 v8 运行时
  thread->isolate= Isolate::New(); 
  //将thread指针嵌入 Isolate, 之后要使用getData
  thread->isolate->SetData(thread);
  
  if (useLocker) { //如果当前V8实例使用了 useLocker
    //printf("**** USING LOCKER: YES\n");
    v8::Locker myLocker(thread->isolate); //则在 刚才新建的 isolate 中也使用 useLocker
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  else { //否则不使用 useLocker
    //printf("**** USING LOCKER: NO\n");
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  thread->isolate->Exit(); //退出 isolate
  thread->isolate->Dispose(); //销毁 isolate
  
  // wake up callback

  uv_async_send(&thread->async_watcher);

  
  return NULL;
}













static Handle<Value> threadEmit (const Arguments &args);

//事件循环
//TAGG核心代码程序
static void eventLoop (typeThread* thread) {
  thread->isolate->Enter(); //进入 isolate
  thread->context= Context::New(); //创建一个sandbox沙箱用来执行js代码，他的上下文将有他自己控制
  thread->context->Enter(); //进入这个上下文
  
  {
    HandleScope scope1;
    
    //返回这个分离的沙箱内的全局对象
    //Local< Object >   Global ()
    Local<Object> global= thread->context->Global();

	global->Set(String::NewSymbol("_Global"), global);
    
	//将puts方法设置到全局中去，代替console.log
	Handle<Object> console_obj = Object::New();
	console_obj->Set(String::New("log"), FunctionTemplate::New(Puts)->GetFunction());
	

    global->Set(String::New("console"), console_obj);
	global->Set(String::New("require"), FunctionTemplate::New(require_file)->GetFunction(), ReadOnly);
  
	
	//定义一个thread对象到全局变量中
    Local<Object> threadObject= Object::New();
    global->Set(String::NewSymbol("thread"), threadObject);
    
    //设置这个全局对象thread的id和emit属性
    threadObject->Set(String::NewSymbol("id"), Number::New(thread->id));
    threadObject->Set(String::NewSymbol("emit"), FunctionTemplate::New(threadEmit)->GetFunction());
    threadObject->Set(String::NewSymbol("end"), FunctionTemplate::New(threadEnd)->GetFunction());
    threadObject->Set(String::New("_TAGG_RES"), Undefined()); //返回的全局变量字符串
    
	//将global对象放入全局global中，每次都会初始化
	Handle<Object> user_global = Object::New();
	global->Set(String::NewSymbol("global"), user_global);
	global->Set(String::NewSymbol("Global"), user_global);
	
	
	//让threadObject继承event接口
    Local<Object> dispatchEvents= Script::Compile(String::New(kEvents_js))->Run()->ToObject()->CallAsFunction(threadObject, 0, NULL)->ToObject();
	
	//获得下个事件循环的函数
    Local<Object> dispatchNextTicks= Script::Compile(String::New(kThread_nextTick_js))->Run()->ToObject();
	


    //获得thread_nextTick.js中定义的 next回调函数的数组
    Local<Array> _ntq= (v8::Array*) *threadObject->Get(String::NewSymbol("_ntq"));
   

    double nextTickQueueLength= 0; //事件循环次数
    long int ctr= 0;
    
    //SetFatalErrorHandler(FatalErrorCB);
    
    while (!thread->sigkill) { //当线程的信号不为kill时，则循环执行下面代码
      typeJob* job;
      typeQueueItem* qitem;
      
      {//while循环代码块


        HandleScope scope2;
        //v8的TryCatch类，
        //一个外部的异常控制类
        TryCatch onError;    
        String::Utf8Value* str;//临时保存js源代码的指针
        Local<String> source; //保存js源代码字符串
        Local<Script> script; //保存js源代码
        Local<Value> resultado; //保存js源代码执行的return 结果值
        
        
        while ((qitem= queue_pull(&thread->inQueue))) { //当队列中有项目时，循环执行如下代码
          
          job= (typeJob*) qitem->asPtr; //队列中的任务
          
          if ((++ctr) > 2e3) { //如果ctr 大于 2*10的3次方
            ctr= 0;
            V8::IdleNotification(); //强制V8进行GC while(!V8::IdleNotification()) {};
          }
          
          if (job->jobType == kJobTypeEval) { //如果执行的任务
            //Ejecutar un texto
            
            if (job->typeEval.useStringObject) { //如果是eval方法传入的参数进来的
              str= job->typeEval.scriptText_StringObject;
              source= String::New(**str, (*str).length()); 
              delete str; //删除str指针
            }
            else { //如果是load js 文件进来的
              source= String::New(job->typeEval.scriptText_CharPtr);//创建js源代码string，
              free(job->typeEval.scriptText_CharPtr); //释放
            }
            
            script= Script::New(source); //将源代码字符串，转存为js代码
            
            //这里进行判断，如果上下问里的js代码执行抛出了异常，则本次循环将不执行js代码了
            
			//将global对象放入全局global中，每次都会初始化
			Handle<Object> user_global = Object::New();
			global->Set(String::NewSymbol("global"), user_global);
			global->Set(String::NewSymbol("Global"), user_global);
            
			if (!onError.HasCaught()){  //如果没有错误


				//Local<Object> obj = job->buffer->Clone();
				//resultado = String::NewSymbol("id");
				//将char* 的job->buf_ptr又转换为Buffer指针
				//node::Buffer *buf = node::Buffer::New(job->buf_ptr, job->buf_len); 					
				//Local<Object> buf_obj = External::Wrap(buf)->ToObject();				
				threadObject->Set(String::New("_TAGG_RES"), Undefined()); //返回的全局变量字符串

				Local<Object> buf_obj = Object::New();
				

				if(job->buf_ptr){
					buf_obj->SetHiddenValue(String::New("buffer"), String::New(job->buf_ptr, job->buf_len));
					buf_obj->SetHiddenValue(String::New("isBuffer"),Number::New(1));
				}
				else{
					buf_obj->SetHiddenValue(String::New("isBuffer"),Number::New(0));
				}

				buf_obj->Set(String::NewSymbol("toString"), FunctionTemplate::New(Buffer_toString)->GetFunction());
				
				threadObject->Set(String::NewSymbol("buffer"),  buf_obj);				
				
				global->Set(String::NewSymbol("__dirname"), String::New(**job->__dirname));
				global->SetPointerInInternalField(0, job->__dirname);
				  

				script->Run(); //执行js代码，返回值为 resultado
			    resultado = threadObject->Get(String::New("_TAGG_RES"));
				

		   }//如果没有错误

            if (_ntq->Length() && !onError.HasCaught()) { //当有错误时，不执行异步
            
                  if ((++ctr) > 2e3) {
                      ctr= 0;
                      V8::IdleNotification();//强制GC
                  }

                  resultado = dispatchNextTicks->CallAsFunction(global, 0, NULL); //调用线程内的 nexttick
			
            }

            if (job->typeEval.tiene_callBack) { //如果执行任务具有回调函数
              //如果有错误，则 job->typeEval.error 是1，否则为0；
              job->typeEval.error= onError.HasCaught() ? 1 : 0; 
			  if(job->typeEval.error){
					 _ntq= Array::New();
			  }
              //如果有异常，则返回值 resultado 为异常内容，否则为函数返回内容
              job->typeEval.resultado= new String::Utf8Value(job->typeEval.error ? onError.Exception() : threadObject->Get(String::New("_TAGG_RES")));
              //执行完毕，将qtiem项丢入线程的出列队列
              queue_push(qitem, &thread->outQueue);
              // wake up callback
              //丢入异步队列

              uv_async_send(&thread->async_watcher);

            }
            else { //如果没有回调函数，则把改item丢入闲置任务队列
              queue_push(qitem, freeJobsQueue);
            }

            if (onError.HasCaught()){
				nextTickQueueLength= 1;
				onError.Reset(); //如果此次执行有错误，则清空
			}
			else{
				 nextTickQueueLength= resultado->NumberValue();
			}

     }//如果执行的任务


          else if (job->jobType == kJobTypeEvent) { //如果是事件的任务
            //Emitir evento.
            
            Local<Value> args[2]; //定义一个数组长度为2，保存 Local<Value> 类型的值
            str= job->typeEvent.eventName; //获得事件名字
            args[0]= String::New(**str, (*str).length()); //数组第一个保存event事件的名字，保存为local<string>类型
            delete str;
            
            Local<Array> array= Array::New(job->typeEvent.length);
            args[1]= array; //设置参数长度
            
            int i= 0;
            while (i < job->typeEvent.length) { //将参数保存入Local<Array>
              str= job->typeEvent.argumentos[i];
              array->Set(i, String::New(**str, (*str).length()));
              delete str;
              i++;
            }
            
            free(job->typeEvent.argumentos); //释放任务的 typeEvent 内存
            queue_push(qitem, freeJobsQueue); //将本项丢入闲置任务队列
            dispatchEvents->CallAsFunction(global, 2, args); //执行回调函数，并且加入参数
          



          }//如果是事件的任务


        }//while2 结束

	    
		/* 
		if (_ntq->Length()) { //执行异步
				  
				   if ((++ctr) > 2e3) {
                      ctr= 0;
                      V8::IdleNotification();//强制GC
                  }

                  resultado = dispatchNextTicks->CallAsFunction(global, 0, NULL); //调用线程内的 nexttick
                  if (onError.HasCaught()) { //如果有错误
                      nextTickQueueLength= 1; //nexttick队列长度为1
                      onError.Reset();
                  }
                  else {
                      nextTickQueueLength= resultado->NumberValue();
                  }
            }
		*/



      if (thread->inQueue.length) continue;
      if (thread->sigkill) break; //如果收到线程杀死信号，条春循环
      
      pthread_mutex_lock(&thread->IDLE_mutex); //锁住线程
      if (!thread->inQueue.length) { //如果线程的入队列长度为0
        thread->IDLE= 1; //则把线程设置为闲置
        pthread_cond_wait(&thread->IDLE_cv, &thread->IDLE_mutex); //休眠线程
        thread->IDLE= 0;
      }
      pthread_mutex_unlock(&thread->IDLE_mutex); //解锁线程




    }//while 结束



  }
  
  thread->context.Dispose(); //摧毁上下文
}

}




static void destroyaThread (typeThread* thread) { //摧毁一个线程
  
  thread->sigkill= 0;
  //TODO: hay que vaciar las colas y destruir los trabajos antes de ponerlas a NULL
  thread->inQueue.first= thread->inQueue.last= NULL;
  thread->outQueue.first= thread->outQueue.last= NULL;
  thread->JSObject->SetPointerInInternalField(0, NULL);
  thread->JSObject.Dispose();
  

  uv_close((uv_handle_t*) &thread->async_watcher, NULL);
  //uv_unref(&thread->async_watcher);

  
  if (freeThreadsQueue) { //如果有闲置线程队列则丢进去
    queue_push(nuItem(kItemTypePointer, thread), freeThreadsQueue);
  }
  else {//如果没有则摧毁它
    free(thread);
  }
}






// C callback that runs in the main nodejs thread. This is the one responsible for
// calling the thread's JS callback.
//线程丢入唤醒的回调
//

static void Callback ( 

  uv_async_t *watcher

, int revents) 

{
  typeThread* thread= (typeThread*) watcher;
  
  if (thread->sigkill) {
    destroyaThread(thread); //杀死线程
    return;
  }
  
  HandleScope scope;
  typeJob* job;
  Local<Value> argv[2];
  Local<Value> null= Local<Value>::New(Null());
  typeQueueItem* qitem;
  String::Utf8Value* str;
  
  TryCatch onError;
  while ((qitem= queue_pull(&thread->outQueue))) { //丢入到主线程的回调
    job= (typeJob*) qitem->asPtr;

    if (job->jobType == kJobTypeEval) { //如果是执行

      if (job->typeEval.tiene_callBack) { //如果任务具有回调函数
        str= job->typeEval.resultado; //线程执行的结果

        if (job->typeEval.error) { //如果线程执行有错误
          argv[0]= Exception::Error(String::New(**str, (*str).length())); //第一个参数是error
          argv[1]= null; //第二个参数是null
        } else {
          argv[0]= null; //如果没有错误，第一个参数是null
          argv[1]= String::New(**str, (*str).length()); //第二个参数是线程执行结果
        }
        job->cb->CallAsFunction(thread->JSObject, 2, argv); //调用任务的回调函数，并且上下文是 thread->JSObject
        job->cb.Dispose(); //释放 Persistent 资源，不然可能内存泄露
        job->typeEval.tiene_callBack= 0; //将改任务的回调设置0，表示没有回调了
		
        delete str; //删除str指针
		delete job->__dirname;//删除dirname的指针

		job->buf_ptr = 0; //将buffer指针重置
		job->buf_len = 0; //将buffer长度重置

        job->typeEval.resultado= NULL; //将线程执行结果的返回值设置为null
      }

      queue_push(qitem, freeJobsQueue); //将任务丢入闲置任务队列
      
      if (onError.HasCaught()) { //如果在执行回调时出错
        if (thread->outQueue.first) {

          uv_async_send(&thread->async_watcher); // wake up callback again

        }
        node::FatalException(onError);
        return;
      }
    }
    else if (job->jobType == kJobTypeEvent) { //如果是事件的回调
      
      //fprintf(stdout, "*** Callback\n");
      
      Local<Value> args[2];
      
      str= job->typeEvent.eventName;
      args[0]= String::New(**str, (*str).length());
      delete str;
      
      Local<Array> array= Array::New(job->typeEvent.length);
      args[1]= array;
      
      int i= 0;
      while (i < job->typeEvent.length) {
        str= job->typeEvent.argumentos[i];
        array->Set(i, String::New(**str, (*str).length()));
        delete str;
        i++;
      }
      
      free(job->typeEvent.argumentos);
      queue_push(qitem, freeJobsQueue);
      thread->dispatchEvents->CallAsFunction(thread->JSObject, 2, args); //执行回调函数
    }
  }
}






// unconditionally destroys a thread by brute force.
static Handle<Value> Destroy (const Arguments &args) {//调用摧毁线程
  HandleScope scope;
  //TODO: Hay que comprobar que this en un objeto y que tiene hiddenRefTotypeThread_symbol y que no es nil
  //TODO: Aquí habría que usar static void TerminateExecution(int thread_id);
  //TODO: static void v8::V8::TerminateExecution  ( Isolate *   isolate= NULL   )   [static]
  
  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.destroy(): the receiver must be a thread object")));
  }
  
  if (!thread->sigkill) {
    //pthread_cancel(thread->thread);
    thread->sigkill= 1; 
    pthread_mutex_lock(&thread->IDLE_mutex);
    if (thread->IDLE) {
      pthread_cond_signal(&thread->IDLE_cv);
    }
    pthread_mutex_unlock(&thread->IDLE_mutex);
  }
  
  return Undefined();
}






// Eval: Pushes a job into the thread's ->inQueue.
//args[0] thread func
//args[1] buffer
//args[2] callback
//args[3] __dirname
static Handle<Value> Eval (const Arguments &args) {
  HandleScope scope;
  
  //合法性判断
  if (!args.Length() || args.Length() != 4){
       return ThrowException(Exception::TypeError(String::New("thread.eval(program,buf,callback,dirname): arguments must be 4")));
  }

  if (!args[0]->IsString()){
       return ThrowException(Exception::TypeError(String::New("thread.eval(program,buf,callback,dirname): arguments[0] must be string")));
  }

  if (!args[1]->IsObject()){
       return ThrowException(Exception::TypeError(String::New("thread.eval(program,buf,callback,dirname): arguments[1] must be buffer object or number")));
  }

  if (!args[2]->IsFunction()){
       return ThrowException(Exception::TypeError(String::New("thread.eval(program,buf,callback,dirname): arguments[2] must be a function")));
  }

   if (!args[3]->IsString()){
       return ThrowException(Exception::TypeError(String::New("thread.eval(program,buf,callback,dirname): arguments[3] must be a function")));
  }



  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.eval(): the receiver must be a thread object")));
  }

  
  typeQueueItem* qitem= nuJobQueueItem(); //如果有闲置线程队列，则获取一个，如果没有则创建一个
  typeJob* job= (typeJob*) qitem->asPtr; //赋值任务指针
  
  job->typeEval.tiene_callBack= true; //是具有回调,修改api之后，这里永远有回掉
  if (job->typeEval.tiene_callBack) {
    job->cb= Persistent<Object>::New(args[2]->ToObject()); //保存用户上传的回调函数
  }

  //这里是修改重点，这类保存用户传上来的 字符串参数  program
  job->typeEval.scriptText_StringObject= new String::Utf8Value(args[0]); //将program参数转化成utf-8，使用new来分配内存
  job->typeEval.useStringObject= 1; //使用字符串表示对象
  job->jobType = kJobTypeEval; //job的类型是执行线程，而不是事件

  
  //args[1]必须是buffer类型
  int buf_len =  node::Buffer::Length(args[1].As<Object>());
  if(buf_len == 0){
	job->buf_ptr = 0;
	job->buf_len = 0;
  }
  else{
	job->buf_ptr = node::Buffer::Data(args[1].As<Object>()); //接受参数buffer用来传递给线程，modify by snoopy
	job->buf_len = buf_len;//接受参数buffer用来传递给线程，modify by snoopy
  }
  job->__dirname = new String::Utf8Value(args[3]);//保存dirname

  pushToInQueue(qitem, thread); //将次qitem项入队列
  return scope.Close(args.This()); //返回js调用对象本身
}





static char* readFile (Handle<String> path) { //读取文件
  v8::String::Utf8Value c_str(path);
  FILE* fp= fopen(*c_str, "rb");
  if (!fp) {
    fprintf(stderr, "Error opening the file %s\n", *c_str);
    //@bruno: Shouldn't we throw, here ?
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  long len= ftell(fp);
  rewind(fp); //fseek(fp, 0, SEEK_SET);
  char *buf= (char*) calloc(len + 1, sizeof(char)); // +1 to get null terminated string
  fread(buf, len, 1, fp);
  fclose(fp);
  /*
  printf("SOURCE:\n%s\n", buf);
  fflush(stdout);
  */
  return buf;
}






// Load: Loads from file and passes to Eval
static Handle<Value> Load (const Arguments &args) { //从文件中读取代码
  HandleScope scope;

  if (!args.Length()) {
    return ThrowException(Exception::TypeError(String::New("thread.load(filename [,callback]): missing arguments")));
  }

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.load(): the receiver must be a thread object")));
  }
  
  char* source= readFile(args[0]->ToString());  //@Bruno: here we don't know if the file was not found or if it was an empty file
  if (!source) return scope.Close(args.This()); //@Bruno: even if source is empty, we should call the callback ?

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->typeEval.tiene_callBack= ((args.Length() > 1) && (args[1]->IsFunction()));
  if (job->typeEval.tiene_callBack) {
    job->cb= Persistent<Object>::New(args[1]->ToObject());
  }
  job->typeEval.scriptText_CharPtr= source; //保存从文件读取的js代码
  job->typeEval.useStringObject= 0;
  job->jobType= kJobTypeEval;

  pushToInQueue(qitem, thread);

  return scope.Close(args.This());
}




//主进程触发
static Handle<Value> processEmit (const Arguments &args) { 
  HandleScope scope;
  
  //fprintf(stdout, "*** processEmit\n");
  
  if (!args.Length()) return scope.Close(args.This());
  
  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.emit(): the receiver must be a thread object")));
  }
  
  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;
  
  job->jobType= kJobTypeEvent;
  job->typeEvent.length= args.Length()- 1;
  job->typeEvent.eventName= new String::Utf8Value(args[0]);
  job->typeEvent.argumentos= (v8::String::Utf8Value**) malloc(job->typeEvent.length* sizeof(void*));
  
  int i= 1;
  do {
    job->typeEvent.argumentos[i-1]= new String::Utf8Value(args[i]);
  } while (++i <= job->typeEvent.length);
  
  pushToInQueue(qitem, thread);
  
  return scope.Close(args.This()); 
}





//线程触发
static Handle<Value> threadEmit (const Arguments &args) {
  HandleScope scope;
  
  //fprintf(stdout, "*** threadEmit\n");
  
  if (!args.Length()) return scope.Close(args.This());
  
  int i;
  //Isolate 提供一个独立的V8引擎实例
  //v8 Isolate 是一个完全独立的状态，在一个 Isolate 中是无法访问其他Isolate中的对象的
  //v8在初始化时已经创建了并且进入了一个 Isolate
  //嵌入式开发者可以自己添加 Isolate，并且让他们并行运行在不同的线程中，一个 Isolate 在任一时刻只能被一个线程访问
  //Locker/Unlocker api会被同步执行
  typeThread* thread= (typeThread*) Isolate::GetCurrent()->GetData(); //获取进入线程的 Isolate 并且返回 v8::Isolate::SetData 的指针
  
  typeQueueItem* qitem= nuJobQueueItem(); //穿件一个job
  typeJob* job= (typeJob*) qitem->asPtr; //初始化job
  
  job->jobType= kJobTypeEvent;
  job->typeEvent.length= args.Length()- 1;
  job->typeEvent.eventName= new String::Utf8Value(args[0]);
  job->typeEvent.argumentos= (v8::String::Utf8Value**) malloc(job->typeEvent.length* sizeof(void*));
  
  i= 1;
  do {
    job->typeEvent.argumentos[i-1]= new String::Utf8Value(args[i]);
  } while (++i <= job->typeEvent.length);
  
  queue_push(qitem, &thread->outQueue); //将队列项丢入输出队列
  

  uv_async_send(&thread->async_watcher); // wake up callback

  
  //fprintf(stdout, "*** threadEmit END\n");
  
  return scope.Close(args.This());
}








// Creates and launches a new isolate in a new background thread.
//创建并启动一个新的 isolate v8 运行时
static Handle<Value> Create (const Arguments &args) {
    HandleScope scope;
    
    typeThread* thread; 
    typeQueueItem* qitem= NULL;


    qitem= queue_pull(freeThreadsQueue); //从闲置队列中获取一个item
    if (qitem) {//如果获取到了这个item，则把这个item指向的线程赋值给thread指针
      thread= (typeThread*) qitem->asPtr;
      destroyItem(qitem); //摧毁这个item
    }
    else { //如果没有找到
      thread= (typeThread*) calloc(1, sizeof(typeThread)); //则创建一个空的 thread 指针指向 typeThread
      thread->threadMagicCookie= kThreadMagicCookie; //设置这个 thread 的标示
    }
    
    static long int threadsCtr= 0; //静态变量 threadsCtr ，用来做线程id的标示，每次创建都 +1
    thread->id= threadsCtr++;
    
    //为thread的jsobject实例设置属性，返回一个 threadTemplate 的实例 Local< Object >
    //然后调用  
    //Persistent<T> v8::Persistent<T>::New(Handle<T> that)   [inline, static]
    //将Handle<T> 改变为 Persistent<Object>
    thread->JSObject= Persistent<Object>::New(threadTemplate->NewInstance());
    //设置一个属性id在js对象中，让js对象可以访问
    //id_symbol 为 Persistent<String>::New(String::NewSymbol("id"));
    thread->JSObject->Set(id_symbol, Integer::New(thread->id));
    //SetPointerInInternalField (int index, void *value)
    //设置一个内部c++指针在js的对象中
    thread->JSObject->SetPointerInInternalField(0, thread);

/*
    这里比较复杂，需要一点点分析了
    1.static Local<Script> v8::Script::Compile  ( Handle< String >  source,
    ScriptOrigin *  origin = NULL,
    ScriptData *  pre_data = NULL,
    Handle< String >  script_data = Handle< String >()   
    )
    第一步compile将返回 Local<Script> 
    第二步run()将返回 kEvents_js 返回的内容，
    第三步将返回的内容，转为ToObject(), Local< Object >
    第四部调用CallAsFunction，环境对象使用 threadTemplate 实例，这样就继承了 events 对象
*/
    Local<Value> dispatchEvents= Script::Compile(String::New(kEvents_js))->Run()->ToObject()->CallAsFunction(thread->JSObject, 0, NULL);

    //将thread的 dispatchEvents 指针指向   Persistent<Object> 类型的 dispatchEvents
    thread->dispatchEvents= Persistent<Object>::New(dispatchEvents->ToObject());
    



    //注册libuv和callback
    //当执行 uv_async_send 时，就会调用注册进入的callback

    uv_async_init(uv_default_loop(), &thread->async_watcher, Callback);

    
    pthread_cond_init(&thread->IDLE_cv, NULL);
    pthread_mutex_init(&thread->IDLE_mutex, NULL);
    pthread_mutex_init(&thread->inQueue.queueLock, NULL);
    pthread_mutex_init(&thread->outQueue.queueLock, NULL);


    if (pthread_create(&thread->thread, NULL, aThread, thread)) { //如果创建线程不成功
      //Ha habido un error no se ha arrancado este hilo
      destroyaThread(thread);
      return ThrowException(Exception::TypeError(String::New("create(): error in pthread_create()")));
    }
    
    /*
    V8::AdjustAmountOfExternalAllocatedMemory(sizeof(typeThread));  //OJO V8 con V mayúscula.
    */
    
    return scope.Close(thread->JSObject);
}








void Init (Handle<Object> target) {
  
  initQueues();
  freeThreadsQueue= nuQueue(-3); //闲置线程队列的 id为 -3
  freeJobsQueue= nuQueue(-4); //闲置任务队列的id 为-4
  
  HandleScope scope;
  
  //static bool   IsActive ()
  //将返回当前V8的运行实例是否加锁
  useLocker= v8::Locker::IsActive(); 
  

  target->Set(String::NewSymbol("create"), FunctionTemplate::New(Create)->GetFunction());
  target->Set(String::NewSymbol("createPool"), Script::Compile(String::New(kCreatePool_js))->Run()->ToObject());
  //target->Set(String::NewSymbol("JASON"), Script::Compile(String::New(kJASON_js))->Run()->ToObject());
  
  //设置js访问线程id的属性名为‘id’
  id_symbol= Persistent<String>::New(String::NewSymbol("id"));
  
  //定义 threadTemplate 的一些属性
  threadTemplate= Persistent<ObjectTemplate>::New(ObjectTemplate::New());
  threadTemplate->SetInternalFieldCount(1);
  threadTemplate->Set(id_symbol, Integer::New(0));
  threadTemplate->Set(String::NewSymbol("eval"), FunctionTemplate::New(Eval));
  //threadTemplate->Set(String::NewSymbol("load"), FunctionTemplate::New(Load));
  threadTemplate->Set(String::NewSymbol("emit"), FunctionTemplate::New(processEmit));
  threadTemplate->Set(String::NewSymbol("destroy"), FunctionTemplate::New(Destroy));
}




NODE_MODULE(threads_a_gogo, Init)

/*
gcc -E -I /Users/jorge/JAVASCRIPT/binarios/include/node -o /o.c /Users/jorge/JAVASCRIPT/threads_a_gogo/src/threads_a_gogo.cc && mate /o.c
*/