
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

add_executable(${PROJECT_NAME}
	${HEADERS}
	${SOURCES}
	${QTUI}
	${QTRC}
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}	# for version.h
)

target_link_libraries(${PROJECT_NAME}
	Denon
	Qt5::Widgets
	Qt5::Network
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
