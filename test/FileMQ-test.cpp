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

    unsigned id = UINT32_MAX;
    ssize_t size;
    char buf2[10];
    result = test_queue.dequeue(buf2, &id, &size, 10);
    EXPECT_EQ(result, FileMQ::Result::QUEUE_EMPTY);
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

TEST_F(FileMQTest, TestDequeueAckFack) {
    #define MAX_LEN 10
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    int value = 0;
    result = test_queue.enqueue(&value, &id, sizeof(value));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);

    ++value;
    result = test_queue.enqueue(&value, &id, sizeof(value));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);

    ssize_t size;
    result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 2);
    
    result = test_queue.ack(id);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    
    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 1);


    result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 1);

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 1);

    result = test_queue.fack(id);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    
    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 0);
}

TEST_F(FileMQTest, TestDequeueNack) {
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    int value = 0;
    result = test_queue.enqueue(&value, &id, sizeof(value));
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);

    ssize_t size;
    result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);

    unsigned id2; // to preserve the previously dequeued id
    result = test_queue.dequeue(&value, &id2, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::QUEUE_EMPTY);

    result = test_queue.nack(id);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);

    result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(id, 0);

    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(queue_size, 1);
}

TEST_F(FileMQTest, TestPurge) {
    FileMQ::Result result;
    unsigned id;

    unsigned value;

    unsigned total_enqueues = 1000;
    unsigned prepurge_dequeues = 500;

    for (unsigned i = 0; i < total_enqueues; ++i) {
        value = i;
        result = test_queue.enqueue(&value, &id, sizeof(value));
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }

    ssize_t size;
    for (unsigned i = 0; i < prepurge_dequeues; ++i) {
        result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
        EXPECT_EQ(value, i);

        result = test_queue.ack(id);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }

    unsigned prepurge_queue_size;
    result = test_queue.get_queue_size(&prepurge_queue_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(prepurge_queue_size, total_enqueues - prepurge_dequeues);

    unsigned prepurge_total_size;
    result = test_queue.get_total_size(&prepurge_total_size);
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    EXPECT_EQ(prepurge_total_size, total_enqueues);

    result = test_queue.purge();
    EXPECT_EQ(result, FileMQ::Result::SUCCESS);

    unsigned queue_size;
    result = test_queue.get_queue_size(&queue_size);
    EXPECT_EQ(queue_size, prepurge_queue_size);

    unsigned total_size;
    result = test_queue.get_queue_size(&total_size);
    EXPECT_LT(total_size, prepurge_total_size);
    EXPECT_EQ(total_size, prepurge_queue_size);

    // test subsequent actions work
    for (unsigned i = 0; i < (total_enqueues - prepurge_dequeues); ++i) {
        value = i;
        result = test_queue.enqueue(&value, &id, sizeof(value));
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }

    for (unsigned i = 0; i < total_enqueues; ++i) {
        result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
        if (i < prepurge_dequeues) {
            EXPECT_EQ(value, i + prepurge_dequeues);
        } else {
            EXPECT_EQ(value, i - prepurge_dequeues);
        }

        result = test_queue.ack(id);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }
}

TEST_F(FileMQTest, TestBenchmark100000) {
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    unsigned value;
    for (unsigned i = 0; i < 100000; ++i) {
        value = i;
        result = test_queue.enqueue(&value, &id, sizeof(value));
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }

    ssize_t size;
    for (unsigned i = 0; i < 100000; ++i) {
        result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
        EXPECT_EQ(value, i);

        result = test_queue.ack(id);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }
}

TEST_F(FileMQTest, TestBenchmark100000Alternating) {
    FileMQ::Result result;
    unsigned queue_size;
    unsigned id;

    unsigned value;
    for (unsigned i = 0; i < 100000; ++i) {
        value = i;
        result = test_queue.enqueue(&value, &id, sizeof(value));
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }

    ssize_t size;
    for (unsigned i = 0; i < 100000; ++i) {
        result = test_queue.dequeue(&value, &id, &size, MAX_LEN);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
        EXPECT_EQ(value, i);

        result = test_queue.ack(id);
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);

        value = i;
        result = test_queue.enqueue(&value, &id, sizeof(value));
        EXPECT_EQ(result, FileMQ::Result::SUCCESS);
    }
}