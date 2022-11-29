add_executable(dictofun_main main.cpp)

add_subdirectory(tasks)
add_subdirectory(config)
add_subdirectory(application)

nrf5_target(dictofun_main)

target_link_libraries(dictofun_main PRIVATE 
    tasks
    spi_flash
    boards
)
