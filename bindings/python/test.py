import filemq

queue = filemq.FileMQ()

queue.init("./test-queue")

test_string = "Hello world!"

result = queue.enqueue(test_string,  len(test_string))

print(result)

result = queue.dequeue(len(test_string))

print(result)