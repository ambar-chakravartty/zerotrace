cmake_minimum_required(VERSION 3.14)
project(zt-client CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.0
)
FetchContent_MakeAvailable(raylib)

# Raylib links to libraries like X11 on Linux which might need FindPackage
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    # usually Raylib handles these internally or via its config
    # but sometimes we need to link explicitly if building from source
endif()

add_executable(zt-client main.cpp dev.cpp gui.cpp)
target_include_directories(zt-client PRIVATE include)
target_link_libraries(zt-client PRIVATE raylib)
