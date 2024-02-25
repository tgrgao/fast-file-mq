#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include "DataStorage.h"
#include "MetadataStorage.h"

class FileMQ {
    enum class Status;
    enum class Result;

    private:
        Status status;
        MetadataStorage metadata_storage;
        DataStorage data_storage;
        
        int lock_fd;
        struct flock file_lock;
        std::mutex thread_lock;
    
    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED
        };

        enum class Result {
            SUCCESS,
            FAILURE
        };

        FileMQ(std::string queue_file_path);
        ~FileMQ();

        Status get_status() const {return status;};

        Result enqueue(void *buf, unsigned *id, size_t size);
        Result dequeue(void *buf, unsigned *id, size_t *size, size_t max_size);
        Result ack(unsigned id);
        Result nack(unsigned id);
        Result fack(unsigned id);
};