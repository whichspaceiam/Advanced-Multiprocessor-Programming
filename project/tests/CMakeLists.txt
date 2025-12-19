

add_executable(test_timer test_timer.cpp)
target_include_directories(test_timer PRIVATE project_includes)
# target_link_libraries((test_timer PRIVATE OpenMP::OpenMP_CXX))
add_test(NAME timer COMMAND test_timer)


add_executable(freelist test_freelist.cpp)
target_include_directories(freelist PRIVATE project_includes)
# target_link_libraries((freelist PRIVATE OpenMP::OpenMP_CXX))
add_test(NAME freelist_sequential COMMAND freelist)

add_executable(queue_seq test_queue_seq.cpp)
target_include_directories(queue_seq PRIVATE project_includes)
# target_link_libraries((queue_seq PRIVATE OpenMP::OpenMP_CXX))
add_test(NAME queue_sequential COMMAND queue_seq)


add_executable(global_lock test_global_lock.cpp)
target_include_directories(global_lock PRIVATE project_includes)
target_link_libraries(global_lock PRIVATE OpenMP::OpenMP_CXX)
add_test(NAME GlobalLock COMMAND global_lock)


# add_executable(tagged_ptr tagged_ptr.cpp)
# target_include_directories(tagged_ptr PRIVATE project_includes)
# target_link_libraries(tagged_ptr PRIVATE OpenMP::OpenMP_CXX)
# add_test(NAME TaggedPointer COMMAND tagged_ptr)