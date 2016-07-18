cmake_minimum_required(VERSION 2.8)
add_executable(final final.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lstdc++")

find_package( Boost 1.40 COMPONENTS thread program_options system filesystem regex )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

TARGET_LINK_LIBRARIES( final ${Boost_LIBRARIES} )