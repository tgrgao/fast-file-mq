#include <fcntl.h>
#include <string>

#define METADATA_QUEUE_SIZE_ADDR    0
#define METADATA_READY_COUNT_ADDR   4
#define METADATA_UNACK_COUNT_ADDR   8
#define METADATA_READY_PTR_ADDR     12
#define METADATA_UNACK_PTR_ADDR     16
#define METADATA_START_PTR_ADDR     20

#define METADATA_AT_ID_ADDR         24


#define METADATA_ENTRY_SIZE         16

#define METADATA_START      48

class FileMQ {
    private:
        int fd_meta;
        int fd_data;
        struct flock metadata_lock;
    
    public:
        FileMQ(std::string queue_file_path);
        ~FileMQ();
        // FileMQ(std::string queue_file_path, int metadata_size, int data_offset);
        int enqueue(void *buf, unsigned *id, size_t size);
        int dequeue(void *buf, unsigned *id, size_t *size, size_t max_size);
        int ack(unsigned id);
        int nack(unsigned id);
        int fack(unsigned id);
};