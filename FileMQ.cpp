#include "FileMQ.h"

FileMQ::FileMQ(std::string queue_dir_path): metadata_storage(queue_dir_path), data_storage(queue_dir_path) {
    if (metadata_storage.get_status() == MetadataStorage::Status::INITIALIZATION_FAILED || data_storage.get_status() == DataStorage::Status::INITIALIZATION_FAILED) {
        status = Status::INITIALIZATION_FAILED;
    }
}

FileMQ::~FileMQ() {}

FileMQ::Result FileMQ::enqueue(void *buf, unsigned *id, size_t size) {

}

FileMQ::Result FileMQ::dequeue(void *buf, unsigned *id, size_t *size, size_t max_size) {

}

FileMQ::Result FileMQ::ack(unsigned id) {

}

FileMQ::Result FileMQ::nack(unsigned id) {

}

FileMQ::Result FileMQ::fack(unsigned id) {

}