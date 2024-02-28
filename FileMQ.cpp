#include "FileMQ.h"

FileMQ::FileMQ(std::string queue_dir_path): metadata_storage(queue_dir_path), data_storage(queue_dir_path), queue_lock(queue_dir_path) {
    // TODO: INITIALIZE AND GET LOCK FIRST, THEN INITIALIZE THE REST
    if (metadata_storage.get_status() == MetadataStorage::Status::INITIALIZATION_FAILED || data_storage.get_status() == DataStorage::Status::INITIALIZATION_FAILED || queue_lock.get_status() == QueueLock::Status::INITIALIZATION_FAILED) {
        status = Status::INITIALIZATION_FAILED;
    }
}

FileMQ::~FileMQ() {}

FileMQ::Result FileMQ::enqueue(void *buf, unsigned *id, size_t size) {
    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    metadata_storage.make_stale();

    if (data_storage.put_data(buf, size) != DataStorage::Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

FileMQ::Result FileMQ::dequeue(void *buf, unsigned *id, size_t *size, size_t max_size) {

}

FileMQ::Result FileMQ::ack(unsigned id) {

}

FileMQ::Result FileMQ::nack(unsigned id) {

}

FileMQ::Result FileMQ::fack(unsigned id) {

}