#include "vcd_parser.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <ostream>
#include <regex>
#include <sstream>

auto
vcd_parser::parse_header(std::string const &input, vcd_file_input::VcdFileInput *const file_input)
        -> bool
{
    std::string              input_copy = input;
    std::vector<std::string> commands {};
    std::size_t              pos       = 0;
    std::string const        delimiter = "$end ";
    commands.reserve(input_copy.size());
    while ((pos = input_copy.find(delimiter)) != std::string::npos) {
        if (std::string const token = input_copy.substr(0, pos - 1);
                token != " " and not token.empty()) {
            commands.emplace_back(token);
        }
        input_copy.erase(0, pos + delimiter.length());
    }
    if (input_copy != " " and not input_copy.empty()) {
        commands.emplace_back(input_copy);
    }
    return parse_commands(commands, file_input);
}

auto
vcd_parser::split_payload_commands(std::string const &input,
        vcd_file_input::VcdFileInput *const           file_input) -> bool
{

    std::regex const           rgx("(#[0-9]+)(((?!#[0-9]+).)*)");
    std::string                input_copy   = input;
    constexpr int              submatches[] = { 1, 2 };
    std::sregex_token_iterator iter_timestamp(input_copy.begin(), input_copy.end(), rgx,
            submatches);
    for (std::sregex_token_iterator const end; iter_timestamp != end; ++iter_timestamp) {
        std::string token = *iter_timestamp;
        if (iter_timestamp != end) {
            ++iter_timestamp;
            token.append(*iter_timestamp);
        }
        file_input->payload_commands.emplace_back(token);
    }

    for (auto const &[identifier, signal] : file_input->signal_map) {
        signal->data.reserve(file_input->payload_commands.size());
    }
    return true;
}

auto
vcd_parser::parse_commands(std::vector<std::string> const &commands,
        vcd_file_input::VcdFileInput *const                file_input) -> bool
{

    for (auto const &command : commands) {
        std::stringstream        input_stream(command);
        std::string              segment;
        std::vector<std::string> seglist;

        while (std::getline(input_stream, segment, ' ')) {
            seglist.push_back(segment);
        }
        if (seglist[0] == "$timescale") {
            parse_timescale(seglist, file_input);
        }
        if (seglist[0] == "$scope") {
            parse_scope(seglist, file_input, false);
        }
        if (seglist[0] == "$upscope") {
            parse_scope(seglist, file_input, true);
        }
        if (seglist[0] == "$var") {
            parse_header_var(seglist, file_input);
        }
    }
    return true;
}
auto
vcd_parser::parse_timescale(std::vector<std::string> const &input,
        vcd_file_input::VcdFileInput *const                 file_input) -> bool
{
    std::uint64_t quotient {};
    if (input.size() > 2) {
        if (input[2] == "fs") {
            quotient              = QUOTIENT_FS;
            file_input->timescale = vcd_file_input::TIMESCALE::FEMTOSECONDS;
        } else if (input[2] == "ps") {
            quotient              = QUOTIENT_PS;
            file_input->timescale = vcd_file_input::TIMESCALE::PICOSECONDS;
        } else if (input[2] == "ns") {
            quotient              = QUOTIENT_NS;
            file_input->timescale = vcd_file_input::TIMESCALE::NANOSECONDS;
        } else if (input[2] == "us") {
            quotient              = QUOTIENT_US;
            file_input->timescale = vcd_file_input::TIMESCALE::MICROSECONDS;
        } else if (input[2] == "ms") {
            quotient              = QUOTIENT_MS;
            file_input->timescale = vcd_file_input::TIMESCALE::MILLISECONDS;
        } else if (input[2] == "s") {
            quotient              = QUOTIENT_S;
            file_input->timescale = vcd_file_input::TIMESCALE::SECONDS;
        } else {
            throw std::logic_error("Unknown time scale");
            return false;
        }
        file_input->timescale_factor = std::strtoull(input[1].c_str(), nullptr, DECIMAL_BASE);
        file_input->sample_rate      = quotient / file_input->timescale_factor;
        return true;
    }
    return false;
}

