project(dagorbs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(_TARGET_PC_WIN _TARGET_64BIT)

add_subdirectory(vendor/eastl)

add_executable(${PROJECT_NAME}
  src/main.cpp

  src/memory/mem.cpp
  src/memory/win32MoreCore.cpp
  src/memory/dlmalloc-2.8.4.cpp
  src/memory/allocStep.cpp
  src/memory/dag_memBase.cpp
  src/memory/dagmem.cpp
  src/memory/stubRtl.cpp

  src/osApiWrappers/stackHlpFill.cpp
  src/kernel/fatalerr.cpp
  src/osApiWrappers/critsec.cpp
  src/osApiWrappers/limBufWriter.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
  $<TARGET_PROPERTY:EASTL,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME} PRIVATE EASTL)