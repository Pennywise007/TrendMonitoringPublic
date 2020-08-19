#pragma once

#include <afx.h>
#include <afxwin.h>
#include <assert.h>
#include <optional>
#include <memory>

#ifndef DONT_USE_PATHCCH    // использование функции PathCchRemoveFileSpec для убирания имен экзешек

#include <pathcch.h>
#pragma comment(lib, "Pathcch.lib")

#endif  // DONT_USE_PATHCCH

#include "Singleton.h"

// Список источников путей
// Список генерируется с помощью макрофункции
#define GENERATE_SOURCE_TABLE(TD) \
    TD(SOURCE_REG_ZET,    L"SOFTWARE\\ZET\\ZETLab",     KEY_READ) \
    TD(SOURCE_REG_ZETLAB, L"SOFTWARE\\ZETLAB\\Settings",KEY_READ | KEY_WOW64_64KEY)

#pragma comment(lib, "Advapi32.lib")

typedef enum
{
#define TD(index, subkey, sam) index,
    GENERATE_SOURCE_TABLE(TD)
#undef TD
} SOURCE_INDEX;

typedef struct
{
    LPCWSTR subKey;
    DWORD samDesired;
} SOURCE_ENTRY;

static const SOURCE_ENTRY SOURCE_TABLE[] =
{
#define TD(index, subkey, sam) { subkey, sam },
    GENERATE_SOURCE_TABLE(TD)
#undef TD
};

//****************************************************************************//
// класс отправки сообщений
class DirsService
{
    friend class CSingleton<DirsService>;
public: // директории связанные с зетлабом
    CString getZetSignalsDir() const;
    CString getZetCompressedDir() const;
    // получение папки установки зетлаба
    CString getZetInstallDir() const;

public: // Прочие функции
    // получаем текущую директорию
    CString getExeDir() const;

public:
    // установить директорию с сигналами, используется для подмены расположения данных
    void setZetSignalsDir(const CString& signalsDir);
    // установить директорию со сжатыми сигналами, используется для подмены расположения данных
    void setZetCompressedDir(const CString& compressedDir);

private:
    static CString querySource(SOURCE_INDEX source, LPCWSTR lpName);

private:
    // директория с сигналами, если не задана, используется директория из реестра
    std::optional<CString> m_signalsDir;
    // директория со сжатыми сигналами, если не задана, используется директория из реестра
    std::optional<CString> m_compressedDir;
};

//****************************************************************************//
inline CString DirsService::getZetSignalsDir() const
{
    if (m_signalsDir.has_value())
        return *m_signalsDir;

    return querySource(SOURCE_REG_ZETLAB, L"DirSignal");
}

//****************************************************************************//
inline CString DirsService::getZetCompressedDir() const
{
    if (m_compressedDir.has_value())
        return *m_compressedDir;

    return querySource(SOURCE_REG_ZETLAB, L"DirCompressed");
}

//****************************************************************************//
inline CString DirsService::getZetInstallDir() const
{
    return querySource(SOURCE_REG_ZET, L"InstallLocation");
}

//****************************************************************************//
inline CString DirsService::getExeDir() const
{
    const size_t maxPathLength = _MAX_PATH * 10;
    TCHAR szPath[maxPathLength];
    // не используем GetCurrentDir т.к. наше приложение может быть запущено через
    // командную строку, а мы ищем конкретно расположение экзешки
    VERIFY(::GetModuleFileName(/*AfxGetApp()->m_hInstance*/NULL, szPath, maxPathLength));

#ifndef DONT_USE_PATHCCH
    // удаляем имя экзешки и добавляем в конце '\'
    ::PathCchRemoveFileSpec(szPath, maxPathLength);
    return CString(szPath) + L"\\";
#else
    CString currentDir(szPath);
    int nIndex = currentDir.ReverseFind(_T('\\'));
    if (nIndex > 0)
        currentDir = currentDir.Left(nIndex + 1);

    return currentDir;
#endif DONT_USE_PATHCCH
}

//****************************************************************************//
inline void DirsService::setZetSignalsDir(const CString& signalsDir)
{
    m_signalsDir = signalsDir;
}

//****************************************************************************//
inline void DirsService::setZetCompressedDir(const CString& compressedDir)
{
    m_compressedDir = compressedDir;
}

//****************************************************************************//
// Получение пути с указанного источника (пока только реестр HKLM)
inline CString DirsService::querySource(SOURCE_INDEX source, LPCWSTR lpName)
{
    HKEY key(NULL);
    DWORD cchCount = _MAX_PATH * 10;

    std::shared_ptr<WCHAR> lpPath(new WCHAR[cchCount]);

    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SOURCE_TABLE[source].subKey, 0, SOURCE_TABLE[source].samDesired, &key);
    if (ERROR_SUCCESS != status)
    {
        RegCloseKey(key);
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, SOURCE_TABLE[source].subKey, 0, nullptr, 0, SOURCE_TABLE[source].samDesired | KEY_WRITE, NULL, &key, NULL);
        RegCloseKey(key);
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SOURCE_TABLE[source].subKey, 0, SOURCE_TABLE[source].samDesired, &key);
    }
    if (status == ERROR_SUCCESS)
    {
        DWORD cb = cchCount * sizeof(WCHAR);
        DWORD type;
        status = RegQueryValueEx(key, lpName, NULL, &type, (LPBYTE)lpPath.get(), &cb);

        RegCloseKey(key);
    }

    return CString(lpPath.get());
}
