#include "QueueLock.h"

#include <fstream>
#include <iostream>

QueueLock::QueueLock() {
    status = Status::NOT_INITIALIZED;
}

QueueLock::~QueueLock() {}

QueueLock::Result QueueLock::init(std::string queue_dir_path) {
    std::string file_lock_path = queue_dir_path + "/lock.file";

    if (!std::filesystem::exists(file_lock_path)) {
        std::ofstream file_lock(file_lock_path);
        if (!file_lock.is_open()) {
            std::cout << "Error: failed to create lock file.\n";
            std::cout << std::filesystem::current_path() << "\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
        file_lock.close();
    }

    file_lock_fd = open(file_lock_path.c_str(), O_RDWR);
    
    if (file_lock_fd == -1) {
        std::cout << "Error: failed to open lock file.\n";
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    status = Status::OK;

    return Result::SUCCESS;
}

QueueLock::Result QueueLock::lock() {
    thread_mutex.lock();

    file_lock.l_type = F_WRLCK; // Exclusive lock
    if (fcntl(file_lock_fd, F_SETLK, &file_lock) == -1) {
        std::cout << "Error: failed to lock file lock.\n";
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

QueueLock::Result QueueLock::unlock() {
    file_lock.l_type = F_UNLCK;
    if (fcntl(file_lock_fd, F_SETLK, &file_lock) == -1) {
        std::cout << "Error: failed to unlock file lock.\n";
        return Result::FAILURE;
    }

    thread_mutex.unlock();
    
    return Result::SUCCESS;
}