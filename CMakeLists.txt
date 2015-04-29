# Use autotools-based build system instead if you can.
# For now CMake limited to building libfityk and cfityk.
# But it can automatically download and build Lua, xylib and zlib!
# You still need to have Boost libraries (headers only).
# If they can't be found automatically, use -D BOOST_ROOT=C:\Path\to\boost

cmake_minimum_required(VERSION 2.8)
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

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 OLD)
endif()

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
#include(${SWIG_USE_FILE})

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
    URL "http://zlib.net/zlib-1.2.8.tar.gz"
    URL_MD5 44d667c142d7cda120332623eab69f40
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
    set(ZLIB_NAME zlibstatic)
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
  set(XY_LIBRARY xy)
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
fityk/cmpfit/mpfit.c
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

#add_definitions(-DVERSION="1.3.0")
set_target_properties (fityk cfityk PROPERTIES
                       COMPILE_DEFINITIONS VERSION="1.3.0")


#swig_add_module(fityk python fityk/swig/fityk.i fityk/ui_api.h)
#swig_link_libraries(fityk fityk ${PYTHON_LIBRARIES})


install(TARGETS cfityk DESTINATION bin)
install(TARGETS fityk ${BUILT_LUA}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}")


enable_testing()
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/tests/catch.hpp")
  message(STATUS "Downloading catch.hpp...")
  file(DOWNLOAD
       https://raw.githubusercontent.com/philsquared/Catch/develop/single_include/catch.hpp
       "${CMAKE_SOURCE_DIR}/tests/catch.hpp" SHOW_PROGRESS)
endif()
add_library(catch STATIC tests/catch.cpp)
foreach(t gradient guess psvoigt num lua)
  add_executable(test_${t} tests/${t}.cpp)
  target_link_libraries(test_${t} fityk catch)
  add_test(NAME ${t} COMMAND $<TARGET_FILE:test_${t}>)
endforeach()
