#include <gtest/gtest.h>

#include <limits>

#include "../src/FileMQ.h"

class FileMQTest : public testing::Test {
    protected:
        void SetUp() override {
            std::filesystem::remove_all("test-queue");
            if (std::filesystem::exists("test-queue")) {
                FAIL() << "Setup failed. Previous test queue directory still exists.\n";
            }
            test_queue.init("test-queue");
        }
    
        void TearDown() override {
            std::filesystem::remove_all("test-queue");
        }

        FileMQ test_queue;
};

TEST_F(FileMQTest, TestInit) {
    FileMQ::Result result;
    EXPECT_EQ(test_queue.get_status(), FileMQ::Status::OK);
    unsigned queue_size;
    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 0);
}

TEST_F(FileMQTest, TestEnqueueDequeue1) {
    #define MAX_LEN 10
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    char buf[MAX_LEN] = "Hello";

    result = test_queue.enqueue(buf, &id, strlen(buf));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);
    
    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 1);

    id = UINT32_MAX;
    ssize_t size;
    char buf2[MAX_LEN];
    result = test_queue.dequeue(buf2, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);
    EXPECT_EQ(size, strlen(buf));
    for (unsigned i = 0; i < size; ++i) {
        EXPECT_EQ(buf2[i], buf[i]);
    }

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 1);
}

TEST_F(FileMQTest, TestEnqueueDequeue2) {
    #define MAX_LEN 10
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    char buf[MAX_LEN] = "Hello";
    result = test_queue.enqueue(buf, &id, strlen(buf));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);

    strcpy(buf, "world!");
    result = test_queue.enqueue(buf, &id, strlen(buf));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 1);

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 2);

    ssize_t size;
    char buf2[MAX_LEN];

    strcpy(buf, "Hello");
    result = test_queue.dequeue(buf2, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);
    EXPECT_EQ(size, strlen(buf));
    for (unsigned i = 0; i < size; ++i) {
        EXPECT_EQ(buf2[i], buf[i]);
    }
    
    strcpy(buf, "world!");
    result = test_queue.dequeue(buf2, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 1);
    EXPECT_EQ(size, strlen(buf));
    for (unsigned i = 0; i < size; ++i) {
        EXPECT_EQ(buf2[i], buf[i]);
    }

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 2);
}

// TEST_F(FileMQTest, TestDequeueAck) {
//     #define MAX_LEN 10
//     FileMQ::Result result;
//     unsigned queue_size;
//     unsigned id;
// }