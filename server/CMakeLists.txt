cmake_minimum_required(VERSION 3.16)
project(GUI_Marathon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к PCAN
set(PCAN_INCLUDE "${CMAKE_SOURCE_DIR}/Stand_Marathon/Include")
# у тебя .lib лежит в подпапке VC_LIB
set(PCAN_LIB_DIR "${CMAKE_SOURCE_DIR}/Stand_Marathon/lib/x64/VC_LIB")
set(YOKO_LIB_DIR "${CMAKE_SOURCE_DIR}/Stand_Marathon/lib")

# Зависимости через vcpkg
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json CONFIG REQUIRED)

# Подключаем CAN_DBC_MODULE (правильный относительный путь)
add_subdirectory(Stand_Marathon/Include/CAN_DBC_MODULE)

# Источники ядра
set(CORE_SOURCES
    Stand_Marathon/src/CANInterface.cpp
    Stand_Marathon/src/CommandSender.cpp
    Stand_Marathon/src/ConfigManager.cpp
    Stand_Marathon/src/MarathonLogic.cpp
    Stand_Marathon/src/StateMachine.cpp
    Stand_Marathon/src/YokogawaDL850E.cpp
)

# Экзешник с WS
add_executable(MarathonWS ws_server.cpp ${CORE_SOURCES})

# ===== Yokogawa DL850E FreeRun API (ScAPI) =====
if (WIN32)
    # x64 vs x86
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SCAPI_LIB  "${YOKO_LIB_DIR}/ScAPI64.lib")
        set(SCAPI_DLL  "${YOKO_LIB_DIR}/ScAPI64.dll")
        set(TMCTL_DLL  "${YOKO_LIB_DIR}/tmctl64.dll")
    else()
        set(SCAPI_LIB  "${YOKO_LIB_DIR}/ScAPI.lib")
        set(SCAPI_DLL  "${YOKO_LIB_DIR}/ScAPI.dll")
        set(TMCTL_DLL  "${YOKO_LIB_DIR}/tmctl.dll")
    endif()

    if (NOT EXISTS "${SCAPI_LIB}")
        message(FATAL_ERROR "ScAPI lib not found: ${SCAPI_LIB}. Проверь, что лежит x64 ScAPI64.lib")
    endif()

    target_link_libraries(MarathonWS PRIVATE "${SCAPI_LIB}")

    # Копируем DLL рядом с exe, чтобы не ловить 'dll not found'
    if (EXISTS "${SCAPI_DLL}")
        add_custom_command(TARGET MarathonWS POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${SCAPI_DLL}"
                    "$<TARGET_FILE_DIR:MarathonWS>/"
        )
    else()
        message(WARNING "ScAPI DLL not found: ${SCAPI_DLL} (exe может не запуститься без DLL)")
    endif()

    if (EXISTS "${TMCTL_DLL}")
        add_custom_command(TARGET MarathonWS POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${TMCTL_DLL}"
                    "$<TARGET_FILE_DIR:MarathonWS>/"
        )
    else()
        message(WARNING "tmctl DLL not found: ${TMCTL_DLL} (exe может не запуститься без DLL)")
    endif()
endif()
# ===== end ScAPI =====


target_include_directories(MarathonWS PRIVATE
    ${PCAN_INCLUDE}
    ${CMAKE_SOURCE_DIR}/Stand_Marathon/src
)

target_link_directories(MarathonWS PRIVATE ${PCAN_LIB_DIR})
target_link_directories(MarathonWS PRIVATE ${YOKO_LIB_DIR})

target_link_libraries(MarathonWS PRIVATE
    CAN_DBC_MODULE
    Boost::system Boost::thread
    nlohmann_json::nlohmann_json
    PCANBasic
)

# Для Windows добавим системные сокеты (на всякий случай)
if (WIN32)
  target_link_libraries(MarathonWS PRIVATE ws2_32)
endif()


# Выбираем корректную x64 DLL PCANBasic и копируем рядом с exe
# Сначала пробуем DLL из репозитория, если нет — из установленного PCAN-Basic
if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64 сборка
    if(EXISTS ${CMAKE_SOURCE_DIR}/Stand_Marathon/lib/x64/PCANBasic.dll)
        set(PCANBASIC_DLL ${CMAKE_SOURCE_DIR}/Stand_Marathon/lib/x64/PCANBasic.dll)
    elseif(EXISTS "C:/Program Files/PEAK-System/PCAN-Basic/Win64/PCANBasic.dll")
        set(PCANBASIC_DLL "C:/Program Files/PEAK-System/PCAN-Basic/Win64/PCANBasic.dll")
    else()
        message(FATAL_ERROR "PCANBasic.dll (x64) not найден. Укажи правильный путь.")
    endif()
else() # на всякий случай для x86
    if(EXISTS "C:/Users/n.tulupov/Desktop/StandRemoteUI/server/Stand_Marathon/lib/x86/PCANBasic.dll")
        set(PCANBASIC_DLL "C:/Users/n.tulupov/Desktop/StandRemoteUI/server/Stand_Marathon/lib/x86/PCANBasic.dll")
    elseif(EXISTS "C:/Program Files (x86)/PEAK-System/PCAN-Basic/Win32/PCANBasic.dll")
        set(PCANBASIC_DLL "C:/Program Files (x86)/PEAK-System/PCAN-Basic/Win32/PCANBasic.dll")
    else()
        message(FATAL_ERROR "PCANBasic.dll (x86) not найден. Укажи правильный путь.")
    endif()
endif()

add_custom_command(TARGET MarathonWS POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${PCANBASIC_DLL}"
          "$<TARGET_FILE_DIR:MarathonWS>/PCANBasic.dll")    
