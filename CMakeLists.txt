cmake_minimum_required(VERSION 3.16)
project(RSSReader VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ============================================================
# إعدادات خاصة بكل نظام تشغيل
# ============================================================

# ---------- ويندوز ----------
if (WIN32)
    # إخفاء نافذة الطرفية
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()

# ---------- ماك (macOS) ----------
if (APPLE)
    # تعيين اسم التطبيق ونوع الحزمة
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
    
    # تحديد ملف Info.plist
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist)
    set(CMAKE_MACOSX_BUNDLE YES)
endif()

# ============================================================
# البحث عن مكتبات Qt
# ============================================================

find_package(Qt5 REQUIRED COMPONENTS
    Widgets Network Sql Xml Svg
)

# ============================================================
# ملفات المصدر
# ============================================================

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

# ============================================================
# إنشاء الملف التنفيذي
# ============================================================

add_executable(rssreader ${SOURCES})

# إذا كان النظام ماك، غيّر اسم الملف التنفيذي إلى RSSReader.app
if (APPLE)
    set_target_properties(rssreader PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist
        OUTPUT_NAME "RSSReader"
    )
endif()

# ============================================================
# نسخ الموارد والترجمات إلى مجلد البناء (للتطوير)
# ============================================================

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

# ============================================================
# ربط المكتبات
# ============================================================

target_link_libraries(rssreader
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    Qt5::Xml
    Qt5::Svg
)

# ============================================================
# التثبيت (لينكس فقط)
# ============================================================

if (UNIX AND NOT APPLE)
    install(TARGETS rssreader RUNTIME DESTINATION bin)
    install(DIRECTORY resources/ DESTINATION share/RSSReader/resources)
    install(DIRECTORY translations/ DESTINATION share/RSSReader/translations)
    install(FILES icons/tray_icon.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME rssreader.svg)
    install(FILES rssreader.desktop DESTINATION share/applications)
    install(FILES LICENSE DESTINATION share/doc/RSSReader)

    # إنشاء مجلد rsss في منزل المستخدم (لينكس فقط)
    install(CODE "
        execute_process(COMMAND whoami OUTPUT_VARIABLE USERNAME OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(RSSS_DIR \"/home/\${USERNAME}/rsss\")
        file(MAKE_DIRECTORY \${RSSS_DIR}/resources/images/news_icons)
        file(MAKE_DIRECTORY \${RSSS_DIR}/resources/images/backgrounds)
        file(MAKE_DIRECTORY \${RSSS_DIR}/translations)
        file(MAKE_DIRECTORY \${RSSS_DIR}/cache/images)
        file(COPY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION \${RSSS_DIR}/resources/)
        file(COPY ${CMAKE_SOURCE_DIR}/translations/ DESTINATION \${RSSS_DIR}/translations/)
    ")
endif()

# ============================================================
# إعدادات CPack (لحزمة .deb فقط على لينكس)
# ============================================================

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
    include(CPack)
endif()
