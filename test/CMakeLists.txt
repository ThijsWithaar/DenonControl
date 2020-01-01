cmake_minimum_required(VERSION 3.10)

project(Denon.test)

find_package(Catch2 REQUIRED)

file(GLOB HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB SOURCES CONFIGURE_DEPENDS *.cc)
file(GLOB DUMPS CONFIGURE_DEPENDS *.dump)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Convert the network dumps to headers
set(BIN2C ${CMAKE_SOURCE_DIR}/cmake/bin2c.cmake)
foreach(DUMP ${DUMPS})
	get_filename_component(DUMP_WE ${DUMP} NAME_WE)
	set(DUMP_OUT ${CMAKE_CURRENT_BINARY_DIR}/${DUMP_WE}.dump.h)

	add_custom_command(OUTPUT ${DUMP_OUT}
		COMMAND ${CMAKE_COMMAND} -DINFILE=${DUMP} -DOUTFILE=${DUMP_OUT} -DVARNAME="g_${DUMP_WE}" -P ${BIN2C}
		DEPENDS ${DUMP}
	)
	target_sources(${PROJECT_NAME} PRIVATE ${DUMP_OUT})
endforeach()


target_link_libraries(${PROJECT_NAME}
	Denon
	Catch2::Catch2
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
