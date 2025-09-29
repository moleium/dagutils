cmake_minimum_required(VERSION 3.20)
project(bitstream)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(vendor/eastl)

set(COMMON_SOURCES
  src/memory/mem.cpp
  src/memory/dlmalloc-2.8.4.cpp
  src/memory/allocStep.cpp
  src/memory/dag_memBase.cpp
  src/memory/dagmem.cpp
  src/memory/stubRtl.cpp
  src/kernel/fatalerr.cpp
  src/osApiWrappers/limBufWriter.cpp
  src/util/strImpl.cpp
  src/math/defs.cpp
  src/math/namemap.cpp
  src/util/dag_safeArg.cpp

  include/datablock/datablock.cpp
)

set(WINDOWS_SOURCES
  src/memory/win32MoreCore.cpp
  src/osApiWrappers/stackHlpFill.cpp
  src/osApiWrappers/critsec.cpp
)

set(LINUX_SOURCES
  src/osApiWrappers/linux_critsec.cpp
  src/osApiWrappers/linux_stackhlp.cpp
  src/kernel/linux_globals.cpp
)

set(PROJECT_SOURCES ${COMMON_SOURCES})

if(WIN32)
  list(APPEND PROJECT_SOURCES ${WINDOWS_SOURCES})
elseif(UNIX AND NOT APPLE)
  list(APPEND PROJECT_SOURCES ${LINUX_SOURCES})
endif()

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC _TARGET_STATIC_LIB)

target_include_directories(${PROJECT_NAME} PUBLIC
  src
  include
  $<TARGET_PROPERTY:EASTL,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PUBLIC _TARGET_PC_WIN)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(${PROJECT_NAME} PUBLIC _TARGET_PC_LINUX)
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(${PROJECT_NAME} PUBLIC _TARGET_64BIT)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC EASTL)