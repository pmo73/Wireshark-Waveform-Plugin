#ifndef VCD_HPP
#define VCD_HPP

// Include glib.h before wireshark header and the extern C statement, because of
// a bug in GLib
#include "glib.h"

extern "C" {
///////////////////////
// Wireshark version //
///////////////////////
#if __has_include("ws_version.h")
#include "ws_version.h"
#define VCD_WIRESHARK_VERSION_MAJOR WIRESHARK_VERSION_MAJOR
#define VCD_WIRESHARK_VERSION_MINOR WIRESHARK_VERSION_MINOR
#elif __has_include("config.h")
#include "config.h"
#define VCD_WIRESHARK_VERSION_MAJOR VERSION_MAJOR
#define VCD_WIRESHARK_VERSION_MINOR VERSION_MINOR
#else
#error "No wireshark for version check found"
#endif

///////////////////////
// Wireshark header  //
///////////////////////
#include "wiretap/file_wrappers.h"
#include "wiretap/wtap-int.h"
}

#endif //VCD_HPP
