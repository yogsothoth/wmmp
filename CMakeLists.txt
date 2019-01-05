cmake_minimum_required(VERSION 2.8)
project(WMmp)

set(WMmp_SRCS   src/dockapp.c
		src/libmpdclient.c
		src/mouse_regions.c
		src/mpd_func.c
		src/wmmp.c)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(WMmp ${WMmp_SRCS})

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(WMmp ${X11_LIBRARIES} ${X11_Xpm_LIB})

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/doc/WMmp.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/WMmp DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
