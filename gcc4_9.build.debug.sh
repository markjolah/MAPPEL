#!/bin/bash
#
# gcc4_9.build.debug.sh
#

ARCH=gcc4_9
FULL_ARCH=x86_64-${ARCH}-linux-gnu
TOOLCHAIN_FILE=./cmake/UncommonCMakeModules/Toolchains/Toolchain-${FULL_ARCH}.cmake
INSTALL_PATH=_${ARCH}.install
BUILD_PATH=_${ARCH}.build/Debug

NUM_PROCS=$(grep -c ^processor /proc/cpuinfo)
ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_PATH"
ARGS="${ARGS} -DBUILD_TESTING=On"
ARGS="${ARGS} -DBUILD_STATIC_LIBS=Off"
ARGS="${ARGS} -DOPT_EXPORT_BUILD_TREE=On"
ARGS="${ARGS} -DOPT_DEBUG=On"
ARGS="${ARGS} -DOPT_MATLAB=On"
ARGS="${ARGS} -DOPT_PYTHON=On"

set -ex
rm -rf $BUILD_PATH $INSTALL_PATH
cmake -H. -B$BUILD_PATH -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE=Debug ${ARGS} $@
VERBOSE=1 cmake --build $BUILD_PATH --target install -- -j${NUM_PROCS}
