#pragma once

#include <afxcmn.h>
#include <map>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// ���������� ��� �������� � ������ ��������� ��������� ������� �� �������
// � ������������� ����� ���� / ���������� �� �� ����� � ����������� ������ � ���������
class CTabControl : public CTabCtrl
{
public:
    CTabControl() = default;

public:
    /// ������� ���� � ����� ����������� � ����, ��� ������������ �������
    /// ����� ���������� ���� ����������� � �������.
    ///
    /// ��������! ��������� ���������� �� ������� (�� ����������� ��������)
    /// ������� ����� ������������� ������ ����� �������� � ���������(�� �����������)
    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem,
                   _In_ std::shared_ptr<CWnd> tabWindow);

    /// ������� ������� � ���, ���� � ������� ��� ���� - ����� ������
    /// CDialog::Create � �������� ��������������� nIDTemplate � ��������� ����� ���������� ��� �������
    ///
    /// ��������! ��������� ���������� �� ������� (�� ����������� ��������)
    /// ������� ����� ������������� ������ ����� �������� � ���������(�� �����������)
    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem,
                   _In_ std::shared_ptr<CDialog> tabDialog, UINT nIDTemplate);

public:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);

private:
    // ����������� ������� �������� ���� � ���������� ������� ����
    void layoutCurrentWindow();

private:
    // ������� ��� ������ �������
    std::map<LONG, std::shared_ptr<CWnd>> m_tabWindows;
};