cmake_minimum_required(VERSION 3.16)                          
project(RSSReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)                                     
set(CMAKE_AUTORCC ON)                                     
find_package(Qt5 REQUIRED COMPONENTS                       
    Widgets Network Sql Xml Svg LinguistTools
)
  
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

 
 
set(RESOURCE_FILES resources.qrc)

# 7 
qt5_add_translation(QM_FILES
    translations/rssreader_en.ts
    translations/rssreader_ar.ts
    translations/rssreader_fr.ts
    translations/rssreader_ru.ts
    translations/rssreader_zh.ts
)

# 8 
add_executable(RSSReader_Qt ${SOURCES} ${RESOURCE_FILES} ${QM_FILES})

# 9   
target_link_libraries(RSSReader_Qt
    Qt5::Widgets Qt5::Network Qt5::Sql Qt5::Xml Qt5::Svg
)

# 10  
install(TARGETS RSSReader_Qt RUNTIME DESTINATION bin)
install(FILES icons/tray_icon.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.svg)
install(FILES rssreader.desktop DESTINATION share/applications)
install(FILES LICENSE DESTINATION share/doc/RSSReader)

 
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "rssreader-qt")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Samer Merhj <mjosak7@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RSSReader - Smart News Magazine")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5sql5, libqt5xml5, libqt5svg5")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "fonts-noto-color-emoji, qttranslations5-l10n")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/samermerhj/RSSReader")


include(CPack)
