add_executable(fujinami
    main.cpp
    input.cpp
)
target_link_libraries(fujinami PRIVATE
    fujinami_common
)
target_compile_features(fujinami PRIVATE
    cxx_std_14
)

# include
install(TARGETS fujinami DESTINATION .)
