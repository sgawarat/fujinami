cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fujinami)

# headers
set(root_headers
    "include/fujinami/command.hpp"
    "include/fujinami/flagset.hpp"
    "include/fujinami/key.hpp"
    "include/fujinami/key_property.hpp"
    "include/fujinami/keyboard.hpp"
    "include/fujinami/keyboard_config.hpp"
    "include/fujinami/keyboard_layout.hpp"
    "include/fujinami/keyset.hpp"
    "include/fujinami/keyset_property.hpp"
    "include/fujinami/logging.hpp"
    "include/fujinami/time.hpp"
)
set(buffering_headers
    "include/fujinami/buffering/context.hpp"
    "include/fujinami/buffering/engine.hpp"
    "include/fujinami/buffering/event.hpp"
    "include/fujinami/buffering/state.hpp"
)
set(buffering_flow_headers
    "include/fujinami/buffering/flow/deferred.hpp"
    "include/fujinami/buffering/flow/immediate.hpp"
    "include/fujinami/buffering/flow/result.hpp"
    "include/fujinami/buffering/flow/simul.hpp"
)
set(config_headers
    "include/fujinami/config/errors.hpp"
    "include/fujinami/config/loader.hpp"
)
set(logging_headers
    "include/fujinami/logging/macro.hpp"
    "include/fujinami/logging/print.hpp"
)
set(mapping_headers
    "include/fujinami/mapping/context.hpp"
    "include/fujinami/mapping/engine.hpp"
    "include/fujinami/mapping/event.hpp"
)
set(platform_headers
    "include/fujinami/platform/platform.hpp"
)
set(platform_win32_headers
    "include/fujinami/platform/win32/action.hpp"
    "include/fujinami/platform/win32/key.hpp"
    "include/fujinami/platform/win32/time.hpp"
)

#sources
set(root_sources
    "src/buffering_engine.cpp"
    "src/config_loader.cpp"
    "src/deferred_key_flow.cpp"
    "src/immediate_key_flow.cpp"
    "src/keyboard.cpp"
    "src/logging.cpp"
    "src/mapping_engine.cpp"
    "src/simul_key_flow.cpp"
)

add_executable(fujinami
    ${root_headers}
    ${buffering_headers}
    ${buffering_flow_headers}
    ${config_headers}
    ${logging_headers}
    ${mapping_headers}
    ${platform_headers}
    ${platform_win32_headers}
    ${root_sources}
)

target_include_directories(fujinami PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/boost/preprocessor/include"
    "${PROJECT_SOURCE_DIR}/libs/gsl-lite/include"
    "${PROJECT_SOURCE_DIR}/libs/spdlog/include"
    "${LUA_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/sol2"
)

add_executable(fujinami_test
    src/immediate_key_flow.cpp
    src/logging.cpp
    test/immediate_key_flow.cpp
    test/main.cpp
)
target_include_directories(fujinami_test PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/boost/preprocessor/include"
    "${PROJECT_SOURCE_DIR}/libs/gsl-lite/include"
    "${PROJECT_SOURCE_DIR}/libs/spdlog/include"
    "${LUA_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/sol2"
    "${PROJECT_SOURCE_DIR}/libs/catch/include"
)
enable_testing()
add_test(NAME fujinami_test COMMAND fujinami_test)

if(UNIX AND NOT APPLE)
    #find_package(Boost REQUIRED COMPONENTS preprocessor)
    find_package(Lua REQUIRED)

    target_include_directories(fujinami PUBLIC
        "${Boost_INCLUDE_DIRS}"
        "${LUA_INCLUDE_DIR}"
    )

    target_sources(fujinami PUBLIC
        "src/linux/main.cpp"
        "src/linux/input.cpp"
    )
    #target_compile_features(fujinami PUBLIC cxx_std_14)
    set_target_properties(fujinami PROPERTIES CXX_STANDARD 14)
    target_compile_definitions(fujinami PUBLIC
        $<$<CONFIG:RelWithDebInfo>:DEVEL>
    )
    target_link_libraries(fujinami
        pthread
        ${LUA_LIBRARIES}
    )

    target_include_directories(fujinami_test PUBLIC
        "${Boost_INCLUDE_DIRS}"
        "${LUA_INCLUDE_DIR}"
    )
    #target_compile_features(fujinami_test PUBLIC cxx_std_14)
    set_target_properties(fujinami_test PROPERTIES CXX_STANDARD 14)
    target_compile_options(fujinami_test BEFORE PUBLIC
        --coverage
    )
    target_compile_definitions(fujinami_test PUBLIC
        $<$<CONFIG:RelWithDebInfo>:DEVEL>
    )
    target_link_libraries(fujinami_test
        gcov
        pthread
        ${LUA_LIBRARIES}
    )

    # install(TARGETS fujinami DESTINATION bin)

elseif(MSVC)
    set(LUA_DIR "" CACHE PATH "Lua directory")
    option(LUA_DIR "Lua directory" "")

    target_include_directories(fujinami PUBLIC
        "${LUA_DIR}/include"
    )
    target_sources(fujinami PUBLIC
        "src/win32/main.cpp"
        "src/win32/fujinami.rc"
    )
    target_compile_options(fujinami BEFORE PUBLIC
        /W3
        /GR-
    )
    target_compile_definitions(fujinami PUBLIC
        $<$<CONFIG:RelWithDebInfo>:DEVEL>
        WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_link_libraries(fujinami
        imm32.lib
        ${LUA_DIR}/lua53.lib
        fujinami_hook.lib
    )
    set_target_properties(fujinami PROPERTIES LINK_FLAGS
        "/LIBPATH:$(OutDir) /SUBSYSTEM:WINDOWS"
    )

    add_library(fujinami_hook SHARED
        "src/win32/fujinami_hook.cpp"
    )
    target_include_directories(fujinami_hook PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
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

    target_compile_options(fujinami_test BEFORE PUBLIC
        /W3
        /GR-
    )
    target_compile_definitions(fujinami_test PUBLIC
        $<$<CONFIG:RelWithDebInfo>:DEVEL>
        WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_link_libraries(fujinami_test
        imm32.lib
        ${LUA_DIR}/lua53.lib
    )
    set_target_properties(fujinami_test PROPERTIES LINK_FLAGS
        "/LIBPATH:$(OutDir)"
    )

    add_dependencies(fujinami fujinami_hook)

    install(TARGETS fujinami DESTINATION .)
    install(TARGETS fujinami_hook RUNTIME DESTINATION .)
endif()
