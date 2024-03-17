# FileMQ
This project is inspired by [persist-queue](https://github.com/peter-wangxu/persist-queue/tree/master). The acknowledgement queue in persist-queue uses SQLite to implement its persistent acknowledgement queue, but it has been found to not be performant when the queue becomes too large. Investigations have shown that the performance bottleneck is in how SQLite is used as the queue's persistence engine. This project borrows many aspects of the interface from persist-queue's SQLAckQueue and seeks to implement a persistent acknowledgement queue with improved performance by implementing in C++ and writing directly to metadata and data files instead of using SQLite. This queue is both thread-safe and process-safe.

## Usage
FileMQ persists its data by writing every change of queue state into metadata and data files that are stored in a directory, whose path is passed as part of the queue's initialization method.

`FileMQ test_queue;`
`test_queue.init("test-queue");`

The initialization method, if in the passed directory there doesn't already exist, creates four files: `file.lock`, `metadata.bytes`, `entries.bytes`, `data.bytes`. `file.lock` is an empty file used to provide process-safety through the taking of file locks on it. `metadata.bytes` contains the metadata of the queue as a whole. `entries.bytes` contains the metadata of each element of the queue. `data.bytes` contains the data values of the elements of the queue.

The entirety of a queue's state is stored in these files. Operations on the queue involve directly reading and writing to these files. This means that if a program using the queue terminates, the queue at the time of termination persists in these files and can be used again in another program that initializes a queue with these files.

The queue is also thread-safe and process-safe, allowing multiple threads in a process to access the same queue instance, as well as multiple processing to concurrently initialize on the same files and share access to the samequeue. Thread-safety is achieved using a`std::mutex` in the C++ standard library. Process-safety is achieved using file locks on `file.lock`. The locking semantics allow only one thread or process to concurrently perform operations on the same queue, forcing other threads or processes to have to wait until the current access is completed.