auto
vcd_parser::parse_scope(std::vector<std::string> const &input,
        vcd_file_input::VcdFileInput *const file_input, bool const is_up) -> bool
{
    if (is_up) {
        file_input->append_to_common_module = true;
        return true;
    }
    file_input->append_to_common_module = false;
    if (input.size() > 2) {
        file_input->modules.emplace_back(
                std::make_shared<vcd_file_input::Module>(vcd_file_input::Module { input[2] }));
        return true;
    }
    return false;
}

auto
vcd_parser::parse_header_var(std::vector<std::string> const &input,
        vcd_file_input::VcdFileInput *const                  file_input) -> bool
{
    if (input.size() > 4) {
        vcd_file_input::ChannelType channel_type {};
        std::string const          &type = input[1];
        std::size_t const  bit_width     = std::strtoull(input[2].c_str(), nullptr, DECIMAL_BASE);
        std::string const &identifier    = input[3];
        std::string const &name          = input[4];
        if (type == "reg" || type == "wire" || type == "logic") {
            channel_type = vcd_file_input::ChannelType::LOGICAL;
        } else {
            std::cerr << "Unknown channel type" << std::endl;
            return false;
        }
        if (channel_type == vcd_file_input::ChannelType::LOGICAL) {
            auto signal = std::make_shared<vcd_file_input::Signal>(name, identifier, bit_width);
            file_input->signal_map.emplace(identifier, signal);
            if (file_input->append_to_common_module) {
                file_input->modules.front()->signals.emplace_back(signal);
            } else {
                file_input->modules.back()->signals.emplace_back(signal);
            }
        }
        return true;
    }
    return false;
}
auto
vcd_parser::parse_text_line(std::string const &input,
        vcd_file_input::VcdFileInput *const    file_input) -> bool
{
    std::stringstream        input_stream(input);
    std::string              segments;
    std::vector<std::string> seglist;

    while (std::getline(input_stream, segments, ' ')) {
        seglist.push_back(segments);
    }
    for (auto const &segment : seglist) {
        if (segment[0] == '#') {
            process_timestamp(segment, file_input);
        } else if (segment[0] == '0' || segment[0] == '1') {
            process_single_bit(segment, file_input);
        }
    }

    return true;
}

auto
vcd_parser::process_timestamp(std::string const &input,
        vcd_file_input::VcdFileInput *const      file_input) -> bool
{

    std::string const timestamp = input.substr(1);
    file_input->current_timestamp =
            static_cast<std::int64_t>(std::strtoull(timestamp.c_str(), nullptr, DECIMAL_BASE));
    return true;
}

auto
vcd_parser::get_current_timestamp_from_command(std::string const &input) -> std::int64_t
{
    std::stringstream        input_stream(input);
    std::string              segments;
    std::vector<std::string> seglist;

    while (std::getline(input_stream, segments, ' ')) {
        seglist.push_back(segments);
    }
    return std::strtoll(seglist[0].substr(1).c_str(), nullptr, DECIMAL_BASE);
}

auto
vcd_parser::process_single_bit(std::string const &input,
        vcd_file_input::VcdFileInput *const       file_input) -> bool
{

    std::string const identifier = input.substr(1);
    std::string const value      = input.substr(0, 1);
    file_input->signal_map[identifier]->data[file_input->current_timestamp] =
            std::strtoull(value.c_str(), nullptr, DECIMAL_BASE);
    return true;
}
auto
vcd_parser::generate_packet(vcd_file_input::VcdFileInput *const file_input) -> bool
{
    for (auto const &[key, value] : file_input->signal_map) {
        value->data.emplace_back(value->data.back());
    }
    return true;
}

namespace
{
    auto
    both_are_spaces(char const lhs, char const rhs) -> bool
    {
        return (lhs == rhs) && (lhs == ' ');
    }
} // namespace

auto
vcd_parser::helper::string_append_and_replace(std::string &dest, std::string const &src) -> void
{
    std::string src_replaced = src;
    std::replace(src_replaced.begin(), src_replaced.end(), '\n', ' ');

    // Trim multiple whitespaces
    std::string::iterator const new_end =
            std::unique(src_replaced.begin(), src_replaced.end(), both_are_spaces);
    src_replaced.erase(new_end, src_replaced.end());
    dest.append(src_replaced);
}
