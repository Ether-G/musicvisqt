# musicvisqt/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(musicvisqt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt Auto Tools ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# --- Find External Libraries ---
# You might want to remove this hardcoded path if you pass it via the command line anyway
# set(CMAKE_PREFIX_PATH "/home/looter/Qt/6.9.0/gcc_64") # Qt Path
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Multimedia)

# *** Use PkgConfig to find SndFile ***
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile) # Check for 'sndfile.pc'

# This function comes from find_package(Qt6...)
qt_standard_project_setup()

# --- Set static path for presets ---
# Instead of fetching presets, use the local path
set(STATIC_PRESET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/presets")

# --- Configure and Add projectM Subdirectory ---
# Make sure the path 'external/projectm' relative to this CMakeLists.txt is correct
# Add any necessary projectM CMake options here if needed, e.g.:
# set(PROJECTM_BUILD_TESTS OFF CACHE BOOL "Disable projectM tests")
# set(PROJECTM_BUILD_SDL_TESTS OFF CACHE BOOL "Disable projectM SDL UI")
add_subdirectory(external/projectm)

# --- Define Your Executable ---
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    projectmwindow.cpp
    projectmwindow.h
    playercontroller.cpp
    playercontroller.h
)

add_executable(musicvisqt
    ${PROJECT_SOURCES}
)

# --- Add Compile Definition for Preset Path ---
target_compile_definitions(musicvisqt PRIVATE
    "PRESET_PATH_FROM_CMAKE=\"${STATIC_PRESET_PATH}\""
)

# --- Add Include Directories ---
target_include_directories(musicvisqt PUBLIC
    # projectM includes
    ${CMAKE_CURRENT_SOURCE_DIR}/external/projectm/src/libprojectM
    ${CMAKE_CURRENT_SOURCE_DIR}/external/projectm/src/api/include
    ${CMAKE_CURRENT_BINARY_DIR}/external/projectm/src/api/include

    # *** Add libsndfile include directories found by pkg-config ***
    ${SNDFILE_INCLUDE_DIRS}

    # Add current source dir for headers
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- Link Libraries ---
target_link_libraries(musicvisqt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    projectM
    ${SNDFILE_LIBRARIES}
)

# --- Install (Optional) ---
install(TARGETS musicvisqt
    RUNTIME DESTINATION bin
)

# Install presets
install(DIRECTORY presets/
    DESTINATION share/musicvisqt/presets
    FILES_MATCHING PATTERN "*.milk"
)

# Install textures
install(DIRECTORY presets/Textures/
    DESTINATION share/musicvisqt/presets/Textures
    FILES_MATCHING PATTERN "*.png"
    PATTERN "*.jpg"
    PATTERN "*.jpeg"
)