zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_KEY_TEMPO src/behavior_key_tempo.c)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
