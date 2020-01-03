project(Denon)

find_package(Boost QUIET REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS *.cc)
file(GLOB HDR CONFIGURE_DEPENDS include/*/*.h include/*/*/*.h)

add_library(${PROJECT_NAME}
	${SRC}
	${HDR}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		include
)

target_link_libraries(${PROJECT_NAME}
	Boost::system
	Threads::Threads
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT dev)
install(FILES ${HDR} DESTINATION include/Denon COMPONENT dev)

if (CPACK_DEB_COMPONENT_INSTALL)
	set(CPACK_DEBIAN_DEV_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)
endif()
