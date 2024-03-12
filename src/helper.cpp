#include "helper.h"

#include <vector>

#include <filesystem>
#include <iostream>

const std::size_t CHUNK_SIZE = 4096;

void trim_file_from_beginning(std::fstream& file, off_t n, std::string queue_dir_path, std::string file_name) {
    //TODO: SHOULD CHECK RETURN CODES AND RETURN ONE ITSELF

    std::fstream temp_file = std::fstream(queue_dir_path + "/temp.bytes", std::ios::out | std::ios::binary);

    // Move file pointer to the (n+1)-th byte from the beginning
    file.seekg(n, std::ios::beg);

    // Create a buffer to read and write data in chunks
    std::vector<char> buffer(CHUNK_SIZE);

    // Read and write data in chunks
    while (!file.eof()) {
        file.read(buffer.data(), CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();
        temp_file.write(buffer.data(), bytes_read);
    }

    // Close fstreams and rename temp file to file
    file.close();
    temp_file.close();
    std::filesystem::rename(queue_dir_path + "/temp.bytes", queue_dir_path + file_name);

    file.open(queue_dir_path + file_name);
}