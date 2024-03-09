#include <fstream>

class MetadataStorage {

    public:
        enum class Status {
            NOT_INITIALIZED,
            OK,
            INITIALIZATION_FAILED,
            STALE_METADATA
        };
        
        enum class Result {
            SUCCESS,
            FAILURE,
            QUEUE_EMPTY
        };
        
        MetadataStorage();
        ~MetadataStorage();

        Result init(std::string queue_dir_path);

        Status get_status() const {return status;};
        Result get_queue_size(unsigned *queue_size);
        void make_stale() {status = Status::STALE_METADATA;};
        off_t get_data_end_ptr() const {return metadata.data_end_ptr;};

        Result enqueue(unsigned *id, ssize_t size);
        Result dequeue(unsigned *id, off_t *data_off, ssize_t *size);
        Result ack(unsigned id);
        Result fack(unsigned id);
        Result nack(unsigned id);

        Result purge(off_t *data_bytes_trimmed);
    
    private: 

        struct Metadata {
            unsigned queue_size;
            unsigned smallest_id;
            unsigned at_id;
            unsigned ready_count;
            unsigned unack_count;
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

        Result read_metadata();
        Result write_metadata();
        Result read_entry(struct MetadataEntry *metadata_entry, unsigned position);
        Result write_entry(struct MetadataEntry *metadata_entry, unsigned position);

        Result advance_ready_ptr();
        Result advance_ack_ptr();
};
