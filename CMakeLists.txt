# Use autotools-based build system instead if you can.
# CMake is used only to build libfityk in special cases.

cmake_minimum_required(VERSION 2.8)
project(fityk C CXX)
add_definitions(-DVERSION="1.3.0")

option(USE_ZLIB "Handle compressed GZ files - requires Zlib library" ON)
option(DOWNLOAD_ZLIB "Download and build the Zlib library" OFF)
option(BUILD_SHARED_LIBS "Build as a shared library" ON)

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 OLD)
endif()

# if we build static library we likely prefer 3rd-party libraries also static
if(NOT BUILD_SHARED_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES
      ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/fityk) # only for fityk_lua.cpp
include_directories(${CMAKE_BINARY_DIR})

find_package(Boost REQUIRED)
message(STATUS "Boost headers in: ${Boost_INCLUDE_DIR}")
include_directories(${Boost_INCLUDE_DIR})

find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

find_package(SWIG REQUIRED)
#include(${SWIG_USE_FILE})

if (BUILD_SHARED_LIBS AND WIN32)
#  add_definitions(-DXYLIB_DLL=1)
endif()

if (DOWNLOAD_ZLIB)
  include(ExternalProject)
#TODO
elseif (USE_ZLIB)
  find_package(ZLIB REQUIRED)
  add_definitions(-DHAVE_LIBZ=1)
  include_directories(${ZLIB_INCLUDE_DIR})
endif()

#if (MSVC)
#    # warning C4275: non dll-interface class 'std::runtime_error' used as base
#    # for dll-interface class ...
#    add_definitions( /wd4275 )
#endif()

# show warnings by default
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wextra HAS_WEXTRA_FLAG)
if (HAS_WEXTRA_FLAG)
  set(EXTRA_CXX_FLAGS "-Wall -Wextra" CACHE STRING "")
endif()
set(EXTRA_CXX_FLAGS ${EXTRA_CXX_FLAGS} CACHE STRING "Flags for compiler" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")

set(lua_runtime swig/luarun.h)
set(lua_cxx swig/fityk_lua.cpp)
add_custom_command(OUTPUT ${lua_runtime}
                   COMMAND ${CMAKE_COMMAND} -E make_directory swig
                   COMMAND ${SWIG_EXECUTABLE}
                   ARGS -lua -external-runtime ${lua_runtime})
add_custom_command(OUTPUT ${lua_cxx}
                   COMMAND ${CMAKE_COMMAND} -E make_directory swig
                   COMMAND ${SWIG_EXECUTABLE}
                   ARGS -c++ -lua -I${CMAKE_SOURCE_DIR}/fityk
                        -o ${lua_cxx}
                        ${CMAKE_SOURCE_DIR}/fityk/swig/fityk.i
                   DEPENDS fityk/fityk.h)

set_source_files_properties(fityk/swig/fityk.i PROPERTIES CPLUSPLUS ON)
#set_source_files_properties(fityk/swig/fityk.i PROPERTIES SWIG_FLAGS "-includeall")
add_library(fityk
fityk/ast.cpp        fityk/fityk.cpp      fityk/mgr.cpp        fityk/udf.cpp
fityk/bfunc.cpp      fityk/func.cpp       fityk/model.cpp      fityk/ui_api.cpp
fityk/CMPfit.cpp     fityk/GAfit.cpp      fityk/NLfit.cpp      fityk/ui.cpp
fityk/common.cpp     fityk/guess.cpp      fityk/NMfit.cpp      fityk/var.cpp
fityk/cparser.cpp    fityk/info.cpp       fityk/numfuncs.cpp   fityk/view.cpp
fityk/data.cpp       fityk/lexer.cpp      fityk/runner.cpp     fityk/vm.cpp
fityk/eparser.cpp    fityk/LMfit.cpp      fityk/settings.cpp   fityk/voigt.cpp
fityk/f_fcjasym.cpp  fityk/logic.cpp      fityk/tplate.cpp
fityk/fit.cpp        fityk/luabridge.cpp  fityk/transform.cpp
fityk/cmpfit/mpfit.c
${lua_runtime} ${lua_cxx})

if (DOWNLOAD_ZLIB)
  add_dependencies(fityk zlib)
endif()

target_link_libraries(fityk xy ${LUA_LIBRARIES} ${ZLIB_LIBRARIES})
set_target_properties(fityk PROPERTIES SOVERSION 4 VERSION 4.0.0)

# ignoring libreadline for now
add_executable(cfityk cli/gnuplot.cpp cli/main.cpp)
target_link_libraries(cfityk fityk)

#swig_add_module(fityk python fityk/swig/fityk.i fityk/ui_api.h)
#swig_link_libraries(fityk fityk ${PYTHON_LIBRARIES})


install(TARGETS cfityk DESTINATION bin)
install(TARGETS fityk
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}")
