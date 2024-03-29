#include "MetadataStorage.h"
#include "helper.h"

#include <iostream>
#include <vector>

MetadataStorage::MetadataStorage() {
    status = Status::NOT_INITIALIZED;
}

MetadataStorage::~MetadataStorage() {}

MetadataStorage::Result MetadataStorage::init(std::string queue_dir_path) {
    this->queue_dir_path = queue_dir_path;
    std::string metadata_file_path = queue_dir_path + "/metadata.bytes";
    std::string entries_file_path = queue_dir_path + "/entries.bytes";

    if (std::filesystem::exists(metadata_file_path)) {
        std::cout << "Found existing metadata. Restoring queue.\n";

        metadata_fstream.open(metadata_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!metadata_fstream.is_open()) {
            std::cout << "Error: failed to open metadata file.\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }

        if (read_metadata() != Result::SUCCESS) {
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
    } else {
        std::cout << "No existing metadata found. Creating fresh queue.\n";

        metadata_fstream.open(metadata_file_path, std::ios::out | std::ios::binary);
        if (!metadata_fstream.is_open()) {
            std::cout << "Error: failed to create metadata file.\n";
            std::cout << std::filesystem::current_path() << "\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
        metadata_fstream.close();

        metadata_fstream.open(metadata_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!metadata_fstream.is_open()) {
            std::cout << "Error: failed to open metadata file.\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }

        metadata.total_size = 0;
        metadata.queue_size = 0;
        metadata.smallest_id = 0;
        metadata.at_id = 0;
        metadata.ready_count = 0;
        metadata.unack_count = 0;
        metadata.ack_ptr = 0;
        metadata.ready_ptr = 0;
        metadata.data_end_ptr = 0;

        if (write_metadata() != Result::SUCCESS) {
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
    }

    if (!std::filesystem::exists(entries_file_path)) {
        entries_fstream.open(entries_file_path, std::ios::out | std::ios::binary);
        if (!entries_fstream.is_open()) {
            std::cout << "Error: failed to create entries file.\n";
            status = Status::INITIALIZATION_FAILED;
            return Result::FAILURE;
        }
        entries_fstream.close();
    }

    entries_fstream.open(entries_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!entries_fstream.is_open()) {
        std::cout << "Error: failed to open entries file.\n";
        status = Status::INITIALIZATION_FAILED;
        return Result::FAILURE;
    }

    status = Status::OK;
    return Result::SUCCESS;
}

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
        std::cout << position << " " << position * sizeof(struct MetadataEntry) << "\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;  
}

MetadataStorage::Result MetadataStorage::write_entry(struct MetadataEntry *metadata_entry, unsigned position) {
    entries_fstream.seekp(position * sizeof(struct MetadataEntry), std::ios_base::beg);
    entries_fstream.write(reinterpret_cast<char*>(metadata_entry), sizeof(struct MetadataEntry));
    if (!entries_fstream) {
        std::cout << "Error: failed to write entry to file.\n";
        return Result::FAILURE;
    }
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::advance_ready_ptr() {
    ++metadata.ready_ptr;
    while (metadata.ready_ptr < metadata.at_id) {
        MetadataEntry entry;
        unsigned pos = metadata.ready_ptr - metadata.smallest_id;
        if (read_entry(&entry, pos) != Result::SUCCESS) {
            return Result::FAILURE;
        }
        if (entry.status == MetadataEntry::EntryStatus::READY) {
            break;
        }
    }
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::advance_ack_ptr() {
    ++metadata.ack_ptr;
    while (metadata.ack_ptr < metadata.at_id) {
        MetadataEntry entry;
        unsigned pos = metadata.ack_ptr - metadata.smallest_id;
        if (read_entry(&entry, pos) != Result::SUCCESS) {
            return Result::FAILURE;
        }
        if (entry.status != MetadataEntry::EntryStatus::ACK || entry.status != MetadataEntry::EntryStatus::FACK) {
            break;
        }
    }
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::get_queue_size(unsigned *queue_size) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }
    *queue_size = metadata.queue_size;
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::get_total_size(unsigned *total_size) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }
    *total_size = metadata.total_size;
    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::enqueue(unsigned *id, ssize_t size) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

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

    ++metadata.total_size;
    ++metadata.queue_size;
    ++metadata.ready_count;
    ++metadata.at_id;
    metadata.data_end_ptr += size;

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::dequeue(unsigned *id, off_t *data_off, ssize_t *size) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (metadata.ready_ptr >= metadata.at_id) {
        return Result::QUEUE_EMPTY;
    }

    struct MetadataEntry entry;
    unsigned entry_pos = metadata.ready_ptr - metadata.smallest_id;

    if (read_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    *id = entry.id;
    *data_off = entry.data_off;
    *size = entry.data_size;
    entry.status = MetadataEntry::EntryStatus::UNACK;

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    --metadata.ready_count;
    ++metadata.unack_count;

    if (advance_ready_ptr() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::ack(unsigned id) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    struct MetadataEntry entry;
    unsigned entry_pos = id - metadata.smallest_id;

    if (read_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (entry.status != MetadataEntry::EntryStatus::UNACK) {
        return Result::FAILURE;
    }

    entry.status = MetadataEntry::EntryStatus::ACK;

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    --metadata.unack_count;
    --metadata.queue_size;
    
    if (advance_ack_ptr() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::fack(unsigned id) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    struct MetadataEntry entry;
    unsigned entry_pos = id - metadata.smallest_id;

    if (read_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (entry.status != MetadataEntry::EntryStatus::UNACK) {
        return Result::FAILURE;
    }

    entry.status = MetadataEntry::EntryStatus::FACK;

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    --metadata.unack_count;
    --metadata.queue_size;
    
    if (advance_ack_ptr() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::nack(unsigned id) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    struct MetadataEntry entry;
    unsigned entry_pos = id - metadata.smallest_id;

    if (read_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (entry.status != MetadataEntry::EntryStatus::UNACK) {
        return Result::FAILURE;
    }

    entry.status = MetadataEntry::EntryStatus::READY;

    if (write_entry(&entry, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    --metadata.unack_count;
    ++metadata.ready_count;
    
    if (id < metadata.ready_ptr) {
        metadata.ready_ptr = id;
    }

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}

MetadataStorage::Result MetadataStorage::purge(off_t *data_bytes_trimmed) {
    if (status != Status::OK && status != Status::STALE_METADATA) {
        return Result::FAILURE;
    }

    if (status == Status::STALE_METADATA && read_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    if (metadata.ack_ptr == metadata.at_id) {
        return Result::SUCCESS; // nothing to do
    }

    struct MetadataEntry first_not_acked;
    unsigned entry_pos = metadata.ack_ptr - metadata.smallest_id;
    if (read_entry(&first_not_acked, entry_pos) != Result::SUCCESS) {
        return Result::FAILURE;
    }

    *data_bytes_trimmed = first_not_acked.data_off;
    off_t entries_bytes_trimmed = entry_pos * sizeof(struct MetadataEntry);

    trim_file_from_beginning(entries_fstream, entries_bytes_trimmed, queue_dir_path, "/entries.bytes");

    metadata.smallest_id = metadata.ack_ptr;
    metadata.data_end_ptr -= *data_bytes_trimmed;
    metadata.total_size = metadata.at_id - metadata.smallest_id;
    metadata.queue_size = 0;

    // update the data offset values in all the remaining entries
    for (unsigned i = metadata.smallest_id; i < metadata.at_id; ++i) {
        MetadataEntry entry;
        unsigned pos = i - metadata.smallest_id;
        if (read_entry(&entry, pos) != Result::SUCCESS) {
            return Result::FAILURE;
        }
        if (entry.status == MetadataEntry::EntryStatus::READY || entry.status == MetadataEntry::EntryStatus::UNACK) {
            ++metadata.queue_size;
        }
        entry.data_off -= *data_bytes_trimmed;
        if (write_entry(&entry, pos) != Result::SUCCESS) {
            return Result::FAILURE;
        }
    }

    if (write_metadata() != Result::SUCCESS) {
        return Result::FAILURE;
    }

    return Result::SUCCESS;
}