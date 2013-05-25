//2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
//queues_a_gogo.cc


 #ifdef WIN32
  #include "pthreads.2\pthread.h"
#else
  #include <pthread.h>
#endif

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>

#endif

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
//作者没用c++的类，而是使用了struct结构体来代替类
//枚举类型types
enum types{
  kItemTypeNONE, //0 
  kItemTypeNumber, //1
  kItemTypePointer, //2
  kItemTypeQUIT //3
};
struct typeQueueItem
{ //队列项结构体
  int itemType;   
  typeQueueItem* next; //指向下一个队列内容的指针
  union { 
    void* asPtr;
    double asNumber;
  };
};
//队列结构体
struct typeQueue{
  //指向队列第一个项的指针
  typeQueueItem* first;  
   //指向队列最后一个项的指针 
  typeQueueItem* last;   
  //线程互斥锁
  pthread_mutex_t queueLock;  
   //队列id号
  long int id; 
   //队列的长度，使用 volatile 声明表示每次从内存中获取队列长度，不要从cpu寄存器中拿
  volatile long int length;
};
 //初始化队列池的指针
static typeQueue* queuesPool; 
 //初始化有空项的队列指针
static typeQueue* freeItemsQueue;
//向队列中插入项，接受参数 项的指针 和待插入队列的指针，无返回值
static void queue_push(typeQueueItem* qitem, typeQueue* queue){
  //将插入到队列最后的 项的下一个指针指向NULL，0也可以
  qitem->next= NULL; 
  //对队列上锁，防止其他线程操作队列
  pthread_mutex_lock(&queue->queueLock); 
  if (queue->last) { //对过队列不为空，
    queue->last->next= qitem; //则把队列最后一项的next指针指向新加入项
  }
  else { //如果队列为空，则把队列第一个指针first指向该项
    queue->first= qitem; 
  }
  queue->last= qitem; //将队列指向last的指针指向新加入项，完成队列的first和last指针指向
  queue->length++; //将队列长度加1
  pthread_mutex_unlock(&queue->queueLock); //解除锁，让其他线程可以操作队列
};




static typeQueueItem* queue_pull (typeQueue* queue) { //队列中第一项出列，返回改项的指针
  typeQueueItem* qitem;
  
  pthread_mutex_lock(&queue->queueLock); //加锁
  if ((qitem= queue->first)) { //获取队列第一项
    if (queue->last == qitem) { //如果队列中只有一项
      queue->first= queue->last= NULL; //则把队列中的first和last指针都指向null，表示队列为空
    }
    else {
      queue->first= qitem->next; //否则把队列的first指针指向出列项的next指针所指的位置，就是当时队列的第二项
    }
    queue->length--; //队列长度减1
    qitem->next= NULL; //把出列的项的next指针设为NULL
  }
  pthread_mutex_unlock(&queue->queueLock);
  
  return qitem; //返回改出列项的指针
}



//该方法出列队列第一项，然后根据参数，对出列的那项进行 itemType 的赋值，分别赋上 double 或者 item 指针
static typeQueueItem* nuItem (int itemType, void* item) {
  
  typeQueueItem* qitem= queue_pull(freeItemsQueue); //出列空闲队列中的项
  if (!qitem) { //如果队列没有东西
    qitem= (typeQueueItem*) calloc(1, sizeof(typeQueueItem)); //calloc分配1块大小为typeQueueItem的内存，然后qitem指针指向它
  }
  
  qitem->next= NULL; 
  qitem->itemType= itemType; //itemType表示项的类型
  if (itemType == kItemTypeNumber) { //如果是 kItemTypeNumber（即为1）则把 qitem 的asNumber 赋值为 item的值
    qitem->asNumber= *((double*) item);
  }
  else if (itemType == kItemTypePointer) { //如果是 kItemTypePointer（即为2）则把 qitem 的 asPtr 赋值为 item 指针
    qitem->asPtr= item;
  }
  
  return qitem; //返回改项
}




static void destroyItem (typeQueueItem* qitem) {  //顾名思义，摧毁一个项
  
  if (freeItemsQueue) {  //如果空闲队列存在，则把此item放入
    queue_push(qitem, freeItemsQueue);
  }
  else {
    free(qitem); //否则直接free释放内存，在c++中是new/delete，这是C的用法malloc/free
  }
}




static typeQueue* nuQueue (long int id) {
  
  typeQueue* queue= NULL;
  typeQueueItem* qitem= NULL;

  if (queuesPool && queuesPool->first) qitem= queue_pull(queuesPool); //如果队列池存在，并且队列池不为空，则返回队列池中的一个项
  if (qitem) { //如果 queuesPool 存在
    queue= (typeQueue*) qitem->asPtr; //将出列的项的指针指向的队列赋值给queue指针
    destroyItem(qitem); //摧毁项
  }
  else { //如果 queuesPool 不存在
    queue= (typeQueue*) calloc(1, sizeof(typeQueue)); //则分配一个queue的内存空间
    pthread_mutex_init(&queue->queueLock, NULL); //然后初始化队列锁
  }
  
  queue->id= id; //为queue分配一个id
  queue->length= 0; //初始化队列的长度
  queue->first= queue->last= NULL; //队列清空
  return queue; //返回改队列
}


/*

static void destroyQueue (typeQueue* queue) {
  if (queuesPool) {
    queue_push(nuItem(kItemTypePointer, queue), queuesPool);
  }
  else {
    free(queue);
  }
}

*/


static void initQueues (void) { //初始化队列
  freeItemsQueue= nuQueue(-2);  //MUST be created before queuesPool，设置空闲队列id为-2
  //queuesPool= nuQueue(-1);
}




