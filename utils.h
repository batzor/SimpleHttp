#ifndef UTILS_H
#define UTILS_H

#include "macros.h"
#include <cstring>
#include <iostream>
#include <sys/stat.h>

// checks if string ends with the given string
inline bool strEndsWith(char *s, const char e[]) {
    size_t len1 = strlen(s);
    size_t len2 = strlen(e);
    
    return len1 >= len2
        && strcmp(s + len1 - len2, e) == 0;
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

inline void getMimeType(char *file_name, std::string &mime_type) {
    if(strEndsWith(file_name, ".html") || strEndsWith(file_name, ".htm"))
        mime_type = "text/html";
    else if(strEndsWith(file_name, ".css"))
        mime_type = "text/css";
}
#endif
