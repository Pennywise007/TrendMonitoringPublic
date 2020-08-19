#pragma once

#include <afxdialogex.h>
#include <functional>

////////////////////////////////////////////////////////////////////////////////
// ��������������� ����� ��� ��������� ���� � ������ ����������
class CTrayHelper : public CDialogEx
{
public:
    CTrayHelper();
    ~CTrayHelper();

    // ������� ������� �������� ����
    typedef std::function<HMENU()> CreateTrayMenu;
    // ������� ���������� ��� ����������� ���������� ����
    typedef std::function<void(HMENU)> DestroyTrayMenu;
    // ������� ������ �������� � ����
    typedef std::function<void(UINT)> OnSelectTrayMenu;
    // ������� ���������� ��� ���������� � �����
    typedef std::function<void()> OnUserClick;
    // ������� ���������� ��� ���������� � ���� �����
    typedef std::function<void()> OnUserDBLClick;
public:
    /// <summary>�������� ������ � ����.</summary>
    /// <param name="icon">������ � ����.</param>
    /// <param name="tipText">��������� � ����.</param>
    /// <param name="createTrayMenu">�������� ���� �� ���/��� �� ������ ����.</param>
    /// <param name="destroyTrayMenu">�������� ����, ���� nullptr ����� ������ ����������� ::DestroyMenu.</param>
    /// <param name="onSelectTrayMenu">������� ���������� ��� ����� �� ������� ����.</param>
    /// <param name="onDBLClick">������� ���� �� ���� ������.</param>
    void addTrayIcon(const HICON& icon,
                     const CString& tipText,
                     const CreateTrayMenu& createTrayMenu = nullptr,
                     const DestroyTrayMenu& destroyTrayMenu = nullptr,
                     const OnSelectTrayMenu& onSelectTrayMenu = nullptr,
                     const OnUserDBLClick& onDBLClick = nullptr);
    /// <summary>�������� ������ �� ����.</summary>
    void removeTrayIcon();
    /// <summary>�������� �������� ����������.</summary>
    /// <param name="title">��������� ����������.</param>
    /// <param name="descr">��������� ����������.</param>
    /// <param name="dwBubbleFlags">���� � �������� ���������� ����������.</param>
    /// <param name="onUserClick">���� ������������ �� ������.</param>
    void showBubble(const CString& title,
                    const CString& descr,
                    const DWORD dwBubbleFlags = NIIF_WARNING,
                    const OnUserClick& onUserClick = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);

// ��������� ����������� ������ � ����
private:
    // ���� ������� ������ � ����
    bool m_bTrayIconExist = false;

    // ������� ��� ���������� �����
    CreateTrayMenu   m_trayCreateMenu       = nullptr;   // �������� ����
    DestroyTrayMenu  m_trayDestroyMenu      = nullptr;   // �������� ����
    OnSelectTrayMenu m_traySelectMenuItem   = nullptr;   // ����� �� ����
    OnUserDBLClick   m_trayDBLUserClick     = nullptr;   // ���� ���� � ����

// ��������� ��� �����������
private:
    OnUserClick      m_notificationClick    = nullptr;   // ���� �� �����������

private:
    // ��������� � ����������� �����������
    NOTIFYICONDATA   m_niData;
};

