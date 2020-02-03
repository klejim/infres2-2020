include(ExternalProject)

if( UNIX AND NOT NOLIBMOUNT )
    ExternalProject_Add(
        libmount
        URL ${CMAKE_CURRENT_SOURCE_DIR}/util-linux-2.29.tar.gz
        CONFIGURE_COMMAND ./configure 
        BUILD_COMMAND make libmount.la
        BUILD_IN_SOURCE true
        INSTALL_COMMAND cmake -E copy libmount/src/libmount.h ${DEPROOT_INCLUDE}
        COMMAND cmake -E copy_directory .libs ${DEPROOT_LIB}
        )

    add_dependencies(externals libmount)
endif()
