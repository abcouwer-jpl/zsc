cmake_minimum_required(VERSION 2.8.2)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -pedantic -Werror -Wno-unused-local-typedefs")

set(VERSION "1.2.11.f-abcouwer-safety-critical-v0")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)


#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    include/zsc/zlib_types_pub.h
    include/zsc/zlib.h
    include/zsc/zsc_pub.h
)
set(ZLIB_PRIVATE_HDRS
    include/zsc/deflate.h
    include/zsc/inffast.h
    include/zsc/inflate.h
    include/zsc/inftrees.h
    include/zsc/zutil.h
    include/zsc/zsc_conf_global_types.h
    include/zsc/zsc_conf_private.h
)
set(ZLIB_SRCS
    src/adler32.c
    src/crc32.c
    src/deflate.c
    src/inflate.c
    src/inftrees.c
    src/inffast.c
    src/trees.c
    src/zsc_compress.c
    src/zsc_uncompr.c
    src/zutil.c
)

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/include/zsc/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})


add_library(zlib SHARED ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
set_target_properties(zlib PROPERTIES SOVERSION 1)

#============================================================================
# Check build type
#============================================================================

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
  # build the unit tests
  set(CMAKE_BUILD_TYPE "Test")
  set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/test)
  add_custom_target(ctest COMMAND ${CMAKE_CTEST_COMMAND})
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
  set(COVERAGE_EXCLUDES 
    '/usr/*' 
    '${CMAKE_CURRENT_SOURCE_DIR}/test/*'
    '${CMAKE_CURRENT_SOURCE_DIR}/build/*')
  MESSAGE( STATUS "COVERAGE_EXCLUDES: " ${COVERAGE_EXCLUDES} )
  setup_target_for_coverage(
    NAME coverage 
    EXECUTABLE ctest
    DEPENDENCIES ctest)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Test")

  configure_file(
      ${CMAKE_SOURCE_DIR}/test/zsc_test_global_types.h 
      ${CMAKE_SOURCE_DIR}/include/zsc/zsc_conf_global_types.h)
      
  configure_file(
      ${CMAKE_SOURCE_DIR}/test/zsc_test_private.h 
      ${CMAKE_SOURCE_DIR}/include/zsc/zsc_conf_private.h)

  set(CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DSTATIC=)
  # make "local"s (statics) visible
  add_definitions(-Dlocal=)
  set(ENV{CTEST_OUTPUT_ON_FAILURE} TRUE)

  include(CTest)
  enable_testing()
  
  if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/corpus/cantrbry/README.md)
  else()
  
      project(cantrbry NONE)
    
      include(ExternalProject)
      ExternalProject_Add(cantrbry
          GIT_REPOSITORY    https://github.com/nabcouwer/cantrbry.git
          GIT_TAG           master
          SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/corpus/cantrbry"
          BINARY_DIR        ""
          CONFIGURE_COMMAND ""
          BUILD_COMMAND     ""
          INSTALL_COMMAND   ""
          TEST_COMMAND      ""
        )
  endif()
  
  if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/corpus/artificl/README.md)
  else()
  
      project(artificl NONE)
    
      include(ExternalProject)
      ExternalProject_Add(artificl
          GIT_REPOSITORY    https://github.com/nabcouwer/artificl.git
          GIT_TAG           master
          SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/corpus/artificl"
          BINARY_DIR        ""
          CONFIGURE_COMMAND ""
          BUILD_COMMAND     ""
          INSTALL_COMMAND   ""
          TEST_COMMAND      ""
        )
  endif()
  
  if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/corpus/large/README.md)
  else()
  
      project(large NONE)
    
      include(ExternalProject)
      ExternalProject_Add(large
          GIT_REPOSITORY    https://github.com/nabcouwer/large.git
          GIT_TAG           master
          SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/corpus/large"
          BINARY_DIR        ""
          CONFIGURE_COMMAND ""
          BUILD_COMMAND     ""
          INSTALL_COMMAND   ""
          TEST_COMMAND      ""
        )
  endif()
  
  if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/corpus/calgary/README.md)
  else()
  
      project(calgary NONE)
    
      include(ExternalProject)
      ExternalProject_Add(calgary
          GIT_REPOSITORY    https://github.com/nabcouwer/calgary.git
          GIT_TAG           master
          SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/corpus/calgary"
          BINARY_DIR        ""
          CONFIGURE_COMMAND ""
          BUILD_COMMAND     ""
          INSTALL_COMMAND   ""
          TEST_COMMAND      ""
        )
  endif()
  
endif()

#============================================================================
# Download and unpack googletest at configure time
#============================================================================

configure_file(test/GoogleTestCMakeLists.txt.in googletest-download/CMakeLists.txt)




execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()


#============================================================================
# gtest binaries
#============================================================================


add_executable(zlib_gtest test/zlib_gtest.cpp ${ZLIB_SRCS} ${ZLIB_ASMS}  ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
target_link_libraries(zlib_gtest gtest_main)

add_test(NAME zlib_gtest_test COMMAND zlib_gtest)


