cmake_minimum_required(VERSION 3.21)

project(ChaoticFiles VERSION 1.2.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick Widgets DBus Network)   # Widgets: the menus are QMenus
find_package(Threads REQUIRED)

qt_standard_project_setup(REQUIRES 6.10)

# ---------------------------------------------------------------- third-party code
# BLAKE3's portable C code only: slower than its assembly, and needs nothing but a C compiler.
add_library(blake3 STATIC
    dependencies/blake3/blake3.c
    dependencies/blake3/blake3_dispatch.c
    dependencies/blake3/blake3_portable.c)
target_include_directories(blake3 PUBLIC dependencies/blake3)
target_compile_definitions(blake3 PRIVATE BLAKE3_NO_SSE2 BLAKE3_NO_SSE41 BLAKE3_NO_AVX2 BLAKE3_NO_AVX512)

# Monocypher checks the signature of an update.
add_library(monocypher STATIC
    dependencies/monocypher/monocypher.c
    dependencies/monocypher/monocypher-ed25519.c)
target_include_directories(monocypher PUBLIC dependencies/monocypher)

# ---------------------------------------------------------------- the engine (no Qt)
# Everything under backend/ is plain C++20. It is built as its own library and does not link to Qt,
# so a Qt include in there fails to compile.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/backend/*.cpp")

add_library(fileengine STATIC ${ENGINE_SOURCES})
target_include_directories(fileengine PUBLIC "${CMAKE_CURRENT_LIST_DIR}/backend")
target_link_libraries(fileengine PUBLIC Threads::Threads ${CMAKE_DL_LIBS} blake3 monocypher)
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
set_source_files_properties(frontend/common/DragState.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
        resources/fonts/fa-solid-900.ttf
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

target_link_libraries(appChaoticFiles PRIVATE Qt6::Quick Qt6::Widgets Qt6::DBus Qt6::Network fileengine)

# The version the About section shows. A release passes its tag (-DAPP_VERSION=1.1), other builds use the project version.
if(NOT DEFINED APP_VERSION)
    set(APP_VERSION "${PROJECT_VERSION}")
endif()

# The key that checks updates. The test build replaces it and the addresses, to use a local server.
if(NOT DEFINED UPDATE_PUBLIC_KEY)
    file(READ packaging/update-public-key.txt UPDATE_PUBLIC_KEY)
    string(STRIP "${UPDATE_PUBLIC_KEY}" UPDATE_PUBLIC_KEY)
endif()
target_compile_definitions(appChaoticFiles PRIVATE
    APP_VERSION="${APP_VERSION}"
    UPDATE_PUBLIC_KEY="${UPDATE_PUBLIC_KEY}")
if(DEFINED UPDATE_API_URL)
    target_compile_definitions(appChaoticFiles PRIVATE UPDATE_API_URL="${UPDATE_API_URL}" UPDATE_SITE_URL="${UPDATE_SITE_URL}")
endif()

# Drives the real window from a command file, for tests/update. Never in a release.
if(CHAOTICFILES_TEST_DRIVER)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    target_sources(appChaoticFiles PRIVATE tests/update/testdriver.cpp tests/update/testdriver.h)
    target_compile_definitions(appChaoticFiles PRIVATE CHAOTICFILES_TEST_DRIVER)
    target_link_libraries(appChaoticFiles PRIVATE Qt6::Test)
endif()

# ---------------------------------------------------------------- installing (used by packaging/build-appimage.sh)
include(GNUInstallDirs)

install(TARGETS appChaoticFiles RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# `ChaoticFiles` in a terminal (and in ChaoticFiles.desktop) starts the app: a link next to the real program.
install(CODE "
    set(link \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/ChaoticFiles\")
    file(REMOVE \"\${link}\")
    file(CREATE_LINK appChaoticFiles \"\${link}\" SYMBOLIC)
")

install(FILES packaging/ChaoticFiles.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/ChaoticFiles.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# ---------------------------------------------------------------- the tests (no Qt)
enable_testing()
add_executable(backendtests tests/backendtests.cpp)
target_link_libraries(backendtests PRIVATE fileengine)
add_test(NAME backend COMMAND backendtests)
