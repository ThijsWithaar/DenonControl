cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(Denon
	VERSION 0.2
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
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_EXECUTABLE "denonUi;Denon")
set(CPACK_NSIS_MENU_LINKS "bin/denonUi.exe" "Denon")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thijs Withaar <thijs.withaar@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5, libboost-system1.67.0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_STRIP_FILES TRUE)


if(UNIX)
	execute_process(
		COMMAND lsb_release -si
		OUTPUT_VARIABLE LSB_RELEASE_DISTRIBUTOR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(EXISTS /etc/arch-release)
		set(CPACK_GENERATOR "RPM")
		set(CPACK_SYSTEM_NAME "Arch-amd64")
	endif()
	if(EXISTS /etc/fedora-release)
		set(CPACK_GENERATOR "RPM")
		set(CPACK_SYSTEM_NAME "Fedora-amd64")
	endif()
	if(EXISTS /etc/debian_version)
		set(CPACK_GENERATOR "DEB")

		execute_process(COMMAND dpkg --print-architecture
			OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${LSB_RELEASE_DISTRIBUTOR}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
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
