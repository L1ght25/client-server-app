add_executable(client client.c helpers.c)

target_link_libraries(client PRIVATE
    network
    pthread
)
