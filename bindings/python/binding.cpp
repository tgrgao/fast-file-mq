#include <pybind11/pybind11.h>
#include "/Users/tigergao/tigerzone/tgrgao/file-mq/src/FileMQ.h"

#define DATA_MAX_SIZE 4096

namespace py = pybind11;

PYBIND11_MODULE(filemq, m) {
    // m.def("hello", &FileMQ::hello);

    py::enum_<FileMQ::Result>(m, "Result")
        .value("SUCCESS", FileMQ::Result::SUCCESS)
        .value("FAILURE", FileMQ::Result::FAILURE)
        .value("CRITICAL_FAILURE", FileMQ::Result::CRITICAL_FAILURE)
        .value("SUCCESS_TRUNCATED_READ", FileMQ::Result::SUCCESS_TRUNCATED_READ)
        .value("QUEUE_EMPTY", FileMQ::Result::QUEUE_EMPTY)
        .export_values()
    ;

    py::class_<FileMQ>(m, "FileMQ")
        .def(py::init<>())
        .def("init", &FileMQ::init)
        .def("enqueue", [](FileMQ &queue, char *buf, int size){
            unsigned id;
            FileMQ::Result result = queue.enqueue(buf, &id, size);
            return py::make_tuple(result, id);
        })
        .def("dequeue", [](FileMQ &queue, ssize_t max_size){
            unsigned id;
            ssize_t size;
            char* buf = new char[max_size];
            FileMQ::Result result = queue.dequeue(buf, &id, &size, max_size);
            std::string s(buf, size);
            free(buf);
            return py::make_tuple(result, id, py::bytes(s));
        })
        .def("ack", &FileMQ::ack)
        .def("fack", &FileMQ::fack)
        .def("nack", &FileMQ::nack)
        .def("purge", &FileMQ::purge)
    ;
}