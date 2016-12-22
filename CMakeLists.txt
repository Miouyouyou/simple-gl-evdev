cmake_minimum_required(VERSION 2.8)

project(MyyMakeTests)

set (MyyMakeTests_VERSION_MAJOR 1)
set (MyyMakeTests_VERSION_MINOR 0)

set (MyyCoreFiles myy.c myy/helpers/base_gl.c)
set (MyyPlatformsDir myy/platforms)

include_directories(. myy)

option(MYY_X11 "Compile the X11 version of this project" OFF)
option(MYY_ANDROID "Compile the Android version of this project" OFF)
option(MYY_DEBUG "Activate debug messages" ON)

if (MYY_DEBUG)
	add_definitions(-DDEBUG)
endif (MYY_DEBUG)

if (MYY_X11)
	file(COPY shaders DESTINATION .)
	file(COPY textures DESTINATION .)

	file(GLOB_RECURSE MyyX11Sources myy/platforms/PC_X11/*.c)
	add_executable(Program ${MyyCoreFiles} ${MyyX11Sources})
	target_link_libraries(Program GLESv2 EGL X11)
endif (MYY_X11)

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
	file(COPY shaders DESTINATION ${first-android-assets-dir})
	file(COPY textures DESTINATION ${first-android-assets-dir})

	# message(STATUS "FIRST DIR : ${first-android-assets-dir}")
	# message(STATUS "ANDROID ASSETS DIRECTORY : ${ANDROID_ASSETS_DIRECTORIES}")

	file(GLOB_RECURSE MyyAndroidSources myy/platforms/android/*.c)
	find_library(android-lib-log log)
	find_library(android-lib-GLESv2 GLESv2)
	find_library(android-lib-EGL EGL)
	find_library(android-lib-android android)
	add_library(main SHARED ${MyyCoreFiles} ${MyyAndroidSources})
	target_link_libraries(main ${android-lib-log} ${android-lib-GLESv2} ${android-lib-EGL} ${android-lib-android})

endif (MYY_ANDROID)
