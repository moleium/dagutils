cmake_minimum_required(VERSION 3.20)
project(bitstream)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
  main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  dagutils
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)