add_executable(dictofun_main main.cpp)

add_subdirectory(application)
add_subdirectory(tasks)

target_link_libraries(dictofun_main PRIVATE 
    application
    tasks)
