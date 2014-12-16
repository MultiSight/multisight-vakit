cmake_minimum_required(VERSION 2.8)
project(VAKit)

set(SOURCES source/BitStream.cpp
            source/NALTypes.cpp
            source/VAH264Encoder.cpp
            source/VAH264Decoder.cpp)

set(WINDOWS_LIBS XSDK AVKit MediaParser)
set(LINUX_LIBS XSDK AVKit MediaParser avformat avcodec avutil va va-drm)

include(common.cmake NO_POLICY_SCOPE)
include("${devel_artifacts_path}/build/base_lib.cmake" NO_POLICY_SCOPE)
