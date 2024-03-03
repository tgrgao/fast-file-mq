#include <gtest/gtest.h>

#include <unistd.h>

#include "../src/FileMQ.h"

class FileMQTest : public testing::Test {
    protected:
        void SetUp() override {
            rmdir("test/test-queue");
            test_queue.init("test/test-queue");
        }
    
        void TearDown() override {
            rmdir("test/test-queue");
        }

        FileMQ test_queue;
};

TEST_F(FileMQTest, EnqueueDequeue1) {
    #define MAX_LEN 10
    char buf[MAX_LEN] = "Hello";
    unsigned id;

    FileMQ::Result result;
    result = test_queue.enqueue(buf, &id, strnlen(buf, MAX_LEN));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);
}