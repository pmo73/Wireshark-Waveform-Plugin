#!/bin/sh

NAME=""
FILE=""

usage() {
    # Display Help
    echo "This script builds the given docker image from a given file"
    echo
    echo "Syntax: $0 -n <ImageName> -f <Dockerfile>"
    echo "options:"
    echo "n     Name of the produced image"
    echo "f     The Dockerfile to use"
    echo
    exit 1
}

while getopts ":n:f:" o; do
    case "${o}" in
    n)
        NAME=${OPTARG}
        ;;
    f)
        FILE=${OPTARG}
        ;;
    *)
        usage
        ;;
    esac
done
shift $((OPTIND - 1))

if [ -z "${NAME}" ] || [ -z "${FILE}" ]; then
    usage
fi

IMAGE_NAME="dev-$NAME"
echo "Used Image: $IMAGE_NAME"
echo "Used File: $FILE"

# build the commit tag image
cd "$(dirname "$0")" || exit 1
docker build --pull --tag "$IMAGE_NAME" -f "$FILE" .
