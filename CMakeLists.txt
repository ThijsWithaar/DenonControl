cmake_minimum_required(VERSION 3.10)

project(Denon
	VERSION 0.2
	DESCRIPTION "Denon Audio/Video Receiver control"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_VENDOR "Withaar")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thijs Withaar <thijs.withaar@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5, libboost-system1.67.0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

add_subdirectory(lib)
add_subdirectory(ssdpBridge)
add_subdirectory(qtui)
add_subdirectory(test)

if(UNIX)
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
