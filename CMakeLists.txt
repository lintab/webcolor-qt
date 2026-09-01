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

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network)
