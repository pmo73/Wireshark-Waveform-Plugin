.PHONY: help
help: # Show help for each of the Makefile recipes.
	@grep -E '^[a-zA-Z0-9 -]+:.*#'  Makefile | sort | while read -r l; do printf "\e[0;36m$$(echo $$l | cut -f 1 -d':')\033[00m:$$(echo $$l | cut -f 2- -d'#')\n"; done

.PHONY: clangFormat
clangFormat: # Run clang-format for the project and reformat the code according to the .clang-format file
	scripts/run-clang-format.py -j8 -r --clang-format-executable clang-format --color always -i .

buildAllDockerImages: buildUbuntuLatestDockerImage buildUbuntuNobleDockerImage buildUbuntuJammyDockerImage buildUbuntuFocalDockerImage buildUbuntuFocalDockerImage buildOpensuseTumbleweedDockerImage buildCentosDockerImage buildArchLinuxDockerImage # Build all docker images in one job

.PHONY: buildUbuntuLatestDockerImage
buildUbuntuLatestDockerImage: buildUbuntuNobleDockerImage # Build docker image for latest Ubuntu
	scripts/build.sh -f Dockerfile-wireshark-ubuntu-noble -n wireshark-ubuntu:latest

.PHONY: buildUbuntuNobleDockerImage
buildUbuntuNobleDockerImage: # Build docker image for Ubuntu 24.04 LTS
	scripts/build.sh -f Dockerfile-wireshark-ubuntu-noble -n wireshark-ubuntu:24.04

.PHONY: buildUbuntuJammyDockerImage
buildUbuntuJammyDockerImage: # Build docker image for Ubuntu 22.04 LTS
	scripts/build.sh -f Dockerfile-wireshark-ubuntu-jammy -n wireshark-ubuntu:22.04

.PHONY: buildUbuntuFocalDockerImage
buildUbuntuFocalDockerImage: # Build docker image for Ubuntu 20.04 LTS
	scripts/build.sh -f Dockerfile-wireshark-ubuntu-focal -n wireshark-ubuntu:20.04

.PHONY: buildOpensuseTumbleweedDockerImage
buildOpensuseTumbleweedDockerImage: # Build docker image for OpenSuse Tumbleweed
	scripts/build.sh -f Dockerfile-wireshark-opensuse-tumbleweed -n wireshark-opensuse:tumbleweed

.PHONY: buildCentosDockerImage
buildCentosDockerImage: # Build docker image for OpenSuse Tumbleweed
	scripts/build.sh -f Dockerfile-wireshark-centos -n wireshark-centos:stream9

.PHONY: buildArchLinuxDockerImage
buildArchLinuxDockerImage: # Build docker image for OpenSuse Tumbleweed
	scripts/build.sh -f Dockerfile-wireshark-archlinux -n wireshark-archlinux:latest
