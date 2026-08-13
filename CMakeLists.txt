cmake_minimum_required(VERSION 3.16)
project(RSSReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)  # تم التعليق لأننا لم نعد نستخدم الموارد المضمنة

find_package(Qt5 REQUIRED COMPONENTS
    Widgets Network Sql Xml Svg
    # LinguistTools  # معلق مؤقتاً
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
    src/ResourceManager.cpp
)

add_executable(rssreader ${SOURCES})

# نسخ resources إلى مجلد البناء (للتطوير)
add_custom_command(TARGET rssreader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:rssreader>/resources
    COMMENT "نسخ resources إلى مجلد البناء"
)

# نسخ translations إلى مجلد البناء (للتطوير)
add_custom_command(TARGET rssreader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/translations
        $<TARGET_FILE_DIR:rssreader>/translations
    COMMENT "نسخ translations إلى مجلد البناء"
)

target_link_libraries(rssreader
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    Qt5::Xml
    Qt5::Svg
)

# ---------- التثبيت ----------
install(TARGETS rssreader RUNTIME DESTINATION bin)

# تثبيت الموارد
install(DIRECTORY resources/ DESTINATION share/RSSReader/resources)
install(DIRECTORY translations/ DESTINATION share/RSSReader/translations)

# تثبيت الأيقونة
install(FILES icons/tray_icon.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.svg)

# تثبيت ملف سطح المكتب
install(FILES rssreader.desktop DESTINATION share/applications)

# تثبيت الرخصة
install(FILES LICENSE DESTINATION share/doc/RSSReader)

# ---------- إنشاء مجلد rsss في منزل المستخدم ونسخ الملفات إليه أثناء التثبيت ----------
install(CODE "
    # الحصول على اسم المستخدم
    execute_process(COMMAND whoami OUTPUT_VARIABLE USERNAME OUTPUT_STRIP_TRAILING_WHITESPACE)
    # مسار المجلد الجديد
    set(RSSS_DIR \"/home/\${USERNAME}/rsss\")
    # إنشاء المجلدات
    file(MAKE_DIRECTORY \${RSSS_DIR}/resources/images/news_icons)
    file(MAKE_DIRECTORY \${RSSS_DIR}/resources/images/backgrounds)
    file(MAKE_DIRECTORY \${RSSS_DIR}/translations)
    file(MAKE_DIRECTORY \${RSSS_DIR}/cache/images)
    # نسخ الملفات من مجلد التثبيت إلى rsss
    file(COPY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION \${RSSS_DIR}/resources/)
    file(COPY ${CMAKE_SOURCE_DIR}/translations/ DESTINATION \${RSSS_DIR}/translations/)
")

# ---------- إعدادات CPack لإنشاء حزمة .deb ----------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "rssreader")
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

include(CPack)
