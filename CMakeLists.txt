cmake_minimum_required(VERSION 3.10)
project(icpc_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror")

# Create executable named 'code' as required
add_executable(code main.cpp)

# Set output name explicitly
set_target_properties(code PROPERTIES OUTPUT_NAME "code")