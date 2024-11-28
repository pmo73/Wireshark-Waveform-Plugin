#ifndef VCD_PARSER_HPP
#define VCD_PARSER_HPP

#include "vcd_file_input.hpp"

#include <memory>
#include <string>
#include <vector>

namespace vcd_parser
{
    auto parse_header(std::string const &input, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto
    parse_commands(std::vector<std::string> const &commands, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto parse_timescale(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput                *file_input) -> bool;
    auto parse_scope(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput *file_input, bool is_up) -> bool;
    auto parse_header_var(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput                 *file_input) -> bool;
    auto
    parse_text_line(std::string const &input, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto process_timestamp(std::string const &input, vcd_file_input::VcdFileInput *const file_input)
            -> bool;
    auto process_single_bit(std::string const  &input,
            vcd_file_input::VcdFileInput *const file_input) -> bool;
    auto generate_packet(vcd_file_input::VcdFileInput *const file_input) -> bool;

    namespace helper
    {
        auto string_append_and_replace(std::string &dest, std::string const &src) -> void;
    } // namespace helper
} // namespace vcd_parser
#endif // VCD_PARSER_HPP
