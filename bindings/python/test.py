import filemq
import time

queue = filemq.FileMQ()

result = queue.init("./test-queue")

if result == filemq.Result.FAILURE:
    quit()

test_string = "Hello world!"

start_time = time.time()

for i in range(100000):
    result = queue.enqueue(test_string, len(test_string))

for i in range(100000):
    result = queue.dequeue(len(test_string))
    id = result[1]
    result = queue.ack(id)

end_time = time.time()

print(end_time - start_time)