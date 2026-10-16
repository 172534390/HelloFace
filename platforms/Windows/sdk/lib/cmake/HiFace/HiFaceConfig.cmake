get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "HiFace Windows SDK requires a Windows x64 target")
endif()
if(NOT TARGET HiFace::HiFace)
    add_library(HiFace::HiFace SHARED IMPORTED)
    set_target_properties(HiFace::HiFace PROPERTIES
        IMPORTED_LOCATION "${PACKAGE_PREFIX_DIR}/bin/hiface.dll"
        IMPORTED_IMPLIB "${PACKAGE_PREFIX_DIR}/lib/hiface.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include")
endif()
set(HiFace_RUNTIME_DIR "${PACKAGE_PREFIX_DIR}/bin")
set(HiFace_C_FOUND TRUE)
set(HiFace_FOUND TRUE)
