#include "gtest/gtest.h"
#include "ThreadPool.h"
#include <chrono>

using namespace std::chrono_literals;

namespace
{
    
    void WorkEmulation()
    {
        std::this_thread::sleep_for(100ms); // work emulation
    }

} // namespace

TEST(Constructing, SuccessfullConstructing)
{
    const ThreadPool ThreadPool_;

    // WorkEmulation();

    EXPECT_EQ(0, ThreadPool_.GetErrors().size());
}

TEST(Constructing, SuccessfullConstructingWithSomeCountOfThreadIDs)
{
    ThreadPool ThreadPool_;

    WorkEmulation();
    
    EXPECT_EQ(ThreadPool_.GetRegisteredThreads().size(), std::thread::hardware_concurrency());
    EXPECT_EQ(ThreadPool_.GetThreadsCount(), std::thread::hardware_concurrency());
    EXPECT_EQ(0, ThreadPool_.GetErrors().size());
}

TEST(Constructing, SuccessfullConstructingWith5ThreadIDs)
{
    constexpr std::size_t ThreadsCount = 5;
    ThreadPool ThreadPool_(ThreadsCount);

    WorkEmulation();
    
    EXPECT_EQ(ThreadPool_.GetRegisteredThreads().size(), ThreadsCount);
    EXPECT_EQ(ThreadPool_.GetThreadsCount(), ThreadsCount);
    EXPECT_EQ(0, ThreadPool_.GetErrors().size());
}
