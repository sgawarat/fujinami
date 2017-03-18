add_executable(fujinami_test
    immediate_key_flow.cpp
    main.cpp
)
set_target_properties(fujinami_test PROPERTIES CXX_STANDARD 14)
target_include_directories(fujinami_test PRIVATE
    ${CATCH_INCLUDE_DIR}
)
target_link_libraries(fujinami_test
    fujinami_common)

enable_testing()
add_test(NAME fujinami_test COMMAND fujinami_test)
