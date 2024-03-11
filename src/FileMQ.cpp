#include "FileMQ.h"

FileMQ::FileMQ() {
    status = Status::NOT_INITIALIZED;
}

FileMQ::~FileMQ() {}

FileMQ::Result FileMQ::init(std::string queue_dir_path) {
    if (!std::filesystem::exists(queue_dir_path)) {
        std::cout << "No existing queue directory not found. Creating queue directory.\n";
        if (!std::filesystem::create_directories(queue_dir_path)) {
            std::cout << "Error: Failed to create queue directory.\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
    } else {
        std::cout << "Found existing queue directory.\n";
    }

    if (queue_lock.init(queue_dir_path) == QueueLock::Result::FAILURE || queue_lock.get_status() != QueueLock::Status::OK) {
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    if (metadata_storage.init(queue_dir_path) == MetadataStorage::Result::FAILURE || metadata_storage.get_status() != MetadataStorage::Status::OK || data_storage.init(queue_dir_path) == DataStorage::Result::FAILURE || data_storage.get_status() != DataStorage::Status::OK) {
        status = Status::INITIALIZATION_FAILED;
        if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
            status = Status::INITIALIZATION_FAILED;
            return Result::CRITICAL_FAILURE;
        }
        return Result::FAILURE;
    }

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        status = Status::INITIALIZATION_FAILED;
        return Result::CRITICAL_FAILURE;
    }

    status = Status::OK;
    return Result::SUCCESS;
}

FileMQ::Result FileMQ::get_queue_size(unsigned *queue_size) {
    auto get_queue_size_inner = [=]() -> Result {
        metadata_storage.make_stale();
        if (metadata_storage.get_queue_size(queue_size) != MetadataStorage::Result::SUCCESS) {
            return FileMQ::Result::FAILURE;
        }
        return FileMQ::Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = get_queue_size_inner();

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::get_total_size(unsigned *total_size) {
    auto get_total_size_inner = [=]() -> Result {
        metadata_storage.make_stale();
        if (metadata_storage.get_total_size(total_size) != MetadataStorage::Result::SUCCESS) {
            return FileMQ::Result::FAILURE;
        }
        return FileMQ::Result::SUCCESS;
    };

    if (status != Status::OK) {
        std::cout << "Error: Queue not OK.\n";
        return Result::FAILURE;
    }

    if (queue_lock.lock() == QueueLock::Result::FAILURE) {
        return Result::FAILURE;
    }

    Result result = get_total_size_inner();

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
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

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
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

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
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

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}

FileMQ::Result FileMQ::purge() {
    auto purge_inner = [=]() -> Result {
        metadata_storage.make_stale();

        off_t data_bytes_trimmed;

        if (metadata_storage.purge(&data_bytes_trimmed) != MetadataStorage::Result::SUCCESS) {
            return Result::CRITICAL_FAILURE; // this is a critical failure because likely part of the metadata has already been changed
            // TODO: Add critical failure codes to MetadataStorage::Result to differentiate
        }

        if (data_storage.purge(data_bytes_trimmed) != DataStorage::Result::SUCCESS) {
            return Result::CRITICAL_FAILURE; // this is a critical failure because the metadata has already been purged
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

    Result result = purge_inner();

    if (queue_lock.unlock() == QueueLock::Result::FAILURE) {
        return Result::CRITICAL_FAILURE;
    }

    return result;
}