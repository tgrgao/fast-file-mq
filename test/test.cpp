#include "../file-mq.h"
#include <format>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[]) {
    // if (argc != 2) {
    //     std::cout << "Usage: file-mq.out <directory_path";
    // }

    FileMQ myQ = FileMQ(".");
    int ret;

    char buf[10] = "Hello";
    unsigned id;
    ret = myQ.enqueue(buf, &id, 10);
    strcpy(buf, "world!");
    ret = myQ.enqueue(buf, &id, 10);

    // std::cout << ret << " " << "Enqueue id " << id << std::endl;

    char buf2[10];
    size_t size;
    ret = myQ.dequeue(buf2, &id, &size, 10);
    std::cout << buf2 << std::endl;
    ret = myQ.dequeue(buf2, &id, &size, 10);
    std::cout << buf2 << std::endl;
    // std::cout << ret << " "  << "Dequeue id " << id << " " << buf2 << std::endl;
}