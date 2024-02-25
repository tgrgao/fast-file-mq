#include <fstream>

class MetadataStorage {
    enum class Status;
    enum class Result;

    struct Metadata {
        unsigned queue_size;
        unsigned at_id;
        unsigned ready_count;
        unsigned unack_count;
        off_t start_ptr;
        off_t ack_ptr;
        off_t ready_ptr;
    };

    struct MetadataEntry {
        enum class EntryStatus {
            READY,
            UNACK,
            ACK,
            FACK
        };
        unsigned id;
        unsigned data_ptr;
        unsigned data_size;
        unsigned status;
    };

    Status status;
    struct Metadata metadata;
    std::fstream metadata_fstream;
    std::fstream entries_fstream;

    private:
        Result read_metadata();
        Result write_metadata();

    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED
        };
        
        enum class Result {
            SUCCESS,
            FAILURE
        };
        
        MetadataStorage(std::string queue_dir_path);
        ~MetadataStorage();

        Status get_status() const {return status;};
};
