#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include "metadata.h"



class FileMQ {
    private:
        std::fstream metadata_fstream;
        std::fstream data_fstream;
        
        int lock_fd;
        struct flock file_lock;
        std::mutex thread_lock;
    
    public:
        FileMQ(std::string queue_file_path);
        ~FileMQ();

        int enqueue(void *buf, unsigned *id, size_t size);
        int dequeue(void *buf, unsigned *id, size_t *size, size_t max_size);
        int ack(unsigned id);
        int nack(unsigned id);
        int fack(unsigned id);
};