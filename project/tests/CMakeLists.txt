

add_executable(test_timer test_timer.cpp)
target_include_directories(test_timer PRIVATE project_includes)
add_test(NAME timer COMMAND test_timer)


add_executable(freelist test_freelist.cpp)
target_include_directories(freelist PRIVATE project_includes)
add_test(NAME freelist_sequential COMMAND freelist)

add_executable(queue_seq test_queue_seq.cpp)
target_include_directories(queue_seq PRIVATE project_includes)
add_test(NAME queue_sequential COMMAND queue_seq)