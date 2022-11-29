add_executable(dictofun_main main.cpp)

add_subdirectory(tasks)
add_subdirectory(config)

nrf5_target(dictofun_main)

target_link_libraries(dictofun_main PRIVATE 
    tasks
)
