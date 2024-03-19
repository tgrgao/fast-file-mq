#include <pybind11/pybind11.h>
#include "/Users/tigergao/tigerzone/tgrgao/file-mq/src/FileMQ.h"

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
        .def("enqueue", &FileMQ::enqueue)
        .def("dequeue", &FileMQ::dequeue)
        .def("ack", &FileMQ::ack)
        .def("fack", &FileMQ::fack)
        .def("nack", &FileMQ::nack)
        .def("purge", &FileMQ::purge)
    ;
}