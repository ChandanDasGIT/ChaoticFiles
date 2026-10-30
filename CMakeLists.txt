cmake_minimum_required(VERSION 3.21)

project(ChaoticFiles VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick DBus)
find_package(Threads REQUIRED)

qt_standard_project_setup(REQUIRES 6.10)

# ---------------------------------------------------------------- the engine (no Qt)
# Everything under backend/ is plain C++20. It is built as its own library and does not link to Qt,
# so a Qt include in there fails to compile.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/backend/*.cpp")

add_library(fileengine STATIC ${ENGINE_SOURCES})
target_include_directories(fileengine PUBLIC "${CMAKE_CURRENT_LIST_DIR}/backend")
target_link_libraries(fileengine PUBLIC Threads::Threads)
target_compile_options(fileengine PRIVATE -Wall -Wextra)

# ---------------------------------------------------------------- the app
# wrapper/  : QObject classes that connect the engine to QML (any subfolders)
# frontend/ : the QML files (any subfolders). File names must start with a capital letter and be
#             unique across all subfolders.
file(GLOB_RECURSE WRAPPER_SOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.h")

file(GLOB_RECURSE FRONTEND_QML CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/frontend/*.qml")

set_source_files_properties(frontend/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(frontend/Icons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appChaoticFiles
    main.cpp
)

qt_add_qml_module(appChaoticFiles
    URI ChaoticFiles
    QML_FILES
        Main.qml
        ${FRONTEND_QML}
    SOURCES
        ${WRAPPER_SOURCES}
    RESOURCES
        frontend/fonts/fa-solid-900.ttf
)

target_include_directories(appChaoticFiles PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

# Qt's generated registration code includes the wrapper headers by bare file name,
# so every folder under wrapper/ has to be on the include path.
set(WRAPPER_DIRS "")
foreach(f ${WRAPPER_SOURCES})
    get_filename_component(d "${CMAKE_CURRENT_LIST_DIR}/${f}" DIRECTORY)
    list(APPEND WRAPPER_DIRS "${d}")
endforeach()
list(REMOVE_DUPLICATES WRAPPER_DIRS)
target_include_directories(appChaoticFiles PRIVATE ${WRAPPER_DIRS})

target_link_libraries(appChaoticFiles PRIVATE Qt6::Quick Qt6::DBus fileengine)

# ---------------------------------------------------------------- the tests (no Qt)
enable_testing()
add_executable(backendtests tests/backendtests.cpp)
target_link_libraries(backendtests PRIVATE fileengine)
add_test(NAME backend COMMAND backendtests)
