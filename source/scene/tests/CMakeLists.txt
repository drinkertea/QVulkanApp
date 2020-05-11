add_executable(SceneTests
    ChunkUtilsTests.cpp
)

target_link_libraries(SceneTests
    gtest_main
    Scene
)

target_include_directories(SceneTests
    PRIVATE
         ${CMAKE_CURRENT_LIST_DIR}/..
)

set_target_properties(SceneTests PROPERTIES FOLDER Tests)

add_test(
    NAME
        SceneTests
    COMMAND
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/SceneTests
)
