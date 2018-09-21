cmake_minimum_required(VERSION 3.9)

# Using hunter to resolve dependencies.
include("cmake/HunterGate.cmake")

HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.27.tar.gz"
        SHA1 "70d75a25f895df3da00d8382417e8c2a08c93173"
        LOCAL      # Use the local Hunter/config.camke
)

project(helloworld)

set(CMAKE_CXX_STANDARD 14)

hunter_add_package(thrift)

find_package(thrift CONFIG REQUIRED)



add_executable(server
        server.cpp
        gen-cpp/HelloworldService.cpp
)

add_executable(client
        client.cpp
        gen-cpp/HelloworldService.cpp
)



target_link_libraries(server PUBLIC
        thrift::thrift_static            # Main thrift library, thrift_static for static library
#        thrift::thriftz           # thrift ZLIB support
        thrift::thriftnb_static        # thrift Libevent non-blocking support
)

target_link_libraries(client PUBLIC
        thrift::thrift_static            # Main thrift library, thrift_static for static library
        #        thrift::thriftz           # thrift ZLIB support
        thrift::thriftnb_static        # thrift Libevent non-blocking support
        )
