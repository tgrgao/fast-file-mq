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
    // fcntl(fd_meta, F_SETLKW, &metadata_lock);

    struct stat st;
    fstat(fd_data, &st);

    // make metadata entry
    MetadataEntry metadata_entry; 
    lseek(fd_meta, METADATA_AT_ID_ADDR, SEEK_SET);
    read(fd_meta, &(metadata_entry.id), sizeof(*id));
    metadata_entry.data_ptr = st.st_size;
    metadata_entry.data_size = size;
    metadata_entry.status = EntryStatus::READY;
    lseek(fd_meta, 0, SEEK_END);
    write(fd_meta, &metadata_entry, sizeof(metadata_entry));

    // write the data
    lseek(fd_data, 0, SEEK_END);
    write(fd_data, buf, size);

    // increment queue size
    unsigned queue_size;
    lseek(fd_meta, METADATA_QUEUE_SIZE_ADDR, SEEK_SET);
    read(fd_meta, &queue_size, sizeof(queue_size));
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

    // increment at ID
    unsigned new_at_id = metadata_entry.id + 1;
    lseek(fd_meta, METADATA_AT_ID_ADDR, SEEK_SET);
    write(fd_meta, &(new_at_id), sizeof(new_at_id));
    
    // fcntl(fd_meta, F_UNLCK, &metadata_lock);

    return 0;
}

int FileMQ::dequeue(void *buf, unsigned *id, size_t *size) {
    off_t ready_ptr;
    lseek(fd_meta, METADATA_READY_PTR_ADDR, SEEK_SET);
    read(fd_meta, &ready_ptr, sizeof(ready_ptr));

    struct stat st;
    fstat(fd_data, &st);

    if (ready_ptr == st.st_size) {
        return -1; // nothing ready
    }

    MetadataEntry metadata_entry;
    lseek(fd_meta, ready_ptr, SEEK_SET);
    read(fd_meta, &metadata_entry, sizeof(metadata_entry));
    *id = metadata_entry.id;

    if (metadata_entry.status != EntryStatus::READY) {
        return -1; // should not happen
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
    while (ready_ptr != st.st_size) {
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