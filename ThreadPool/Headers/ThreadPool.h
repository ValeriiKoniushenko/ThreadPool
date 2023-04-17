#pragma once

#include <yvals_core.h>

#ifdef __cpp_lib_source_location
	#include <source_location>
#endif	  // __cpp_lib_source_location

#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class ThreadPool
{
public:
	struct Error final
	{
		std::string Message;
		std::chrono::time_point<std::chrono::system_clock> Timestamp;
		std::thread::id ThreadId;
	};
	
	class ThreadInfo final
	{
	public:
		enum class ThreadStatus : int8_t
		{
			Paused,
			Running,
			Stopped
		};
		
		ThreadInfo(std::thread::id ID, ThreadStatus Status);
		~ThreadInfo() = default;
		ThreadInfo(const ThreadInfo&) = delete;
		ThreadInfo(ThreadInfo&&);
		ThreadInfo& operator=(const ThreadInfo&) = delete;
		ThreadInfo& operator=(ThreadInfo&&);

		_NODISCARD std::thread::id GetID() const;
		
		void SetStatus(ThreadStatus NewThreadStatus);
		_NODISCARD ThreadStatus GetStatus() const;
	
	private:
		std::thread::id ID = std::this_thread::get_id();
		ThreadStatus Status = ThreadStatus::Stopped;
		mutable std::shared_mutex Mutex;
	};
	
	using RegisteredThreadsT = std::map<std::thread::id, ThreadInfo>;
	
	explicit ThreadPool(std::size_t ThreadsCount = 0);
	~ThreadPool();
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;
	
	std::vector<Error> GetErrors() const;
	_NODISCARD const RegisteredThreadsT& GetRegisteredThreads();
	void Stop();
	
protected:

#ifdef __cpp_lib_source_location
	inline void PushError(const std::string& ErrorMessage, std::source_location SourceLocation = std::source_location::current());
#else // __cpp_lib_source_location
	inline void PushError(const std::string& ErrorMessage, const char* FileName = __FILE__, int Line = __LINE__);
#endif // __cpp_lib_source_location
	
private:
	void AllocateThreads();

private:
	std::vector<std::thread> Threads;
	std::vector<Error> Errors;
	bool bIsRun = true;
	RegisteredThreadsT RegisteredThreads;
	mutable std::mutex Mutex; // TODO: change to std::shared_mutex
};

#ifdef __cpp_lib_source_location

inline void ThreadPool::PushError(const std::string& ErrorMessage, std::source_location SourceLocation)
{
	std::stringstream SS;
	SS << SourceLocation.file_name() << "(" << SourceLocation.line() << "): " << ErrorMessage << std::endl;
	std::lock_guard<std::mutex> LG(Mutex);
	Errors.push_back({SS.str(), std::chrono::system_clock::now(), std::this_thread::get_id()});
}

#else // __cpp_lib_source_location

inline void ThreadPool::PushError(const std::string& ErrorMessage, const char* FileName/* = __FILE__*/, int Line/* = __LINE__*/)
{
	std::stringstream SS;
	SS << FileName << "(" << Line << "): " << ErrorMessage << std::endl;
	std::lock_guard<std::mutex> LG(Mutex);
	Errors.push_back({SS.str(), std::chrono::system_clock::now(), std::this_thread::get_id()});
}

#endif // __cpp_lib_source_location
