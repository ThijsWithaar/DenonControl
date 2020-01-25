cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(Denon
	VERSION 0.2.1
	DESCRIPTION "Denon Audio/Video-Receiver control"
)

# Allow grouping of projects in visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND UNIX)
	add_compile_options(-fdata-sections -ffunction-sections)
	add_link_options(-Wl,--gc-sections -Wl,--strip-all)
endif()
if(WIN32)
	add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10
endif()

set(CPACK_PACKAGE_VENDOR "Withaar")
set(CPACK_PACKAGE_CONTACT "Thijs Withaar <thijs.withaar@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI for controlling denon A/V receivers")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_EXECUTABLE "denonUi;Denon")
set(CPACK_STRIP_FILES TRUE)

set(CPACK_NSIS_MENU_LINKS "bin/denonUi.exe" "Denon")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5, libboost-system1.67.0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_ARCH_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_ARCH_PACKAGE_LICENSE_NAME "GPLv2")
set(CPACK_ARCH_PACKAGE_DEPENDS "boost-libs" "libc")
set(CPACK_ARCH_EPOCH 42)


if(UNIX)
	execute_process(
		COMMAND lsb_release -si
		OUTPUT_VARIABLE LSB_RELEASE_DISTRIBUTOR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(EXISTS /etc/arch-release)
		set(CPACK_GENERATOR "Arch" CACHE STRING "Package format")
		set(CPACK_SYSTEM_NAME "Arch-amd64")
	endif()
	if(EXISTS /etc/fedora-release)
		set(CPACK_GENERATOR "RPM" CACHE STRING "Package format")
		set(CPACK_SYSTEM_NAME "Fedora-amd64")
	endif()
	if(LSB_RELEASE_DISTRIBUTOR STREQUAL "openSUSE")
		set(CPACK_GENERATOR "RPM" CACHE STRING "Package format")
		set(CPACK_SYSTEM_NAME "Suse-amd64")
	endif()
	if(EXISTS /etc/debian_version)
		set(CPACK_GENERATOR "DEB" CACHE STRING "Package format")

		execute_process(COMMAND dpkg --print-architecture
			OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		set(CPACK_SYSTEM_NAME "${LSB_RELEASE_DISTRIBUTOR}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
	endif()
endif()


add_subdirectory(lib)
add_subdirectory(ssdpBridge)
add_subdirectory(qtui)

enable_testing()
add_subdirectory(test)

include(CPack)
cpack_add_component(GUI DISPLAY_NAME "Graphical User Interface")
cpack_add_component(DEV DISPLAY_NAME "Development" DISABLED)
cpack_add_component(SERVER DISPLAY_NAME "Server tools" DISABLED)
