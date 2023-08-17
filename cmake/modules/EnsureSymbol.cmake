include(CheckSymbolExists)

function(ensure_symbol)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "PUBLIC" "TARGET;SYMBOL" "INCLUDES;DEFINITIONS")

    # Determine visibility for symbols that need to be added
    get_property(type TARGET "${ARG_TARGET}" PROPERTY TYPE)
    if (type STREQUAL "INTERFACE_LIBRARY")
        set(visibility INTERFACE)
    elseif (ARG_PUBLIC)
        set(visibility PUBLIC)
    else ()
        set(visibility PRIVATE)
    endif ()

    # The base name of the cache variables to create
    string(TOUPPER "HAVE_${ARG_SYMBOL}" var_name)
    message(STATUS "symbol is ${var_name}")

    # Check the base version
    check_symbol_exists("${ARG_SYMBOL}" "${ARG_INCLUDES}" "${var_name}")

    if (${var_name})
        return ()
    endif ()

    # Otherwise, start trying alternatives
    foreach (def IN LISTS ARG_DEFINITIONS)
        set(local_name "${var_name}${def}")

        set(CMAKE_REQUIRED_DEFINITIONS "-D${def}")
        check_symbol_exists("${ARG_SYMBOL}" "${ARG_INCLUDES}" "${local_name}")

        if (${local_name})
            target_compile_definitions("${ARG_TARGET}" "${visibility}" "${def}")
            return()
        endif ()
    endforeach ()

    message(FATAL_ERROR "Could not ensure ${ARG_TARGET} can use ${ARG_SYMBOL}")
endfunction()
