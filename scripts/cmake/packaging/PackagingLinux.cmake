set(CPACK_GENERATOR TGZ)
# Adds the binaries location to the LD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH \$ORIGIN/)

if(MODULE_CMD)
    message(STATUS "Found module cmd -> writing module file.")
    execute_process(COMMAND ${MODULE_CMD} bash list --terse
        ERROR_VARIABLE MODULE_LIST_OUTPUT
    )
    string(REPLACE "\n" ";" MODULE_LIST_OUTPUT ${MODULE_LIST_OUTPUT})
    foreach(line ${MODULE_LIST_OUTPUT})
        if(NOT line STREQUAL "Currently Loaded Modulefiles:")
            set(MODULE_LOAD_STRING "${MODULE_LOAD_STRING}module load ${line}\n")
        endif()
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/scripts/cmake/packaging/module.in
        ${PROJECT_BINARY_DIR}/module
    )
    if(OGS_MODULEFILE)
        get_filename_component(MODULE_DIR ${OGS_MODULEFILE} DIRECTORY)
        get_filename_component(MODULE_NAME ${OGS_MODULEFILE} NAME)
        install(FILES ${PROJECT_BINARY_DIR}/module DESTINATION ${MODULE_DIR}
            RENAME ${MODULE_NAME})
    endif()
endif()

set(README_PLATFORM_INSTRUCTIONS
    "When running the Data Explorer make sure to set the LD_LIBRARY_PATH path to the bin-folder. E.g.:\n\nLD_LIBRARY_PATH=$LD_LIBRARY_PATH:./ ./DataExplorer"
    CACHE INTERNAL ""
)
