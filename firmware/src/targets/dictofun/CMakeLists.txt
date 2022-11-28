add_executable(dictofun_main main.cpp)

add_subdirectory(application)
add_subdirectory(tasks)

nrf5_target(dictofun_main)

target_link_libraries(dictofun_main PRIVATE 
    application
    tasks)

target_link_libraries(dictofun_main PRIVATE
    nrf5_nrfx_common
)