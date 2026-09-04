cmake_minimum_required(VERSION 3.16)
project(iK CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 图标资源编译（windres）
# ============================================================
set(WINDRES "D:/Qt/qt/Tools/mingw1310_64/bin/windres.exe")
set(ICON_RES "${CMAKE_BINARY_DIR}/app_icon.res")

add_custom_command(
    OUTPUT "${ICON_RES}"
    COMMAND ${WINDRES} --use-temp-file -J rc -O coff
        -I "${CMAKE_SOURCE_DIR}/resources"
        -i "${CMAKE_SOURCE_DIR}/resources/app.rc"
        -o "${ICON_RES}"
    DEPENDS "${CMAKE_SOURCE_DIR}/resources/app.rc"
            "${CMAKE_SOURCE_DIR}/resources/app.ico"
    COMMENT "编译图标资源 app_icon.res"
)

add_custom_target(IconResource ALL DEPENDS "${ICON_RES}")

# ============================================================
# Qt6 configuration
# ============================================================
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core Gui)

set(CMAKE_AUTOMOC ON)

# ============================================================
# Source files
# ============================================================
set(LIB_SOURCES
    src/LicenseClient.cpp
    src/MachineInfo.cpp
    src/Base64.cpp
)

set(LIB_HEADERS
    include/LicenseClient.h
    include/MachineInfo.h
    include/Base64.h
)

set(GUI_SOURCES
    gui/main.cpp
    gui/mainwindow.cpp
    gui/licenseworker.cpp
    gui/toastnotification.cpp
    gui/logdialog.cpp
    gui/licensestorage.cpp
    gui/splashscreen.cpp
)

set(GUI_HEADERS
    gui/mainwindow.h
    gui/licenseworker.h
    gui/toastnotification.h
    gui/logdialog.h
    gui/licensestorage.h
    gui/splashscreen.h
)

add_executable(${PROJECT_NAME}
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)

# 确保图标资源在链接前生成
add_dependencies(${PROJECT_NAME} IconResource)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    gui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
    Qt6::Gui
    advapi32
    crypt32
    bcrypt
    secur32
    ncrypt
    Winhttp
    ws2_32
    Iphlpapi
    ntdll
    shlwapi
)

# ============================================================
# Windows subsystem: no console window
# ============================================================
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ============================================================
# 将 .res 资源文件链接到可执行文件
# ============================================================
# 使用 LINK_FLAGS 方式确保资源被正确链接
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "${ICON_RES}"
)
