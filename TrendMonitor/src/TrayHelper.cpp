#include "stdafx.h"

#include <assert.h>
#include <strsafe.h>
#include <typeinfo>

#include "TrayHelper.h"

////////////////////////////////////////////////////////////////////////////////
// ��������� ������ � ����
#define WM_TRAY_ICON        (WM_APP + 100)

// ������������� ������ ����, ������ ���� �� 0 ����� ������������ �����
#define TRAY_ID             1

//----------------------------------------------------------------------------//
BEGIN_MESSAGE_MAP(CTrayHelper, CDialogEx)
    ON_MESSAGE(WM_TRAY_ICON, &CTrayHelper::OnTrayIcon)
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
CTrayHelper::CTrayHelper()
{
    HINSTANCE instance = AfxGetInstanceHandle();
    CString className(typeid(*this).name());

    // ������������ ��� ����
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, className, &wndClass))
    {
        // ����������� ������ ���� ������� ������������ ��� �������������� �����
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = className;

        if (!RegisterClassEx(&wndClass))
            ::MessageBox(NULL, L"Can`t register class", L"Error", MB_OK);
    }

    // ������� ��������� ���� ������� ����� ������������ ������ � �����, �����
    // ����� ������ ������ ������� �� ����
    if (CDialogEx::CreateEx(WS_EX_TOOLWINDOW, className, L"",
                            0,
                            0, 0, 0, 0,
                            NULL, nullptr, nullptr) == FALSE)
        assert(!"�� ������� ������� ���� ���������� ���� �����!");

    // �������������� ��������� � ����������� �����������
    ::ZeroMemory(&m_niData, sizeof(NOTIFYICONDATA));

    m_niData.cbSize = sizeof(NOTIFYICONDATA);
    m_niData.hWnd = m_hWnd;
    m_niData.uID = TRAY_ID;
    m_niData.uCallbackMessage = WM_TRAY_ICON;
}

//----------------------------------------------------------------------------//
CTrayHelper::~CTrayHelper()
{
    removeTrayIcon();

    CDialogEx::DestroyWindow();
}

//----------------------------------------------------------------------------//
void CTrayHelper::addTrayIcon(const HICON& icon,
                              const CString& tipText,
                              const CreateTrayMenu& onCreateTrayMenu /*= nullptr*/,
                              const DestroyTrayMenu& onDestroyTrayMenu /*= nullptr*/,
                              const OnSelectTrayMenu& onSelectTrayMenu /*= nullptr*/,
                              const OnUserDBLClick& onDBLClick /*= nullptr*/)
{
    m_trayCreateMenu        = onCreateTrayMenu;
    m_trayDestroyMenu       = onDestroyTrayMenu;
    m_traySelectMenuItem    = onSelectTrayMenu;
    m_trayDBLUserClick      = onDBLClick;

    // ������� ������ ����
    m_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_niData.hIcon = icon;

    StringCchCopy(m_niData.szTip, ARRAYSIZE(m_niData.szTip), tipText);

    ::Shell_NotifyIcon(m_bTrayIconExist ? NIM_MODIFY : NIM_ADD, &m_niData);

    m_bTrayIconExist = true;
}

//----------------------------------------------------------------------------//
void CTrayHelper::removeTrayIcon()
{
    if (!m_bTrayIconExist)
        return;

    // ������� ������ �� ����
    ::Shell_NotifyIcon(NIM_DELETE, &m_niData);

    m_bTrayIconExist     = false;
    m_trayCreateMenu     = nullptr;
    m_trayDestroyMenu    = nullptr;
    m_traySelectMenuItem = nullptr;
    m_trayDBLUserClick   = nullptr;
}

//----------------------------------------------------------------------------//
void CTrayHelper::showBubble(const CString& title,
                             const CString& descr,
                             DWORD dwBubbleFlags /*= NIIF_WARNING*/,
                             const OnUserClick& onUserClick /*= nullptr*/)
{
    m_notificationClick = onUserClick;

    m_niData.uFlags |= NIF_INFO;
    // ���� ������ ����������� � ����� - ���� ���������� ��������� ���������
    if (m_notificationClick)
        m_niData.uFlags |= NIF_MESSAGE;

    m_niData.dwInfoFlags = dwBubbleFlags;

    StringCchCopy(m_niData.szInfo, ARRAYSIZE(m_niData.szInfo), descr);
    StringCchCopy(m_niData.szInfoTitle, ARRAYSIZE(m_niData.szInfoTitle), title);

    // ������� ������ � ������ �����, �������� �������� "�������" ������
    ::Shell_NotifyIcon(NIM_DELETE, &m_niData);
    ::Shell_NotifyIcon(NIM_ADD, &m_niData);
}

//----------------------------------------------------------------------------//
BOOL CTrayHelper::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // ��������� � ������ � ����
    if (m_traySelectMenuItem)
        m_traySelectMenuItem(LOWORD(wParam));

    return CDialogEx::OnCommand(wParam, lParam);
}

//----------------------------------------------------------------------------//
afx_msg LRESULT CTrayHelper::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        {
            if (!m_trayCreateMenu)
                break;

            // ������� ����
            HMENU hMenu = m_trayCreateMenu();
            if (hMenu)
            {
                POINT pt;
                ::GetCursorPos(&pt);
                // ���� ������ ���� Foreground ����� �� ���� ���� ������� ��� ����
                // �� ������������ � TrackPopupMenu
                ::SetForegroundWindow(m_hWnd);

                ::TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, nullptr);

                if (m_trayDestroyMenu)
                    m_trayDestroyMenu(hMenu);
                else
                    ::DestroyMenu(hMenu);
            }
        }
        break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        // ��������� � ���� �����
        if (m_trayDBLUserClick)
            m_trayDBLUserClick();

        break;
    case NIN_BALLOONUSERCLICK:  // ���� �� ����������
        if (m_notificationClick)
            m_notificationClick();
        break;
    }

    return 0;
}
