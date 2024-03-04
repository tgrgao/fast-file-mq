#include <string>

class QueueLock {
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

        QueueLock();
        ~QueueLock();

        Result init(std::string queue_dir_path);

        Status get_status() const {return status;};

        Result lock();
        Result unlock();

    private:
        Status status;
};