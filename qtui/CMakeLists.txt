
project(denonUi VERSION 0.2)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Network REQUIRED)

file(GLOB HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB SOURCES CONFIGURE_DEPENDS *.cc)
file(GLOB QTUI CONFIGURE_DEPENDS *.ui)

configure_file(version.h.in denonVersion.h)
list(APPEND HEADERS denonVersion.h)

if(ANDROID)
	# https://gitlab.com/rpdev/qt-cmake-android
	add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES} ${QTUI})

	qt_android_build_apk(TARGET ${PROJECT_NAME}
		PACKAGE_NAME net.withaar.denon
		QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}
	)
else()
	add_executable(${PROJECT_NAME} WIN32 ${HEADERS} ${SOURCES} ${QTUI})
endif()

# Create the icons
file(GLOB SVGS CONFIGURE_DEPENDS *.svg)

#find_program (INKSCAPE NAMES inkscape PATHS "/usr/bin" NO_DEFAULT_PATH)
find_program (MAGICK NAMES convert-im6 magick convert PATHS "/usr/bin" "C:/Program Files/ImageMagick-7.0.9-Q16")
foreach(SVG ${SVGS})
	get_filename_component(SVG_WE ${SVG} NAME_WE)
	set(PNG_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.png)
	set(ICO_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.ico)

	add_custom_command(OUTPUT ${PNG_OUT}
		#COMMAND ${INKSCAPE} -z ${SVG} -w 64 -h 64 -e ${PNG_OUT}
		COMMAND ${MAGICK} -background none ${SVG} -resize 32x32 ${PNG_OUT}
		COMMENT Generating ${PNG_OUT}
		DEPENDS ${SVG}
	)

	add_custom_command(OUTPUT ${ICO_OUT}
		COMMAND ${MAGICK} -background transparent ${PNG_OUT} -define icon:auto-resize=16,32,48,64 ${ICO_OUT}
		COMMENT Generating ${ICO_OUT}
		DEPENDS ${PNG_OUT}
	)

	add_custom_target(${NAME_WE}_gen DEPENDS ${PNG_OUT} ${ICO_OUT})
	configure_file(${SVG} ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.svg COPYONLY)
	add_dependencies(${PROJECT_NAME} ${NAME_WE}_gen)
endforeach()
# Copy the .qrc, to allow local paths there
configure_file(main.qrc ${CMAKE_CURRENT_BINARY_DIR}/main.qrc COPYONLY)
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/main.qrc)
if(WIN32)
	configure_file(denon.rc ${CMAKE_CURRENT_BINARY_DIR}/denon.rc COPYONLY)
	target_sources(${PROJECT_NAME} PRIVATE denon.rc)
endif()


target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}	# for version.h
)

target_link_libraries(${PROJECT_NAME}
	Denon
	Qt5::Widgets
	Qt5::Network
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT gui)

# Copy the .dlls from the vcpkg build as well
if(WIN32)
	install(
		DIRECTORY ${PROJECT_BINARY_DIR}/MinSizeRel/
		DESTINATION bin
		COMPONENT gui
		FILES_MATCHING PATTERN "*.dll" PATTERN "*.conf"
	)
endif()

if(UNIX)
	# Desktop shortcut and icon
	# https://stackoverflow.com/questions/14235798/specify-an-icon-and-menu-entry-for-cmake-cpack-application
	set(DESKTOP_DIR "${CMAKE_INSTALL_PREFIX}/share/applications")
	set(PIXMAPS_DIR "${CMAKE_INSTALL_PREFIX}/share/icons")
	install(FILES denon.desktop DESTINATION ${DESKTOP_DIR} COMPONENT gui)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/denon.png DESTINATION ${PIXMAPS_DIR} COMPONENT gui)

	# Installer: Add to start-menu
	if (CPACK_DEB_COMPONENT_INSTALL)
		set(CPACK_DEBIAN_GUI_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)
		file(GLOB CONTROL_EXTRA CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/deb/*)
		set(CPACK_DEBIAN_GUI_PACKAGE_CONTROL_EXTRA ${CONTROL_EXTRA} PARENT_SCOPE)
		set(CPACK_DEBIAN_GUI_PACKAGE_CONTROL_STRICT_PERMISSION TRUE PARENT_SCOPE)
	endif()
endif()
