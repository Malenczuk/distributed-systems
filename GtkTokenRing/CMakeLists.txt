cmake_minimum_required(VERSION 3.8)
project(GtkTokenRing)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(GtkTokenRing ${SOURCE_FILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

target_link_libraries(GtkTokenRing ${GTK3_LIBRARIES})
