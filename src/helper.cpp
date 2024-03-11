#include "helper.h"

#include <vector>

#include "iostream"

void trim_file_from_beginning(std::fstream& file, off_t n, ssize_t chunk_size) {
    // TODO: This does not actually resize the file yet. This just lazily trims the first n bytes of by shifting all the data from byte n + 1 onwards to the beginning. fstreams do not support a truncation function, which has to be done with std::filesytem::resize_file, which take the file path as an argument, but for now we do not save the file paths after init.

    //TODO: SHOULD CHECK RETURN CODES AND RETURN ONE ITSELF

    // Move file pointer to the (n+1)-th byte from the beginning
    file.seekg(n, std::ios::beg);

    // Create a buffer to read and write data in chunks
    std::vector<char> buffer(chunk_size);

    off_t bytes_written = 0;

    // Read and write data in chunks
    do {
        // std::cout << ".";
        file.read(buffer.data(), chunk_size);
        std::streamsize bytes_read = file.gcount();
        file.seekp(bytes_written, std::ios::beg);
        file.write(buffer.data(), bytes_read);
        bytes_written += bytes_read;
        file.seekg(n + bytes_written, std::ios::cur);
    }
    while (!file.eof());
}