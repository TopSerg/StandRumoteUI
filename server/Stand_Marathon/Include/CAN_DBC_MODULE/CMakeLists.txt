cmake_minimum_required(VERSION 3.16)

# Автопоиск всех исходников (.c и .cpp) рекурсивно
file(GLOB_RECURSE CAN_DBC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

include_directories(DBC_MSG)

# Создаём статическую библиотеку
add_library(CAN_DBC_MODULE STATIC ${CAN_DBC_SOURCES})

# Инклюды (рекурсивно, чтобы все подпапки подключились)
target_include_directories(CAN_DBC_MODULE PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/DBC_MSG
    ${CMAKE_CURRENT_SOURCE_DIR}/DBC_MSG/build
)
