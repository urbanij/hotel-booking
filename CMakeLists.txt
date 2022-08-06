cmake_minimum_required(VERSION 3.6)

# set the project name
SET(PROJECT_NAME hotel-booking)
SET(TARGET_CLIENT client)
SET(TARGET_SERVER server)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C)
SET(CMAKE_C_STANDARD 99)

# Local header files here ONLY
SET(TARGET_H
    src/Address.h
    src/Booking.h
    src/config.h
    src/Hotel.h
    src/messages.h
    src/User.h
    src/utils.h
    src/xp_getpass.h
    src/xp_sem.h
)

# Local source files here
SET(TARGET_SRC_SER
    src/server.c
)

SET(TARGET_SRC_CLI
    src/client.c
)

# add the executable
add_executable(${TARGET_CLIENT} ${TARGET_SRC_CLI})
add_executable(${TARGET_SERVER} ${TARGET_SRC_SER})

target_link_libraries(${TARGET_CLIENT} pthread)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${TARGET_SERVER} pthread)    
    target_link_libraries(${TARGET_SERVER} crypt)    
endif()
target_link_libraries(${TARGET_SERVER} sqlite3)
