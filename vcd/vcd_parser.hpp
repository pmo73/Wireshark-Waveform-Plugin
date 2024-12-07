#ifndef VCD_PARSER_HPP
#define VCD_PARSER_HPP

#include "vcd_file_input.hpp"

#include <memory>
#include <string>
#include <vector>

namespace vcd_parser
{

    static constexpr std::uint8_t DECIMAL_BASE = 10;
    static constexpr double       QUOTIENT_FS  = 1e15;
    static constexpr double       QUOTIENT_PS  = 1e12;
    static constexpr double       QUOTIENT_NS  = 1e9;
    static constexpr double       QUOTIENT_US  = 1e6;
    static constexpr double       QUOTIENT_MS  = 1e3;
    static constexpr double       QUOTIENT_S   = 1;

    auto parse_header(std::string const &input, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto split_payload_commands(std::string const &input, vcd_file_input::VcdFileInput *file_input)
            -> bool;
    auto parse_commands(std::vector<std::string> const &commands,
            vcd_file_input::VcdFileInput               *file_input) -> bool;
    auto parse_timescale(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput                *file_input) -> bool;
    auto parse_scope(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput *file_input, bool is_up) -> bool;
    auto parse_header_var(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput                 *file_input) -> bool;
    auto parse_dumpvars(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput               *file_input) -> bool;
    auto
    parse_text_line(std::string const &input, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto
    process_timestamp(std::string const &input, vcd_file_input::VcdFileInput *file_input) -> bool;
    auto get_current_timestamp_from_command(std::string const &input) -> std::int64_t;
    auto process_single_bit(std::string const &input, vcd_file_input::VcdFileInput *file_input,
            bool dumpvars = false) -> bool;
    auto process_multi_bit(std::vector<std::string> const &input,
            vcd_file_input::VcdFileInput *file_input, bool dumpvars = false) -> bool;
    auto generate_packet(vcd_file_input::VcdFileInput *file_input) -> bool;

    namespace helper
    {
        auto string_append_and_replace(std::string &dest, std::string const &src) -> void;
    } // namespace helper
} // namespace vcd_parser
#endif // VCD_PARSER_HPP
