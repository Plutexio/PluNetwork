file(GLOB PLUNETWORK_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(PluNetworkTests ${PLUNETWORK_TEST_SOURCES})

# The runner is PluSTL's TestFramework.h, a header with no dependencies of its own.
target_include_directories(PluNetworkTests PRIVATE "${PROJECT_SOURCE_DIR}/PluSTL/tests")
target_link_libraries(PluNetworkTests PRIVATE PluNetwork::PluNetwork)
plunetwork_copy_runtime_dlls(PluNetworkTests)

add_test(NAME PluNetworkTests COMMAND PluNetworkTests)
