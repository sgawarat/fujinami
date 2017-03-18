add_library(fujinami_common STATIC
    buffering/buffering_engine.cpp
    buffering/flow/deferred_key_flow.cpp
    buffering/flow/immediate_key_flow.cpp
    buffering/flow/simul_key_flow.cpp
    config/config_loader.cpp
    logging/logging.cpp
    mapping/mapping_engine.cpp
    keyboard.cpp
)
set_target_properties(fujinami_common PROPERTIES CXX_STANDARD 14)
target_include_directories(fujinami_common PUBLIC
    ${FUJINAMI_INCLUDE_DIR}
    ${BOOST_PREPROCESSOR_INCLUDE_DIR}
    ${GSL_LITE_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${SOL2_INCLUDE_DIR}
)
target_compile_definitions(fujinami_common PUBLIC
    $<$<CONFIG:RelWithDebInfo>:DEVEL>
)
target_link_libraries(fujinami_common PUBLIC
    ${LUA_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(fujinami_common PUBLIC
        pthread
    )

    if(ENABLE_CODECOVERAGE)
        target_compile_options(fujinami_common BEFORE PUBLIC
            --coverage
        )
        target_link_libraries(fujinami_common PUBLIC
            gcov
        )
    endif()
elseif(MSVC)
endif()
