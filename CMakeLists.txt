cmake_minimum_required(VERSION 2.8.4)
include(ExternalProject)

project(ProjectFriendship)

set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY ON)

option(RN_BUILD_BULLET_MODULE "Build the Bullet physics module" OFF)
option(RN_BUILD_PHYSX_MODULE "Build the PhysX physics module" ON)
option(RN_BUILD_ODE_MODULE "Build the ODE physics module" OFF)
option(RN_BUILD_SPLASH_MODULE "Build the Splash physics module" OFF)
option(RN_BUILD_ASSIMP_MODULE "Build the Assimp module" OFF)
option(RN_BUILD_STEAMAUDIO_MODULE "Build the Steam Audio module" OFF)
option(RN_BUILD_OCULUSAUDIO_MODULE "Build the Oculus Audio module" OFF)
option(RN_BUILD_ENET_MODULE "Build the ENet module" OFF)
option(RN_BUILD_SAMPLES "Enable building the sample projects" OFF)
option(RN_BUILD_RECAST_MODULE "Build the Recast pathfinding module" OFF)

set(RN_BUILD_CONFIGURATION "independent" CACHE STRING "The build configuration.")
set_property(CACHE RN_BUILD_CONFIGURATION PROPERTY STRINGS independent oculus steam)

option(RN_BUILD_OGG_MODULE "Build the ogg audio file module" OFF)
option(RN_BUILD_OPENAL_MODULE "Build the OpenAL module" OFF)
option(RN_BUILD_UI_MODULE "Build the UI module" OFF)
if(WIN32)
	set(CMAKE_SYSTEM_VERSION "10.0.10240.0")

	if(MSVC)
		option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" OFF)
		option(RN_BUILD_D3D12_MODULE "Build the D3D12 renderer module" ON)
		option(RN_BUILD_OCULUS_MODULE "Build the Oculus module" ON)
		option(RN_BUILD_OPENVR_MODULE "Build the OpenVR module" ON)
	else()
		option(RN_BUILD_D3D12_MODULE "Build the D3D12 renderer module" OFF)
		option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" ON)
		option(RN_BUILD_OPENVR_MODULE "Build the OpenVR module" ON)
	endif()
elseif(ANDROID)
	option(RN_BUILD_OCULUSMOBILE_MODULE "Build the Oculus GearVR module" ON)
	option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" ON)
else()
	if(APPLE)
		option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" OFF)
		option(RN_BUILD_OCULUS_MODULE "Build the Oculus module" OFF)
		option(RN_BUILD_OPENVR_MODULE "Build the OpenVR module" ON)
	else()
		option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" ON)
		option(RN_BUILD_OCULUS_MODULE "Build the Oculus module" OFF)
		option(RN_BUILD_OPENVR_MODULE "Build the OpenVR module" ON)
	endif()
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if(APPLE)
	set(CMAKE_INSTALL_RPATH @executable_path;@executable_path/../Resources/Resources/Modules/RaynePhysX;@executable_path/../Resources/Resources/Modules/RayneMetal;@executable_path/../Resources/Resources/Modules/RayneOpenVR;@executable_path/../Resources/Resources/Modules/RayneVR)
elseif(UNIX)
	set(CMAKE_INSTALL_RPATH \$ORIGIN;\$ORIGIN/Resources/Modules/RaynePhysX;\$ORIGIN/Resources/Modules/RayneVulkan;\$ORIGIN/Resources/Modules/RayneOpenVR;\$ORIGIN/Resources/Modules/RayneVR)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne" "${CMAKE_CURRENT_BINARY_DIR}/Rayne")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne/CMake/Rayne.cmake)

set(SOURCES
	Sources/main.cpp
	Sources/PFApplication.cpp
	Sources/PFCameraManager.cpp
	Sources/PFWorld.cpp
	Sources/PFTypes.cpp
	Sources/PFPlayer.cpp
	Sources/PFGoldfish.cpp)

set(HEADERS
	Sources/PFApplication.h
	Sources/PFCameraManager.h
	Sources/PFWorld.h
	Sources/PFTypes.h
	Sources/PFPlayer.h
	Sources/PFGoldfish.h)

