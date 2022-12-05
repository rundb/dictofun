add_library(project_config INTERFACE
    ./sdk_config.h
)

target_include_directories(project_config INTERFACE ./)
