#include <string>
#include <mutex>
#include <fcntl.h>

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
        std::mutex thread_mutex;
        int file_lock_fd;
        struct flock file_lock;
};