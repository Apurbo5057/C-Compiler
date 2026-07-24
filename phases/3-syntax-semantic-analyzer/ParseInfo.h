#ifndef PARSEINFO_HPP
#define PARSEINFO_HPP

#include <string>
#include <memory>

struct ParseInfo {
    int line_number = -1;
    std::string parsed_code;

    ParseInfo() = default;

    ParseInfo(int line, const std::string& code)
        : line_number(line), parsed_code(code) {}

};

#endif
