#include "FileMQ.h"

FileMQ::FileMQ() {
    status = Status::NOT_INITIALIZED;
}

FileMQ::~FileMQ() {}

FileMQ::Result FileMQ::init(std::string queue_file_path) {
    if (queue_lock.init(queue_file_path) == QueueLock::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    if (metadata_storage.init(queue_file_path) == MetadataStorage::Result::FAILURE || data_storage.init(queue_file_path) == DataStorage::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
    }

    if (metadata_storage.get_status() == MetadataStorage::Status::INITIALIZATION_FAILED || data_storage.get_status() == DataStorage::Status::INITIALIZATION_FAILED || queue_lock.get_status() == QueueLock::Status::INITIALIZATION_FAILED) {
        status = Status::INITIALIZATION_FAILED;
    }

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    status = Status::OK;
    return Result::SUCCESS;
}

FileMQ::Result FileMQ::enqueue(void *buf, unsigned *id, ssize_t size) {
    auto enqueue_inner = [=]() -> Result {
        metadata_storage.make_stale();

        if (data_storage.put_data(buf, size) != DataStorage::Result::SUCCESS) {
            return Result::FAILURE;
        }

        if (metadata_storage.enqueue(id, size) != MetadataStorage::Result::SUCCESS) {
            return Result::FAILURE;
        }

        return Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = enqueue_inner();

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::dequeue(void *buf, unsigned *id, ssize_t *size, ssize_t max_size) {
    auto dequeue_inner = [=]() -> Result {
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

        if (truncated) {
            return Result::SUCCESS_TRUNCATED_READ;
        }

        return Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = dequeue_inner();

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::ack(unsigned id) {
    auto ack_inner = [=]() -> Result {
        metadata_storage.make_stale();

        if (metadata_storage.ack(id) != MetadataStorage::Result::SUCCESS) {
            return Result::FAILURE;
        }

        return Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = ack_inner();

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::fack(unsigned id) {
    auto fack_inner = [=]() -> Result {
        metadata_storage.make_stale();

        if (metadata_storage.fack(id) != MetadataStorage::Result::SUCCESS) {
            return Result::FAILURE;
        }

        return Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = fack_inner();

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::nack(unsigned id) {
    auto nack_inner = [=]() -> Result {
        metadata_storage.make_stale();

        if (metadata_storage.nack(id) != MetadataStorage::Result::SUCCESS) {
            return Result::FAILURE;
        }

        return Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = nack_inner();

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}