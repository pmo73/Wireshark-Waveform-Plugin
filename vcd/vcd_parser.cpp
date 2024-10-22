#include "vcd_parser.hpp"

#include <iostream>
#include <ostream>
#include <ranges>
#include <sstream>

auto
vcd_parser::parse_header(std::string const &input, vcd_file_input::VcdFileInput *const file_input)
        -> bool
{
    std::stringstream        input_stream(input);
    std::string              segment;
    std::vector<std::string> seglist;

    while (std::getline(input_stream, segment, ' ')) {
        seglist.push_back(segment);
    }
    if (seglist[0] == "$timescale") {
        parse_timescale(seglist, file_input);
    } else if (seglist[0] == "$scope") {
        parse_scope(seglist, file_input, false);
    } else if (seglist[0] == "$upscope") {
        parse_scope(seglist, file_input, true);
    } else if (seglist[0] == "$var") {
        parse_header_var(seglist, file_input);
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
            quotient              = 1e15;
            file_input->timescale = vcd_file_input::TIMESCALE::FEMTOSECONDS;
        } else if (input[2] == "ps") {
            quotient              = 1e12;
            file_input->timescale = vcd_file_input::TIMESCALE::PICOSECONDS;
        } else if (input[2] == "ns") {
            quotient              = 1e9;
            file_input->timescale = vcd_file_input::TIMESCALE::NANOSECONDS;
        } else if (input[2] == "us") {
            quotient              = 1e6;
            file_input->timescale = vcd_file_input::TIMESCALE::MICROSECONDS;
        } else if (input[2] == "ms") {
            quotient              = 1e3;
            file_input->timescale = vcd_file_input::TIMESCALE::MILLISECONDS;
        } else if (input[2] == "s") {
            quotient              = 1;
            file_input->timescale = vcd_file_input::TIMESCALE::SECONDS;
        } else {
            throw std::logic_error("Unknown time scale");
            return false;
        }
        file_input->timescale_factor = std::strtoull(input[1].c_str(), nullptr, 10);
        file_input->sample_rate      = quotient / file_input->timescale_factor;
    }
    return true;
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
    if (input.size() > 5) {
        vcd_file_input::ChannelType channel_type {};
        std::string const          &type       = input[1];
        std::size_t const           bit_width  = std::strtoull(input[2].c_str(), nullptr, 10);
        std::string const          &identifier = input[3];
        std::string const          &name       = input[4];
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
    file_input->last_command = input;

    return true;
}

auto
vcd_parser::process_timestamp(std::string const &input,
        vcd_file_input::VcdFileInput *const      file_input) -> bool
{

    std::string const timestamp = input.substr(1);
    file_input->next_timestamp  = std::strtoull(timestamp.c_str(), nullptr, 10);
    return true;
}

auto
vcd_parser::process_single_bit(std::string const &input,
        vcd_file_input::VcdFileInput *const       file_input) -> bool
{

    std::string identifier = input.substr(1);
    std::string value      = input.substr(0, 1);
    file_input->signal_map[identifier]->data.emplace_back(
            std::strtoull(value.c_str(), nullptr, 10));
    return true;
}
auto
vcd_parser::generate_packet(vcd_file_input::VcdFileInput *const file_input) -> bool
{
    for (auto const &signal_data : file_input->signal_map | std::views::values) {
        signal_data->data.emplace_back(signal_data->data.back());
    }
    return true;
}

auto
vcd_parser::helper::string_append_and_replace(std::string &dest, std::string const &src) -> void
{
    std::string src_replaced = src;
    std::replace(src_replaced.begin(), src_replaced.end(), '\n', ' ');
    dest.append(src_replaced);
}
