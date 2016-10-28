#!/bin/bash

# Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Author: Kai Zhang (cs.zhangkai@outlook.com)
#
# This script is used to solve the dependency problem, download packages
# and provide proper path to Makefile. Users who want to use a different
# version of dependencies please refer to depends.mk file for more info.
#
# This project is designed for linux platorm, using some c++11 syntax.
# Most platform related code is ThreadPool and Logging library. For
# those who have interests in running this on Mac or other platorms,
# please refer to those codes
#
# Brief:
#   Download essential build dependencies
# Arguments:
#   Option flags starts with '-'
# Returns:
#   True on all done
#

export PROJECT_PATH=$(pwd)
export DEPS_SOURCE=${PROJECT_PATH}/thirdsrc
export DEPS_OUTPUT=${PROJECT_PATH}/thirdparty
# A flag will write to this dir when a dependency problem is fixed
export FLAG_DIR=${PROJECT_PATH}/.build

function usage() {
    echo "build - to build compile environment"
    echo "Usage:"
    echo "  ./build [OPTION]"
    echo "Options:"
    echo "  -h    show this help text"
    echo "  -a    build all needed dependencies"
    echo "  -e    build only essential dependencies"
    echo "  By default this tool only build essential dependencies"
    exit -1
}

function prepare_rpc() {
    cd ${PROJECT_PATH}/deps
    git clone https://github.com/baidu/sofa-pbrpc.git
    cd -
}

function download_boost() {
    if [[ -f "${FLAG_DIR}/boost_1_57_0" ]] && \
       [[ -d "${DEPS_OUTPUT}/boost_1_57_0/boost" ]]; then
        return 0
    fi
    cd ${DEPS_SOURCE}
    wget -O boost_1_57_0.tar.gz --no-check-certificate \
        "http://jaist.dl.sourceforge.net/project/boost/boost/1.57.0/boost_1_57_0.tar.gz"
    tar zxf boost_1_57_0.tar.gz
    rm -rf ${DEPS_OUTPUT}/boost_1_57_0
    mv boost_1_57_0 ${DEPS_OUTPUT}
    cd ${DEPS_OUTPUT}/boost_1_57_0 && ./bootstrap.sh && ./b2 --with-filesystem link=static
    cd -
    touch "${FLAG_DIR}/boost_1_57_0"
}

function download_protobuf() {
    if [[ -f "${FLAG_DIR}/protobuf_2_6_1" ]] && \
       [[ -f "${DEPS_OUTPUT}/lib/libprotobuf.a" ]] && \
       [[ -d "${DEPS_OUTPUT}/include/google/protobuf" ]]; then
        return 0
    fi
    cd ${DEPS_SOURCE}
    # wget -O protobuf-2.6.1.tar.gz --no-check-certificate \
    #     "https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz"
    rm -rf protobuf
    git clone --depth=1 https://github.com/00k/protobuf
    mv protobuf/protobuf-2.6.1.tar.gz .
    tar zxf protobuf-2.6.1.tar.gz
    cd protobuf-2.6.1
    ./configure --prefix=${DEPS_OUTPUT} --disable-shared --with-pic
    make -j 4
    make install
    cd -
    touch "${FLAG_DIR}/protobuf_2_6_1"
}

function download_snappy() {
    if [[ -f "${FLAG_DIR}/snappy_1_1_1" ]] && \
       [[ -f "${DEPS_OUTPUT}/lib/libsnappy.a" ]] && \
       [[ -f "${DEPS_OUTPUT}/include/snappy.h" ]]; then
        return 0
    fi
    cd ${DEPS_SOURCE}
    # wget -O snappy-1.1.1.tar.gz --no-check-certificate \
    #     "https://snappy.googlecode.com/files/snappy-1.1.1.tar.gz"
    rm -rf snappy
    git clone --depth=1 https://github.com/00k/snappy
    mv snappy/snappy-1.1.1.tar.gz .
    tar zxf snappy-1.1.1.tar.gz
    cd snappy-1.1.1
    ./configure --prefix=${DEPS_OUTPUT} --disable-shared --with-pic
    make -j 4
    make install
    cd -
    touch "${FLAG_DIR}/snappy_1_1_1"
}

function compile_rpc() {
    if [[ -f "${FLAG_DIR}/sofa-pbrpc_1_0_0" ]] && \
       [[ -f "${DEPS_OUTPUT}/lib/libsofa-pbrpc.a" ]] && \
       [[ -d "${DEPS_OUTPUT}/include/sofa/pbrpc" ]]; then
        return 0
    fi
    prepare_rpc
    cd ${PROJECT_PATH}/deps/sofa-pbrpc
    echo "BOOST_HEADER_DIR=${DEPS_OUTPUT}/boost_1_57_0" > depends.mk
    echo "PROTOBUF_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "SNAPPY_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "PREFIX=${DEPS_OUTPUT}" >> depends.mk
    cd src
    PROTOBUF_DIR=${DEPS_OUTPUT} sh compile_proto.sh
    cd ..
    make -j 4
    make install
    cd ..
    touch "${FLAG_DIR}/sofa-pbrpc_1_0_0"
}

