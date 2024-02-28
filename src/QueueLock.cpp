#include "QueueLock.h"

QueueLock::QueueLock(std::string queue_file_path) {
    status = QueueLock::Status::OK;
}

QueueLock::~QueueLock() {}

QueueLock::Result QueueLock::lock() {
    return QueueLock::Result::SUCCESS;
};

QueueLock::Result QueueLock::unlock() {
    return QueueLock::Result::SUCCESS;
};