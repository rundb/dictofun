add_library(application_audio INTERFACE)

target_include_directories(application_audio INTERFACE ./)

target_link_libraries(application_audio INTERFACE 
    microphone_pdm
    microphone_interface
    codec_if
    codec_decimator
)
