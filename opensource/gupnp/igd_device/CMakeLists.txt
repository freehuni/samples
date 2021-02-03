cmake_minimum_required(VERSION 3.0)
project(igd_device)


include_directories(/usr/include/gupnp-1.2)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
include_directories(/usr/include/gssdp-1.2)
include_directories(/usr/include/libsoup-2.4)
include_directories(/usr/include/libxml2)

add_executable(igd_device igddevice.cpp menumanager.cpp utility.cpp main.cpp)

target_link_libraries(igd_device 
    glib-2.0
    gobject-2.0
    gupnp-1.2
    gssdp-1.2
    readline
    pthread
    )
