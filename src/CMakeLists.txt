###########
# Library #
###########

set(LIBRARY_NAME "${PROJECT_NAME}l")

set(Header_Files
        "../include/args.h"
        "../include/bc.h"
        "../include/bcl.h"
        "../include/dc.h"
        "../include/file.h"
        "../include/history.h"
        "../include/lang.h"
        "../include/lex.h"
        "../include/library.h"
        "../include/num.h"
        "../include/opt.h"
        "../include/parse.h"
        "../include/program.h"
        "../include/rand.h"
        "../include/status.h"
        "../include/vector.h"
        "../include/version.h"
        "../include/vm.h"
)
source_group("Header Files" FILES "${Header_Files}")

set(Source_Files
        "args.c"
        "bc.c"
        "bc_lex.c"
        "bc_parse.c"
        "data.c"
        "dc.c"
        "dc_lex.c"
        "dc_parse.c"
        "file.c"
        "history.c"
        "lang.c"
        "lex.c"
        "library.c"
        "num.c"
        "opt.c"
        "parse.c"
        "program.c"
        "rand.c"
        "read.c"
        "vector.c"
        "vm.c"
)
source_group("Source Files" FILES "${Source_Files}")

add_library("${LIBRARY_NAME}" "${Header_Files}" "${Source_Files}")

include(GNUInstallDirs)

target_link_libraries(
        "${LIBRARY_NAME}"
        PRIVATE
        "${PROJECT_NAME}_compiler_flags"
)

target_include_directories(
        "${LIBRARY_NAME}"
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# if (HAVE_BCRYPT)
if (WIN32)
    target_link_libraries("${LIBRARY_NAME}" PUBLIC bcrypt.lib)
endif ()

target_compile_definitions(
        "${LIBRARY_NAME}"
        PUBLIC
        BC_ENABLE_EXTRA_MATH=1
        BC_ENABLE_HISTORY=1
        BC_ENABLED=1
)

set_target_properties("${LIBRARY_NAME}" PROPERTIES LINKER_LANGUAGE C)


#######
# CLI #
#######

set(EXEC_NAME "${PROJECT_NAME}")

set(Cli_Source_Files "main.c")
source_group("${EXEC_NAME} Source Files" FILES "${Cli_Source_Files}")

add_executable("${EXEC_NAME}" "${Cli_Source_Files}")

target_link_libraries("${EXEC_NAME}" "${LIBRARY_NAME}")

set_target_properties("${EXEC_NAME}" PROPERTIES LINKER_LANGUAGE C)

#################
# Install rules #
#################

include(GNUInstallDirs)

# setup the version numbering
set_property(TARGET "${LIBRARY_NAME}" PROPERTY VERSION "${${PROJECT_NAME}_VERSION}")
set_property(TARGET "${LIBRARY_NAME}" PROPERTY SOVERSION "${${PROJECT_NAME}_VERSION_MAJOR}")

include(GenerateExportHeader)
set(_export_file "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}_export.h")
generate_export_header("${LIBRARY_NAME}" EXPORT_FILE_NAME "${_export_file}")

install(FILES ${Header_Files} "${_export_file}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

set(installable_libs # "${EXEC_NAME}"
        "${LIBRARY_NAME}" )
if (TARGET "${DEPENDANT_LIBRARY}")
    list(APPEND installable_libs "${DEPENDANT_LIBRARY}")
endif ()
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
    list(APPEND installable_libs "${PROJECT_NAME}_compiler_flags")
endif ()
install(TARGETS ${installable_libs}
        EXPORT "${LIBRARY_NAME}Targets"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(EXPORT "${LIBRARY_NAME}Targets"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${LIBRARY_NAME}")
