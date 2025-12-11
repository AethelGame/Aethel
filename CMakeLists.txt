cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0054 NEW)
if(WIN32)
    string(REPLACE "\\" "/" CMAKE_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

message(STATUS "Configuring Aethel (C++20)")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

option(USE_VENDORED "Use vendored libraries" ON)
option(USE_CONSOLE "Use console application" OFF)

project(Aethel)
add_executable(Aethel ${SOURCES})

if(USE_VENDORED)
    # FIX: Tell GLFW not to include GL headers, preventing glad.h conflict
    target_compile_definitions(Aethel PRIVATE GLFW_INCLUDE_NONE)
endif()

if(MSVC)
    target_compile_options(Aethel PRIVATE /EHa)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    target_compile_options(Aethel PRIVATE "-fsanitize=undefined" "-fsanitize-trap")
    target_link_libraries(Aethel PRIVATE -l:libstdc++.a)
endif()

target_include_directories(Aethel PRIVATE vendored/include)

message(STATUS "Defining BASS (vendored/BASS) as SHARED IMPORTED target.")
add_library(BASS SHARED IMPORTED)

message(STATUS "Adding GLFW from vendored/")
add_subdirectory(vendored/glfw EXCLUDE_FROM_ALL)

message(STATUS "Adding GLM from vendored/")
add_subdirectory(vendored/glm EXCLUDE_FROM_ALL)

message(STATUS "Adding FreeType 
from vendored/")
add_subdirectory(vendored/freetype EXCLUDE_FROM_ALL)

file(GLOB GLAD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/vendored/glad/src/*.c"
)

if(WIN32)
    list(REMOVE_ITEM GLAD_SOURCES 
       "${CMAKE_CURRENT_SOURCE_DIR}/vendored/glad/src/glx.c"
    )
    message(STATUS "Removed UNIX-specific sources (glx.c) from GLAD build on WIN32.")
endif()

message(STATUS "Defining GLAD from vendored/glad (STATIC library).")
add_library(glad STATIC ${GLAD_SOURCES})

target_include_directories(glad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendored/glad/include
)

set_target_properties(glad PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
)

find_package(OpenGL REQUIRED)
message(STATUS "Found OpenGL: ${OPENGL_LIBRARIES}")

if(WIN32)
    # FIX: Use bass.lib for linking in both Debug and Release configs
    set_property(TARGET BASS PROPERTY IMPORTED_IMPLIB_DEBUG
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bass.lib"
    )
    set_property(TARGET BASS PROPERTY IMPORTED_IMPLIB_RELEASE
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bass.lib"
    )
    
    # The DLL names for runtime copying remain different, which is correct
    set_property(TARGET BASS PROPERTY IMPORTED_LOCATION_DEBUG
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bassd.dll"
    )
    set_property(TARGET BASS PROPERTY IMPORTED_LOCATION_RELEASE
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bass.dll"
    )

    # FIX: Copy the BASS import library during configuration (PRE_LINK was unreliable)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/vendored/BASS/c/bass.lib"
     DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    )

elseif(UNIX)
    set_property(TARGET BASS PROPERTY IMPORTED_LOCATION
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libbass.so"
    )
endif()

message(STATUS "Consolidating all target_include_directories...")
target_include_directories(Aethel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendored/BASS/c
    
    ${CMAKE_CURRENT_SOURCE_DIR}/vendored
    ${CMAKE_CURRENT_SOURCE_DIR}/vendored/glad/include 
)

message(STATUS "Consolidating all target_link_libraries...")
target_link_libraries(Aethel 
PRIVATE
    BASS
    glfw
    glm::glm
    freetype
    OpenGL::GL
    pthread
    glad
)

if(WIN32)
    target_link_libraries(Aethel PRIVATE Dbghelp)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Aethel PRIVATE dl)
endif()

message(STATUS "Configuring POST_BUILD steps (Asset/BASS Runtime Copy)...")
set(ASSETS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets)
set(ASSETS_TARGET_DIR $<TARGET_FILE_DIR:Aethel>/assets)

add_custom_command(TARGET Aethel POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -D _SOURCE=${ASSETS_SOURCE_DIR}
        -D _TARGET=${ASSETS_TARGET_DIR}
        -P
        ${CMAKE_CURRENT_SOURCE_DIR}/makefiles/check_and_copy_assets.cmake
    COMMENT "Conditionally copying assets to build directory."
)

add_custom_command(TARGET Aethel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E sleep 0.2
    COMMENT "Waiting for file system sync"
)

if(WIN32)
    set(BASS_DLL_NAME "$<IF:$<CONFIG:Debug>,bass.dll,bass.dll>")
    set(BASS_DLL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/vendored/BASS/${BASS_DLL_NAME}")
    set(BASS_DLL_DST "$<TARGET_FILE_DIR:Aethel>/${BASS_DLL_NAME}")
    
    add_custom_command(TARGET Aethel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BASS_DLL_SRC}"
            "${BASS_DLL_DST}"
        COMMENT "Copying BASS runtime DLL (Config-dependent)."
    )
elseif(UNIX)
    add_custom_command(TARGET Aethel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libbass.so"
            "$<TARGET_FILE_DIR:Aethel>/libbass.so"
        COMMENT "Copying BASS runtime SO."
    )
endif()

message(STATUS "Target directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Architecture: 64-bit")
else()
    message(STATUS "Architecture: 32-bit")
endif()