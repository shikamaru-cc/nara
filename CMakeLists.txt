cmake_minimum_required (VERSION 2.8)

project (nara)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++2a -O0 -g -Wall -Wextra -pedantic -fconstexpr-ops-limit=1000000000")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++2a -O3 -Wall -Wextra -pedantic -fconstexpr-ops-limit=1000000000")

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(nara ./src/main.cpp)
target_link_libraries(nara ${CURSES_LIBRARIES})
