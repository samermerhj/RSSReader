;--------------------------------
; RSSReader Installer for Windows
; باستخدام NSIS
;--------------------------------

Name "RSSReader"
OutFile "RSSReader_Setup.exe"
InstallDir "$PROGRAMFILES\RSSReader"
RequestExecutionLevel admin

;--------------------------------
; الصفحات (بسيطة)
Page directory
Page instfiles

;--------------------------------
; الملفات المطلوب تثبيتها
Section "Install"
    SetOutPath $INSTDIR
    File /r "build\*.*"   ; ينسخ كل محتويات مجلد build (بما فيه rssreader.exe والموارد والمكتبات)

    ; إنشاء اختصارات
    CreateShortCut "$DESKTOP\RSSReader.lnk" "$INSTDIR\rssreader.exe"
    CreateShortCut "$SMPROGRAMS\RSSReader.lnk" "$INSTDIR\rssreader.exe"
SectionEnd

;--------------------------------
; إلغاء التثبيت
Section "Uninstall"
    Delete $INSTDIR\*.*
    RMDir $INSTDIR
    Delete "$DESKTOP\RSSReader.lnk"
    Delete "$SMPROGRAMS\RSSReader.lnk"
SectionEnd
