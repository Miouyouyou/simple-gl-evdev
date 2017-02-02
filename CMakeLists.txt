cmake_minimum_required(VERSION 2.8)

# User defined
project(OpenGLInterfaces)

# -- Modifying these options names require modifying the Common Section
# -- You can modify their descriptions and default values, though.
option(MYY_X11 "Compile the X11 version of this project" ON)
option(MYY_DRM "Compile the Linux Direct Rendering Manager version of this project" OFF)
option(MYY_ANDROID "Compile the Android version of this project" OFF)
option(MYY_DEBUG "Activate debug messages" ON)

set (MyyMakeTests_VERSION_MAJOR 1)
set (MyyMakeTests_VERSION_MINOR 0)

set(MyyProjectSources
	  src/helpers/opengl/loaders.c
    packed_fonts_parser.c)

set(MyyProjectHeaders
	  src/helpers/opengl/loaders.h
		src/helpers/opengl/quads_structures.h
		src/generated/data_config.h)

set(MyyFoldersToCopy
    shaders
    textures
    data)

if (MYY_ANDROID)
	find_library(Freetype REQUIRED)
else (MYY_ANDROID)
	find_package(Freetype REQUIRED)
endif (MYY_ANDROID)
include_directories(${FREETYPE_INCLUDE_DIRS})
set(MyyProjectLibDeps
    ${FREETYPE_LIBRARIES})

set(MyyShaders
    shaders/standard.frag
    shaders/standard.vert)
set(MyyData
    data/codepoints.dat)
set(MyyTextures
    textures/fonts.raw
    textures/super_bitmap.raw)
set(MyyDocumentation
	  README.md)
add_custom_target(Shaders  SOURCES ${MyyShaders})
add_custom_target(Textures SOURCES ${MyyTextures})
add_custom_target(Data     SOURCES ${MyyData})
add_custom_target(Headers  SOURCES ${MyyProjectHeaders})
add_custom_target(Documentation SOURCES ${MyyDocumentation})

include_directories(. myy)

# Common Section
set (MyyCoreFiles
     myy.c
     myy/helpers/myy_strings.c)
set (MyyLinuxHelpers
     myy/platforms/linux/common/helpers/file.c
     myy/platforms/linux/common/helpers/browser.c)
set (MyyPlatformsDir
     myy/platforms)

set (MyyCoreHeaders
     myy/myy.h
		 myy/current/opengl.h
		 myy/egl_common/myy_eglattrs.h
		 myy/helpers/file.h
		 myy/helpers/log.h
		 myy/helpers/string.h
		 myy/helpers/struct.h)

add_custom_target(MyyHeaders  SOURCES ${MyyCoreHeaders})

if (MYY_DEBUG)
	add_definitions(-DDEBUG)
endif (MYY_DEBUG)

if (MYY_X11 OR MYY_DRM)
	file(COPY ${MyyFoldersToCopy} DESTINATION .)

	if (MYY_X11)

		set(MyyX11Sources
		    myy/platforms/linux/X11/init_window.c
		    myy/platforms/linux/X11/main.c)

		add_executable(Program
		              ${MyyCoreFiles}
		              ${MyyLinuxHelpers}
		              ${MyyX11Sources}
		              ${MyyProjectSources})
		target_link_libraries(Program
		                      GLESv2
		                      EGL
		                      X11
		                      ${MyyProjectLibDeps})
	endif (MYY_X11)

	if (MYY_DRM)
		find_package(PkgConfig REQUIRED)
		pkg_search_module(DRM REQUIRED libdrm)
		pkg_search_module(GBM REQUIRED gbm)
		pkg_search_module(EVDEV REQUIRED libevdev)
		set(MyyDRMSources
		    myy/platforms/linux/DRM/main.c
		    myy/platforms/linux/DRM/evdev.c)
		include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${EVDEV_INCLUDE_DIRS})
		add_executable(Program
		               ${MyyCoreFiles}
		               ${MyyLinuxHelpers}
		               ${MyyDRMSources}
		               ${MyyProjectSources})
		target_link_libraries(Program
		                      GLESv2
		                      EGL
		                      ${DRM_LIBRARIES}
		                      ${GBM_LIBRARIES}
		                      ${EVDEV_LIBRARIES}
		                      ${MyyProjectLibDeps})
	endif (MYY_DRM)
endif (MYY_X11 OR MYY_DRM)




if (MYY_ANDROID)
	set(CMAKE_SYSTEM_NAME Android)

	# The variable ANDROID_ASSETS_DIRECTORIES is not passed nor set
	# automatically...
	# Android will NOT provide this information directly (or at least not
	# in a documented way !), even though IT IS ESSENTIAL ! You have to
	# pass it MANUALLY through the Gradle script file using :
	#
	# cmake {
	#   arguments ...,
	#             "-DANDROID_ASSETS_DIRECTORIES='${sourceSets.main.assets.srcDirs}'"
	# }
	#
	# ♪ "Thank" you Android ♪

	# I'm afraid that this make the Android build system copy the same
	# folder n times, 'n' being the number of targeted architectures API
	# (e.g. : If this is built for 'x86', 'arm7a' and 'mips' -> n = 3)

	list(GET ANDROID_ASSETS_DIRECTORIES 0 first-android-assets-dir)
	file(COPY ${MyyFoldersToCopy} DESTINATION ${first-android-assets-dir})

	# message(STATUS "FIRST DIR : ${first-android-assets-dir}")
	# message(STATUS "ANDROID ASSETS DIRECTORY : ${ANDROID_ASSETS_DIRECTORIES}")

	set(MyyAndroidSources
	    myy/platforms/android/android_dummy_main.c
	    myy/platforms/android/android_native_app_glue.c
	    myy/platforms/android/helpers/android_file.c)
	find_library(android-lib-log     log)
	find_library(android-lib-GLESv2  GLESv2)
	find_library(android-lib-EGL     EGL)
	find_library(android-lib-android android)
	add_library(main
	            SHARED
	            ${MyyCoreFiles}
	            ${MyyAndroidSources}
	            ${MyyProjectSources})
	target_link_libraries(main
	                      ${android-lib-log}
	                      ${android-lib-GLESv2}
	                      ${android-lib-EGL}
	                      ${android-lib-android}
	                      ${MyyProjectLibDeps})

endif (MYY_ANDROID)
