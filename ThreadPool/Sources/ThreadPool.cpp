#include "ThreadPool.h"

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

const std::map<std::thread::id, ThreadPool::ThreadInfo>& ThreadPool::GetRegisteredThreads()
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

	for (const auto& Thread : RegisteredThreads)
	{
		while (Thread.second.GetStatus() == ThreadInfo::ThreadStatus::Running);
	}
}

void ThreadPool::AllocateThreads()
{
	auto LocalThreadMain = [this]()
	{
		{
			std::lock_guard<std::mutex> LG(Mutex);
			RegisteredThreads.emplace(
				std::this_thread::get_id(),
				ThreadInfo(std::this_thread::get_id(), ThreadInfo::ThreadStatus::Running)
			);
		}

		Mutex.lock();
		while (bIsRun)
		{
			Mutex.unlock();

			
			Mutex.lock();
		}
		Mutex.unlock();

		{
			std::lock_guard LG(Mutex);
			auto Key = RegisteredThreads.find(std::this_thread::get_id());
			if (Key != RegisteredThreads.end())
			{
				Key->second.SetStatus(ThreadInfo::ThreadStatus::Stopped);
			}
		}
	};

	for (std::thread& Thread : Threads)
	{
		Thread = std::move(std::thread(LocalThreadMain));
		Thread.detach();
	}
}