<!--
 * @Author: 快出来了哦
 * @Date: 2023-08-26 14:54:27
 * @LastEditors: 快出来了哦
 * @LastEditTime: 2023-08-26 14:54:30
 * @FilePath: /ThreadPool/README.md
 * @Description: 
-->
###  c++多线程
#### 项目介绍
作为五大池之一（内存池、连接池、线程池、进程池、协程池），线程池的应用非常广泛，不管是客户端程序，还是后台服务程序，都是提高业务处理能力的必备模块。有很多开源的线程池实现，虽然各自接口使用上稍有区别，但是其核心实现原理都是基本相同的。
#### 项目用到的技术
1.熟练基于C++ 11标准的面向对象编程
  组合和继承、继承多态、STL容器、智能指针、函数对象、绑定器、可变参模板编程等。
2.熟悉C++11多线程编程
  thread、mutex、atomic、condition_variable、unique_lock等。
3.C++17和C++20标准的内容
4.C++17的any类型和C++20的信号量semaphore，项目上都我们自己用代码实现。
5.熟悉多线程理论
6.多线程基本知识、线程互斥、线程同步、原子操作、CAS等。
#### 线程的消耗
  为了完成任务，创建很多的线程可以吗？线程真的是越多越好？
  线程的创建和销毁都是非常"重"的操作
  线程栈本身占用大量内存
  线程的上下文切换要占用大量时间
  大量线程同时唤醒会使系统经常出现锯齿状负载或者瞬间负载量很大导致宕机
#### 线程池的优势
  操作系统上创建线程和销毁线程都是很"重"的操作，耗时耗性能都比较多，那么在服务执行的过程中，如果业务量比较大，实时的去创建线程、执行业务、业务完成后销毁线程，那么会导致系统的实时性能降低，业务的处理能力也会降低。
  线程池的优势就是（每个池都有自己的优势），在服务进程启动之初，就事先创建好线程池里面的线程，当业务流量到来时需要分配线程，直接从线程池中获取一个空闲线程执行task任务即可，task执行完成后，也不用释放线程，而是把线程归还到线程池中继续给后续的task提供服务。
##### fixed模式线程池
  线程池里面的线程个数是固定不变的，一般是ThreadPool创建时根据当前机器的CPU核心数量进行指定。
##### cached模式线程池
线程池里面的线程个数是可动态增长的，根据任务的数量动态的增加线程的数量，但是会设置一个线程数量的阈值，任务处理完成，如果动态增长的线程空闲了60s还没有处理其它任务，那么关闭线程，保持池中最初数量的线程即可。
#### 项目遇到的问题
* 死锁问题
   在线程池析构时，要通知所有的线程，包括处于阻塞中的运行中的，但是由于处于的状态不同，先通知在获取锁后，有的线程才持有锁，后续无通知，导致死锁。将线程池的析构函数改为先获取锁再通知，问题解决。
```c++
	// 线程池析构
	~ThreadPool()
	{
		isPoolRunning_ = false;

		// 等待线程池里面所有的线程返回  有两种状态：阻塞 & 正在执行任务中
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}
```
* 平台差异
   在windows中执行没有问题的代码，在linux中出现死锁。利用gdb调试，发现所有的线程都在阻塞，通过查阅资料，发现在windows平台下，条件变量会释放资源，而linux下什么都没做。在windows的VS编译器下，条件变量析构会释放相应的资源
   也就是说，任务的返回值，会调用result的post函数，此时cond已经析构了，所以notify相当于什么也没做，不会阻塞，任务没有任何问题！！！
```c++
    //linux下的condition_variable()
   ~condition_variable() noexcept;
    //windows下的condition_variable()
   ~condition_variable() noexcept {
        _Cnd_destroy_in_situ(_Mycnd());
    }
```