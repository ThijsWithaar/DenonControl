cmake_minimum_required(VERSION 3.10)

project(Denon
	VERSION 0.2
	DESCRIPTION "Denon Audio/Video Receiver control"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_VENDOR "Withaar")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thijs Withaar <thijs.withaar@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5, libboost-system1.67.0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

add_subdirectory(lib)
add_subdirectory(ssdpBridge)
add_subdirectory(qtui)
add_subdirectory(test)

if(UNIX)
	execute_process(
		COMMAND lsb_release -si
		OUTPUT_VARIABLE LSB_RELEASE_DISTRIBUTOR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(EXISTS /etc/debian_version)
		execute_process(COMMAND dpkg --print-architecture
			OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${LSB_RELEASE_DISTRIBUTOR}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
	endif()

	# Register in the start menu
	# https://stackoverflow.com/questions/14235798/specify-an-icon-and-menu-entry-for-cmake-cpack-application
	set(DESKTOP_DIR "/usr/share/applications")
	set(PIXMAPS_DIR "/usr/share/icons")
	install(FILES qtui/denon.desktop DESTINATION ${DESKTOP_DIR})
	install(FILES qtui/denon.png DESTINATION ${PIXMAPS_DIR})

	#find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
	#install(CODE "execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${DESKTOP_DIR}/denon.desktop)")

	# start-menu entry
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
		"${CMAKE_CURRENT_SOURCE_DIR}/qtui/postinst"
		"${CMAKE_CURRENT_SOURCE_DIR}/qtui/prerm"
	)
endif()


include(CPack)
