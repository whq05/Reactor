#include "ThreadPool.h"

// 在构造函数中将启动threadnum个线程
ThreadPool::ThreadPool(size_t threadnum) : stop_(false)
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上
    for (size_t ii = 0; ii < threadnum; ii++)
    {
        // 用lambda函创建线程
        threads_.emplace_back([this]
        {
            printf("create thread(%ld).\n",syscall(SYS_gettid));     // 显示线程ID

            while (stop_ == false)
            {
                std::function<void()> task;         // 用于存放出队的元素

                {   // 锁作用域的开始。 ///////////////////////////////////
                    std::unique_lock<std::mutex> lock(this->mutex_);

                    // 等待生产者的条件变量
                    this->condition_.wait(lock, [this]
                    {
                        return ((this->stop_ == true) || (this->taskqueue_.empty() == false));
                    });
                    
                    // 在线程池停止之前，如果队列中还有任务，执行完再退出
                    if ((this->stop_ == true) && (this->taskqueue_.empty() == true)) return;
                    
                    // 出队一个任务
                    task = std::move(this->taskqueue_.front());
                    this->taskqueue_.pop();
                }   // 锁作用域的结束。 ///////////////////////////////////

                printf("thread is %ld.\n",syscall(SYS_gettid));
                task(); // 执行任务
            }
        });
    }
}

// 把任务添加到队列中
void ThreadPool::addtask(std::function<void()> task)
{
    {   // 锁作用域的开始。 ///////////////////////////////////
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }   // 锁作用域的结束。 ///////////////////////////////////

    condition_.notify_one(); // 唤醒一个线程
}

// 在析构函数中将停止线程
ThreadPool::~ThreadPool()
{
    stop_ = true;
    
    condition_.notify_all(); // 唤醒全部的线程

    // 等待全部线程执行完任务后退出
    for (std::thread &th : threads_)
        th.join();
}

/*
void show(int no, const std::string &name)
{
    printf("你选择了%d号英雄%s。\n",no,name.c_str());
}

void test()
{
    printf("你赢了。\n");
}

int main()
{
    ThreadPool threadpool(3);
    
    std::string name="貂蝉";
    threadpool.addtask(std::bind(show,1,name));
    sleep(1);

    threadpool.addtask(std::bind(test));
    sleep(1);

    threadpool.addtask(std::bind([]{ printf("你升星了\n");}));
    sleep(1);
}

// g++ -o test ThreadPool.cpp -lpthread

*/