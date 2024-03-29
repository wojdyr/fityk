# CMake build is limited to building libfityk, cfityk and Python module.
# It is used only for building with fityk in Visual Studio, for example
# on AppVeyor: https://ci.appveyor.com/project/wojdyr/fityk
# All offical releases are prepared with autotools instead.

# This build script cannot build GUI, has fewer options than the configure
# script, but it can automatically download and build Lua, xylib and zlib!
# You still need to have Boost libraries (headers only).
# You may need to point to them with -D BOOST_ROOT=C:\Path\to\boost
# If you downloaded the source from git you also need SWIG.
# On Windows it can be installed with "cinst swig".
# When using Microsoft Visual C++ Compiler for Python 2.7 add cmake option
# -D CMAKE_BUILD_TYPE=Release (I haven't investigated why it was needed).

cmake_minimum_required(VERSION 2.8.12)
project(fityk C CXX)

option(DOWNLOAD_LUA "Fetch Lua from its website" ON)
option(DOWNLOAD_XYLIB "Fetch xylib from git" ON)
option(DOWNLOAD_ZLIB "Fetch zlib from its website" ON)
option(BUILD_SHARED_LIBS "Build as a shared library" ON)

# on Windows default CMAKE_INSTALL_PREFIX is Program Files/Fityk
# so it overwrites files from Fityk installer
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND WIN32)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/inst"
      CACHE PATH "install prefix" FORCE)
endif()

# TODO: ${THIRDPARTY_INSTALL} is used during 'make' so should be user-writable,
# OTOH libraries are needed at runtime
set(THIRDPARTY_INSTALL "${CMAKE_INSTALL_PREFIX}")

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

include(ExternalProject)

if (MSVC)
  # C4244: '=' : conversion from 'double' to 'float', possible loss of data
  add_definitions( /wd4244 )
  # same as _CRT_SECURE_NO_WARNINGS
  add_definitions( /wd4996 )
  # warning C4275: non dll-interface class 'std::runtime_error' used as base
  # for dll-interface class ...
  add_definitions( /wd4275 )
  # warning C4251: class (e.g. std::vector<...>) needs to have dll-interface
  # to be used by clients of class ...
  add_definitions( /wd4251 )
endif()

# if we build static library we likely prefer 3rd-party libraries also static
if(NOT BUILD_SHARED_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES
      ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/fityk) # only for fityk_lua.cpp
include_directories(${CMAKE_BINARY_DIR})

if (BUILD_SHARED_LIBS AND WIN32)
  add_definitions(-DLUA_BUILD_AS_DLL)
  add_definitions(-DLIBFITYK_DLL)
#  add_definitions(-DXYLIB_DLL=1)
endif()

find_package(Boost REQUIRED)
message(STATUS "Boost headers in: ${Boost_INCLUDE_DIR}")
include_directories(${Boost_INCLUDE_DIR})

set(thirdparty_dir ${CMAKE_SOURCE_DIR}/3rdparty)
if (DOWNLOAD_LUA)
  ExternalProject_Add(lua52-fetch
      URL "http://www.lua.org/ftp/lua-5.2.4.tar.gz"
      URL_MD5 913fdb32207046b273fdb17aad70be13
      DOWNLOAD_DIR ${thirdparty_dir}
      SOURCE_DIR ${thirdparty_dir}/lua52
      PREFIX lua52
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND "")

  set(lua_filenames
      lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
      lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c
      ltable.c ltm.c lundump.c lvm.c lzio.c
      lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c
      lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c)
  foreach(lua_f ${lua_filenames})
    set(lua_src ${lua_src} ${thirdparty_dir}/lua52/src/${lua_f})
  endforeach()
  set_source_files_properties(${lua_src} PROPERTIES GENERATED TRUE)
  #add_definitions(-DLUA_ANSI=1)
  add_library(lua ${lua_src})
  set(LUA_LIBRARIES lua)
  set(LUA_INCLUDE_DIR "${thirdparty_dir}/lua52/src")
  set(BUILT_LUA lua)
  add_dependencies(lua lua52-fetch)
else()
  find_package(Lua REQUIRED)
endif()
include_directories(${LUA_INCLUDE_DIR})

find_package(SWIG REQUIRED)

