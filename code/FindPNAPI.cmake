# Exports the target PNAPI::pnapi if found.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PNAPI libpnapi)
if(PNAPI_FOUND)
    if(NOT PNAPI_FIND_QUIETLY)
        message(STATUS "Found PNAPI: ${PNAPI_LINK_LIBRARIES}")
    endif()

    add_library(PNAPI::pnapi SHARED IMPORTED)
    set_target_properties(PNAPI::pnapi PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${PNAPI_INCLUDE_DIRS}
            IMPORTED_LOCATION ${PNAPI_LINK_LIBRARIES})
else()
    if(PNAPI_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find PNAPI."
                "\nAdd an appropriate pkg-config path to CMAKE_PREFIX_PATH so 'libpnapi' can be found."
                "\nCMAKE_PREFIX_PATH currently is: ${CMAKE_PREFIX_PATH}")
    endif()
endif()
