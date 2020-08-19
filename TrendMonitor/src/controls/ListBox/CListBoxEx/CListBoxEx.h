#pragma once

// ����� ������ ��� CListBox, ��������� ������������� ����� ��������� � �������� �� � ��������� �����
#include <afxwin.h>

////////////////////////////////////////////////////////////////////////////////
// CListBoxEx
class CListBoxEx : public CListBox
{
public:
    CListBoxEx()
    {

    }
    // �������� ������� � �������� �����
    // ���� nIndex == -1 �������� AddString ����� - ����������� �� ���������� �������
    int AddItem(const CString& itemText, COLORREF color, int nIndex = -1);

    DECLARE_MESSAGE_MAP()

protected:
    virtual void PreSubclassWindow() override;
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
    afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
};
