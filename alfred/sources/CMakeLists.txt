cmake_minimum_required (VERSION 2.8.12)
project("xxxxxDRP")

if( NOT VERSION )
	set(VERSION "1.0.0")
endif()

if ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	message(STATUS "x64 arch detected")
	set( NARCH x64 )
else()
	message(STATUS "x86 arch detected")
	set( NARCH x86 )
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

add_custom_target(externals)
set(DEPROOT ${CMAKE_BINARY_DIR}/deproot)
set(DEPROOT_LIB ${DEPROOT}/lib)
set(DEPROOT_INCLUDE ${DEPROOT}/include)
file(MAKE_DIRECTORY ${DEPROOT})
file(MAKE_DIRECTORY ${DEPROOT_LIB})
file(MAKE_DIRECTORY ${DEPROOT_INCLUDE})
include_directories(${DEPROOT_INCLUDE})
add_subdirectory(externals)

# ==========================
# Compile the daemon binary.
# ==========================
list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/jsoncpp.cpp)
list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/main.cpp)
list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/configuration.cpp)
list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/main_loop.cpp)
list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/log.cpp)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/windows_subprocess.cpp)
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/windows_service.cpp)
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/windows_mountpoint.cpp)
else()
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/linux_mountpoint.cpp)
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/linux_mountpoint_conflict.cpp)
	list(APPEND xxxxx_drp_src ${CMAKE_CURRENT_LIST_DIR}/sources/linux_service.cpp)
endif()

add_executable(xxxxx_drp ${xxxxx_drp_src})

if( NOFORK ) 
	target_compile_definitions(xxxxx_drp PRIVATE NOFORK=1)
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/headers/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h ESCAPE_QUOTES @ONLY)
target_include_directories(xxxxx_drp BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(xxxxx_drp PRIVATE ${CMAKE_CURRENT_LIST_DIR}/headers)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	target_link_libraries(xxxxx_drp mpr netapi32 wtsapi32 secur32)
elseif( NOT NOLIBMOUNT )
	add_dependencies(xxxxx_drp externals)
	target_link_libraries(xxxxx_drp mount)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
elseif( NOLIBMOUNT )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOLIBMOUNT")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
endif()

add_subdirectory(installer)

if (CMAKE_BUILD_TYPE MATCHES Release)
	message("[INFO] Release mode")
else(CMAKE_BUILD_TYPE MATCHES Release)
	message("[INFO] Debug mode")
endif (CMAKE_BUILD_TYPE MATCHES Release)
