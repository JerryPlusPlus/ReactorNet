add_executable(reactornet_echo_server echo_server.cc)
target_link_libraries(reactornet_echo_server PRIVATE ReactorNet::net)
add_executable(reactornet_bench_client tcp_bench.cc)
target_link_libraries(reactornet_bench_client PRIVATE Threads::Threads)
target_compile_options(reactornet_bench_client PRIVATE -Wall -Wextra -Wpedantic)
if(BUILD_TESTING)
  foreach(backend epoll poll)
    add_test(NAME tcp_${backend}
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/network_test.py
              $<TARGET_FILE:reactornet_echo_server> $<TARGET_FILE:reactornet_bench_client> ${backend}
              $<TARGET_FILE:reactornet_tcp_client_test>)
    set_tests_properties(tcp_${backend} PROPERTIES TIMEOUT 45)
  endforeach()
endif()
