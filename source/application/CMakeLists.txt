add_executable(QVulkanApp)

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

target_sources(QVulkanApp
    PRIVATE
        main.cpp
        RenderInterface.h
        RenderInterface.cpp
        Loader.cpp
        ${PROJECT_SOURCE_DIR}/textures/textures.qrc
        ${PROJECT_SOURCE_DIR}/shaders/shaders.qrc
        ${SHADERS}
)

target_link_libraries(QVulkanApp
    VulkanRenderer
    Scene

    Vulkan::Vulkan
    Qt5::Gui
)
