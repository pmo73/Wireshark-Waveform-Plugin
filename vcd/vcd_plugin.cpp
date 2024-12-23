#include "vcd_plugin.hpp"

#include "vcd_file_input.hpp"
#include "vcd_parser.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <regex>
#include <sstream>
#include <vector>

WS_DLL_PUBLIC_DEF gchar plugin_version[]  = VCD_VERSION;                 // NOLINT
WS_DLL_PUBLIC_DEF int   plugin_want_major = VCD_WIRESHARK_VERSION_MAJOR; // NOLINT
WS_DLL_PUBLIC_DEF int   plugin_want_minor = VCD_WIRESHARK_VERSION_MINOR; // NOLINT

namespace
{
    void wtap_register_vcd();

    auto vcd_read(wtap *wth, wtap_rec *rec, Buffer *buf, int *err, gchar **err_info,
            gint64 *data_offset) -> VCD_CALLBACK_BOOL_RETURN_TYPE;
    auto vcd_read_packet(wtap_rec *rec, Buffer *buf, vcd_file_input::VcdFileInput *file_input,
            std::int64_t offset) -> VCD_CALLBACK_BOOL_RETURN_TYPE;
    auto vcd_seek_read(wtap *wth, gint64 seek_off, wtap_rec *rec, Buffer *buf, int *err,
            gchar **err_info) -> VCD_CALLBACK_BOOL_RETURN_TYPE;

    int VCD_FILE_TYPE_SUBTYPE; // NOLINT

    /*
     * Try to interpret a file as a vcd formatted file.
     */
    auto
    vcd_open(wtap *wth, [[maybe_unused]] int *err, [[maybe_unused]] char **err_info)
            -> wtap_open_return_val
    {
        auto *const file_input = new vcd_file_input::VcdFileInput(); // NOLINT

        std::size_t const file_size  = wtap_file_size(wth, err);
        std::size_t       read_bytes = 0;
        std::string       complete_line;
        std::string       first_payload_command;

        // Read file header, until we read first timestamp
        while (read_bytes < file_size - 1) {
            std::vector<char> buf {};
            buf.reserve(file_size);

            // read next new line from file
            if (file_getsp(buf.data(), static_cast<int>(file_size), wth->fh) == nullptr) {
                return WTAP_OPEN_ERROR;
            }

            // convert current read line to string and count bytes
            std::string const partial_line { buf.data() };
            read_bytes += partial_line.size();

            // Abort if we reached the first timestamp
            if (partial_line.starts_with("#")) {
                vcd_parser::helper::string_append_and_replace(first_payload_command, partial_line);
                break;
            }

            // Append bytes to complete line
            vcd_parser::helper::string_append_and_replace(complete_line, partial_line);
        }

        // Parse complete header
        vcd_parser::parse_header(complete_line, file_input);

        wth->priv              = static_cast<void *>(file_input);
        wth->subtype_read      = vcd_read;
        wth->subtype_seek_read = vcd_seek_read;
        wth->file_type_subtype = VCD_FILE_TYPE_SUBTYPE;
        wth->file_encap        = WTAP_ENCAP_USER0;

        switch (file_input->timescale) {

            case vcd_file_input::TIMESCALE::SECONDS:
                wth->file_tsprec = WTAP_TSPREC_SEC;
                break;
            case vcd_file_input::TIMESCALE::MILLISECONDS:
                wth->file_tsprec = WTAP_TSPREC_MSEC;
                break;
            case vcd_file_input::TIMESCALE::MICROSECONDS:
                wth->file_tsprec = WTAP_TSPREC_USEC;
                break;
            case vcd_file_input::TIMESCALE::NANOSECONDS:
                wth->file_tsprec = WTAP_TSPREC_NSEC;
                break;
            case vcd_file_input::TIMESCALE::PICOSECONDS:
            case vcd_file_input::TIMESCALE::FEMTOSECONDS:
            default:
                std::cerr << "Time scale not recognised!" << std::endl;
        }

        // Read payload
        complete_line = first_payload_command;
        while (read_bytes < file_size - 1) {
            std::vector<char> buf {};
            buf.reserve(file_size);
            // read next new line from file
            if (file_getsp(buf.data(), static_cast<int>(file_size), wth->fh) == nullptr) {
                return WTAP_OPEN_ERROR;
            }

            // convert current read line to string and count bytes
            std::string const partial_line { buf.data() };
            read_bytes += partial_line.size();

            // Append bytes to complete line
            vcd_parser::helper::string_append_and_replace(complete_line, partial_line);
        }
        vcd_parser::split_payload_commands(complete_line, file_input);

        return WTAP_OPEN_MINE;
    }

