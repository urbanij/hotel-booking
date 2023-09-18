cmake_minimum_required(VERSION 3.6)

# set the project name
set(PROJECT_NAME hotel-booking)
set(TARGET_CLIENT client)
set(TARGET_SERVER server)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)


# Local source files here
set(TARGET_SRC_SER
    src/server.c
)

set(TARGET_SRC_CLI
    src/client.c
)

# add the executable
add_executable(${TARGET_CLIENT} ${TARGET_SRC_CLI})
add_executable(${TARGET_SERVER} ${TARGET_SRC_SER})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(${TARGET_CLIENT} pthread)
target_link_libraries(${TARGET_SERVER} sqlite3)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${TARGET_SERVER} pthread)    
    target_link_libraries(${TARGET_SERVER} crypt)    
endif()
