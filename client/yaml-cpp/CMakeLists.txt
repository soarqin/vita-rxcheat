###
### CMake settings
###
## Due to Mac OSX we need to keep compatibility with CMake 2.6
# see http://www.cmake.org/Wiki/CMake_Policies
cmake_minimum_required(VERSION 2.6)
# see http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0012
if(POLICY CMP0012)
	cmake_policy(SET CMP0012 OLD)
endif()
# see http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0015
if(POLICY CMP0015)
	cmake_policy(SET CMP0015 OLD)
endif()
# see https://cmake.org/cmake/help/latest/policy/CMP0042.html
if(POLICY CMP0042)
	# Enable MACOSX_RPATH by default.
	cmake_policy(SET CMP0042 NEW)
endif()

include(CheckCXXCompilerFlag)


###
### Project settings
###
project(YAML_CPP)

set(YAML_CPP_VERSION_MAJOR "0")
set(YAML_CPP_VERSION_MINOR "6")
set(YAML_CPP_VERSION_PATCH "2")
set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")

# --> Apple
option(APPLE_UNIVERSAL_BIN "Apple: Build universal binary" OFF)

# --> Microsoft Visual C++
# see http://msdn.microsoft.com/en-us/library/aa278396(v=VS.60).aspx
#     http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=VS.71).aspx
option(MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" ON)
option(MSVC_STHREADED_RT "MSVC: Build with single-threaded static runtime libs (/ML until VS .NET 2003)" OFF)

###
### Sources, headers, directories and libs
###

# From http://www.cmake.org/pipermail/cmake/2010-March/035992.html:
# function to collect all the sources from sub-directories
# into a single list
function(add_sources)
  get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of all source files in the entire project")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY SRCS_LIST "${SRCS}")
endfunction(add_sources)

set(header_directory "include/yaml-cpp/")

file(GLOB sources "src/[a-zA-Z]*.cpp")
file(GLOB_RECURSE public_headers "include/yaml-cpp/[a-zA-Z]*.h")
file(GLOB private_headers "src/[a-zA-Z]*.h")

add_definitions(-DYAML_CPP_NO_CONTRIB)

set(library_sources
  ${sources}
  ${public_headers}
  ${private_headers}
)
add_sources(${library_sources})

if(VERBOSE)
	message(STATUS "sources: ${sources}")
	message(STATUS "public_headers: ${public_headers}")
	message(STATUS "private_headers: ${private_headers}")
endif()

include_directories(${YAML_CPP_SOURCE_DIR}/src)
include_directories(${YAML_CPP_SOURCE_DIR}/include)



###
### General compilation settings
###
set(yaml_c_flags ${CMAKE_C_FLAGS})
set(yaml_cxx_flags ${CMAKE_CXX_FLAGS})

if(BUILD_SHARED_LIBS)
	set(LABEL_SUFFIX "shared")
else()
	set(LABEL_SUFFIX "static")
endif()

if(APPLE)
	if(APPLE_UNIVERSAL_BIN)
		set(CMAKE_OSX_ARCHITECTURES ppc;i386)
	endif()
endif()

if(IPHONE)
	set(CMAKE_OSX_SYSROOT "iphoneos4.2")
	set(CMAKE_OSX_ARCHITECTURES "armv6;armv7")
endif()

# GCC or Clang or Intel Compiler specialities
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
   CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
   CMAKE_CXX_COMPILER_ID MATCHES "Intel")

	### Project stuff
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-O2")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

	set(GCC_EXTRA_OPTIONS "")
	set(FLAG_TESTED "-Wextra")
	check_cxx_compiler_flag(${FLAG_TESTED} FLAG_WEXTRA)
	if(FLAG_WEXTRA)
		set(GCC_EXTRA_OPTIONS "${GCC_EXTRA_OPTIONS} ${FLAG_TESTED}")
	endif()

	set(yaml_cxx_flags "-Wall ${GCC_EXTRA_OPTIONS} -pedantic -Wno-long-long -std=c++11 ${yaml_cxx_flags}")

	### Make specific
	if(${CMAKE_BUILD_TOOL} MATCHES make OR ${CMAKE_BUILD_TOOL} MATCHES gmake)
		add_custom_target(debuggable $(MAKE) clean
			COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
			COMMENT "Adjusting settings for debug compilation"
			VERBATIM)
		add_custom_target(releasable $(MAKE) clean
			COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
			COMMENT "Adjusting settings for release compilation"
			VERBATIM)
	endif()
endif()

# Microsoft VisualC++ specialities
if(MSVC)
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")	# to distinguish static libraries from DLL import libs
	set(yaml_cxx_flags "/W3 /wd4127 /wd4355 ${yaml_cxx_flags}")
endif()


###
### Library
###
add_library(yaml-cpp STATIC ${library_sources})
set_target_properties(yaml-cpp PROPERTIES
  COMPILE_FLAGS "${yaml_c_flags} ${yaml_cxx_flags}"
)

set_target_properties(yaml-cpp PROPERTIES
	VERSION "${YAML_CPP_VERSION}"
	SOVERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}"
	PROJECT_LABEL "yaml-cpp ${LABEL_SUFFIX}"
)

if(IPHONE)
	set_target_properties(yaml-cpp PROPERTIES
		XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "3.0"
	)
endif()

if(MSVC)
	# correct library names
	set_target_properties(yaml-cpp PROPERTIES
		DEBUG_POSTFIX "${LIB_TARGET_SUFFIX}d"
		RELEASE_POSTFIX "${LIB_TARGET_SUFFIX}"
		MINSIZEREL_POSTFIX "${LIB_TARGET_SUFFIX}"
		RELWITHDEBINFO_POSTFIX "${LIB_TARGET_SUFFIX}"
	)
endif()
