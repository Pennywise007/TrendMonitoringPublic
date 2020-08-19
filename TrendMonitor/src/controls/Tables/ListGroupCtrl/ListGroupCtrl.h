#pragma once
#include "afxcmn.h"
#include <list>
#include <memory>
#include <vector>
#include <functional>
//*************************************************************************************************
/*  �������
    1) ��� ����������� ��������, �������� � stdafx.h ����������� ���������� ��������
    2) ��� ������� ����������� ������ ���������� ���������� ��������� BMP ����������� � ������ � ����������������:
        IDB_DOWNARROW ��� ���������� ���� � IDB_UPARROW ��� ���������� �����
    3) ��� ������ � ���������� ������� ����� ��������, ��� ���� ����� ������ ����� ������ ��� ���������� � ������
        ����������� GetDefaultRowIndex ����� � ��� ������� ������, ��� ���� ����� ������ ����� ������ ������ � ��������
        ������� ��� �������� ����������� GetCurrentRowIndex ����� � ��� �������� ������
    3) ��� ����� ������� lparam �������� �������� ������� ������� ��� ��� ���������� �� � ������
        � iItem ������� ������ �������
    4) ���� ���������� ���� ������ � ��������� LVS_EX_CHECKBOXES � ���� ������������� ������������ �������� �������,
        �� � ������� ����������� OnLvnItemchanged  ���������� ������� ������� OnCheckItem

*/
//*************************************************************************************************
/* ������ ������ � �������:

    // ��������� �������
    m_Tree.ModifyStyle(0, LVS_EX_CHECKBOXES);
    // ��������� �������
    m_Tree.InsertColumn(1, L"�������� �������", LVCFMT_CENTER, 100);

    int GroupID = 0;
    int CurIndex = 0;
������ 1:
    m_Tree.InsertGroupHeader(0, GroupID++, GroupName, LVGS_COLLAPSIBLE | LVGS_COLLAPSED, LVGA_HEADER_LEFT);
    m_Tree.InsertItem(CurIndex++, GroupID, Name1);
    m_Tree.InsertItem(CurIndex++, GroupID, Name2);
������ 2:
    m_Tree.InsertItem(CurIndex++, Name1);
    m_Tree.InsertItem(CurIndex++, Name2);
    m_Tree.GroupByColumn(0);
���������� ������� ��� ���������� �������:
    m_DeviceTree.InsertColumn(1, TranslateString(L"��������� �� �����"), LVCFMT_CENTER, 100, -1, SortDateFunct);
������� ����������
    int SortDateFunct(CString First, CString Second)
    {
        CZetTime ztFirst;
        ztFirst.ConvertFromString(First.GetBuffer(), L"dd-MM-yyyy HH:mm:ss");
        CZetTime ztSecond;
        ztSecond.ConvertFromString(Second.GetBuffer(), L"dd-MM-yyyy HH:mm:ss");

        if (ztFirst == ztSecond)
            return 0;
        else if (ztFirst < ztSecond)
            return -1;
        else
            return 1;
    }
*/
//*************************************************************************************************
class CListGroupCtrl : public CListCtrl
{
public:	//*****************************************************************************************
    // ���� true - �� ��� ���������� ���������� ����������� � ������ �� ������ �������
    // ���� false - �� ������ �������������
    bool m_ChangeGroupsWhileSort;
public:	//*****************************************************************************************
    CListGroupCtrl();
    //*********************************************************************************************
    // ������� ������
    // Parameter: nIndex    - ������ ������ � ������, ����� ������ ��������
    // Parameter: nGroupID  - ������ ������
    // Parameter: strHeader - �������� ������
    // Parameter: dwState	- ��������� ������ �������� LVGS_COLLAPSIBLE | LVGS_COLLAPSED
    // Parameter: dwAlign	- �������� �������� ������ ������������ ����� ��������
    LRESULT InsertGroupHeader(_In_ int nIndex, _In_ int nGroupID, _In_ const CString& strHeader,
                              _In_opt_ DWORD dwState = LVGS_NORMAL, _In_opt_ DWORD dwAlign = LVGA_HEADER_LEFT);
    //*********************************************************************************************
    // ���������� ������� � ������� � ��������� � ������
    // Returns:   int		- ������ ������������ ��������
    // Parameter: nItem		- ������ �������� � �������
    // Parameter: nGroupID	- ������ ������ � ������� ����������� �������
    // Parameter: lpszItem	- �������� ��������
    int InsertItem(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem);
    // ����������� ���������� ��������
    int InsertItem(_In_ int nItem, _In_z_ LPCTSTR lpszItem);
    //*********************************************************************************************
    // ���������� ������� � ���������� �� ������� ����������
    // SortFunct - ������� ����������, ������ ���������� ����� � ��������� 2 ������ ���� CString
    // ������������ �������� ������� ����������:
    // <0 string1 is less than string2
    // 0  string1 is identical to string2
    // >0 string1 is greater than string2
    int InsertColumn(_In_ int nCol, _In_ LPCTSTR lpszColumnHeading, _In_opt_ int nFormat = -1, _In_opt_ int nWidth = -1,
                     _In_opt_ int nSubItem = -1, _In_opt_ std::function<int(const CString&, const CString&)> SortFunct = nullptr);
    // ����������� ���������� �������
    int InsertColumn(_In_ int nCol, _In_ const LVCOLUMN* pColumn);
    //******************************************`***************************************************
// 	BOOL SetDefaultItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText);
// 	// ���������� ������ ������� ������ ��� ���
// 	BOOL GetDefaultItemCheck(_In_ int nRow);
    //*********************************************************************************************
    // ������ ����� ������� LVS_EX_CHECKBOXES - ��������
    BOOL ModifyStyle(_In_ DWORD dwRemove, _In_ DWORD dwAdd, _In_opt_ UINT nFlags = 0);
    //*********************************************************************************************
    // ������������������� ������� � ����������� �� �������� ��������, ������� ���������� ����������� �������
    void AutoScaleColumns();
    // ������������������� ������� �� �����������
    void FitToContentColumn(int nCol, bool bIncludeHeaderContent);
    //*********************************************************************************************
    // ����� �� �������
    void FindItems(_In_ CString sFindString);
    // bMatchAll - ���� true �� ���� ���������� �� ����� ���������� �������, ���� false - �� ������ � �����
    void FindItems(_In_ std::list<CString> &sFindStrings, _In_opt_ bool bMatchAll = false);
    //*********************************************************************************************
    // ���������� ������� ������ � ����� ������������ ��������
    int GetCurrentRowIndex(_In_ int nRow);
    // ���������� ����������� ������ �� �������� �������
    int GetDefaultRowIndex(_In_ int nRow);
    //*********************************************************************************************
    // �������� ������ ��������� ���������
    // bCurrentIndexes - true �� ������������ ������� �������� | false - ������� ������� ���� ��� ���������� ������
    void GetCheckedList(_Out_ std::vector<int> &CheckedList, _In_opt_ bool bCurrentIndexes = false);
    //*********************************************************************************************
    CString GetGroupHeader(int nGroupID);
    int GetRowGroupId(int nRow);
    BOOL SetRowGroupId(int nRow, int nGroupID);
    int GroupHitTest(const CPoint& point);
    //*********************************************************************************************
    // ����������� ����������� ������� �� ��������� �����
    BOOL GroupByColumn(int nCol);
    void DeleteEntireGroup(int nGroupId);
    BOOL HasGroupState(int nGroupID, DWORD dwState);
    BOOL SetGroupState(int nGroupID, DWORD dwState);
    BOOL IsGroupStateEnabled();
    //*********************************************************************************************
    // ��� �������� ������� ����� ������� � ����������� �� bCkeched
    void CheckAllElements(bool bChecked);
    void CheckEntireGroup(int nGroupId, bool bChecked);
    //*********************************************************************************************
    // ���������� �������
    bool SortColumn(int columnIndex, bool ascending);
    void SetSortArrow(int colIndex, bool ascending);
    void Resort();
    //*********************************************************************************************
    void CollapseAllGroups();	// �������� ��� ������
    void ExpandAllGroups();		// ���������� ��� ������
    //*********************************************************************************************
    // Removes all items from the control.
    BOOL DeleteAllItems();
    //*********************************************************************************************
    // Sets the text associated with a particular item.
    BOOL SetItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText);
    //*********************************************************************************************
    BOOL SetGroupFooter(int nGroupID, const CString& footer, DWORD dwAlign = LVGA_FOOTER_CENTER);
    BOOL SetGroupTask(int nGroupID, const CString& task);
    BOOL SetGroupSubtitle(int nGroupID, const CString& subtitle);
    BOOL SetGroupTitleImage(int nGroupID, int nImage, const CString& topDesc, const CString& bottomDesc);
