cmake_minimum_required(VERSION 3.16)
project(WebColorQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(SOURCES
    src/main.cpp
    src/api/HttpClient.cpp
    src/api/LoginApi.cpp
    src/api/FormulaApi.cpp
    src/models/FormulaModel.cpp
    src/utils/Auth.cpp
    src/utils/SearchHistory.cpp
    src/utils/SearchGather.cpp
    src/utils/FormulaExport.cpp
    src/ui/LoginWidget.cpp
    src/ui/MainWindow.cpp
    src/ui/FormulaDetailWidget.cpp
    src/ui/ProfileDialog.cpp
    src/ui/RegisterWidget.cpp
)

set(HEADERS
    src/api/HttpClient.h
    src/api/LoginApi.h
    src/api/FormulaApi.h
    src/models/FormulaModel.h
    src/utils/Auth.h
    src/utils/SearchHistory.h
    src/utils/SearchGather.h
    src/utils/FormulaExport.h
    src/ui/LoginWidget.h
    src/ui/MainWindow.h
    src/ui/FormulaDetailWidget.h
    src/ui/ProfileDialog.h
    src/ui/RegisterWidget.h
)

set(RESOURCES
    resources/styles.qrc
)

if(APPLE)
    # 将 PNG 转换为 macOS icns 图标
    set(ICON_PNG ${CMAKE_SOURCE_DIR}/resources/logo.png)
    set(ICONSET_DIR ${CMAKE_BINARY_DIR}/app.iconset)
    set(ICNS_FILE ${CMAKE_BINARY_DIR}/app.icns)
    add_custom_command(
        OUTPUT ${ICNS_FILE}
        COMMAND mkdir -p ${ICONSET_DIR}
        COMMAND sips -z 16 16     ${ICON_PNG} --out ${ICONSET_DIR}/icon_16x16.png
        COMMAND sips -z 32 32     ${ICON_PNG} --out ${ICONSET_DIR}/icon_16x16@2x.png
        COMMAND sips -z 32 32     ${ICON_PNG} --out ${ICONSET_DIR}/icon_32x32.png
        COMMAND sips -z 64 64     ${ICON_PNG} --out ${ICONSET_DIR}/icon_32x32@2x.png
        COMMAND sips -z 128 128   ${ICON_PNG} --out ${ICONSET_DIR}/icon_128x128.png
        COMMAND sips -z 256 256   ${ICON_PNG} --out ${ICONSET_DIR}/icon_128x128@2x.png
        COMMAND sips -z 256 256   ${ICON_PNG} --out ${ICONSET_DIR}/icon_256x256.png
        COMMAND sips -z 512 512   ${ICON_PNG} --out ${ICONSET_DIR}/icon_256x256@2x.png
        COMMAND sips -z 512 512   ${ICON_PNG} --out ${ICONSET_DIR}/icon_512x512.png
        COMMAND sips -z 1024 1024 ${ICON_PNG} --out ${ICONSET_DIR}/icon_512x512@2x.png
        COMMAND iconutil -c icns ${ICONSET_DIR} -o ${ICNS_FILE}
        COMMENT "Generating macOS icns icon"
    )
    set(MACOSX_ICON ${ICNS_FILE})
    set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
    set(MACOSX_ICON "")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES} ${MACOSX_ICON})

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "恺祁颜色系统"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_IDENTIFIER com.kaiqi.webcolorqt
        MACOSX_BUNDLE_ICON_FILE app.icns
        MACOSX_BUNDLE_INFO_STRING "恺祁颜色系统 v${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network)
