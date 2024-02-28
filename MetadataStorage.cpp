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
        metadata.smallest_id = 0;
        metadata.at_id = 0;
        metadata.ready_count = 0;
        metadata.unack_count = 0;
        metadata.start_ptr = 0;
        metadata.ack_ptr = 0;
        metadata.ready_ptr = 0;
        metadata.data_end_ptr = 0;
        if (write_metadata() != Result::SUCCESS) {
            status = Status::INITIALIZATION_FAILED;
            return;
        }
    }

    status = Status::OK;
}

MetadataStorage::~MetadataStorage() {}

MetadataStorage::Result MetadataStorage::read_metadata() {
    metadata_fstream.seekg(0, std::ios_base::beg);
    metadata_fstream.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));
    if (!metadata_fstream) {
        std::cout << "Error: failed to read metadata from file.\n";
        return Result::FAILURE;
    }
    status = Status::OK;
    return Result::SUCCESS;   
}

MetadataStorage::Result MetadataStorage::write_metadata() {
    metadata_fstream.seekp(0, std::ios_base::beg);
    metadata_fstream.write(reinterpret_cast<char*>(&metadata), sizeof(metadata));
    if (!metadata_fstream) {
        std::cout << "Error: failed to write metadata to file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::read_entry(struct MetadataEntry *metadata_entry, unsigned position) {
    entries_fstream.seekg(position * sizeof(struct MetadataEntry), std::ios_base::beg);
    entries_fstream.read(reinterpret_cast<char*>(metadata_entry), sizeof(struct MetadataEntry));
    if (!entries_fstream) {
        std::cout << "Error: failed to read entry from file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;  
}

MetadataStorage::Result MetadataStorage::write_entry(struct MetadataEntry *metadata_entry, unsigned position) {
    entries_fstream.seekp(position * sizeof(struct MetadataEntry), std::ios_base::beg);
    entries_fstream.write(reinterpret_cast<char*>(metadata_entry), sizeof(struct MetadataEntry));
    if (!entries_fstream) {
        std::cout << "Error: failed to read entry from file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::enqueue(unsigned *id, ssize_t size) {
    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    struct MetadataEntry entry;
    entry.id = metadata.at_id;
    entry.data_off = metadata.data_end_ptr;
    entry.data_size = size;
    entry.status = MetadataEntry::EntryStatus::READY;

    *id = metadata.at_id;

    unsigned entry_pos = (entry.id - metadata.smallest_id);

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    ++metadata.queue_size;
    ++metadata.ready_ptr;
    ++metadata.ready_count;
    ++metadata.at_id;
    metadata.data_end_ptr += size;

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::dequeue(unsigned id, off_t *data_off, ssize_t *size) {
    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    struct MetadataEntry entry;
    unsigned entry_pos = id - metadata.smallest_id;

    if (read_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    *data_off = entry.data_off;
    *size = entry.data_size;
    entry.status = MetadataEntry::EntryStatus::UNACK;

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    ++metadata.unack_count;

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}