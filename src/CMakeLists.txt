add_library(numap SHARED
  numap.c
  numap_analyse.c
  )
target_link_libraries(numap LINK_PUBLIC numa pfm)

configure_file (
  "${PROJECT_SOURCE_DIR}/include/numap_config.h.in"
  "${PROJECT_BINARY_DIR}/include/numap_config.h"
  )

install(TARGETS numap DESTINATION lib)
install (
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")
