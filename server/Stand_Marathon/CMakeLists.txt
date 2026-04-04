cmake_minimum_required(VERSION 3.16)
project(Stand_Marathon)

set(CMAKE_CXX_STANDARD 17)

# --- Подключаем DBC-модуль как библиотеку ---
add_subdirectory(${CMAKE_SOURCE_DIR}/Include/CAN_DBC_MODULE CAN_DBC_BUILD)

# --- Сборка исходников проекта ---
file(GLOB SOURCES "src/*.cpp")

add_executable(Stand_Marathon ${SOURCES})

# --- Подключаем include только для этой цели ---
target_include_directories(Stand_Marathon PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Include
)

# --- Линкуем библиотеки ---
target_link_libraries(Stand_Marathon PRIVATE
    CAN_DBC_MODULE   # ✅ DBC библиотека
)

# --- Подключаем PCAN ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PCAN_LIB_DIR "${CMAKE_SOURCE_DIR}/lib/x64")
else()
    set(PCAN_LIB_DIR "${CMAKE_SOURCE_DIR}/lib/x86")
endif()

target_link_directories(Stand_Marathon PRIVATE ${PCAN_LIB_DIR})
target_link_libraries(Stand_Marathon PRIVATE PCANBasic)
