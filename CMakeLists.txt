cmake_minimum_required(VERSION 3.10)
project(RSSReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Network Sql Xml Svg REQUIRED)
 
set(RESOURCES ${CMAKE_SOURCE_DIR}/resources.qrc)
if(NOT EXISTS ${RESOURCES})
    message(WARNING "resources.qrc غير موجود، سيتم المتابعة بدونه")
    set(RESOURCES "")
endif()

 
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/DatabaseManager.cpp
    src/RSSFetcher.cpp
    src/NewsTicker.cpp
    src/MagazinePage.cpp
    src/SettingsDialog.cpp
    src/IconManager.cpp
    src/ImageDatabase.cpp
    src/SmartImageProvider.cpp
)

add_executable(RSSReader_Qt ${SOURCES} ${RESOURCES})

target_link_libraries(RSSReader_Qt
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    Qt5::Xml
    Qt5::Svg
)

 
add_custom_command(TARGET RSSReader_Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:RSSReader_Qt>/resources
    COMMENT "نسخ مجلد resources إلى مجلد البناء"
)

 
install(TARGETS RSSReader_Qt RUNTIME DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/RSSReader/resources)
install(DIRECTORY translations/ DESTINATION share/RSSReader/translations)

if(EXISTS ${CMAKE_SOURCE_DIR}/icons/tray_icon.svg)
    install(FILES icons/tray_icon.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.svg)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/icons/tray_icon.png)
    install(FILES icons/tray_icon.png DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.png)
endif()

install(FILES rssreader.desktop DESTINATION share/applications)
install(FILES LICENSE DESTINATION share/doc/RSSReader)

 
find_package(Qt5 COMPONENTS LinguistTools QUIET)
if(Qt5LinguistTools_FOUND)
    file(GLOB TS_FILES ${CMAKE_SOURCE_DIR}/translations/*.ts)
    if(TS_FILES)
        qt5_create_translation(QM_FILES ${TS_FILES})
        install(FILES ${QM_FILES} DESTINATION share/RSSReader/translations)
    endif()
endif()

 
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "rssreader-qt")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Samer Merhj <mjosak7@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RSSReader - Smart News Magazine")
set(CPACK_PACKAGE_DESCRIPTION "Advanced RSS reader supporting multiple sources, daily magazine view, smart news grouping, archiving, and translation.")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5sql5, libqt5xml5, libqt5svg5")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "fonts-noto-color-emoji, qttranslations5-l10n")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/samermerhj/RSSReader")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    ${CMAKE_SOURCE_DIR}/cmake/debian/postinst
    ${CMAKE_SOURCE_DIR}/cmake/debian/prerm
    ${CMAKE_SOURCE_DIR}/cmake/debian/postrm
)
include(CPack)