    auto
    vcd_read(wtap *wth, wtap_rec *rec, Buffer *buf, [[maybe_unused]] int *err,
            [[maybe_unused]] gchar **err_info, gint64 *data_offset) -> VCD_CALLBACK_BOOL_RETURN_TYPE
    {
        static std::int64_t current_timestamp = 0;
        static bool         send_meta_packet  = true;
        auto *const         file_input = static_cast<vcd_file_input::VcdFileInput *>(wth->priv);
        file_input->current_timestamp  = current_timestamp;

        if (send_meta_packet) {
            send_meta_packet = false;
            *data_offset     = -1;
            return vcd_read_packet(rec, buf, file_input, -1);
        }

        if (file_input->payload_commands.empty()) {
            return false;
        }
        std::string const &current_command = file_input->payload_commands.front();
        std::int64_t const timestamp_from_command =
                vcd_parser::get_current_timestamp_from_command(current_command);

        if (current_timestamp >= timestamp_from_command) {
            vcd_parser::generate_packet(file_input);
            vcd_parser::parse_text_line(current_command, file_input);
            *data_offset = file_input->current_command_timestamp;
            file_input->payload_commands.pop_front();
            current_timestamp++;
            return vcd_read_packet(rec, buf, file_input, file_input->current_command_timestamp);
        }
        vcd_parser::generate_packet(file_input);
        *data_offset = file_input->current_command_timestamp;
        current_timestamp++;
        return vcd_read_packet(rec, buf, file_input, file_input->current_command_timestamp);
    }

    auto
    vcd_read_packet(wtap_rec *rec, Buffer *buf, vcd_file_input::VcdFileInput *const file_input,
            std::int64_t const offset) -> VCD_CALLBACK_BOOL_RETURN_TYPE
    {
        static std::size_t const payload_size = std::accumulate(std::begin(file_input->signal_map),
                std::end(file_input->signal_map), 0,
                [](int const                                                                value,
                        std::map<std::string,
                                std::shared_ptr<vcd_file_input::Signal>>::value_type const &map) {
                    return value + map.second->data_bytes;
                });
        static std::size_t const meta_payload_size = std::accumulate(
                std::begin(file_input->signal_map), std::end(file_input->signal_map), 0,
                [](int const                                                                value,
                        std::map<std::string,
                                std::shared_ptr<vcd_file_input::Signal>>::value_type const &map) {
                    return value + map.second->name.length();
                });
        std::vector<std::uint8_t> line_buf {};

        if (offset == -1) {
            line_buf.reserve(meta_payload_size);
            for (auto const &[identifier, signal] : file_input->signal_map) {
                for (auto const &letter : signal->name) {
                    line_buf.emplace_back(letter);
                }
            }
        } else {
            line_buf.reserve(payload_size);
            for (auto const &[key, value] : file_input->signal_map) {
                for (std::size_t i = 0; i < value->data_bytes; ++i) {
                    line_buf.emplace_back(value->data[offset + i]);
                }
            }
        }

        ws_buffer_assure_space(buf, line_buf.size());
        std::uint8_t *end_ptr = ws_buffer_end_ptr(buf);
        std::memcpy(end_ptr, line_buf.data(), line_buf.size());

        rec->rec_type       = REC_TYPE_PACKET;
        rec->presence_flags = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
        switch (file_input->timescale) {

            case vcd_file_input::TIMESCALE::SECONDS:
                rec->ts.secs  = file_input->current_timestamp;
                rec->ts.nsecs = 0;
                break;
            case vcd_file_input::TIMESCALE::MILLISECONDS:
                rec->ts.secs = static_cast<std::int64_t>(
                        static_cast<double>(file_input->current_timestamp) *
                        vcd_parser::QUOTIENT_US / vcd_parser::QUOTIENT_NS);
                rec->ts.nsecs = static_cast<int>(
                        std::fmod(static_cast<double>(file_input->current_timestamp) *
                                          vcd_parser::QUOTIENT_US,
                                vcd_parser::QUOTIENT_NS));
                break;
            case vcd_file_input::TIMESCALE::MICROSECONDS:
                rec->ts.secs = static_cast<std::int64_t>(
                        static_cast<double>(file_input->current_timestamp) *
                        vcd_parser::QUOTIENT_MS / vcd_parser::QUOTIENT_NS);
                rec->ts.nsecs = static_cast<int>(
                        std::fmod(static_cast<double>(file_input->current_timestamp) *
                                          vcd_parser::QUOTIENT_MS,
                                vcd_parser::QUOTIENT_NS));
                break;
            case vcd_file_input::TIMESCALE::NANOSECONDS:
            case vcd_file_input::TIMESCALE::PICOSECONDS:
            case vcd_file_input::TIMESCALE::FEMTOSECONDS:
            default:
                rec->ts.secs = static_cast<std::int64_t>(
                        static_cast<double>(file_input->current_timestamp) /
                        vcd_parser::QUOTIENT_NS);
                rec->ts.nsecs = static_cast<int>(
                        std::fmod(static_cast<double>(file_input->current_timestamp),
                                vcd_parser::QUOTIENT_NS));
        }

        rec->rec_header.packet_header.caplen = line_buf.size();
        rec->rec_header.packet_header.len    = line_buf.size();
        return true;
    }

