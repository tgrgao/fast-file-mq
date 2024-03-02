#include "FileMQ.h"

FileMQ::FileMQ(std::string queue_dir_path): metadata_storage(queue_dir_path), data_storage(queue_dir_path), queue_lock(queue_dir_path) {
    // TODO: INITIALIZE AND GET LOCK FIRST, THEN INITIALIZE THE REST
    if (metadata_storage.get_status() == MetadataStorage::Status::INITIALIZATION_FAILED || data_storage.get_status() == DataStorage::Status::INITIALIZATION_FAILED || queue_lock.get_status() == QueueLock::Status::INITIALIZATION_FAILED) {
        status = Status::INITIALIZATION_FAILED;
    }
    status = Status::OK;
}

FileMQ::~FileMQ() {}

FileMQ::Result FileMQ::enqueue(void *buf, unsigned *id, ssize_t size) {
    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    if (data_storage.put_data(buf, size) != DataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (metadata_storage.enqueue(id, size) != MetadataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

FileMQ::Result FileMQ::dequeue(void *buf, unsigned *id, ssize_t *size, ssize_t max_size) {
    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    off_t data_offset;

    MetadataStorage::Result res = metadata_storage.dequeue(id, &data_offset, size);
    if (res != MetadataStorage::Result::SUCCESS) {
        if (res == MetadataStorage::Result::QUEUE_EMPTY) {
            return Result::QUEUE_EMPTY;
        }
        return Result::FAILURE;
    }

    bool truncated = false;
    if (*size > max_size) {
        truncated = true;
        *size = max_size;
    }

    if (data_storage.get_data(buf, data_offset, *size) != DataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    if (truncated) {
        return Result::SUCCESS_TRUNCATED_READ;
    }
    return Result::SUCCESS;
}

FileMQ::Result FileMQ::ack(unsigned id) {
    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    if (metadata_storage.ack(id) != MetadataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

FileMQ::Result FileMQ::nack(unsigned id) {
    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    if (metadata_storage.nack(id) != MetadataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

FileMQ::Result FileMQ::fack(unsigned id) {
    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    if (metadata_storage.fack(id) != MetadataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}