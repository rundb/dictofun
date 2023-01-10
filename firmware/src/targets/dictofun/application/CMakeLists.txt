add_library(application STATIC
    assert.c
)

target_include_directories(application PUBLIC ./)

target_link_libraries(application PUBLIC tasks)

target_link_libraries(application PUBLIC 
    boards
)

add_subdirectory(audio)
