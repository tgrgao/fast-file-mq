#include <fstream>

class MetadataStorage {
    enum class Status;
    enum class Result;

    struct Metadata {
        unsigned queue_size;
        unsigned smallest_id;
        unsigned at_id;
        unsigned ready_count;
        unsigned unack_count;
        off_t start_ptr;
        off_t ack_ptr;
        off_t ready_ptr;
        off_t data_end_ptr;
    };

    struct MetadataEntry {
        enum class EntryStatus {
            READY,
            UNACK,
            ACK,
            FACK
        };
        unsigned id;
        off_t data_off;
        unsigned data_size;
        EntryStatus status;
    };

    Status status;
    struct Metadata metadata;
    std::fstream metadata_fstream;
    std::fstream entries_fstream;

    private:
        Result read_metadata();
        Result write_metadata();
        Result read_entry(struct MetadataEntry *metadata_entry, unsigned position);
        Result write_entry(struct MetadataEntry *metadata_entry, unsigned position);

    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED,
            STALE_METADATA
        };
        
        enum class Result {
            SUCCESS,
            FAILURE
        };
        
        MetadataStorage(std::string queue_dir_path);
        ~MetadataStorage();

        Status get_status() const {return status;};
        void make_stale() {status = Status::STALE_METADATA;};
        off_t get_data_end_ptr() const {return metadata.data_end_ptr;};

        Result enqueue(unsigned *id, ssize_t size);
        Result dequeue(unsigned id, off_t *data_off, ssize_t *size);
};