public://******************************************************************************************
    // ���� ��������� ����������
    bool Ascending() const		{ return m_Ascending; }
    void Ascending(bool val)	{ m_Ascending = val; }
    // ����� ������� ��� ����������
    int  SortCol() const		{ return m_SortCol; }
    void SortCol(int val)		{ m_SortCol = val; }
protected://***************************************************************************************
    virtual void PreSubclassWindow() override;
    //*********************************************************************************************
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnGroupTaskClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnHdnItemStateIconClick(NMHDR *pNMHDR, LRESULT *pResult);
    //*********************************************************************************************
    DECLARE_MESSAGE_MAP()
protected://***************************************************************************************
    int m_SortCol;			// ������� ��� ����������
    bool m_Ascending;		// �������� ����������
    //*********************************************************************************************
    struct DeletedItemsInfo
    {
        LVITEM ItemData;
        BOOL bChecked;
        std::vector<CString> ColumnsText;
        DeletedItemsInfo()
            : bChecked		(TRUE)
        {
            ZeroMemory(&ItemData, sizeof(LVITEM));
        }
        DeletedItemsInfo(_In_ const DeletedItemsInfo &Val)
            : bChecked		(Val.bChecked)
            , ItemData		(Val.ItemData)
            , ColumnsText	(Val.ColumnsText)
        {
            int asd = 0;

        }
        DeletedItemsInfo(_In_ BOOL _bChecked, _In_ LVITEM _ItemData)
            : bChecked		(_bChecked)
            , ItemData		(_ItemData)
        {}
        DeletedItemsInfo & operator = (_In_ const DeletedItemsInfo &Val)
        {
            ItemData = Val.ItemData;
            bChecked = Val.bChecked;
            ColumnsText = Val.ColumnsText;
            return *this;
        }
    };
    std::list<DeletedItemsInfo> m_DeletedItems;
    bool m_bMatchAll;
    std::list<CString> m_sFindStrings;
    // ������ ������� ��� ���������� ������ �������
    std::vector<std::pair<int, std::function<int(CString, CString)>>> m_SortFunct;
protected://***************************************************************************************
    // ����� ������ � �������� ������ �������
    bool FindItemInTable(_In_ CString psSearchText, _In_ unsigned RowNumber, _In_opt_ bool bCaseSensitive = false);
    // �������� �� ������������� ������������ ��������� �������
    bool bIsNeedToRestoreDeletedItem(std::list<DeletedItemsInfo>::iterator Item);
    void CheckElements(bool bChecked);
    //*********************************************************************************************
    // ��������� ����������� �������
    bool CheckInsertedElement(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem);
};	//*********************************************************************************************
