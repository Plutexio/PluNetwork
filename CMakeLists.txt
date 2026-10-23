cmake_minimum_required(VERSION 4.3)
project(PluNetwork
        VERSION 0.1.0
        DESCRIPTION "Networking on raw OS sockets"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PluSTL and PluBase are git submodules (github.com/Plutexio/PluSTL, github.com/Plutexio/PluBase).
foreach(PLU_SUBMODULE PluSTL PluBase)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PLU_SUBMODULE}/CMakeLists.txt")
        message(FATAL_ERROR "${PLU_SUBMODULE}/ is empty — the submodule is not checked out. Run:\n"
                "  git submodule update --init")
    endif()
endforeach()

# PluBase's own dependencies: an installed package when there is one, otherwise fetched at the
# versions PluEngine vendors. spdlog has to be shared, for the same reason PluBase is: one logger
# registry per process. PluNetwork itself depends on nothing but the OS socket API. A parent project
# that already defines the targets (PluEngine) keeps its own.
include(FetchContent)
if(NOT TARGET spdlog::spdlog)
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
    FetchContent_Declare(spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.16.0
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS CONFIG)
    FetchContent_MakeAvailable(spdlog)
endif()
if(NOT TARGET glm::glm)
    FetchContent_Declare(glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.2
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS CONFIG)
    FetchContent_MakeAvailable(glm)
endif()

if(NOT TARGET PluSTL::PluSTL)
    add_subdirectory(PluSTL)
endif()
if(NOT TARGET PluBase::PluBase)
    add_subdirectory(PluBase)
endif()

option(PLUNETWORK_BUILD_SANDBOX "Build the PluNetwork sandbox executable" ${PROJECT_IS_TOP_LEVEL})

# ─────────────────────────────────────────────────────────────────────────────
#  Library
# ─────────────────────────────────────────────────────────────────────────────
file(GLOB_RECURSE PLUNETWORK_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE PLUNETWORK_HEADERS CONFIGURE_DEPENDS "include/*.h" "src/*.h")

add_library(PluNetwork SHARED ${PLUNETWORK_SOURCES} ${PLUNETWORK_HEADERS}
        include/PluNetwork/TCP/TcpListener.h
        src/TCP/TcpStream.cpp
        src/TCP/TcpListener.cpp)
add_library(PluNetwork::PluNetwork ALIAS PluNetwork)

target_include_directories(PluNetwork
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(PluNetwork PUBLIC PluBase::PluBase)
if(WIN32)
    target_link_libraries(PluNetwork PUBLIC ws2_32)
endif()
target_compile_definitions(PluNetwork PRIVATE PLU_BUILD_PLUNETWORK)
set_target_properties(PluNetwork PROPERTIES
        # Meant to sit next to the executable that loads it, together with PluBase and spdlog.
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
)

# Windows has no RPATH: every DLL an executable needs (PluNetwork, PluBase, spdlog) must sit next
# to it. On Linux the build-tree RPATH CMake sets already finds them where they were built.
function(plunetwork_copy_runtime_dlls TARGET)
    if(WIN32)
        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
                COMMAND_EXPAND_LISTS)
    endif()
endfunction()

if(PLUNETWORK_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()
