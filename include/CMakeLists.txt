set(LIBRARY_NAME "${PROJECT_NAME}_includes")

set(Header_Files
        "args.h"
        "bc.h"
        "bcl.h"
        "dc.h"
        "file.h"
        "history.h"
        "lang.h"
        "lex.h"
        "library.h"
        "num.h"
        "opt.h"
        "parse.h"
        "program.h"
        "rand.h"
        "read.h"
        "status.h"
        "vector.h"
        "version.h"
        "vm.h"
)
source_group("Header Files" FILES "${Header_Files}")
add_library("${LIBRARY_NAME}" INTERFACE "${Header_Files}")

include(GNUInstallDirs)
target_include_directories(
        "${LIBRARY_NAME}"
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(
        "${LIBRARY_NAME}"
        INTERFACE
        "${PROJECT_NAME}_compiler_flags"
)

set_target_properties("${LIBRARY_NAME}" PROPERTIES LINKER_LANGUAGE C)

install(FILES ${Header_Files}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
