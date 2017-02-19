cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(fujinami VERSION 0.1 LANGUAGES CXX)

add_executable(fujinami
    "src/buffering_engine.cpp"
    "src/config_loader.cpp"
    "src/deferred_key_flow.cpp"
    "src/immediate_key_flow.cpp"
    "src/keyboard.cpp"
    "src/logging.cpp"
    "src/mapping_engine.cpp"
)

target_include_directories(fujinami PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/boost/preprocessor/include"
    "${PROJECT_SOURCE_DIR}/libs/gsl-lite/include"
    "${PROJECT_SOURCE_DIR}/libs/spdlog/include"
    "${LUA_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/sol2"
)

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

    add_dependencies(fujinami fujinami_hook)

    install(TARGETS fujinami DESTINATION .)
    install(TARGETS fujinami_hook RUNTIME DESTINATION .)
endif()
