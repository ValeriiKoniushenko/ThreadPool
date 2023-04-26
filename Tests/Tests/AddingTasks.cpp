#include "gtest/gtest.h"
#include "ThreadPool.h"
#include <chrono>

using namespace std::chrono_literals;

namespace
{
    
    void WorkEmulation()
    {
        
    }

} // namespace

TEST(Submitting, SuccessfullSubmit)
{
    ThreadPool Pool(1);

    WorkEmulation();
    
    std::future<int> i = Pool.Submit([&]()
    {
        return 5;
    });

    EXPECT_EQ(5, i.get());
    EXPECT_EQ(0, Pool.GetErrors().size());
}