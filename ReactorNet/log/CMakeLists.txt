add_library(reactornet_log AsyncLogging.cc LogFile.cc Logging.cc LogStream.cc)
add_library(ReactorNet::log ALIAS reactornet_log)
target_link_libraries(reactornet_log PUBLIC ReactorNet::base)
target_compile_options(reactornet_log PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS reactornet_log ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
