import filemq

queue = filemq.FileMQ()

queue.init("./test-queue")