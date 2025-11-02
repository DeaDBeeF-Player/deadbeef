#!/bin/bash
set -e
if [[ -z "$SRCROOT" ]]; then
    echo "ERROR: SRCROOT is not defined"
    exit 1
fi
XCCONFIG="${SRCROOT}/../build_data/BuildParameters.xcconfig"

VERSION=$(<"$SRCROOT/../build_data/VERSION")
BUILD_NUMBER=$(<"$SRCROOT/../build_data/BUILD_NUMBER")
GIT_REVISION=$(git rev-parse --short HEAD)

echo "MARKETING_VERSION = $VERSION" > "$XCCONFIG"
echo "BUILD_NUMBER = $BUILD_NUMBER" >> "$XCCONFIG"
echo "GIT_REVISION = $GIT_REVISION" >> "$XCCONFIG"
