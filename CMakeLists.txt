cmake_minimum_required(VERSION 2.8)

project(geh)
set(geh_VERSION_MAJOR 0)
set(geh_VERSION_MINOR 4)
set(geh_VERSION_PATCH 0)
set(geh_VERSION "${geh_VERSION_MAJOR}.${geh_VERSION_MINOR}.${geh_VERSION_PATCH}")

# Look for dependencies
find_package(GTK2)
find_package(PkgConfig)
pkg_check_modules(GTK3 gtk+-3.0)

if (GTK3_FOUND)
  set(GTK_INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
  set(GTK_LIBRARIES ${GTK3_LIBRARIES})
elseif (GTK2_FOUND)
  set(GTK_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS})
  set(GTK_LIBRARIES ${GTK2_LIBRARIES})
else (GTK3_FOUND)
  message(FATAL_ERROR "Gtk2 or Gtk3 required." )
endif (GTK3_FOUND)

# Subdirectories
add_subdirectory(src)
