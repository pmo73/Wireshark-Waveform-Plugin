#ifndef VCD_FILE_INPUT_HPP
#define VCD_FILE_INPUT_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace vcd_file_input
{
    enum class TIMESCALE : std::uint8_t {
        SECONDS      = 0,
        MILLISECONDS = 1,
        MICROSECONDS = 2,
        NANOSECONDS  = 3,
        PICOSECONDS  = 4,
        FEMTOSECONDS = 5
    };

    enum class ChannelType : std::uint8_t {
        LOGICAL = 0,
        ANALOG  = 1
    };

    struct Signal {
        explicit
        Signal(std::string name, std::string identifier, std::size_t const bit_width) :
            name(std::move(name)), identifier(std::move(identifier)), bit_width(bit_width),
            data_bytes((bit_width + 8 - 1) / 8)
        {
        }

        std::string               name;
        std::string               identifier;
        std::size_t               bit_width;
        std::size_t               data_bytes;
        std::vector<std::uint8_t> data;
    };

    struct Module {
        explicit
        Module(std::string name) : name(std::move(name))
        {
        }

        std::string                          name;
        std::size_t                          number_of_signals {};
        std::vector<std::shared_ptr<Signal>> signals;
    };

    struct VcdFileInput {
        VcdFileInput() = default;

        std::uint64_t                        sample_rate {};
        std::uint64_t                        timescale_factor {};
        TIMESCALE                            timescale {};
        bool                                 append_to_common_module {};
        std::vector<std::shared_ptr<Module>> modules { std::make_shared<Module>("Common") };
        std::map<std::string, std::shared_ptr<Signal>> signal_map;
        std::string                                    last_read_command;
        std::string                                    current_command;
        std::int64_t                                   last_timestamp { -1 };
        std::int64_t                                   current_timestamp { -1 };
        std::int64_t                                   next_timestamp { -1 };
    };

} // namespace vcd_file_input

#endif // VCD_FILE_INPUT_HPP
