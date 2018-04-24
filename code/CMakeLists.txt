cmake_minimum_required(VERSION 3.0.0)

add_executable(particles particles.cpp)
add_executable(metaballs metaballs.cpp)
target_include_directories(particles PRIVATE "${CMAKE_ROOT}/../../include")
target_include_directories(metaballs PRIVATE "${CMAKE_ROOT}/../../include")

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ggdb") # debug compilation
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -Werror")
endif()
