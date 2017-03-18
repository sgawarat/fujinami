add_executable(fujinami
    main.cpp
    input.cpp
)
target_link_libraries(fujinami PRIVATE
    fujinami_common
)

# include
#install(TARGETS fujinami DESTINATION bin)
