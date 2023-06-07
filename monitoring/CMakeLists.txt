add_executable(monitoring monitoring.c)

target_link_libraries(monitoring PRIVATE
    tools
)
