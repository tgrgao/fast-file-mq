#include <fstream>

class DataStorage {
    public:
        enum class Status {
            NOT_INITIALIZED,
            OK,
            INITIALIZATION_FAILED
        };
        
        enum class Result {
            SUCCESS,
            FAILURE
        };

        DataStorage();
        ~DataStorage();

        Result init(std::string queue_dir_path);

        Status get_status() const {return status;};

        Result put_data(void *buf, ssize_t size);
        Result get_data(void *buf, off_t offset, ssize_t size);

    private:
        Status status;
        std::fstream data_fstream;
};