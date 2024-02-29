#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include "DataStorage.h"
#include "MetadataStorage.h"
#include "QueueLock.h"

class FileMQ {
    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED
        };

        enum class Result {
            SUCCESS,
            FAILURE,
            SUCCESS_TRUNCATED_READ,
            QUEUE_EMPTY
        };

        FileMQ(std::string queue_file_path);
        ~FileMQ();

        Status get_status() const {return status;};

        Result enqueue(void *buf, unsigned *id, ssize_t size);
        Result dequeue(void *buf, unsigned *id, ssize_t *size, ssize_t max_size);
        Result ack(unsigned id);
        Result nack(unsigned id);
        Result fack(unsigned id);

    private:
        Status status;
        QueueLock queue_lock;
        MetadataStorage metadata_storage;
        DataStorage data_storage;
};