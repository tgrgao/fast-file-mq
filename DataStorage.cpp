#include "DataStorage.h"

#include <iostream>

DataStorage::DataStorage(std::string queue_dir_path) {
    std::string data_file_path = queue_dir_path + "/data.bytes";

    data_fstream.open(data_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!data_fstream.is_open()) {
        std::cout << "Error: failed to open metadata file.\n";
        status = Status::INITIALIZATION_FAILED;
        return;
    }

    status = Status::OK;
}

DataStorage::Result DataStorage::put_data(void *buf, ssize_t size) {
    data_fstream.seekp(0, std::ios::end);
    data_fstream.write(reinterpret_cast<char*>(buf), size);
    if (!data_fstream) {
        std::cout << "Error: failed to write data to file.\n";
    }
    return Result::FAILURE;
}

DataStorage::Result DataStorage::get_data(void *buf, off_t offset, ssize_t size) {
    data_fstream.seekp(offset);
    data_fstream.read(reinterpret_cast<char*>(buf), size);
    if (!data_fstream) {
        std::cout << "Error: failed to read data from file.\n";
    }
    return Result::FAILURE;
}