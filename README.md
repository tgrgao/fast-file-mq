# FileMQ
This project is inspired by [persist-queue](https://github.com/peter-wangxu/persist-queue/tree/master). The acknowledgement queue in persist-queue uses SQLite to implement its persistent acknowledgement queue, but it has been found to not be performant when the queue becomes too large. Investigations have shown that the performance bottleneck is in how SQLite is used as the queue's persistence engine. This project borrows many aspects of the interface and queue semantics from persist-queue's SQLAckQueue and seeks to implement a persistent acknowledgement queue with improved performance by implementing in C++ and writing directly to metadata and data files instead of using SQLite. This queue is both thread-safe and process-safe.

## Usage
FileMQ persists its data by writing every change of queue state into metadata and data files that are stored in a directory, whose path is passed as part of the queue's initialization method.

    FileMQ queue;
    queue.init("./my-queue");

The initialization method, if in the passed directory there doesn't already exist, creates four files: `file.lock`, `metadata.bytes`, `entries.bytes`, `data.bytes`. `file.lock` is an empty file used to provide process-safety through the taking of file locks on it. `metadata.bytes` contains the metadata of the queue as a whole. `entries.bytes` contains the metadata of each element of the queue. `data.bytes` contains the data values of the elements of the queue.

The entirety of a queue's state is stored in these files. Operations on the queue involve directly reading and writing to these files. This means that if a program using the queue terminates, the queue at the time of termination persists in these files and can be used again in another program that initializes a queue with these files.

The queue is also thread-safe and process-safe, allowing multiple threads in a process to access the same queue instance, as well as multiple processing to concurrently initialize on the same files and share access to the samequeue. Thread-safety is achieved using a`std::mutex` in the C++ standard library. Process-safety is achieved using file locks on `file.lock`. The locking semantics allow only one thread or process to concurrently perform operations on the same queue, forcing other threads or processes to have to wait until the current access is completed.

Data is operated on in the queue as bytes. To enqueue an element, the `enqueue` method is called, along with the address of the data to be enqueued and the length of the data, as well as a pointer to an integer that an id value for the element in the queue would be written to. Queued elements are initially given a status of `READY`.

    FileMQ::Result result;
    unsigned id;
    int value = 123;
    result = queue.enqueue(&value, &id, sizeof(value));

To dequeue an element, the `dequeue` element is called, along with the address of where to write the dequeued data to, a maximum size of data to read, a pointer to an integer for the actual number of bytes read from queue is, and a pointer to an integer for the id of the element dequeued. The element that is dequeued is the earliest enqueued element that has a status of `READY`.

    ssize_t size;
    result = queue.dequeue(&value, &id, &size, sizeof(value));

After dequeueing an element, the element is given a status of `UNACK`. The application logic then processes the dequeued data and can call either the `ack` or `fack` (failed ack) method for the element, by id, which would give the element the status of `ACK` or `FACK`. This would semantically remove the element from the queue. Alternatively, to allow the element to be dequeued again, the `nack` method can be called, by id, which would give the element the `READY` status again.

    result = queue.ack(id);
            or
    result = queue.fack(id);
            or
    result = queue.nack(id);

Since the data and metadata of the queue is stored in files, as the queue has elements enqueued, the size of the `entries.bytes` and `data.bytes` files will grow as well. When elements are dequeued and acked or facked, these files DO NOT shrink in size. Over continuous use, these files can grow to a size that can affect queue performance or is just prohibitively large. To address this, the `purge` method can be called to garbage-collect and trim the files down by removing from `entries.bytes` and `data.bytes` all the metadata and data related to the elements before the first element that does not have status `ACK` or `FACK`. (NOTE: This does mean that if the very first element is left in an `UNACK` state, even if all the subsequent elements are `ACK` or `FACK`, calling `purge` WILL NOT reduce the size of the files, as the aforementioned first element is the first element of the queue.)

    result = queue.purge();