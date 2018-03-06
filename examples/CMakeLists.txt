add_executable (mem-bdw-top mem-bdw-top.c)
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${NUMACTL_LIB_DIR} -L${PFM_LIB_DIR}")
target_link_libraries (mem-bdw-top numap)

add_executable (example example.c)
target_link_libraries (example numap pthread)
set_target_properties(example PROPERTIES COMPILE_FLAGS "-D_REENTRANT -DLinux -D_GNU_SOURCE")
