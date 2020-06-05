cmake_minimum_required(VERSION 3.6)

project(shk-emu)

add_executable(shk-emu src/main.cpp)
target_include_directories(shk-emu PRIVATE include ../shk/include)
set_property(TARGET shk-emu PROPERTY CXX_STANDARD 17)
set_property(TARGET shk-emu PROPERTY CXX_STANDARD_REQUIRED ON)
