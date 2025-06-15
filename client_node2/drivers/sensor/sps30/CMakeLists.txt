zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SPS30 sps30.c)
# zephyr_include_directories(../sensirion_lib)
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/../sensirion_lib)
