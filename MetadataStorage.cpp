#include "MetadataStorage.h"

#include "iostream"

MetadataStorage::MetadataStorage(std::string queue_dir_path) {
    std::string metadata_file_path = queue_dir_path + "/metadata.bytes";
    std::string entries_file_path = queue_dir_path + "/entries.bytes";

    metadata_fstream.open(metadata_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!metadata_fstream.is_open()) {
        std::cout << "Error: failed to open metadata file.\n";
        status = Status::INITIALIZATION_FAILED;
        return;
    }

    entries_fstream.open(entries_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!entries_fstream.is_open()) {
        std::cout << "Error: failed to open entries file.\n";
        status = Status::INITIALIZATION_FAILED;
        return;
    }

    if (std::filesystem::exists(metadata_file_path)) {
        std::cout << "Found existing metadata. Restoring queue.\n";
        if (read_metadata() != Result::SUCCESS) {
            status = Status::INITIALIZATION_FAILED;
            return;
        }
    } else {
        std::cout << "No existing metadata found. Creating fresh queue.\n";
        metadata.queue_size = 0;
        metadata.at_id = 0;
        metadata.ready_count = 0;
        metadata.unack_count = 0;
        metadata.start_ptr = 0;
        metadata.ack_ptr = 0;
        metadata.ready_ptr = 0;
        if (write_metadata() != Result::SUCCESS) {
            status = Status::INITIALIZATION_FAILED;
            return;
        }
    }

    status = Status::OK;
}

MetadataStorage::Result MetadataStorage::read_metadata() {
    metadata_fstream.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));
    if (!metadata_fstream) {
        std::cout << "Error: failed to read metadata from file.\n";
    }
    return Result::FAILURE;
}

MetadataStorage::Result MetadataStorage::write_metadata() {
    metadata_fstream.write(reinterpret_cast<char*>(&metadata), sizeof(metadata));
    if (!metadata_fstream) {
        std::cout << "Error: failed to write metadata to file.\n";
    }
    return Result::FAILURE;
}