cmake_minimum_required(VERSION 3.20)
project(dagutils)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(vendor/eastl)

set(COMMON_SOURCES
  libs/core/memory/mem.cpp
  libs/core/memory/dlmalloc-2.8.4.cpp
  libs/core/memory/allocStep.cpp
  libs/core/memory/dag_memBase.cpp
  libs/core/memory/dagmem.cpp
  libs/core/memory/stubRtl.cpp
  libs/core/kernel/fatalerr.cpp
  libs/core/osApiWrappers/limBufWriter.cpp
  libs/core/util/strImpl.cpp
  libs/core/math/defs.cpp
  libs/core/math/namemap.cpp
  libs/core/util/dag_safeArg.cpp

  libs/datablock/datablock.cpp
)

set(WINDOWS_SOURCES
  libs/core/memory/win32MoreCore.cpp
  libs/core/osApiWrappers/stackHlpFill.cpp
  libs/core/osApiWrappers/critsec.cpp
)

set(LINUX_SOURCES
  libs/core/osApiWrappers/linux_critsec.cpp
  libs/core/osApiWrappers/linux_stackhlp.cpp
  libs/core/kernel/linux_globals.cpp
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
  libs
  libs/core
  $<TARGET_PROPERTY:EASTL,INTERFACE_INCLUDE_DIRECTORIES>
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

add_subdirectory(examples/datablock)
add_subdirectory(examples/bitstream)