cmake_minimum_required(VERSION 2.8.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(geh_SOURCES
  dir.c
  file_fetch.c
  file_fetch_img.c
  file_multi.c
  file_queue.c
  image.c
  md5.c
  orientation.c
  thumb.c
  ui_window.c
  util.c
  main.c)

set(geh_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
set(geh_LIBRARIES ${GTK_LIBRARIES})

add_executable(geh ${geh_SOURCES})
add_definitions(-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE)
target_include_directories(geh PUBLIC ${geh_INCLUDE_DIRS})
target_link_libraries(geh ${geh_LIBRARIES})

install(TARGETS geh DESTINATION bin)
