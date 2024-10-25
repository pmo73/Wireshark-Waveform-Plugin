# Wireshark-Waveform-Plugin

| **Distro** | **Version**                         | **Wireshark Version**   | **Status**                                                                                                                 |
|------------|-------------------------------------|-------------------------|----------------------------------------------------------------------------------------------------------------------------|
| Ubuntu     | 20.04 LTS<br>22.04 LTS<br>24.04 LTS | 3.2.6<br>3.6.2<br>4.2.2 | ![Ubuntu](https://github.com/pmo73/Wireshark-Waveform-Plugin/actions/workflows/ubuntu.yml/badge.svg?branch=develop)        |
| openSUSE   | Tumbleweed                          | latest                  | ![openSUSE](https://github.com/pmo73/Wireshark-Waveform-Plugin/actions/workflows/opensuse.yml/badge.svg?branch=develop)    |
| CentOS     | Stream 9                            | 3.4.10                  | ![CentOS](https://github.com/pmo73/Wireshark-Waveform-Plugin/actions/workflows/centos.yml/badge.svg?branch=develop)        |
| Archlinux  | latest                              | latest                  | ![Arch Linux](https://github.com/pmo73/Wireshark-Waveform-Plugin/actions/workflows/archlinux.yml/badge.svg?branch=develop) |

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
for this is a [bug](https://gitlab.com/wireshark/wireshark/-/issues/20003) that plugins require data structures from the
internal headers, which are no longer installed with the
develop packages. Therefore, the headers have to be added manually. For Ubuntu 24.04 the following commands can be used:

```shell
sudo wget -O /usr/include/wireshark/wiretap/wtap-int.h https://raw.githubusercontent.com/wireshark/wireshark/refs/tags/wireshark-4.2.2/wiretap/wtap-int.h
sudo wget -O /usr/include/wireshark/wsutil/file_util.h https://raw.githubusercontent.com/wireshark/wireshark/refs/tags/wireshark-4.2.2/wsutil/file_util.h
```

## Development Environment

The project is configured for usage with [ADE](https://ade-cli.readthedocs.io/en/latest/usage.html). Inside the
container, all required resources are installed to develop a plugin for wireshark and built wireshark. For development,
CLion is installed and can be used inside the container. Please make sure, you have installed ADE and followed the
instructions. Ensure that a .gitconfig file exists, otherwise there will be problems with git inside the container.

Inside the ADE home directory (e.g. adehome), the project source can be cloned:

```shell
git clone git@github.com:pmo73/Wireshark-Waveform-Plugin.git
```

Per default the project will launch a container with the latest Ubuntu image. But there are also images supported for
**Arch Linux**, **openSUSE** or **CentOS**. Before they can be used, the need to be built on the local computer. This
can be done with the following commands:

```shell
# Built all container
make buildAllDockerImages

# Built latest Ubuntu container
make buildUbuntuLatestDockerImage

# Built Ubuntu 24.04 container
make buildUbuntuNobleDockerImage

# Built Ubuntu 22.04 container
make buildUbuntuJammyDockerImage

# Built Ubuntu 20.04 container
make buildUbuntuFocalDockerImage

# Built CentOS container
make buildCentosDockerImage

# Built openSUSE container
make buildOpensuseTumbleweedDockerImage

# Built Arch Linux container
make buildArchLinuxDockerImage
```

After building, the following can be used to launch the container e.g.: `ade --rc scripts/.aderc-centos start`.
