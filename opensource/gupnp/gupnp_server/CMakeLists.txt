cmake_minimum_required(VERSION 2.8)
set(project_name "gupnp_server")
project(${project_name})

include_directories()
include_directories(/usr/include/gupnp-1.0)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
include_directories(/usr/include/gssdp-1.0)
include_directories(/usr/include/libsoup-2.4)
include_directories(/usr/include/libxml2)

add_definitions(-std=c++11)

add_executable(${project_name} main.cpp utility.cpp)

target_link_libraries(${project_name} /usr/lib/x86_64-linux-gnu/libglib-2.0.so)
target_link_libraries(${project_name} /usr/lib/x86_64-linux-gnu/libgobject-2.0.so)
target_link_libraries(${project_name} /usr/lib/x86_64-linux-gnu/libgupnp-1.0.so)
target_link_libraries(${project_name} /usr/lib/x86_64-linux-gnu/libgssdp-1.0.so)
