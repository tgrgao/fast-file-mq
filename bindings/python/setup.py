from glob import glob
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "filemq",
        sorted(["./binding.cpp"] + glob("/Users/tigergao/tigerzone/tgrgao/file-mq/src/*.cpp")),  # Sort source files for reproducibility,
        cxx_std=20
    ),
]

setup(name="filemq", ext_modules=ext_modules)