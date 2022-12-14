include(UsePkgConfig)

MACRO(PKGCONFIG_GETVAR _package _var _output_variable)
    SET(${_output_variable})

    # if pkg-config has been found
    IF (PKGCONFIG_EXECUTABLE)

        EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --exists RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull)

        # and if the package of interest also exists for pkg-config, then get the information
        IF (NOT _return_VALUE)

            EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable ${_var} OUTPUT_VARIABLE ${_output_variable})

        ENDIF (NOT _return_VALUE)

    ENDIF (PKGCONFIG_EXECUTABLE)
ENDMACRO(PKGCONFIG_GETVAR _package _var _output_variable)

macro(dbus_add_activation_service _sources)
    PKGCONFIG_GETVAR(dbus-1 session_bus_services_dir _install_dir)
    foreach (_i ${_sources})
        get_filename_component(_service_file ${_i} ABSOLUTE)
        string(REGEX REPLACE "\\.service.*$" ".service" _output_file ${_i})
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_output_file})
        configure_file(${_service_file} ${_target})
        install(FILES ${_target} DESTINATION ${_install_dir} RENAME "${LIBCLOUDPROVIDERS_DBUS_BUS_NAME}.service")
    endforeach (_i ${ARGN})
endmacro(dbus_add_activation_service _sources)

macro(libcloudproviders_add_config _sources)
    set(_install_dir "${CMAKE_INSTALL_PREFIX}/share/cloud-providers")
    foreach (_i ${_sources})
        get_filename_component(_service_file ${_i} ABSOLUTE)
        string(REGEX REPLACE "\\.ini.*$" ".ini" _output_file ${_i})
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_output_file})
        configure_file(${_service_file} ${_target})
        install(FILES ${_target} DESTINATION ${_install_dir} RENAME "${LIBCLOUDPROVIDERS_DBUS_BUS_NAME}.ini")
    endforeach (_i ${ARGN})
endmacro(libcloudproviders_add_config _sources)


find_package(Qt5 5.15 COMPONENTS DBus)
IF (Qt5DBus_FOUND)
    STRING(TOLOWER "${APPLICATION_VENDOR}" DBUS_VENDOR)
    STRING(REGEX REPLACE "[^A-z0-9]" "" DBUS_VENDOR "${DBUS_VENDOR}")
    STRING(REGEX REPLACE "[^A-z0-9]" "" DBUS_APPLICATION_NAME "${APPLICATION_SHORTNAME}")
    if (NOT DBUS_PREFIX)
        set(DBUS_PREFIX "com")
    endif ()

    set(LIBCLOUDPROVIDERS_DBUS_BUS_NAME "${DBUS_PREFIX}.${DBUS_VENDOR}.${DBUS_APPLICATION_NAME}")
    set(LIBCLOUDPROVIDERS_DBUS_OBJECT_PATH "/${DBUS_PREFIX}/${DBUS_VENDOR}/${DBUS_APPLICATION_NAME}")

    dbus_add_activation_service(org.freedesktop.CloudProviders.service.in)
    libcloudproviders_add_config(org.freedesktop.CloudProviders.ini.in)
ENDIF ()
