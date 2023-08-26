/*
 * @Author: 快出来了哦
 * @Date: 2023-08-22 13:51:34
 * @LastEditors: 快出来了哦
 * @LastEditTime: 2023-08-24 15:57:27
 * @FilePath: /ThreadPool/code/Pool/threadpool.h
 * @Description: 
 */
#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
/*Any指向任意数据类型*/
class Any
{
public:
    Any() = default;
    Any(const Any&) = delete;
    Any& operator = (const Any&) = delete;
    Any(Any&&) = default;
    Any& operator = (Any&&) = default;
    template <class T>
    Any(T data):base_(std::make_unique<Drived<T>>(data)){}
	~Any() = default;
    template <class T_>
    T_ cast_()
    {
        Drived<T_>* pt = dynamic_cast<Drived<T_>*>(base_.get());
        if(pt == nullptr)
        {
            throw "type unmatch";
        }     
        return pt->data_;
    }
private:
    class Base{
    public:
        virtual ~Base() = default;
    };
    template <class T>
    class Drived: public Base
    {
    public:
        Drived(T data):data_(data){}
    public:
        T data_;
    };
private:
    std::unique_ptr<Base> base_;
};
//自定义信号量
class Semaphore
{
public:
    Semaphore(int limit = 0):resLimit_(limit),exit_(false){}
    ~Semaphore(){exit_ = true;}
    void wait()
    {
		if(exit_)return;
        std::unique_lock<std::mutex> lock(resmutex_);
        rescv_.wait(lock,[&](){return resLimit_ > 0;});
        resLimit_--;
    }
    void post()
    {
		 if(exit_)return;
         std::unique_lock<std::mutex> lock(resmutex_);
         resLimit_++;
         rescv_.notify_all();
    }
private:
    int resLimit_;//资源数量
	std::atomic_bool exit_;
    std::mutex resmutex_;//互斥锁
    std::condition_variable rescv_;//条件变量
};
// Task类型的前置声明
class Task;

// 实现接收提交到线程池的task任务执行完成后的返回值类型Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// 问题一：setVal方法，获取任务执行完的返回值的
	void setVal(Any any);

	// 问题二：get方法，用户调用这个方法获取task的返回值
	Any get();
private:
	Any any_; // 存储任务的返回值
	Semaphore sem_; // 线程通信信号量
	std::shared_ptr<Task> task_; //指向对应获取返回值的任务对象 
	std::atomic_bool isValid_; // 返回值是否有效
};

// 任务抽象基类
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	// 用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务处理
	virtual Any run() = 0;

private:
	Result* result_; // Result对象的声明周期 》 Task的
};

// 线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,  // 固定数量的线程
	MODE_CACHED, // 线程数量可动态增长
};

// 线程类型
class Thread
{
public:
	// 线程函数对象类型
	using ThreadFunc = std::function<void(int)>;

	// 线程构造
	Thread(ThreadFunc func);
	// 线程析构
	~Thread();
	// 启动线程
	void start();

	// 获取线程id
	int getId()const;
private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;  // 保存线程id
};

class ThreadPool
{
public:
	// 线程池构造
	ThreadPool();

	// 线程池析构
	~ThreadPool();

	// 设置线程池的工作模式
	void setMode(PoolMode mode);

	// 设置task任务队列上线阈值
	void setTaskQueMaxThreshHold(int threshhold);

	// 设置线程池cached模式下线程阈值
	void setThreadSizeThreshHold(int threshhold);

	// 给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);

	// 开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	// 定义线程函数
	void threadFunc(int threadid);

	// 检查pool的运行状态
	bool checkRunningState() const;

private:
	// std::vector<std::unique_ptr<Thread>> threads_; // 线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_; // 线程列表

	int initThreadSize_;  // 初始的线程数量
	int threadSizeThreshHold_; // 线程数量上限阈值
	std::atomic_int curThreadSize_;	// 记录当前线程池里面线程的总数量
	std::atomic_int idleThreadSize_; // 记录空闲线程的数量

	std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
	std::atomic_int taskSize_; // 任务的数量
	int taskQueMaxThreshHold_;  // 任务队列数量上限阈值

	std::mutex taskQueMtx_; // 保证任务队列的线程安全
	std::condition_variable notFull_; // 表示任务队列不满
	std::condition_variable notEmpty_; // 表示任务队列不空
	std::condition_variable exitCond_; // 等到线程资源全部回收

	PoolMode poolMode_; // 当前线程池的工作模式
	std::atomic_bool isPoolRunning_; // 表示当前线程池的启动状态
};
#endif