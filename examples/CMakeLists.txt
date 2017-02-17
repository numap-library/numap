add_executable (mem-bdw-top mem-bdw-top.c)
target_link_libraries (mem-bdw-top LINK_PUBLIC numap)

add_executable (example example.c)
target_link_libraries (example LINK_PUBLIC numap pthread)
set_target_properties(example PROPERTIES COMPILE_FLAGS "-D_REENTRANT -DLinux -D_GNU_SOURCE")
