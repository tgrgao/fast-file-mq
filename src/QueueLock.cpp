#include "QueueLock.h"

QueueLock::QueueLock() {
    status = Status::NOT_INITIALIZED;
}

QueueLock::~QueueLock() {}

QueueLock::Result QueueLock::init(std::string queue_file_path) {
    status = Status::OK;
}

QueueLock::Result QueueLock::lock() {
    return Result::SUCCESS;
};

QueueLock::Result QueueLock::unlock() {
    return Result::SUCCESS;
};