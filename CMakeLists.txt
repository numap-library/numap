cmake_minimum_required (VERSION 2.6)
INCLUDE (CheckLibraryExists)
find_package(PkgConfig)


project (numap)
set (NUMAP_VERSION_MAJOR 0)
set (NUMAP_VERSION_MINOR 1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

SET(PKG_CONFIG_LIBDIR  "\${prefix}/lib"  )
SET(PKG_CONFIG_INCLUDEDIR  "\${prefix}/include"  )
SET(PKG_CONFIG_LIBS  "-L\${libdir} -lnumap"  )
SET(PKG_CONFIG_CFLAGS  "-I\${includedir}"  )

find_package (pfm)
if (PFM_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${PFM_INCLUDE_DIR}")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  ${PFM_LIBRARIES}")
  set(PKG_CONFIG_CFLAGS "${PKG_CONFIG_CFLAGS} -I${PFM_INCLUDE_DIR}")
  set(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} ${PFM_LIBRARIES}")
else (PFM_FOUND)
  message(FATAL_ERROR "libpfm not found")
endif (PFM_FOUND)

find_package (numactl)
if (NUMACTL_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${NUMACTL_INCLUDE_DIRS}")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  ${NUMACTL_LIBRARIES}")
  set(PKG_CONFIG_CFLAGS "${PKG_CONFIG_CFLAGS} -I${NUMACTL_INCLUDE_DIRS}")
  set(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} ${NUMACTL_LIBRARIES}")
else (NUMACTL_FOUND)
  message(FATAL_ERROR "numactl not found")
endif (NUMACTL_FOUND)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/numap_config.h.in"
  "${PROJECT_BINARY_DIR}/include/numap_config.h"
  )


# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}/include")

# add the sub directories
include_directories("${PROJECT_SOURCE_DIR}/include")
add_subdirectory ("src")
add_subdirectory ("examples")

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")


CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  )

install(TARGETS DESTINATION bin)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
          DESTINATION lib/pkgconfig)
