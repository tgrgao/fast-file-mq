#include "DataStorage.h"
#include "helper.h"

#include <iostream>

DataStorage::DataStorage() {
    status = Status::NOT_INITIALIZED;
}

DataStorage::~DataStorage() {}

DataStorage::Result DataStorage::init(std::string queue_dir_path) {
    std::string data_file_path = queue_dir_path + "/data.bytes";

    if (!std::filesystem::exists(data_file_path)) {
        data_fstream.open(data_file_path, std::ios::out | std::ios::binary);
        if (!data_fstream.is_open()) {
            std::cout << "Error: failed to create data file.\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
        data_fstream.close();
    }

    data_fstream.open(data_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!data_fstream.is_open()) {
        std::cout << "Error: failed to open data file.\n";
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    status = Status::OK;
    return Result::SUCCESS;
}

DataStorage::Result DataStorage::put_data(void *buf, ssize_t size) {
    data_fstream.seekp(0, std::ios::end);
    data_fstream.write(reinterpret_cast<char*>(buf), size);
    if (!data_fstream) {
        std::cout << "Error: failed to write data to file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;
}

DataStorage::Result DataStorage::get_data(void *buf, off_t offset, ssize_t size) {
    data_fstream.seekp(offset);
    data_fstream.read(reinterpret_cast<char*>(buf), size);
    if (!data_fstream) {
        std::cout << "Error: failed to read data from file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;
}

DataStorage::Result DataStorage::purge(off_t data_bytes_trimmed) {
    trim_file_from_beginning(data_fstream, data_bytes_trimmed, 4096);

    return Result::SUCCESS;
}