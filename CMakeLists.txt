cmake_minimum_required(VERSION 3.16)

# Sets project name
project(projects_realtime LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Xml)

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

include_directories(src/shapes)
include_directories(src/utils)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp

    src/realtime.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/utils/scenefilereader.cpp
    src/utils/sceneparser.cpp

    src/mainwindow.h
    src/realtime.h
    src/settings.h
    src/utils/scenedata.h
    src/utils/scenefilereader.h
    src/utils/sceneparser.h
    src/utils/shaderloader.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp
    src/utils/camera.h src/utils/camera.cpp
    src/utils/cone.h src/utils/cone.cpp
    src/utils/cube.h src/utils/cube.cpp
    src/utils/cylinder.h src/utils/cylinder.cpp
    src/utils/sphere.h src/utils/sphere.cpp
    src/terraingenerator.h src/terraingenerator.cpp
    src/utils/gbuffer.h src/utils/gbuffer.cpp
    src/utils/debug.h
    src/utils/snakegame.h src/utils/snakegame.cpp
    src/portal.h src/portal.cpp
)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# Specifies libraries to be linked (Qt components, glew, etc)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
)

# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX
        "/"
    FILES
        resources/shaders/default.frag
        resources/shaders/default.vert
        resources/textures/brick_diffuse.jpg
        resources/textures/brick_normal.jpg
        resources/textures/Rock051_1K-JPG_Color.jpg
        resources/textures/Rock051_1K-JPG_NormalGL.jpg
        resources/textures/grass_color.jpg
        resources/textures/grass_height.jpg
        resources/textures/start_screen.jpg
        resources/textures/wall_texture.png

        # deferred rendering pipeline shaders
        resources/shaders/blur.frag
        resources/shaders/composite.frag
        resources/shaders/deferredLighting.frag
        resources/shaders/fullscreen_quad.vert
        resources/shaders/gbuffer.frag
        resources/shaders/gbuffer.vert

        # ----- NEW PORTAL SHADERS -----
        resources/shaders/portal.frag
        resources/shaders/portal.vert
        resources/shaders/portal_border.frag
        resources/shaders/portal_border.vert
)


# ----- Textures -----
file(GLOB TEXTURE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/textures/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/textures/*.jpg"
)

# Make Qt Creator show textures under a named group
source_group("projects_realtime_other_files/resources/textures" FILES ${TEXTURE_FILES})

# Ensure destination directory exists
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources/textures")

# Copy textures one by one, preserving filenames
foreach(tex ${TEXTURE_FILES})
    get_filename_component(tex_name "${tex}" NAME)
    configure_file(
        "${tex}"
        "${CMAKE_CURRENT_BINARY_DIR}/resources/textures/${tex_name}"
        COPYONLY
    )
endforeach()



# ----- Shaders -----
file(GLOB SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.frag"
)

# Show shaders in Qt Creator
source_group("projects_realtime_other_files/resources/shaders" FILES ${SHADER_FILES})

# Ensure destination directory exists
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders")

# Copy shaders one by one, preserving filenames
foreach(shader ${SHADER_FILES})
    get_filename_component(shader_name "${shader}" NAME)
    configure_file(
        "${shader}"
        "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${shader_name}"
        COPYONLY
    )
endforeach()




# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
