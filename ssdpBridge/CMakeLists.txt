project(ssdpBridge VERSION 0.2)

find_package(Boost QUIET REQUIRED COMPONENTS program_options serialization)


file(GLOB HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB SOURCES CONFIGURE_DEPENDS *.cc)

#configure_file(version.h.in denonVersion.h)
#list(APPEND HEADERS denonVersion.h)

add_executable(${PROJECT_NAME}
	${HEADERS}
	${SOURCES}
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}	# for version.h
)

target_link_libraries(${PROJECT_NAME}
	Denon
	Boost::program_options
	Boost::serialization
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT server)

if (CPACK_DEB_COMPONENT_INSTALL)
	set(CPACK_DEBIAN_SERVER_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)
	set(CPACK_DEBIAN_SERVER_PACKAGE_CONFLICTS "minissdpd" PARENT_SCOPE)
	file(GLOB CONTROL_EXTRA CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/deb/*)
	set(CPACK_DEBIAN_SERVER_PACKAGE_CONTROL_EXTRA ${CONTROL_EXTRA} PARENT_SCOPE)
	set(CPACK_DEBIAN_SERVER_PACKAGE_CONTROL_STRICT_PERMISSION TRUE PARENT_SCOPE)
endif()

if(UNIX)
	# Directory with systemd unit files
	set (SYSTEMD_UNIT_DIR "lib/systemd/user/")
	install(FILES ssdpBridge.service DESTINATION ${SYSTEMD_UNIT_DIR} COMPONENT server)
endif()
