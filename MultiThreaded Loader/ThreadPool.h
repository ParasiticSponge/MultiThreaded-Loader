#pragma once
#include <thread>
#include <vector>
#include "CTask.h"
#include "WorkQueue.h"

class ThreadPool
{
public:

	~ThreadPool() 
	{
		/*stop();*/
	}

	void Initialize() 
	{
	}
	//submit the task to the work queue
	void Submit(CTask _iworkItem) 
	{
		//TODO
	}
	void DoWork() 
	{
	}
	void Start() 
	{
	}
	void Stop() 
	{
	}

private:
	std::vector<std::thread> _Threads;

	// Make the ThreadPool a singleton
	//Make the ThreadPool non-copyable.
protected:
	static ThreadPool* s_pThreadPool;
private:
	//The Threadpool has a workqueue
	WorkQueue<CTask >* m_pWorkQueue; //threadpool uses WorkQueue and WorkQueue uses CTask
	//The ThreadPool has a number of worker threads
	std::vector<std::thread> m_workerThreads;
	//A variable to hold the number of threads we want in the pool
	unsigned int m_iNumberOfThreads;
};