if (BUILD_SHARED_LIBS)
  set(LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
  if (WIN32)
    set(LIB_SUFFIX ${CMAKE_IMPORT_LIBRARY_SUFFIX})
  else()
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()
else()
  set(LIB_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
  set(LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if (DOWNLOAD_ZLIB)
  ExternalProject_Add(zlib
    URL "http://zlib.net/zlib-1.2.12.tar.gz"
    URL_HASH SHA256=91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9
    DOWNLOAD_DIR ${thirdparty_dir}
    SOURCE_DIR ${thirdparty_dir}/zlib
    BINARY_DIR ${CMAKE_BINARY_DIR}/zlib
    PREFIX zlib
    CMAKE_ARGS -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
               -DCMAKE_INSTALL_PREFIX:PATH=${THIRDPARTY_INSTALL}
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DSKIP_INSTALL_FILES=ON)
  set(ZLIB_INCLUDE_DIR "${THIRDPARTY_INSTALL}/include")
  if (NOT WIN32)
    set(ZLIB_NAME libz)
  elseif (BUILD_SHARED_LIBS)
    set(ZLIB_NAME zlib$<$<CONFIG:Debug>:d>)
  else() # static windows
    set(ZLIB_NAME zlibstatic$<$<CONFIG:Debug>:d>)
  endif()
  set(ZLIB_LIBRARIES "${THIRDPARTY_INSTALL}/lib/${ZLIB_NAME}${LIB_SUFFIX}")
  message(STATUS "Using local zlib: ${ZLIB_LIBRARIES}")
else()
  find_package(ZLIB REQUIRED)
endif()
include_directories("${ZLIB_INCLUDE_DIR}")

if (DOWNLOAD_XYLIB)
  ExternalProject_Add(xylib
    GIT_REPOSITORY https://github.com/wojdyr/xylib
    DOWNLOAD_DIR ${thirdparty_dir}
    SOURCE_DIR ${thirdparty_dir}/xylib
    BINARY_DIR ${CMAKE_BINARY_DIR}/xylib
    PREFIX xylib
    CMAKE_ARGS "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
               "-DCMAKE_INSTALL_PREFIX:PATH=${THIRDPARTY_INSTALL}"
               "-DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}"
               "-DZLIB_LIBRARIES=${ZLIB_LIBRARIES}"
               -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
               -DGUI=OFF
               "-DBoost_INCLUDE_DIR=${Boost_INCLUDE_DIR}")
  set(XY_LIBRARY "${THIRDPARTY_INSTALL}/lib/${LIB_PREFIX}xy${LIB_SUFFIX}")
  set(XY_INCLUDE_DIR "${THIRDPARTY_INSTALL}/include")
else()
  find_path(XY_INCLUDE_DIR xylib/xylib.h HINTS ${CMAKE_INSTALL_PREFIX}/include)
  find_library(XY_LIBRARY NAMES xy HINTS ${LIB_INSTALL_DIR})
endif()
include_directories("${XY_INCLUDE_DIR}")

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
fityk/cmpfit/mpfit.c fityk/root/background.cpp
${lua_runtime} ${lua_cxx})

if (DOWNLOAD_XYLIB)
  add_dependencies(fityk xylib)
endif()
if (DOWNLOAD_ZLIB)
  add_dependencies(fityk zlib)
endif()
if (DOWNLOAD_ZLIB AND DOWNLOAD_XYLIB)
  add_dependencies(xylib zlib)
endif()

target_link_libraries(fityk ${XY_LIBRARY} ${LUA_LIBRARIES} ${ZLIB_LIBRARIES})
set_target_properties(fityk PROPERTIES SOVERSION 4 VERSION 4.0.0)

# ignoring libreadline for now
add_executable(cfityk cli/gnuplot.cpp cli/main.cpp)
target_link_libraries(cfityk fityk)

#add_definitions(-DVERSION="1.3.2")
set_target_properties (fityk cfityk PROPERTIES
                       COMPILE_DEFINITIONS VERSION="1.3.2")


if(MSVC90) # compiling with VS2008? It's most likely for Python 2.7.
  set(Python_ADDITIONAL_VERSIONS 2.7)
endif()
find_package(PythonLibs)
if (PYTHONLIBS_FOUND)
  message(STATUS "Building extension for Python ${PYTHONLIBS_VERSION_STRING}")
  include_directories(${PYTHON_INCLUDE_DIRS})
  include(${SWIG_USE_FILE})
  set_property(SOURCE fityk/swig/fityk.i PROPERTY SWIG_MODULE_NAME fityk)
  swig_add_module(fityk python fityk/swig/fityk.i fityk/ui_api.h)
  swig_link_libraries(fityk fityk ${PYTHON_LIBRARIES})
  string(REGEX REPLACE "\\.[0-9]+$" ""
         py_dir python${PYTHONLIBS_VERSION_STRING})
  install(TARGETS ${SWIG_MODULE_fityk_REAL_NAME} DESTINATION ${py_dir})
  install(FILES ${CMAKE_BINARY_DIR}/fityk.py DESTINATION ${py_dir})
endif()


install(TARGETS cfityk DESTINATION bin)
install(TARGETS fityk ${BUILT_LUA}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}")
install(FILES fityk/fityk.h fityk/ui_api.h DESTINATION include/fityk)


enable_testing()
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/tests/catch.hpp")
  message(STATUS "Downloading catch.hpp...")
# the latest version of Catch 1.x does not compile with VS 2008
  file(DOWNLOAD
       https://github.com/philsquared/Catch/releases/download/v1.9.5/catch.hpp
       "${CMAKE_SOURCE_DIR}/tests/catch.hpp" SHOW_PROGRESS
       STATUS download_status)
  list(GET download_status 0 status_code)
  if (NOT ${status_code} EQUAL 0)
    message(FATAL_ERROR "Downloading catch.hpp FAILED: ${download_status}")
  endif()
endif()
add_library(catch STATIC tests/catch.cpp)
foreach(t gradient guess psvoigt num lua)
  add_executable(test_${t} tests/${t}.cpp)
  target_link_libraries(test_${t} fityk catch)
  add_test(NAME ${t} COMMAND $<TARGET_FILE:test_${t}>)
endforeach()

# make sure that API examples can be compiled
add_executable(hello_cc samples/hello.cc)
target_link_libraries(hello_cc fityk)
add_test(NAME "helloC++" COMMAND $<TARGET_FILE:hello_cc>)
add_executable(hello_c samples/hello.c)
if(UNIX)
  find_library(MATH_LIBRARY m)
endif()
target_link_libraries(hello_c fityk ${MATH_LIBRARY})
add_test(NAME "helloC" COMMAND $<TARGET_FILE:hello_c>)
