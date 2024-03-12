#ifndef HELPER_H
#define HELPER_H

#include <fstream>

void trim_file_from_beginning(std::fstream& file, off_t n, std::string queue_dir_path, std::string file_name);

#endif