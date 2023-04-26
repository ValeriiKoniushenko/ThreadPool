#include "ThreadPool.h"

#include <future>

ThreadPool::ThreadInfo::ThreadInfo(std::thread::id ID, ThreadStatus Status) : ID(ID), Status(Status)
{
}

ThreadPool::ThreadInfo::ThreadInfo(ThreadInfo&& Other)
{
	*this = std::move(Other);
}

ThreadPool::ThreadInfo& ThreadPool::ThreadInfo::operator=(ThreadInfo&& Other)
{
	ID = Other.ID;
	Status = Other.Status;
	
	return *this;
}

void ThreadPool::ThreadInfo::SetStatus(ThreadStatus NewThreadStatus)
{
	std::lock_guard LG(Mutex);
	Status = NewThreadStatus;
}

ThreadPool::ThreadInfo::ThreadStatus ThreadPool::ThreadInfo::GetStatus() const
{
	std::shared_lock LG(Mutex);
	return Status;
}

ThreadPool::TaskPool::FunctionT ThreadPool::TaskPool::Pop()
{
	std::lock_guard LG(Mutex);
	if (!Tasks.empty())
	{
		FunctionT Task = Tasks.front();
		Tasks.pop();
		return Task;
	}

	return {};
}

void ThreadPool::TaskPool::Push(FunctionT Task)
{
	std::lock_guard LG(Mutex);
	Tasks.push(Task);
}

std::thread::id ThreadPool::ThreadInfo::GetID() const
{
	return ID;
}

ThreadPool::ThreadPool(std::size_t ThreadsCount /* = 0*/)
{
	if (ThreadsCount != 0)
	{
		Threads.resize(ThreadsCount);
	}
	else
	{
		if (std::thread::hardware_concurrency())
		{
			Threads.resize(std::thread::hardware_concurrency());
		}
		else
		{
			PushError("Can't define count of the hardware threads. The default count of threads will be defined as 1");
			Threads.resize(1);
		}
	}

	AllocateThreads();
}

ThreadPool::~ThreadPool()
{
	Stop();
}

std::vector<ThreadPool::Error> ThreadPool::GetErrors() const
{
	std::lock_guard LG(Mutex);
	return Errors;
}

const ThreadPool::RegisteredThreadsT& ThreadPool::GetRegisteredThreads()
{
	std::lock_guard LG(Mutex);
	return RegisteredThreads;
}

void ThreadPool::Stop()
{
	{
		std::lock_guard LG(Mutex);
		bIsRun = false;
	}

	for (auto& Thread : Threads)
	{
		if (!Thread.joinable())
		{
			PushError("Can't join a thread");
		}
		else
		{
			Thread.join();
			auto Key = RegisteredThreads.find(Thread.get_id());
			if (Key != RegisteredThreads.end())
			{
				Key->second.SetStatus(ThreadInfo::ThreadStatus::Stopped);
			}
		}
	}
}

std::size_t ThreadPool::GetThreadsCount() const
{
	return RegisteredThreads.size();
}

std::future<int> ThreadPool::Submit(std::function<int()> Task)
{
	std::shared_ptr<std::promise<int>> Promise = std::make_shared<std::promise<int>>();
	Tasks.Push([Promise, Task]()
	{
		Promise->set_value(Task());
	});
	return Promise->get_future(); 
}

void ThreadPool::AllocateThreads()
{
	auto LocalThreadMain = [this]()
	{
		Mutex.lock();
		while (bIsRun)
		{
			Mutex.unlock();

			TaskPool::FunctionT Task = Tasks.Pop();
			if (Task)
			{
				Task();
			}
			
			Mutex.lock();
		}
		Mutex.unlock();
	};

	for (std::thread& Thread : Threads)
	{
		Thread = std::thread(LocalThreadMain);
		RegisteredThreads.emplace(Thread.get_id(), ThreadInfo(Thread.get_id(), ThreadInfo::ThreadStatus::Running));
	}
}