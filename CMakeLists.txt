cmake_minimum_required(VERSION 2.8)

# User defined
project(SimpleGLEvdev)

# -- Modifying these options names require modifying the Common Section
# -- You can modify their descriptions and default values, though.
option(MYY_DEBUG "Activate debug messages" ON)

set(MyyProjectSources
    src/main.c
    src/drm.c
    src/evdev.c
    src/myy.c
    src/helpers/file.c
    src/helpers/gl_loaders.c
    )

if (MYY_DEBUG)
	add_definitions(-DDEBUG)
endif (MYY_DEBUG)

file(COPY shaders textures DESTINATION .)

find_package(PkgConfig REQUIRED)
pkg_search_module(DRM REQUIRED libdrm)
pkg_search_module(GBM REQUIRED gbm)
pkg_search_module(EVDEV REQUIRED libevdev)
include_directories(${DRM_INCLUDE_DIRS}
                    ${GBM_INCLUDE_DIRS}
                    ${EVDEV_INCLUDE_DIRS}
                    src/)
add_executable(Program
              ${MyyProjectSources})
target_link_libraries(Program
                      GLESv2
                      EGL
                      ${DRM_LIBRARIES}
                      ${GBM_LIBRARIES}
                      ${EVDEV_LIBRARIES})


