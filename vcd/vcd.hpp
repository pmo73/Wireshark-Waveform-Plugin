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
#define VCD_WIRESHARK_VERSION_PATCH WIRESHARK_VERSION_MICRO
#elif __has_include("config.h")
#include "config.h"
#define VCD_WIRESHARK_VERSION_MAJOR VERSION_MAJOR
#define VCD_WIRESHARK_VERSION_MINOR VERSION_MINOR
#define VCD_WIRESHARK_VERSION_PATCH VERSION_MIRCO
#else
#error "No wireshark for version check found"
#endif

///////////////////////
// Wireshark header  //
///////////////////////
#include "wiretap/file_wrappers.h"
#include "wiretap/wtap-int.h"
}

// Define return type for callback functions, because of issue 19116
// (https://gitlab.com/wireshark/wireshark/-/issues/19116)
#if VCD_WIRESHARK_VERSION_MAJOR >= 4 && VCD_WIRESHARK_VERSION_MINOR >= 4
using VCD_CALLBACK_BOOL_RETURN_TYPE = bool;
#else
using VCD_CALLBACK_BOOL_RETURN_TYPE = gboolean;
#endif

#endif // VCD_HPP
