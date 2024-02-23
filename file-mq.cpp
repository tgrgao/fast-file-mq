#include "file-mq.h"

#include <format>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

enum EntryStatus {
    READY,
    UNACK,
    ACK,
    FACK
};

struct MetadataEntry {
    unsigned id;
    unsigned data_ptr;
    unsigned data_size;
    unsigned status;
};

int increment_unsigned(int fd, off_t addr) {
    unsigned num;
    lseek(fd, addr, SEEK_SET);
    read(fd, &num, sizeof(num));
    ++num;
    lseek(fd, addr, SEEK_SET);
    write(fd, &num, sizeof(num));
    return 0;
}

int read_unsigned(int fd, off_t addr, unsigned *num_ptr) {
    lseek(fd, addr, SEEK_SET);
    read(fd, num_ptr, sizeof(*num_ptr));
    return 0;
}

ssize_t get_file_size(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

int lock_file(int fd, struct flock *lock_ptr) {
    // fcntl(fd, F_SETLKW, lock_ptr);
    return 0;
}

int unlock_file(int fd, struct flock *lock_ptr) {
    // fcntl(fd, F_UNLCK, lock_ptr);
    return 0;
}

FileMQ::FileMQ(std::string queue_dir_path) {
    std::string queue_metadata_file_path = queue_dir_path + "/metadata.bytes";
    char* file_path_chars = new char[queue_metadata_file_path.size()];
    std::copy(queue_metadata_file_path.begin(), queue_metadata_file_path.end(), file_path_chars);
    fd_meta = open(file_path_chars, O_RDWR);
    if (fd_meta == -1) { // metadata file did not exist before
        unsigned zeros[METADATA_START];
        std::memset(zeros, 0, METADATA_START);
        fd_meta = open(file_path_chars, O_CREAT | O_RDWR);
        write(fd_meta, zeros, METADATA_START);
    }

    free(file_path_chars);

    std::string queue_data_file_path = queue_dir_path + "/data.bytes";
    file_path_chars = new char[queue_data_file_path.size()];
    std::copy(queue_data_file_path.begin(), queue_data_file_path.end(), file_path_chars);
    fd_data = open(file_path_chars, O_CREAT | O_RDWR);
    free(file_path_chars);

    metadata_lock.l_type = F_WRLCK;
    metadata_lock.l_whence = SEEK_SET;
    metadata_lock.l_start = 0;
    metadata_lock.l_len = 0;  // Lock the entire file
}

FileMQ::~FileMQ() {
    fcntl(fd_meta, F_UNLCK, &metadata_lock);
    close(fd_meta);
    close(fd_data);
}

int FileMQ::enqueue(void *buf, unsigned *id, size_t size) {
    lock_file(fd_meta, &metadata_lock);

    // make metadata entry
    MetadataEntry metadata_entry; 
    lseek(fd_meta, METADATA_AT_ID_ADDR, SEEK_SET);
    read(fd_meta, &(metadata_entry.id), sizeof(*id));
    metadata_entry.data_ptr = get_file_size(fd_meta);
    metadata_entry.data_size = size;
    metadata_entry.status = EntryStatus::READY;
    lseek(fd_meta, 0, SEEK_END);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    *id = metadata_entry.id;

    // write the data
    lseek(fd_data, 0, SEEK_END);
    write(fd_data, buf, size);

    // increment queue size
    increment_unsigned(fd_meta, METADATA_QUEUE_SIZE_ADDR);

    // increment ready count
    increment_unsigned(fd_meta, METADATA_READY_COUNT_ADDR);

    // increment at ID
    increment_unsigned(fd_meta, METADATA_AT_ID_ADDR);
    
    unlock_file(fd_meta, &metadata_lock);

    return 0;
}

int FileMQ::dequeue(void *buf, unsigned *id, size_t *size, size_t max_size) {
    off_t ready_ptr;
    lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
    read(fd_meta, &ready_ptr, sizeof(ready_ptr));

    if (ready_ptr == get_file_size(fd_meta)) {
        return -1; // nothing ready
    }

    MetadataEntry metadata_entry;
    lseek(fd_meta, ready_ptr, SEEK_SET);
    read(fd_meta, &metadata_entry, sizeof(metadata_entry));
    *id = metadata_entry.id;

    if (metadata_entry.status != EntryStatus::READY) {
        return -1; // should not happen
    }

    // queue item too big for buffer
    if (metadata_entry.data_size > max_size) {
        *size = metadata_entry.data_size;
        return -1;
    }

    lseek(fd_data, metadata_entry.data_ptr, SEEK_SET);
    *size = read(fd_data, buf, metadata_entry.data_size);

    metadata_entry.status = EntryStatus::UNACK;
    lseek(fd_meta, ready_ptr, SEEK_SET);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    // increment unack count
    unsigned unack_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    read(fd_meta, &unack_count, sizeof(unack_count));
    ++unack_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    write(fd_meta, &unack_count, sizeof(unack_count));

    // advance ready pointer
    ready_ptr += sizeof(metadata_entry);
    lseek(fd_meta, ready_ptr, SEEK_SET);
    while (ready_ptr != get_file_size(fd_meta)) {
        read(fd_meta, &metadata_entry, sizeof(metadata_entry));
        if (metadata_entry.status == EntryStatus::READY) {
            break;
        }
        ready_ptr += sizeof(metadata_entry);
    }
    lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
    write(fd_meta, &ready_ptr, sizeof(ready_ptr));

    return 0;
}

int FileMQ::ack(unsigned id) {
    MetadataEntry metadata_entry;
    off_t metadata_entry_ptr = METADATA_START + (id * sizeof(metadata_entry));
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    read(fd_meta, &metadata_entry, sizeof(metadata_entry));

    if (metadata_entry.status != EntryStatus::UNACK) {
        return -1; // should not happen
    }

    metadata_entry.status = EntryStatus::ACK;
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    return 0;
}

int FileMQ::nack(unsigned id) {
    MetadataEntry metadata_entry;
    off_t metadata_entry_ptr = METADATA_START + (id * sizeof(metadata_entry));
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    read(fd_meta, &metadata_entry, sizeof(metadata_entry));

    if (metadata_entry.status != EntryStatus::UNACK) {
        return -1; // should not happen
    }

    metadata_entry.status = EntryStatus::READY;
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    // update ready pointer if nacked entry ptr is lower
    int ready_ptr;
    lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
    read(fd_meta, &ready_ptr, fd_meta);
    if (metadata_entry_ptr < ready_ptr) {
        lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
        write(fd_meta, &metadata_entry_ptr, sizeof(metadata_entry_ptr));
    }

    return 0;
}

int FileMQ::fack(unsigned id) {
    MetadataEntry metadata_entry;
    off_t metadata_entry_ptr = METADATA_START + (id * sizeof(metadata_entry));
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    read(fd_meta, &metadata_entry, sizeof(metadata_entry));

    if (metadata_entry.status != EntryStatus::UNACK) {
        return -1; // should not happen
    }

    metadata_entry.status = EntryStatus::FACK;
    lseek(fd_meta, metadata_entry_ptr, SEEK_SET);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    return 0;
}