cmake_minimum_required (VERSION 3.1)

project (LogView C)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)


add_executable(LogView src/main.c src/log.c src/plot.c src/wind.c src/map.c src/polar.c src/timeline.c)
target_include_directories(LogView PRIVATE ${GTK_INCLUDE_DIRS})
target_link_libraries(LogView PRIVATE m ${GTK_LIBRARIES} lapacke)
set_property(TARGET LogView PROPERTY C_STANDARD 99)
