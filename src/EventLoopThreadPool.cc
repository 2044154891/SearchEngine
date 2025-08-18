#include "EventLoopThreadPool.h"



EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{

}
EventLoopThreadPool::~EventLoopThreadPool()
{
      // Don't delete loop, it's stack variable(线程的栈区)
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for(int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf,sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t =new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); //底层创建一个线程 绑定一个新的EventLoop 并返回该loop的地址
    }
}

// 如果工作再多线程中，baseLoop_(mainLoop) 会默认以轮询的方式分配Channel给subLoop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    //如果只设置一个线程 也就是只有一个mainReactor,无subReactor
    //那么轮询只有一个线程，每次getNextLoop()每次都返回当前的baseLoop_
    EventLoop *loop = baseLoop_;

    //通过轮询获取下一个处理事件的loop
    //没有设置多线程数量，不会进入，相当于直接返回baseLoop_
    if( !loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getALLloops()// 获取所有的EventLoop
{
    if(loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
} 