set(RESOURCES
	manifest.json
	Resources)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(ANDROID)
    add_library(ProjectFriendship SHARED "${SOURCES}" "${HEADERS}")
else()
    add_executable(ProjectFriendship "${SOURCES}" "${HEADERS}")
endif()

target_include_directories(ProjectFriendship PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Sources)

set(ADDITIONAL_RESOURCE_COPY_PARAMS "")
rayne_copy_resources(ProjectFriendship "${RESOURCES}" "${ADDITIONAL_RESOURCE_COPY_PARAMS}")

rayne_link_with(ProjectFriendship)
rayne_use_modules(ProjectFriendship RaynePhysX)
rayne_use_modules(ProjectFriendship RayneVR)

if(ANDROID)
    rayne_use_modules(ProjectFriendship RayneVulkan)
    rayne_use_modules(ProjectFriendship RayneOculusMobile)

	if(${RN_BUILD_CONFIGURATION} STREQUAL "oculus")
        set(PF_COMPILE_DEFINITIONS ${PF_COMPILE_DEFINITIONS} -DBUILD_FOR_OCULUS)
    endif()
elseif(UNIX AND NOT APPLE)
	rayne_use_modules(ProjectFriendship RayneVulkan)
	rayne_use_modules(ProjectFriendship RayneOpenVR)

	if(${RN_BUILD_CONFIGURATION} STREQUAL "steam")
	    set(PF_COMPILE_DEFINITIONS ${PF_COMPILE_DEFINITIONS} -DBUILD_FOR_STEAM)
	endif()
else()
	if(NOT APPLE)
			if(${RN_BUILD_VULKAN_MODULE})
				rayne_use_modules(ProjectFriendship RayneVulkan)
			endif()

			if(WIN32 AND ${RN_BUILD_D3D12_MODULE})
				rayne_use_modules(ProjectFriendship RayneD3D12)
				rayne_use_modules(ProjectFriendship RayneOculus)
			endif()
	else()
		if(${RN_BUILD_METAL_MODULE})
	    	rayne_use_modules(ProjectFriendship RayneMetal)
	    endif()
	endif()

	if(${RN_BUILD_CONFIGURATION} STREQUAL "steam")
	    rayne_use_modules(ProjectFriendship RayneOpenVR)
	    set(PF_COMPILE_DEFINITIONS ${PF_COMPILE_DEFINITIONS} -DBUILD_FOR_STEAM)
	elseif(${RN_BUILD_CONFIGURATION} STREQUAL "oculus")
	    set(PF_COMPILE_DEFINITIONS ${PF_COMPILE_DEFINITIONS} -DBUILD_FOR_OCULUS)
	else()
		rayne_use_modules(ProjectFriendship RayneOpenVR)

	    if(NOT APPLE)
		    add_custom_command(TARGET ProjectFriendship POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/RunPancake-windows.bat $<TARGET_FILE_DIR:ProjectFriendship>/RunPancake-windows.bat)
		    add_custom_command(TARGET ProjectFriendship POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/.vrmanifest $<TARGET_FILE_DIR:ProjectFriendship>/.vrmanifest)
		else()
			add_custom_command(TARGET ProjectFriendship POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/RunWithPreview-macOS.sh $<TARGET_BUNDLE_CONTENT_DIR:ProjectFriendship>/../../RunWithPreview-macOS.sh)
		    add_custom_command(TARGET ProjectFriendship POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/RunPancake-macOS.sh $<TARGET_BUNDLE_CONTENT_DIR:ProjectFriendship>/../../RunPancake-macOS.sh)
		    add_custom_command(TARGET ProjectFriendship POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/.vrmanifest $<TARGET_BUNDLE_CONTENT_DIR:ProjectFriendship>/Resources/.vrmanifest)
		endif()
	endif()
endif()

target_compile_definitions(ProjectFriendship PRIVATE ${PF_COMPILE_DEFINITIONS})

if(APPLE)
	set_target_properties(ProjectFriendship PROPERTIES MACOSX_BUNDLE TRUE)
	set_target_properties(ProjectFriendship PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Other/Info.plist.in)
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ProjectFriendship)
endif()
