#include "file-mq.h"

#include <format>
#include <iostream>
#include <unistd.h>

FileMQ::FileMQ(std::string queue_dir_path) {
    std::string queue_metadata_file_path = queue_dir_path + "/metadata.bytes";
    char* file_path_chars = new char[queue_metadata_file_path.size()];
    std::copy(queue_metadata_file_path.begin(), queue_metadata_file_path.end(), file_path_chars);
    fd_meta = open(file_path_chars, O_CREAT | O_RDWR);
    free(file_path_chars);

    std::string queue_data_file_path = queue_dir_path + "/data.bytes";
    file_path_chars = new char[queue_data_file_path.size()];
    std::copy(queue_data_file_path.begin(), queue_data_file_path.end(), file_path_chars);
    fd_data = open(file_path_chars, O_RDWR);
    free(file_path_chars);

    metadata_lock.l_type = F_WRLCK;
    metadata_lock.l_whence = SEEK_SET;
    metadata_lock.l_start = 0;
    metadata_lock.l_len = 0;  // Lock the entire file
}

int FileMQ::enqueue(void *buf, unsigned *id, size_t size) {
    fcntl(fd_meta, F_SETLKW, &metadata_lock);

    // make metadata entry
    unsigned queue_size;
    lseek(fd_meta, METADATA_QUEUE_SIZE_ADDR, SEEK_SET);
    read(fd_meta, &queue_size, sizeof(queue_size));
    lseek(fd_meta, METADATA_START, SEEK_SET);
    write(fd_meta, &queue_size, sizeof(queue_size));
    
    


    // increment queue size
    ++queue_size;
    lseek(fd_meta, METADATA_QUEUE_SIZE_ADDR, SEEK_SET);
    write(fd_meta, &queue_size, sizeof(queue_size));

    // increment ready count
    unsigned ready_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    read(fd_meta, &ready_count, sizeof(ready_count));
    ++ready_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    write(fd_meta, &ready_count, sizeof(ready_count));
    
    fcntl(fd_meta, F_UNLCK, &metadata_lock);
}

int FileMQ::dequeue(void *buf, unsigned *id, size_t *size) {
    return 0;
}

int FileMQ::ack(int *id) {

}

int FileMQ::nack(int *id) {
    
}

int FileMQ::fack(int *id) {

}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: file-mq.out <directory_path";
    }


}