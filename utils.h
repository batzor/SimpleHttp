#ifndef UTILS_H
#define UTILS_H

#include "macros.h"
#include <iostream>
#include <sys/stat.h>

// checks if string ends with the given string
inline bool strEndsWith(const std::string &s, const std::string &e) {
    return s.length() >= e.length()
        && s.compare(s.length() - e.length(), e.length(), e) == 0;
}

// checks if file exists and returns its size
// returns -1 in case the file does not exist
inline long getFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

inline void getFilePathFromUri(std::string uri, std::string &file_path) {
    size_t delim_idx = uri.find('?');
    file_path = SERVE_DIR + uri.substr(0, delim_idx);
}

inline void getMimeType(std::string &file_name, std::string &mime_type) {
    if(strEndsWith(file_name, ".html") || strEndsWith(file_name, ".htm"))
        mime_type = "text/html";
    else if(strEndsWith(file_name, ".css"))
        mime_type = "text/css";
}
#endif
