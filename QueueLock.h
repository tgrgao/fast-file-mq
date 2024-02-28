#include <string>

class QueueLock {
    enum class Status;
    enum class Result;

    private:
        Status status;

    public:
        enum class Status {
            OK,
            INITIALIZATION_FAILED
        };

        enum class Result {
            SUCCESS,
            FAILURE
        };

        QueueLock(std::string queue_file_path);
        ~QueueLock();

        Status get_status() const {return status;};

        Result lock();
        Result unlock();
};