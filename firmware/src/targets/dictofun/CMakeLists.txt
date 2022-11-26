add_executable(dictofun_main main.cpp)

add_subdirectory(application)

target_link_libraries(dictofun_main PRIVATE application)
