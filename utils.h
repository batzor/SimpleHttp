#ifndef UTILS_H
#define UTILS_H

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
#endif
