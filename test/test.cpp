#include "../src/FileMQ.h"
#include <format>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[]) {
    FileMQ myQ = FileMQ("./queue");
    FileMQ::Result ret;

    char buf[10] = "Hello";
    unsigned id;
    ret = myQ.enqueue(buf, &id, 10);
    strcpy(buf, "world!");
    ret = myQ.enqueue(buf, &id, 10);

    // std::cout << ret << " " << "Enqueue id " << id << std::endl;

    char buf2[10];
    ssize_t size;
    ret = myQ.dequeue(buf2, &id, &size, 10);
    std::cout << buf2 << std::endl;
    ret = myQ.dequeue(buf2, &id, &size, 10);
    std::cout << buf2 << std::endl;
    // std::cout << ret << " "  << "Dequeue id " << id << " " << buf2 << std::endl;
}