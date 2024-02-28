#include <fstream>

class DataStorage {
    enum class Status;
    enum class Result;

    private:
        Status status;
        std::fstream data_fstream;

    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED
        };
        
        enum class Result {
            SUCCESS,
            FAILURE
        };

        DataStorage(std::string queue_dir_path);
        ~DataStorage();

        Status get_status() const {return status;};

        Result put_data(void *buf, ssize_t size);
        Result get_data(void *buf, off_t offset, ssize_t size);
};