#pragma once
#include <afxwin.h>
#include <map>

////////////////////////////////////////////////////////////////////////////////
// ��������������� ��������� ���������� ��������� � ����������
interface ISelectionSaver
{
    virtual ~ISelectionSaver() = default;

    // �������� ������� ���������
    virtual int  getRealCurSel() = 0;
    // ������������� ������� ���������
    virtual void setRealCurSel(int selection) = 0;
};

////////////////////////////////////////////////////////////////////////////////
class ComboWithSearch :
    public CComboBox,
    public ISelectionSaver
{
    typedef CComboBox BaseCtrl;
public:
    ComboWithSearch() = default;

protected:
    DECLARE_MESSAGE_MAP();

    // for processing Windows messages
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

// ISelectionSaver
public:
    // �������� ������� ���������
    int  getRealCurSel() override;
    // ������������� ������� ���������
    void setRealCurSel(int selection) override;

public:
    // ��������� �� ��������� ������
    BOOL OnCbnEditchange();
    // ��������� � ������ �������� ��������
    BOOL OnCbnSelendok();
    // ��������� �� ������ ������
    BOOL OnCbnSelendcancel();
    // ���������� � �������� ������ � �������
    BOOL OnCbnKillfocus();
    // ������
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    // ������� ������
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
    // ��������� �����
    void executeSearch();
    // ���������� ������
    bool resetSearch();
    // ��������� ������� ������� ���������� �������
    void updateRealCurSelFromCurrent();
    // ���������� ���������� ������ � �������� ����������
    void adjustComboBoxToContent();

private:    // ������� ����������� ��������
    // �� �������� ������� � �������� ������
    int convertToRealIndex(int index);
    // �� ��������� ������� � �������
    int convertFromRealIndex(int index);

private:
    // ��������������� ������
    //      ������ | �����
    std::map<int, std::wstring> m_filteredItems;

    // �������� �������� ����������� ��������, � ������ ��������� �����
    int m_realSel = -1;
};