#include <windows.h>
#include <stdio.h>

// دالة لكتابة قيمة DWORD في السجل
void SetRegistryDWORD(HKEY hKeyRoot, const char* subKey, const char* valueName, DWORD data) {
    HKEY hKey;
    LONG result = RegCreateKeyExA(hKeyRoot, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&data, sizeof(DWORD));
        RegCloseKey(hKey);
        printf("✅ تم تعيين: %s\\%s = %lu\n", subKey, valueName, data);
    } else {
        printf("❌ فشل في إنشاء/فتح المفتاح: %s\n", subKey);
    }
}

int main() {
    printf("🔧 أداة تمكين TLS 1.2 لـ Windows 7\n");
    printf("=======================================\n\n");

    // 1. تمكين TLS 1.2 في Schannel (Client)
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\TLS 1.2\\Client",
        "DisabledByDefault", 0);
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\TLS 1.2\\Client",
        "Enabled", 1);

    // 2. تمكين TLS 1.2 في Schannel (Server) - اختياري
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\TLS 1.2\\Server",
        "DisabledByDefault", 0);
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\TLS 1.2\\Server",
        "Enabled", 1);

    // 3. إعدادات .NET Framework (للتطبيقات التي تستخدم .NET 4.x)
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\.NETFramework\\v4.0.30319",
        "SchUseStrongCrypto", 1);
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\.NETFramework\\v4.0.30319",
        "SystemDefaultTlsVersions", 1);

    // 4. إعدادات .NET Framework (لأنظمة 64-bit مع تطبيقات 32-bit)
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Microsoft\\.NETFramework\\v4.0.30319",
        "SchUseStrongCrypto", 1);
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Microsoft\\.NETFramework\\v4.0.30319",
        "SystemDefaultTlsVersions", 1);

    // 5. تمكين TLS 1.2 في WinHTTP (لأنظمة 64-bit)
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp",
        "DefaultSecureProtocols", 0x00000800);

    // 6. تمكين TLS 1.2 في WinHTTP (لأنظمة 32-bit)
    SetRegistryDWORD(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp",
        "DefaultSecureProtocols", 0x00000800);

    printf("\n✅ تم تطبيق جميع الإعدادات بنجاح.\n");
    printf("⚠️ يرجى إعادة تشغيل الكمبيوتر لتطبيق التغييرات.\n");

    // طلب الضغط على مفتاح قبل الخروج (لرؤية النتيجة)
    printf("\nاضغط على أي مفتاح للخروج...");
    getchar();
    return 0;
}
