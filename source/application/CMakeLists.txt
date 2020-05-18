set(Target QVulkanApp)
add_executable(${Target})

set(SHADER_DIR "${PROJECT_SOURCE_DIR}/shaders")
file(GLOB SHADERS
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.comp"
    "${SHADER_DIR}/*.geom"
    "${SHADER_DIR}/*.tesc"
    "${SHADER_DIR}/*.tese"
    "${SHADER_DIR}/*.mesh"
    "${SHADER_DIR}/*.task"
    "${SHADER_DIR}/*.rgen"
    "${SHADER_DIR}/*.rchit"
    "${SHADER_DIR}/*.rmiss"
)
source_group("Shaders" FILES ${SHADERS})

target_sources(${Target}
    PRIVATE
        main.cpp
        RenderInterface.h
        RenderInterface.cpp
        Loader.cpp
        ${PROJECT_SOURCE_DIR}/textures/textures.qrc
        ${PROJECT_SOURCE_DIR}/shaders/shaders.qrc
        ${SHADERS}
)

target_link_libraries(${Target}
    VulkanRenderer
    Scene

    Vulkan::Vulkan
    Qt5::Gui
)

set_target_properties(${Target} PROPERTIES FOLDER Applications)

set (QT5LIBS
    Gui
    Guid
    Core
    Cored
)

foreach(LIB ${QT5LIBS})
    add_custom_command(TARGET ${Target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${QT_DIR}/bin/Qt5${LIB}.dll $<TARGET_FILE_DIR:${Target}>)
endforeach()

add_custom_command(TARGET ${Target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${QT_DIR}/plugins/platforms/qwindows.dll $<TARGET_FILE_DIR:${Target}>/platforms/qwindows.dll)
add_custom_command(TARGET ${Target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${QT_DIR}/plugins/platforms/qwindowsd.dll $<TARGET_FILE_DIR:${Target}>/platforms/qwindowsd.dll)
