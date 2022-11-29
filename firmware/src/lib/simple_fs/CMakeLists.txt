add_library(simple_fs STATIC
    simple_fs.cpp
)

target_include_directories(simple_fs PUBLIC ./)

target_link_libraries(simple_fs PUBLIC common)
# target_link_libraries(simple_fs PUBLIC project_config)
