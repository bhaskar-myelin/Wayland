cmake_minimum_required(VERSION 3.10)
project(dev C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags
add_compile_options(-Wall -Wextra -g)

# Find required packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED
    wayland-client
    wayland-cursor
    cairo
)

# Create executable target
add_executable(${PROJECT_NAME}
    dev.c
    xdg-shell-client-protocol.c
)

# Include directories for Wayland and the current directory (for config.h)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${WAYLAND_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}  # This adds the current directory to the include path
)

# Link libraries for Wayland and custom so 
target_link_libraries(
    ${PROJECT_NAME} PRIVATE 
    ${WAYLAND_LIBRARIES}
    ${WAYLAND_CURSORS_LIBRARIES} m
    ${CMAKE_SOURCE_DIR}/libhelloWorld.so)

# Link flags for Wayland
target_link_options(${PROJECT_NAME} PRIVATE ${WAYLAND_LDFLAGS})


