cmake_minimum_required(VERSION 3.20)

project(testing)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(BpfObject REQUIRED)

# Create an executable for each application
file(GLOB apps *.bpf.c)
foreach(app ${apps})
  get_filename_component(app_stem ${app} NAME_WE)

  # Build object skeleton and depend skeleton on libbpf build
  bpf_object(${app_stem} ${app_stem}.bpf.c)
  add_dependencies(${app_stem}_skel libbpf-build bpftool-build)

  add_library(${app_stem} ${app_stem}.c)
	target_link_libraries(${app_stem} ${app_stem}_skel)
	set_target_properties(${app_stem} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endforeach()

