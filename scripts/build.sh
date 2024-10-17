#!/bin/sh

cd "$(dirname "$0")"
docker build --pull --tag dev-wireshark -f Dockerfile-wireshark .
