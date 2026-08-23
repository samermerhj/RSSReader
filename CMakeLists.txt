cmake_minimum_required(VERSION 3.16)
project(RSSReader VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS
    Widgets Network Sql Xml Svg
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

# ------------------ تعريف الهدف (مرة واحدة فقط) ------------------
if (WIN32)
    # إخفاء نافذة الطرفية على ويندوز
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    # إضافة الأيقونة
    list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/resources/app.ico)
    add_executable(rssreader WIN32 ${SOURCES})
else()
    add_executable(rssreader ${SOURCES})
endif()
# ------------------ نهاية التعريف ------------------

# نسخ resources و translations إلى مجلد البناء (للتطوير)
add_custom_command(TARGET rssreader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:rssreader>/resources
    COMMENT "نسخ resources إلى مجلد البناء"
)

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

# ---------- التثبيت (لينكس فقط) ----------
if (UNIX AND NOT APPLE)
    install(TARGETS rssreader RUNTIME DESTINATION bin)
    install(DIRECTORY resources/ DESTINATION share/RSSReader/resources)
    install(DIRECTORY translations/ DESTINATION share/RSSReader/translations)
    install(FILES icons/tray_icon.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.svg)
    install(FILES rssreader.desktop DESTINATION share/applications)
    install(FILES LICENSE DESTINATION share/doc/RSSReader)

    # ملاحظة: تم حذف كتلة install(CODE ...) القديمة لأنها لا تعمل على أجهزة المستخدمين
    # وتم استبدالها بسكربت postinst (انظر قسم CPack أدناه)
endif()

# ---------- إعدادات CPack (لحزمة .deb) ----------
if (UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "rssreader")
    set(CPACK_PACKAGE_VERSION "1.1.0")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Samer Merhj <mjosak7@gmail.com>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RSSReader - Smart News Magazine")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5sql5, libqt5xml5, libqt5svg5")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "fonts-noto-color-emoji, qttranslations5-l10n")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/samermerhj/RSSReader")

    # 🆕 تضمين سكربتات التحكم (postinst / postrm) في الحزمة
    # تأكد من وجود الملفات في مجلد cmake/debian/ داخل مشروعك
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/cmake/debian/postinst"
        "${CMAKE_SOURCE_DIR}/cmake/debian/postrm"
    )

    include(CPack)
endif()
