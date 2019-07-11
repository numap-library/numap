set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${NUMACTL_LIB_DIR} -L${PFM_LIB_DIR}")

add_executable (example example.c)
target_link_libraries (example numap pthread)
set_target_properties(example PROPERTIES COMPILE_FLAGS "-D_REENTRANT -DLinux -D_GNU_SOURCE")

add_executable (example2 example2.c)
target_link_libraries (example2 numap pthread)
set_target_properties(example2 PROPERTIES COMPILE_FLAGS "-D_REENTRANT -DLinux -D_GNU_SOURCE")

add_executable (showevtinfo showevtinfo.c)
target_link_libraries (showevtinfo numap pthread)

