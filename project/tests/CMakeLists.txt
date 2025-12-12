add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE core)
add_test(NAME timer COMMAND test_timer)


add_executable(freelist test_freelist.cpp)
target_link_libraries(freelist PRIVATE core)
add_test(NAME freelist_sequential COMMAND freelist)

add_executable(queue_seq test_queue_seq.cpp)
target_link_libraries(queue_seq PRIVATE core)
add_test(NAME queue_sequential COMMAND queue_seq)


add_executable(global_lock test_global_lock.cpp)
target_link_libraries(global_lock PRIVATE core OpenMP::OpenMP_CXX)
add_test(NAME GlobalLock COMMAND global_lock)


add_executable(tagged_ptr tagged_ptr.cpp)
target_link_libraries(tagged_ptr PRIVATE core OpenMP::OpenMP_CXX)
add_test(NAME TaggedPointer COMMAND tagged_ptr)
