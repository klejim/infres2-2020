set(INSTALLER_TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/installer_tmp)
file(MAKE_DIRECTORY ${INSTALLER_TMP_DIR})

include(ConfigureFileGenerate)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	add_custom_target(installer ALL)
	add_dependencies(installer xxxxx_drp)
	set(MAKE_NSIS "C:\\Program Files (x86)\\NSIS\\makensis.exe")
	set(INSTALLER_NAME xxxxxdrp-${VERSION}-${NARCH}.exe)
	configure_file(${CMAKE_CURRENT_LIST_DIR}/conf.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/conf.cfg @ONLY)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    #set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_TMP_DIR})
    include(InstallRequiredSystemLibraries)

    #Bug corrigé dans la 3.8.0 https://gitlab.kitware.com/cmake/cmake/merge_requests/341
    if( CMAKE_VERSION VERSION_LESS 3.8.0 )
        list(APPEND ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} "${MSVC14_CRT_DIR}\\concrt140.dll")
    endif()

    set(INSTALLER_TMP_DIR_DLL "${INSTALLER_TMP_DIR}\\dll")
    file(MAKE_DIRECTORY ${INSTALLER_TMP_DIR_DLL})
    foreach(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        add_custom_command(TARGET installer PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${lib}" "."
            WORKING_DIRECTORY "${INSTALLER_TMP_DIR_DLL}"
            VERBATIM)
    endforeach()

	add_custom_command(TARGET installer
        POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:xxxxx_drp>" "."
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/nsis/installer.nsi" "."
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/nsis/logo_xxxxx_210.bmp" "."
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/conf.cfg" "."
		COMMAND ${MAKE_NSIS} -DVERSION=${VERSION} -DARCH=${NARCH} -DOS=${NOS} "./installer.nsi"
		# name of installer is hardcoded in installer.nsi
		COMMAND ${CMAKE_COMMAND} -E rename "./xxxxx_drp_installer.exe" "${CMAKE_CURRENT_BINARY_DIR}/${INSTALLER_NAME}"
        WORKING_DIRECTORY ${INSTALLER_TMP_DIR}
		VERBATIM
	)
else()
	set(INSTALL_PATH "/usr/local/xxxxx-technologies/drp")
	SET(CPACK_GENERATOR "RPM;DEB")
	SET(CPACK_PACKAGE_NAME "xxxxxdrp")
    SET(CPACK_PACKAGE_VERSION "${VERSION}")
	SET(CPACK_PACKAGE_VENDOR "Xxxxx Technologies")
	SET(CPACK_PACKAGE_CONTACT "Xxxxx Technologies")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${NARCH}")
	if( ${CMAKE_VERSION} VERSION_GREATER "3.7")
		SET(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	endif()

    if( NARCH MATCHES "x64" )
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
    endif()

	SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Xxxxx DRP agent to automatize failover and failback mount points.")
	SET(CPACK_PACKAGE_DESCRIPTION_SHORT "Xxxxx DRP agent")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3.1-6), libstdc++6 (>= 6.1.1-10), lsb-base (>= 9.20161125)")
    SET(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.5-123, libstdc++ >= 4.1.2-55")

	SET(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/rpm/preinst")
	SET(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/rpm/postinst")
	file(GLOB DEBIAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/deb/*)
	SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${DEBIAN_FILES})

	configure_file_generate(${CMAKE_CURRENT_LIST_DIR}/xxxxxDRP.in ${CMAKE_CURRENT_BINARY_DIR}/xxxxxDRP @ONLY)
	configure_file(${CMAKE_CURRENT_LIST_DIR}/conf.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/conf.cfg @ONLY)

	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/xxxxxDRP
		DESTINATION /etc/init.d/
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/conf.cfg
		DESTINATION ${INSTALL_PATH}
		PERMISSIONS OWNER_READ OWNER_WRITE)

	INSTALL(FILES $<TARGET_FILE:xxxxx_drp>
		DESTINATION ${INSTALL_PATH}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/" "/usr" "/usr/local")
	INCLUDE(CPack)
endif()