function download_cmake() {
    if [[ "$(which cmake)" != "" ]]; then
        return 0
    fi
    cd ${DEPS_SOURCE}
    wget -O CMake-3.2.1.tar.gz --no-check-certificate \
        "https://github.com/Kitware/CMake/archive/v3.2.1.tar.gz"
    tar zxf CMake-3.2.1.tar.gz
    cd CMake-3.2.1
    ./configure --prefix=${DEPS_PREFIX}
    make -j 4
    make install
    cd -
}

function download_gflags() {
    if [[ -f "${FLAG_DIR}/gflags_2_1_1" ]] && \
       [[ -f "${DEPS_PREFIX}/lib/libgflags.a" ]] && \
       [[ -d "${DEPS_PREFIX}/include/gflags" ]]; then
        return 0
    fi
    download_cmake
    cd ${DEPS_OUTPUT}
    wget -O gflags-2.1.1.tar.gz --no-check-certificate \
        "https://github.com/schuhschuh/gflags/archive/v2.1.1.tar.gz"
    tar zxf gflags-2.1.1.tar.gz
    cd gflags-2.1.1
    cmake -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX} -DGFLAGS_NAMESPACE=google -DCMAKE_CXX_FLAGS=-fPIC
    make -j 4
    make install
    cd -
    touch "${FLAG_DIR}/gflags_2_1_1"
}

function download_gtest() {
    if [[ -f "${FLAG_DIR}/gtest_1_7_0" ]] && \
       [[ -f "${DEPS_PREFIX}/lib/libgtest.a" ]] && \
       [[ -d "${DEPS_PREFIX}/include/gtest" ]]; then
        return 0
    fi
    cd ${DEPS_SOURCE}
    rm -rf ./googletest-release-1.7.0
    wget -O gtest_1_7_0.tar.gz --no-check-certificate \
        "https://github.com/google/googletest/archive/release-1.7.0.tar.gz"
    tar -xzf gtest_1_7_0.tar.gz
    cd googletest-release-1.7.0
    sed -i "s/-Wno-missing-field-initializers//g" cmake/internal_utils.cmake
    cmake .
    make
    cp -af lib*.a ${DEPS_PREFIX}/lib
    cp -af include/gtest ${DEPS_PREFIX}/include
    cd -
    touch "${FLAG_DIR}/gtest_1_7_0"
}

function check_return() {
    ret_value=$1
    if [[ "${ret_value}" != "0" ]]; then
        echo $2 "build failed, please check ${DEPS_SOURCE}/build.log for more details"
        exit 1
    else
        echo $2 "build succeeded"
    fi
}

function create_depend_file() {
    echo "BOOST_HEADER_DIR=${DEPS_OUTPUT}/boost_1_57_0" > depends.mk
    echo "PROTOBUF_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "SNAPPY_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "SOFA_PBRPC_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "GFLAGS_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "GTEST_DIR=${DEPS_OUTPUT}" >> depends.mk
    echo "PREFIX=${DEPS_OUTPUT}" >> depends.mk
}

function build_all() {
    download_boost >> ${DEPS_SOURCE}/build.log
    check_return $? "boost"
    download_protobuf >> ${DEPS_SOURCE}/build.log
    check_return $? "protobuf"
    download_snappy >> ${DEPS_SOURCE}/build.log
    check_return $? "snappy"
    compile_rpc >> ${DEPS_SOURCE}/build.log
    check_return $? "sofa-pbrpc"
    download_gflags >> ${DEPS_SOURCE}/build.log
    check_return $? "gflags"
    download_gtest >> ${DEPS_SOURCE}/build.log
    check_return $? "gtest"
}

function build_essential() {
    download_boost >> ${DEPS_SOURCE}/build.log
    check_return $? "boost"
    download_protobuf >> ${DEPS_SOURCE}/build.log
    check_return $? "protobuf"
    download_snappy >> ${DEPS_SOURCE}/build.log
    check_return $? "snappy"
    compile_rpc >> ${DEPS_SOURCE}/build.log
    check_return $? "sofa-pbrpc"
    download_gflags >> ${DEPS_SOURCE}/build.log
    check_return $? "gflags"
}

function main() {
    mkdir -p ${DEPS_SOURCE} ${DEPS_OUTPUT} ${FLAG_DIR}
    # for future usage of cmake
    export PATH=${DEPS_OUTPUT}/bin:${PATH}
    flag_has_arg=""
    while getopts "ahe" arg; do
        case $arg in
            h) usage ;;
            a) build_all ;;
            e) build_essential ;;
            *) usage ;;
        esac
        flag_has_arg="true"
    done
    if [[ "${flag_has_arg}" == "" ]]; then
        build_essential
    fi
}

main $@

