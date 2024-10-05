# Wireshark-Waveform-Plugin

This repository adds to Wireshark the ability to analyze an AXI stream in Wireshark and combines this with a waveform
based on PulseView.


## Prerequisites

The dissector is a cmake based wireshark plugin. For building please make sure that the required wireshark dependencies,
including wireshark headers, are installed. For Debian based systems the following command line may be
used: `apt install wireshark-dev wireshark-common`

## Installation

The Plugin can be installed with the following steps.

```shell
cmake .
make
make install
```

Per default, the plugin will be installed inside the local plugin folder of wireshark. For installing the plugin global
on your system, run the following command:

```shell
cmake -DINSTALL_PLUGIN_LOCAL=OFF .
make
sudo make install
```

## Troubleshooting

Unfortunately, the plugin can only be used in Wireshark from Ubuntu version 22.04 with manual customization. The reason
for this is a [bug](https://gitlab.com/wireshark/wireshark/-/issues/20003) that plugins require data structures from the internal headers, which are no longer installed with the
develop packages. Therefore, the headers have to be added manually. For Ubuntu 24.04 the following commands can be used:
```shell
sudo wget -O /usr/include/wireshark/wiretap/wtap-int.h https://raw.githubusercontent.com/wireshark/wireshark/refs/tags/wireshark-4.2.2/wiretap/wtap-int.h
sudo wget -O /usr/include/wireshark/wsutil/file_util.h https://raw.githubusercontent.com/wireshark/wireshark/refs/tags/wireshark-4.2.2/wsutil/file_util.h
```
