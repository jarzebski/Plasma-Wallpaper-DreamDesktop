project(plasma-wallpaper-dreamdesktop)

include(CheckCXXSourceCompiles)

add_definitions(${CMAKE_CXX_FLAGS} "-Wno-sign-compare")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(FFmpeg REQUIRED)
find_package(KDE4 REQUIRED)
find_package(KDE4Workspace REQUIRED)

include(KDE4Defaults)

add_definitions(${KDE4_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${FFmpeg_INCLUDE_DIRS})

set(dreamdesktop_SRCS dreamdesktop.cpp)

kde4_add_ui_files(dreamdesktop_SRCS dreamdesktop.ui)
kde4_add_plugin(dreamdesktop ${dreamdesktop_SRCS})
target_link_libraries(dreamdesktop ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS} ${FFmpeg_LIBRARIES})

install(TARGETS dreamdesktop DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES dreamdesktop.desktop DESTINATION ${SERVICES_INSTALL_DIR})

add_subdirectory(thumbs)
