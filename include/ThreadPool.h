#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <memory>
#include <cassert>

 class ThreadPool {
 public:
     //删除默认构造函数，防止空shared_ptr解引用崩溃
     ThreadPool()=delete;
     ThreadPool(ThreadPool&&)=default;
     explicit ThreadPool(size_t threadCount=8):pool_(std::make_shared<Pool>()) {
         assert(threadCount>0);//threadCount>0才往下继续执行，否则就直接abort终止程序
         for (size_t i=0;i<threadCount;i++) {
             std::thread([pool=pool_]{
             std::unique_lock<std::mutex> locker(pool->mtx);
             while (true) {
                 if (!pool->tasks.empty()) {
                     auto task=std::move(pool->tasks.front());
                     pool->tasks.pop();
                     locker.unlock();
                     try {
                         task();
                     }
                     catch (const std::exception& e) {
    std::cerr << "[ThreadPool] task exception: " << e.what() << std::endl;
}
                     catch (...) {
    std::cerr << "[ThreadPool] unknown task exception" << std::endl;
}
                     locker.lock();
                 }
                 else if (pool->isClosed) {
                     break;
                 }
                 else {
                     pool->cond.wait(locker);
                 }
             }
         }).detach();
         }
     }
~ThreadPool() {
         if (static_cast<bool>(pool_)) {
             {
                 std::lock_guard<std::mutex> locker(pool_->mtx);
                 pool_->isClosed=true;
             }//这里加上大括号是为了手动控制作用域
             //这样就能让RAII的变量在离开作用域时自动销毁
             //这里是希望lock_guard在接下来要执行的“唤醒其他线程”之前就被释放
             //这样唤醒其他线程时不需要先释放锁再唤醒，因为已经销毁
             //不这样做，唤醒线程时线程会立即去枪锁，但是锁还没释放，会导致增加一次不必要的锁竞争，让性能下降
             pool_->cond.notify_all();
         }
     }

     template<class F>
         void addTask(F&& task) {
         {
             std::lock_guard<std::mutex> locker(pool_->mtx);
             pool_->tasks.emplace(std::forward<F>(task));
         }
         pool_->cond.notify_one();
     }

private:
     struct Pool {
         std::mutex mtx;
         std::condition_variable cond;
         bool isClosed=false;
         std::queue<std::function<void()>> tasks;
     };
     std::shared_ptr<Pool> pool_;
     //共享指针会在所有持有者都释放后,即引用计数归零时，自动销毁内存，不需要手动delete
     //这样即使ThreadPool析构了，线程还活着，Pool也不会提前销毁
 };