    /*
     * Random access read.
     * Read the frame at the given offset in the file. Store the frame data
     * in a buffer and fill in the packet header info.
     */
    auto
    vcd_seek_read([[maybe_unused]] wtap *wth, gint64 seek_off, wtap_rec *rec, Buffer *buf,
            [[maybe_unused]] int *err, [[maybe_unused]] gchar **err_info)
            -> VCD_CALLBACK_BOOL_RETURN_TYPE
    {
        auto *const file_input = static_cast<vcd_file_input::VcdFileInput *>(wth->priv);
        vcd_read_packet(rec, buf, file_input, seek_off);
        return true;
    }

    /*
     * Register with wiretap.
     * Register how we can handle an unknown file to see if this is a valid
     * vcd file and register information about this file format.
     */
    void
    wtap_register_vcd()
    {
        open_info oi = {
            .name           = "Value Change Dump",
            .type           = OPEN_INFO_HEURISTIC,
            .open_routine   = vcd_open,
            .extensions     = "vcd",
            .extensions_set = nullptr,
            .wslua_data     = nullptr,
        };

        wtap_register_open_info(&oi, false);

#if VCD_WIRESHARK_VERSION_GE(3, 6)
        static constexpr std::array<supported_block_type, 1> usbdump_blocks_supported {
            { WTAP_BLOCK_PACKET, BLOCK_NOT_SUPPORTED, NO_OPTIONS_SUPPORTED },
        };
#endif

        constexpr file_type_subtype_info fi {
            "Value Change Dump",
            "VCD",
            "vcd",
            nullptr,
            false,
#if VCD_WIRESHARK_VERSION_GE(3, 6)
            BLOCKS_SUPPORTED(usbdump_blocks_supported.data()),
            nullptr,
#else
            false,
            0,
#endif
            nullptr,
            nullptr,
        };

#if VCD_WIRESHARK_VERSION_GE(3, 6)
        VCD_FILE_TYPE_SUBTYPE = wtap_register_file_type_subtype(&fi);
#else
        VCD_FILE_TYPE_SUBTYPE =
                wtap_register_file_type_subtypes(&fi, WTAP_FILE_TYPE_SUBTYPE_UNKNOWN);
#endif
    }
} // namespace

extern "C" {
    WS_DLL_PUBLIC
    void
    plugin_register()
    {
        static wtap_plugin plug_vcd;

        plug_vcd.register_wtap_module = wtap_register_vcd;
        wtap_register_plugin(&plug_vcd);
    }
}
