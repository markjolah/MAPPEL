# Mappel - src/examples/CMakeLists.txt
# Example c++ Mapple programs
#

file(GLOB EXAMPLES_SRCS mappel_*.cpp)
foreach(_src IN LISTS EXAMPLES_SRCS)
    get_filename_component(_tgt ${_src} NAME_WE)
    add_executable(${_tgt} ${_src})
    target_link_libraries(${_tgt} Mappel::Mappel)
endforeach()
