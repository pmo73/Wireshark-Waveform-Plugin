#include "vcd_plugin.hpp"

#include "vcd_file_input.hpp"
#include "vcd_parser.hpp"

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

        // Read file header, until we reach keyword "$enddefinitions"
        while (read_bytes < file_size - 1) {
            std::string complete_line;
            do {
                std::vector<char> buf {};
                buf.reserve(file_size);
                if (file_getsp(buf.data(), static_cast<int>(file_size - read_bytes), wth->fh) ==
                        nullptr) {
                    return WTAP_OPEN_ERROR;
                }
                std::string const partial_line { buf.data() };
                vcd_parser::helper::string_append_and_replace(complete_line, partial_line);
            } while (complete_line.find("$end ") == std::string::npos);
            vcd_parser::parse_header(complete_line, file_input);
            read_bytes += complete_line.size();
            if (complete_line.find("$enddefinitions $end") != std::string::npos) {
                break;
            }
        }

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

        return WTAP_OPEN_MINE;
    }

    auto
    vcd_read(wtap *wth, wtap_rec *rec, Buffer *buf, [[maybe_unused]] int *err,
            [[maybe_unused]] gchar **err_info, gint64 *data_offset) -> VCD_CALLBACK_BOOL_RETURN_TYPE
    {

        auto *const       file_input = static_cast<vcd_file_input::VcdFileInput *>(wth->priv);
        std::size_t       read_bytes = file_tell(wth->fh);
        std::size_t const file_size  = wtap_file_size(wth, err);
        static std::size_t const payload_size = std::accumulate(std::begin(file_input->signal_map),
                std::end(file_input->signal_map), 0,
                [](int const                                                                value,
                        std::map<std::string,
                                std::shared_ptr<vcd_file_input::Signal>>::value_type const &map) {
                    return value + map.second->data_bytes;
                });

        if (file_input->current_timestamp >= file_input->next_timestamp - 1) {
            std::string complete_command;
            if (read_bytes >= file_size - 1) {
                return false;
            }
            while (read_bytes < file_size - 1) {
                std::vector<char> line_buf {};
                line_buf.reserve(file_size);
                if (file_getsp(line_buf.data(), static_cast<int>(file_size - read_bytes),
                            wth->fh) == nullptr) {
                    // EOF or error
                    std::cerr << "Error reading file or EOF!" << std::endl;
                }
                std::string const line { line_buf.data() };
                read_bytes += line.size();
                if (line.starts_with("#")) {
                    vcd_parser::helper::string_append_and_replace(complete_command, line);
                    break;
                }
            }
            vcd_parser::parse_text_line(complete_command, file_input);
            *data_offset = file_input->current_timestamp;

            std::vector<std::uint8_t> line_buf {};
            line_buf.reserve(payload_size);
            for (auto const &[key, value] : file_input->signal_map) {
                line_buf.emplace_back(value->data[file_input->current_timestamp]);
            }
            ws_buffer_assure_space(buf, line_buf.size());
            std::uint8_t *end_ptr = ws_buffer_end_ptr(buf);
            std::memcpy(end_ptr, line_buf.data(), line_buf.size());
            end_ptr = static_cast<std::uint8_t *>(end_ptr + line_buf.size());

            rec->rec_type                        = REC_TYPE_PACKET;
            rec->presence_flags                  = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
            rec->ts.secs                         = file_input->current_timestamp;
            rec->ts.nsecs                        = 0;
            rec->rec_header.packet_header.caplen = 0;
            rec->rec_header.packet_header.len    = line_buf.size();
            file_input->current_timestamp++;
            return true;
        }
        vcd_parser::generate_packet(file_input);
        *data_offset = file_input->current_timestamp;

        std::vector<std::uint8_t> line_buf {};
        line_buf.reserve(payload_size);
        for (auto const &[key, value] : file_input->signal_map) {
            line_buf.emplace_back(value->data[file_input->current_timestamp]);
        }
        ws_buffer_assure_space(buf, line_buf.size());
        std::uint8_t *end_ptr = ws_buffer_end_ptr(buf);
        std::memcpy(end_ptr, line_buf.data(), line_buf.size());
        end_ptr = static_cast<std::uint8_t *>(end_ptr + line_buf.size());

        rec->rec_type                        = REC_TYPE_PACKET;
        rec->presence_flags                  = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
        rec->ts.secs                         = file_input->current_timestamp;
        rec->ts.nsecs                        = 0;
        rec->rec_header.packet_header.caplen = 0;
        rec->rec_header.packet_header.len    = line_buf.size();
        file_input->current_timestamp++;
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
        static std::size_t const payload_size = std::accumulate(std::begin(file_input->signal_map),
                std::end(file_input->signal_map), 0,
                [](int const                                                                value,
                        std::map<std::string,
                                std::shared_ptr<vcd_file_input::Signal>>::value_type const &map) {
                    return value + map.second->data_bytes;
                });

        std::vector<std::uint8_t> line_buf {};
        line_buf.reserve(payload_size);
        for (auto const &[key, value] : file_input->signal_map) {
            line_buf.emplace_back(value->data[seek_off]);
        }
        ws_buffer_assure_space(buf, line_buf.size());
        std::uint8_t *end_ptr = ws_buffer_end_ptr(buf);
        std::memcpy(end_ptr, line_buf.data(), line_buf.size());
        end_ptr = static_cast<std::uint8_t *>(end_ptr + line_buf.size());

        rec->rec_type                        = REC_TYPE_PACKET;
        rec->presence_flags                  = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
        rec->ts.secs                         = seek_off;
        rec->ts.nsecs                        = 0;
        rec->rec_header.packet_header.caplen = 0;
        rec->rec_header.packet_header.len    = line_buf.size();
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

        constexpr file_type_subtype_info fi = {
            .description                = "Value Change Dump",
            .name                       = "VCD",
            .default_file_extension     = "vcd",
            .additional_file_extensions = nullptr,
            .writing_must_seek          = false,
            .num_supported_blocks       = false,
#if VCD_WIRESHARK_VERSION_MAJOR <= 3 && VCD_WIRESHARK_VERSION_MINOR < 6
            .supported_blocks = 0,
#else
            .supported_blocks = nullptr,
#endif
            .can_write_encap = nullptr,
            .dump_open       = nullptr,
            .wslua_info      = nullptr,
        };

#if VCD_WIRESHARK_VERSION_MAJOR <= 3 && VCD_WIRESHARK_VERSION_MINOR < 6
        VCD_FILE_TYPE_SUBTYPE =
                wtap_register_file_type_subtypes(&fi, WTAP_FILE_TYPE_SUBTYPE_UNKNOWN);
#else
        VCD_FILE_TYPE_SUBTYPE = wtap_register_file_type_subtype(&fi);
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
