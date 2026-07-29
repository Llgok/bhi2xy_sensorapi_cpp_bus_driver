set(bhi2xy_source_root "${CMAKE_CURRENT_LIST_DIR}/third_party/BHI2xy_SensorAPI")

set(
        bhi2xy_sources
        "${bhi2xy_source_root}/bhy2.c"
        "${bhi2xy_source_root}/bhy2_bsec.c"
        "${bhi2xy_source_root}/bhy2_head_tracker.c"
        "${bhi2xy_source_root}/bhy2_hif.c"
        "${bhi2xy_source_root}/bhy2_klio.c"
        "${bhi2xy_source_root}/bhy2_parse.c"
        "${bhi2xy_source_root}/bhy2_swim.c"
)

idf_component_register(
        SRCS
        "src/bhi2xy.cpp"
        ${bhi2xy_sources}
        INCLUDE_DIRS
        "src"
        "${bhi2xy_source_root}"
        REQUIRES
        "cpp_bus_driver"
)
