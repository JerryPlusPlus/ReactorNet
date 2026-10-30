add_library(reactornet_base
  CurrentThread.cc Date.cc Exception.cc FileUtil.cc ProcessInfo.cc
  Timestamp.cc Thread.cc ThreadPool.cc TimeZone.cc)
add_library(ReactorNet::base ALIAS reactornet_base)
target_include_directories(reactornet_base PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(reactornet_base PUBLIC Threads::Threads)
target_compile_options(reactornet_base PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS reactornet_base ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
