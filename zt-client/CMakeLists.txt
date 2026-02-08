cmake_minimum_required(VERSION 3.21)
project(zt-client CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include_directories(include)

find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(zt-client main.cpp dev.cpp wipe.cpp cert.cpp gui.cpp)
target_include_directories(zt-client PRIVATE include ${GTK4_INCLUDE_DIRS})
target_link_libraries(zt-client PRIVATE ${GTK4_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto CURL::libcurl)
target_compile_options(zt-client PRIVATE ${GTK4_CFLAGS_OTHER})
