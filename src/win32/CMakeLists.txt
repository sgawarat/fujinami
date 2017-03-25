# fujinami_hook
add_library(fujinami_hook SHARED
    fujinami_hook.cpp
)
target_include_directories(fujinami_hook PRIVATE
    ${FUJINAMI_INCLUDE_DIR}
)
set_target_properties(fujinami_hook PROPERTIES CXX_STANDARD 14)
target_compile_options(fujinami_hook BEFORE PUBLIC
    /W3
    /GR-
)
target_compile_definitions(fujinami_hook PUBLIC
    WIN32
    _WINDOWS
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# fujinami
add_executable(fujinami
    main.cpp
    fujinami.rc
)
set_target_properties(fujinami PROPERTIES CXX_STANDARD 14)
target_link_libraries(fujinami PRIVATE
    fujinami_common
    fujinami_hook
    imm32.lib
)
set_target_properties(fujinami PROPERTIES LINK_FLAGS
    "/SUBSYSTEM:WINDOWS"
)

# dependencies
add_dependencies(fujinami
    fujinami_common
    fujinami_hook
)

# install
install(TARGETS fujinami DESTINATION .)
install(TARGETS fujinami_hook RUNTIME DESTINATION .)
