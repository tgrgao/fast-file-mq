#include "file-mq.h"

#include <format>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

ssize_t get_file_size(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

void print_metadata_bytes(int fd) {
    ssize_t file_size = get_file_size(fd);

    // Ensure the file size is a multiple of 4
    if (file_size % 4 != 0) {
        std::cout <<  "File size is not a multiple of 4 bytes.\n";
        return;
    }

    // Rewind the file pointer to the beginning
    lseek(fd, 0, SEEK_SET) == -1;

    // Read and process the file in chunks of 4 bytes
    uint8_t buffer[4];
    int i = 0;
    while (read(fd, buffer, sizeof(buffer)) == sizeof(buffer)) {
        int value = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
        printf("%d: %d\n", i, value);
        i += 4;
    }
}

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

int read_metadata_entry(int fd, off_t addr, struct MetadataEntry *entry) {
    lseek(fd, addr, SEEK_SET);
    read(fd, entry, sizeof(*entry));
    return 0;
}

int write_unsigned(int fd, off_t addr, unsigned *num_ptr) {
    lseek(fd, addr, SEEK_SET);
    write(fd, num_ptr, sizeof(*num_ptr));
    return 0;
}

int write_metadata_entry(int fd, off_t addr, struct MetadataEntry *entry) {
    lseek(fd, addr, SEEK_SET);
    write(fd, entry, sizeof(*entry));
    return 0;
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

        lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
        unsigned metadata_start = METADATA_START;
        write(fd_meta, &metadata_start, sizeof(metadata_start));

        lseek(fd_meta, METADATA_UNACK_PTR_ADDR, SEEK_SET);
        write(fd_meta, &metadata_start, sizeof(metadata_start));
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

    read_unsigned(fd_meta, METADATA_AT_ID_ADDR, id);

    // make metadata entry and write at end of metadata file
    MetadataEntry metadata_entry; 
    metadata_entry.id = *id;
    metadata_entry.data_ptr = get_file_size(fd_data);
    metadata_entry.data_size = size;
    metadata_entry.status = EntryStatus::READY;
    write_metadata_entry(fd_meta, get_file_size(fd_meta), &metadata_entry);

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
    lock_file(fd_meta, &metadata_lock);

    unsigned ready_ptr;
    read_unsigned(fd_meta, METADATA_READY_PTR_ADDR, &ready_ptr);

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
    lseek(fd_meta, METADATA_UNACK_COUNT_ADDR, SEEK_SET);
    read(fd_meta, &unack_count, sizeof(unack_count));
    ++unack_count;
    lseek(fd_meta, METADATA_UNACK_COUNT_ADDR, SEEK_SET);
    write(fd_meta, &unack_count, sizeof(unack_count));

    // decrement ready count
    unsigned ready_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    read(fd_meta, &ready_count, sizeof(ready_count));
    --ready_count;
    lseek(fd_meta, METADATA_READY_COUNT_ADDR, SEEK_SET);
    write(fd_meta, &ready_count, sizeof(ready_count));

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

    unlock_file(fd_meta, &metadata_lock);

    return 0;
}

int FileMQ::ack(unsigned id) {
    lock_file(fd_meta, &metadata_lock);

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

    unlock_file(fd_meta, &metadata_lock);

    return 0;
}

int FileMQ::nack(unsigned id) {
    lock_file(fd_meta, &metadata_lock);

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

    unlock_file(fd_meta, &metadata_lock);

    return 0;
}

int FileMQ::fack(unsigned id) {
    lock_file(fd_meta, &metadata_lock);

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

    unlock_file(fd_meta, &metadata_lock);

    return 0;
}