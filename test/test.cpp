#include "../file-mq.h"
#include <format>
#include <iostream>

int main(int argc, char *argv[]) {
    // if (argc != 2) {
    //     std::cout << "Usage: file-mq.out <directory_path";
    // }

    FileMQ myQ = FileMQ(".");
    int ret;

    char buf[10] = "Hello";
    unsigned id;
    ret = myQ.enqueue(buf, &id, 10);

    std::cout << ret << " " << "Enqueue id" << id << std::endl;

    char buf2[10];
    size_t size;
    myQ.dequeue(buf2, &id, &size, 10);
    std::cout << ret << " "  << "Dequeue id" << id << " " << buf2 << std::endl;
}