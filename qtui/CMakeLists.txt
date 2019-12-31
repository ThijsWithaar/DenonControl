
project(denonUi VERSION 0.2)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Network REQUIRED)

file(GLOB HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB SOURCES CONFIGURE_DEPENDS *.cc)
file(GLOB QTUI CONFIGURE_DEPENDS *.ui)
file(GLOB QTRC CONFIGURE_DEPENDS *.qrc)

configure_file(version.h.in denonVersion.h)
list(APPEND HEADERS denonVersion.h)

if(ANDROID)
	# https://gitlab.com/rpdev/qt-cmake-android
	add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES} ${QTUI} ${QTRC})

	qt_android_build_apk(TARGET ${PROJECT_NAME}
		PACKAGE_NAME net.withaar.denon
		QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}
	)
else()
	add_executable(${PROJECT_NAME} WIN32 ${HEADERS} ${SOURCES} ${QTUI} ${QTRC})
	if(WIN32)
		target_sources(${PROJECT_NAME} PRIVATE denon.rc)
	endif()
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
	# Generate the icons
	#execute_process(COMMAND convert -background none denon.svg -define icon:auto-resize denon.ico OUTPUT_FILE denon.ico)
	#execute_process(COMMAND convert -background none denon.svg -resize 32x32 denon.png OUTPUT_FILE denon.png)
endif()