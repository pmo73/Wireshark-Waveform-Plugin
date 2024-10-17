#include "vcd_parser.hpp"

#include <iostream>
#include <ostream>
#include <sstream>

auto
vcd_parser::parse_header(std::string const &input) -> bool
{
    std::stringstream input_stream(input);
    std::string       segment;
    while (std::getline(input_stream, segment, ' ')) {
        char const first_character = static_cast<char>(std::tolower(segment.front()));
        std::cout << first_character << std::endl;
    }
    return true;
}
