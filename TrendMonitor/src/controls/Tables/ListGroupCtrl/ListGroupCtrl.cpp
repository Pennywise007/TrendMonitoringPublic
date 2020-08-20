#include "stdafx.h"
#include "ListGroupCtrl.h"

#include <VersionHelpers.h>
#include <vector>
#include <shlwapi.h>
#include "Resource.h"
#include <algorithm>

#include "Controls/ThemeManagement.h"

static CString sExpandAllGroups;
static CString sCallapseAllGroups;

static CString sGroupBy;
static CString sDisableGrouping;

static CString sCheckAll;
static CString sUnCheckALL;

static CString sCheckGroup;
static CString sUnCheckGroup;
static CString sDeleteGroup;

static CString sExpandGroup;
static CString sCollapseGroup;

BEGIN_MESSAGE_MAP(CListGroupCtrl, CListCtrl)
    ON_WM_CONTEXTMENU()	// OnContextMenu
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT_EX(LVN_COLUMNCLICK, OnHeaderClick)	// Column Click
#if _WIN32_WINNT >= 0x0600
    ON_NOTIFY_REFLECT_EX(LVN_LINKCLICK, OnGroupTaskClick)	// Column Click
#endif
    ON_NOTIFY(HDN_ITEMSTATEICONCLICK, 0, &CListGroupCtrl::OnHdnItemStateIconClick)
    ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, OnListItemChanged)    // ��������� � �����
END_MESSAGE_MAP()

CListGroupCtrl::CListGroupCtrl()
    : m_SortCol					(-1)
    , m_Ascending				(false)
    , m_ChangeGroupsWhileSort	(false)
{
#ifdef _TRANSLATE	// ������� ����� <Dialog_ZET/Translate.h>
    sExpandAllGroups	= TranslateString(L"���������� ��� ������");
    sCallapseAllGroups	= TranslateString(L"�������� ��� ������");
    sGroupBy			= TranslateString(L"������������ ��");
    sDisableGrouping	= TranslateString(L"�������� �����������");
    sCheckGroup			= TranslateString(L"������� ��� �������� ������");
    sCheckAll			= TranslateString(L"������� ��� ��������");
    sUnCheckALL			= TranslateString(L"�������� ����� ���� ���������");
    sUnCheckGroup		= TranslateString(L"�������� ����� ���� ��������� ������");
    sDeleteGroup		= TranslateString(L"������� ������");
    sExpandGroup		= TranslateString(L"���������� ������");
    sCollapseGroup		= TranslateString(L"�������� ������");
#else
    sExpandAllGroups	= L"���������� ��� ������";
    sCallapseAllGroups	= L"�������� ��� ������";
    sGroupBy			= L"������������ ��";
    sDisableGrouping	= L"�������� �����������";
    sCheckGroup			= L"������� ��� �������� ������";
    sUnCheckGroup		= L"�������� ����� ���� ��������� ������";
    sDeleteGroup		= L"������� ������";
    sExpandGroup		= L"���������� ������";
    sCollapseGroup		= L"�������� ������";
    sCheckAll			= L"������� ��� ��������";
    sUnCheckALL			= L"�������� ����� ���� ���������";
#endif
}

LRESULT CListGroupCtrl::InsertGroupHeader(_In_ int nIndex, _In_ int nGroupID, _In_ const CString& strHeader,
                                          _In_opt_ DWORD dwState /*= LVGS_NORMAL*/, _In_opt_ DWORD dwAlign /*= LVGA_HEADER_LEFT*/)
{
    EnableGroupView(TRUE);

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.iGroupId = nGroupID;
    lg.state = dwState;
    lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN | LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
    lg.uAlign = dwAlign;

    // Header-title must be unicode (Convert if necessary)
#ifdef UNICODE
    lg.pszHeader = (LPWSTR)(LPCTSTR)strHeader;
    lg.cchHeader = strHeader.GetLength();
#else
    CComBSTR header = strHeader;
    lg.pszHeader = header;
    lg.cchHeader = header.Length();
#endif

    int Res = InsertGroup(nIndex, (PLVGROUP)&lg);

    if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
    {
        SetGroupTask(nGroupID, sCheckGroup);	// Check Group
    }

    return Res;
}

bool CListGroupCtrl::CheckInsertedElement(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem)
{
    bool bRes(true);
    CString NewText(lpszItem);
    // ��������� ��� ������� ������������� �������� ������
    bool bFind(m_bMatchAll || m_sFindStrings.empty());
    for (auto &String : m_sFindStrings)
    {
        bool bFindInColumn = NewText.MakeLower().Find(String.MakeLower()) != -1;

        if (m_bMatchAll)	// ��������� ��� ������� ������� ������ ��������� �� ����� ���������� �������
            bFind &= bFindInColumn;
        else            // ��������� ��� ������� ������� ������ ��������� ������ � ����� ��������� �������
            bFind |= bFindInColumn;
    }

    // ���� ������� �� ������� � ���� ��� �� ���� �� ������� ��
    if (!bFind)
    {
        // ��� �������� ���� �� ������� ������ ������
        LVGROUP Str = { 0 };
        Str.cbSize = sizeof(LVGROUP);
        Str.stateMask = LVM_GETGROUPINFO;
        Str.mask = LVGF_GROUPID;

        LVITEM lvi = { 0 };
        lvi.mask = LVIF_GROUPID | TVIF_PARAM | LVIF_STATE | LVIF_TEXT;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        lvi.iItem = nItem;
        lvi.iSubItem = 0;
        lvi.lParam = nItem;
        lvi.iGroupId = CListCtrl::GetGroupInfo(nGroupID, &Str);
        lvi.pszText = NewText.GetBuffer();
        lvi.cchTextMax = NewText.GetLength();

        // ���� ��������� ������� ����� ������������ �������� ���������� �������� �����
        for (auto & DeletedItem : m_DeletedItems)
        {
            if (DeletedItem.ItemData.iItem >= nItem)
                DeletedItem.ItemData.iItem++;
        }
        // ��������� ����� �������
        m_DeletedItems.push_back({ FALSE, lvi });
        m_DeletedItems.back().ColumnsText.push_back(NewText);

        bRes = false;
    }
    else
    {
        // ���� ��������� ������� ����� ������������ �������� ���������� �������� �����// ��������
        for (auto &DeletedItem : m_DeletedItems)
        {
            if (DeletedItem.ItemData.iItem >= nItem)
                DeletedItem.ItemData.iItem++;
        }
    }

    return bRes;
}

int CListGroupCtrl::InsertItem(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem)
{
    int Res(nItem);
    if (CheckInsertedElement(nItem, nGroupID, lpszItem))
    {
        Res = CListCtrl::InsertItem(nItem, lpszItem);
        SetItemData(Res, nItem);
        SetRowGroupId(Res, nGroupID);

        //Resort();
    }

    return Res;
}
int CListGroupCtrl::InsertItem(_In_ int nItem, _In_z_ LPCTSTR lpszItem)
{
    int Res(nItem);
    if (CheckInsertedElement(nItem, -1, lpszItem))
    {
        int Res = CListCtrl::InsertItem(nItem, lpszItem);
        SetItemData(Res, nItem);

        //Resort();
    }

    return Res;
}

int CListGroupCtrl::InsertColumn(_In_ int nCol, _In_ LPCTSTR lpszColumnHeading, _In_opt_ int nFormat /*= -1*/, _In_opt_ int nWidth /*= -1*/,
                                 _In_opt_ int nSubItem /*= -1*/, _In_opt_ std::function<int(const CString&, const CString&)> SortFunct /*= nullptr*/)
{
    if (SortFunct != nullptr)
        m_SortFunct.push_back(std::make_pair(nCol, SortFunct));

    int Res = CListCtrl::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);

    // ��������� ������ ������� ����� �������
    if (GetExtendedStyle() & LVS_EX_CHECKBOXES && nCol == 0 || nCol == 1)
    {
        CHeaderCtrl *header = GetHeaderCtrl();
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        Header_GetItem(*header, 0, &hdi);
        hdi.fmt |= HDF_CHECKBOX;
        Header_SetItem(*header, 0, &hdi);
    }

    return Res;
}

int CListGroupCtrl::InsertColumn(_In_ int nCol, _In_ const LVCOLUMN* pColumn)
{
    return CListCtrl::InsertColumn(nCol, pColumn);
}

BOOL CListGroupCtrl::SetItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText)
{
    // ���� ����� ��������� ��������� ���� �� ��� ������� ������� �� ����� ��������
    auto it = std::find_if(m_DeletedItems.begin(), m_DeletedItems.end(), [&](const DeletedItemsInfo& Item)
    {
        return Item.ItemData.iItem == nItem;
    });

    if (it == m_DeletedItems.end())
    {
        // ���� �� ����� �� ������������� �����
        return CListCtrl::SetItemText(nItem, nSubItem, lpszText);
    }
    else
    {
        // ������� ����� ������ � ����������� ������� ���������� ��������
        if ((int)it->ColumnsText.size() <= nSubItem)
            it->ColumnsText.resize(nSubItem + 1);
        it->ColumnsText[nSubItem] = lpszText;

        if (bIsNeedToRestoreDeletedItem(it))
            m_DeletedItems.erase(it);
    }

    return TRUE;
}

BOOL CListGroupCtrl::SetRowGroupId(int nRow, int nGroupId)
{
    //OBS! Rows not assigned to a group will not show in group-view
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_GROUPID;
    lvItem.iItem = nRow;
    lvItem.iSubItem = 0;
    lvItem.iGroupId = nGroupId;
    return SetItem(&lvItem);
}

int CListGroupCtrl::GetRowGroupId(int nRow)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_GROUPID;
    lvi.iItem = nRow;
    VERIFY(GetItem(&lvi));
    return lvi.iGroupId;
}

CString CListGroupCtrl::GetGroupHeader(int nGroupId)
{
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.iGroupId = nGroupId;
    lg.mask = LVGF_HEADER | LVGF_GROUPID;
    VERIFY(GetGroupInfo(nGroupId, (PLVGROUP)&lg) != -1);

#ifdef UNICODE
    return lg.pszHeader;
#else
    CComBSTR header(lg.pszHeader);
    return (LPCTSTR)COLE2T(header);
#endif
}

BOOL CListGroupCtrl::IsGroupStateEnabled()
{
    if (!IsGroupViewEnabled())
        return FALSE;

    if (!IsWindowsVistaOrGreater())
        return FALSE;

    return TRUE;
}

// Vista SDK - ListView_GetGroupState / LVM_GETGROUPSTATE
BOOL CListGroupCtrl::HasGroupState(int nGroupId, DWORD dwState)
{
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_STATE;
    lg.stateMask = dwState;
    if (GetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
        return FALSE;

    return lg.state == dwState;
}

// Vista SDK - ListView_SetGroupState / LVM_SETGROUPINFO
BOOL CListGroupCtrl::SetGroupState(int nGroupId, DWORD dwState)
{
    if (!IsGroupStateEnabled())
        return FALSE;

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_STATE;
    lg.state = dwState;
    lg.stateMask = dwState;

#ifdef LVGS_COLLAPSIBLE
    // Maintain LVGS_COLLAPSIBLE state
    if (HasGroupState(nGroupId, LVGS_COLLAPSIBLE))
        lg.state |= LVGS_COLLAPSIBLE;
#endif

    if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
}

int CListGroupCtrl::GroupHitTest(const CPoint& point)
{
    if (!IsGroupViewEnabled())
        return -1;

    if (HitTest(point) != -1)
        return -1;

    if (IsGroupStateEnabled())
    {
        // Running on Vista or newer, but compiled without _WIN32_WINNT >= 0x0600
#ifndef LVM_GETGROUPINFOBYINDEX
#define LVM_GETGROUPINFOBYINDEX   (LVM_FIRST + 153)
#endif
#ifndef LVM_GETGROUPCOUNT
#define LVM_GETGROUPCOUNT         (LVM_FIRST + 152)
#endif
#ifndef LVM_GETGROUPRECT
#define LVM_GETGROUPRECT          (LVM_FIRST + 98)
#endif
#ifndef LVGGR_HEADER
#define LVGGR_HEADER		      (1)
#endif

        LRESULT groupCount = SNDMSG((m_hWnd), LVM_GETGROUPCOUNT, (WPARAM)0, (LPARAM)0);
        if (groupCount <= 0)
            return -1;
        for (int i = 0; i < groupCount; ++i)
        {
            LVGROUP lg = { 0 };
            lg.cbSize = sizeof(lg);
            lg.mask = LVGF_GROUPID;

            VERIFY(SNDMSG((m_hWnd), LVM_GETGROUPINFOBYINDEX, (WPARAM)(i), (LPARAM)(&lg)));

            CRect rect(0, LVGGR_HEADER, 0, 0);
            VERIFY(SNDMSG((m_hWnd), LVM_GETGROUPRECT, (WPARAM)(lg.iGroupId), (LPARAM)(RECT*)(&rect)));

            if (rect.PtInRect(point))
                return lg.iGroupId;
        }
        // Don't try other ways to find the group
        return -1;
    }

    // We require that each group contains atleast one item
    if (GetItemCount() == 0)
        return -1;

    // This logic doesn't support collapsible groups
    int nFirstRow = -1;
    CRect gridRect;
    GetWindowRect(&gridRect);
    for (CPoint pt = point; pt.y < gridRect.bottom; pt.y += 2)
    {
        nFirstRow = HitTest(pt);
        if (nFirstRow != -1)
            break;
    }

    if (nFirstRow == -1)
        return -1;

    int nGroupId = GetRowGroupId(nFirstRow);

    // Extra validation that the above row belongs to a different group
    int nAboveRow = GetNextItem(nFirstRow, LVNI_ABOVE);
    if (nAboveRow != -1 && nGroupId == GetRowGroupId(nAboveRow))
        return -1;

    return nGroupId;
}

void CListGroupCtrl::CheckEntireGroup(int nGroupId, bool bChecked)
{
    if (!(GetExtendedStyle() & LVS_EX_CHECKBOXES))
        return;

    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        if (GetRowGroupId(nRow) == nGroupId)
        {
            SetCheck(nRow, bChecked ? TRUE : FALSE);
        }
    }
}

void CListGroupCtrl::DeleteEntireGroup(int nGroupId)
{
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        if (GetRowGroupId(nRow) == nGroupId)
        {
            DeleteItem(nRow);
            nRow--;
        }
    }
    RemoveGroup(nGroupId);
}

BOOL CListGroupCtrl::GroupByColumn(int nCol)
{
    if (!IsCommonControlsEnabled())
        return FALSE;

    CWaitCursor waitCursor;

    SetSortArrow(-1, false);

    SetRedraw(FALSE);

    RemoveAllGroups();

    EnableGroupView(GetItemCount() > 0);

    if (IsGroupViewEnabled())
    {
        CSimpleMap<CString, CSimpleArray<int> > groups;

        // Loop through all rows and find possible groups
        for (int nRow = 0; nRow < GetItemCount(); ++nRow)
        {
            CString cellText = GetItemText(nRow, nCol);

            int nGroupId = groups.FindKey(cellText);
            if (nGroupId == -1)
            {
                CSimpleArray<int> rows;
                groups.Add(cellText, rows);
                nGroupId = groups.FindKey(cellText);
            }
            groups.GetValueAt(nGroupId).Add(nRow);
        }

        // Look through all groups and assign rows to group
        for (int nGroupId = 0; nGroupId < groups.GetSize(); ++nGroupId)
        {
            const CSimpleArray<int>& groupRows = groups.GetValueAt(nGroupId);
            DWORD dwState = LVGS_NORMAL;

#ifdef LVGS_COLLAPSIBLE
            if (IsGroupStateEnabled())
                dwState = LVGS_COLLAPSIBLE;
#endif

            VERIFY(InsertGroupHeader(nGroupId, nGroupId, groups.GetKeyAt(nGroupId), dwState) != -1);
            SetGroupTask(nGroupId, _T("Task: ") + sCheckGroup);	// Task: Check Group
            CString subtitle;
            subtitle.Format(_T("Subtitle: %i rows"), groupRows.GetSize());
            SetGroupSubtitle(nGroupId, subtitle);
            SetGroupFooter(nGroupId, _T("Group Footer"));

            for (int groupRow = 0; groupRow < groupRows.GetSize(); ++groupRow)
            {
                VERIFY(SetRowGroupId(groupRows[groupRow], nGroupId));
            }
        }
        SetRedraw(TRUE);
        Invalidate(FALSE);
        return TRUE;
    }

    SetRedraw(TRUE);
    Invalidate(FALSE);
    return FALSE;
}

void CListGroupCtrl::CollapseAllGroups()
{
    if (!IsGroupStateEnabled())
        return;

    // Loop through all rows and find possible groups
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        int nGroupId = GetRowGroupId(nRow);
        if (nGroupId != -1)
        {
            if (!HasGroupState(nGroupId, LVGS_COLLAPSED))
            {
                SetGroupState(nGroupId, LVGS_COLLAPSED);
            }
        }
    }
}

void CListGroupCtrl::ExpandAllGroups()
{
    if (!IsGroupStateEnabled())
        return;

    // Loop through all rows and find possible groups
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        int nGroupId = GetRowGroupId(nRow);
        if (nGroupId != -1)
        {
            if (HasGroupState(nGroupId, LVGS_COLLAPSED))
            {
                SetGroupState(nGroupId, LVGS_NORMAL);
            }
        }
    }
}

void CListGroupCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (pWnd == GetHeaderCtrl())
    {
        CPoint pt = point;
        ScreenToClient(&pt);

        HDHITTESTINFO hdhti = { 0 };
        hdhti.pt = pt;
        hdhti.pt.x += GetScrollPos(SB_HORZ);
        ::SendMessage(GetHeaderCtrl()->GetSafeHwnd(), HDM_HITTEST, 0, (LPARAM)&hdhti);
        if (hdhti.iItem != -1)
        {
            // Retrieve column-title
            LVCOLUMN lvc = { 0 };
            lvc.mask = LVCF_TEXT;
            TCHAR sColText[256] = {0};
            lvc.pszText = sColText;
            lvc.cchTextMax = sizeof(sColText) - 1;
            VERIFY(GetColumn(hdhti.iItem, &lvc));

            CMenu menu;
            UINT uFlags = MF_BYPOSITION | MF_STRING;
            VERIFY(menu.CreatePopupMenu());
            if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
            {
                menu.InsertMenu(0, uFlags, 1, sCheckAll);						// Check All
                menu.InsertMenu(1, uFlags, 2, sUnCheckALL);						// UnCheck All
                menu.InsertMenu(2, uFlags | MF_SEPARATOR, 3, _T(""));
            }
            menu.InsertMenu(3, uFlags, 4, sGroupBy + L": " + lvc.pszText);	// Group by:
            if (IsGroupViewEnabled())
                menu.InsertMenu(4, uFlags, 5, sDisableGrouping);			// Disable grouping
            int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
            switch (nResult)
            {
                case 1: CheckElements(true);  break;
                case 2: CheckElements(false); break;
                case 4:	GroupByColumn(hdhti.iItem); break;
                case 5: RemoveAllGroups(); EnableGroupView(FALSE); break;
            }
        }
        return;
    }

    if (IsGroupViewEnabled())
    {
        if (point.x != -1 && point.y != -1)
        {
            CMenu menu;
            UINT uFlags = MF_BYPOSITION | MF_STRING;
            VERIFY(menu.CreatePopupMenu());

            CPoint pt = point;
            ScreenToClient(&pt);
            int nGroupId = GroupHitTest(pt);
            if (nGroupId >= 0)
            {
                const CString& groupHeader = GetGroupHeader(nGroupId);

                if (IsGroupStateEnabled())
                {
                    if (HasGroupState(nGroupId, LVGS_COLLAPSED))
                    {
                        CString menuText = sExpandGroup + L": " + groupHeader;		// Expand group:
                        menu.InsertMenu(0, uFlags, 1, menuText);
                    }
                    else
                    {
                        CString menuText = sCollapseGroup + L": " + groupHeader;	// Collapse group:
                        menu.InsertMenu(0, uFlags, 2, menuText);
                    }
                }
                CString menuText = sCheckGroup + L": " + groupHeader;	// Check group:
                menu.InsertMenu(1, uFlags, 3, menuText);
                menuText = sUnCheckGroup + L": " + groupHeader;			// Uncheck group:
                menu.InsertMenu(2, uFlags, 4, menuText);
                menuText = sDeleteGroup  + L": " + groupHeader;			// Delete group
                menu.InsertMenu(3, uFlags, 5, menuText);

                menu.InsertMenu(4, uFlags | MF_SEPARATOR, 6, _T(""));
            }

            int nRow = HitTest(pt);
            if (nRow == -1)
            {
                if (IsGroupStateEnabled())
                {
                    menu.InsertMenu(5, uFlags, 7, sExpandAllGroups );	// Expand all groups
                    menu.InsertMenu(6, uFlags, 8, sCallapseAllGroups);	// Collapse all groups
                }
                menu.InsertMenu(7, uFlags, 9, sDisableGrouping);		// Disable grouping
            }
            else
            {
                nGroupId = GetRowGroupId(nRow);
                if (IsGroupStateEnabled())
                {
                    const CString& groupHeader = GetGroupHeader(nGroupId);

                    if (HasGroupState(nGroupId, LVGS_COLLAPSED))
                    {
                        CString menuText = sExpandGroup + L": " + groupHeader;		// Expand group:
                        menu.InsertMenu(0, uFlags, 1, menuText);
                    }
                    else
                    {
                        CString menuText = sCollapseGroup + L": " + groupHeader;	// Collapse group:
                        menu.InsertMenu(0, uFlags, 2, menuText);
                    }
                }
            }

            int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
            switch (nResult)
            {
                case 1: SetGroupState(nGroupId, LVGS_NORMAL); break;
                case 2: SetGroupState(nGroupId, LVGS_COLLAPSED); break;
                case 3: CheckEntireGroup(nGroupId, true); break;
                case 4: CheckEntireGroup(nGroupId, false); break;
                case 5: DeleteEntireGroup(nGroupId); break;
                case 7: ExpandAllGroups(); break;
                case 8: CollapseAllGroups(); break;
                case 9: RemoveAllGroups(); EnableGroupView(FALSE); break;
            }
        }
    }
}

namespace {
    struct PARAMSORT
    {
        HWND m_hWnd;
        int  m_ColumnIndex;
        bool m_Ascending;
        CSimpleMap<int, CString> m_GroupNames;
        std::function<int(CString, CString)> m_SortFunct;

        PARAMSORT(HWND hWnd, int nCol, bool bAscending, std::function<int(CString, CString)> Sort = _tcscmp)
            : m_hWnd(hWnd)
            , m_ColumnIndex(nCol)
            , m_Ascending(bAscending)
            , m_SortFunct(Sort)
        {}

        const CString& LookupGroupName(int nGroupId)
        {
            int groupIdx = m_GroupNames.FindKey(nGroupId);
            if (groupIdx == -1)
            {
                static const CString emptyStr;
                return emptyStr;
            }
            return m_GroupNames.GetValueAt(groupIdx);
        }
    };

    // Comparison extracts values from the List-Control
    int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        PARAMSORT& ps = *(PARAMSORT*)lParamSort;

        TCHAR left[256] = _T(""), right[256] = _T("");
        ListView_GetItemText(ps.m_hWnd, lParam1, ps.m_ColumnIndex, left, sizeof(left));
        ListView_GetItemText(ps.m_hWnd, lParam2, ps.m_ColumnIndex, right, sizeof(right));

        if (ps.m_Ascending)
            return ps.m_SortFunct(left, right);
        else
            return ps.m_SortFunct(right, left);
    }

    int CALLBACK SortFuncGroup(int nLeftId, int nRightId, void* lParamSort)
    {
        PARAMSORT& ps = *(PARAMSORT*)lParamSort;

        const CString& left = ps.LookupGroupName(nLeftId);
        const CString& right = ps.LookupGroupName(nRightId);

        if (ps.m_Ascending)
            return _tcscmp(left, right);
        else
            return _tcscmp(right, left);
    }
}

bool CListGroupCtrl::SortColumn(int nCol, bool bAscending)
{
    CWaitCursor waitCursor;

    auto it = std::find_if(m_SortFunct.begin(), m_SortFunct.end(), [&nCol](const std::pair<int, std::function<int(CString, CString)>>& Element)
    {
        return Element.first == nCol;
    });

    PARAMSORT paramsort(m_hWnd, nCol, bAscending);

    if (it != std::end(m_SortFunct))
        paramsort.m_SortFunct = it->second;
    else
        paramsort.m_SortFunct = _tcscmp;

    if (IsGroupViewEnabled())
    {
        SetRedraw(FALSE);

        if (m_ChangeGroupsWhileSort)
            GroupByColumn(nCol);

        // Cannot use GetGroupInfo during sort
        for (int nRow = 0; nRow < GetItemCount(); ++nRow)
        {
            int nGroupId = GetRowGroupId(nRow);
            if (nGroupId != -1 && paramsort.m_GroupNames.FindKey(nGroupId) == -1)
                paramsort.m_GroupNames.Add(nGroupId, GetGroupHeader(nGroupId));
        }

        SetRedraw(TRUE);
        Invalidate(FALSE);

        // Avoid bug in CListCtrl::SortGroups() which differs from ListView_SortGroups
        if (!ListView_SortGroups(m_hWnd, SortFuncGroup, &paramsort))
            return false;

        // ��������� �������� �� ���������
        ListView_SortItemsEx(m_hWnd, SortFunc, &paramsort);
    }
    else
    {
        ListView_SortItemsEx(m_hWnd, SortFunc, &paramsort);
    }

    return true;
}

BOOL CListGroupCtrl::SetGroupFooter(int nGroupID, const CString& footer, DWORD dwAlign /*= LVGA_FOOTER_CENTER*/)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_FOOTER | LVGF_ALIGN;
    lg.uAlign = dwAlign;
#ifdef UNICODE
    lg.pszFooter = (LPWSTR)(LPCTSTR)footer;
    lg.cchFooter = footer.GetLength();
#else
    CComBSTR bstrFooter = footer;
    lg.pszFooter = bstrFooter;
    lg.cchFooter = bstrFooter.Length();
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupTask(int nGroupID, const CString& task)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TASK;
#ifdef UNICODE
    lg.pszTask = (LPWSTR)(LPCTSTR)task;
    lg.cchTask = task.GetLength();
#else
    CComBSTR bstrTask = task;
    lg.pszTask = bstrTask;
    lg.cchTask = bstrTask.Length();
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupSubtitle(int nGroupID, const CString& subtitle)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_SUBTITLE;
#ifdef UNICODE
    lg.pszSubtitle = (LPWSTR)(LPCTSTR)subtitle;
    lg.cchSubtitle = subtitle.GetLength();
#else
    CComBSTR bstrSubtitle = subtitle;
    lg.pszSubtitle = bstrSubtitle;
    lg.cchSubtitle = bstrSubtitle.Length();
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupTitleImage(int nGroupID, int nImage, const CString& topDesc, const CString& bottomDesc)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TITLEIMAGE;
    lg.iTitleImage = nImage;	// Index of the title image in the control imagelist.

#ifdef UNICODE
    if (!topDesc.IsEmpty())
    {
        // Top description is drawn opposite the title image when there is
        // a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
        lg.mask |= LVGF_DESCRIPTIONTOP;
        lg.pszDescriptionTop = (LPWSTR)(LPCTSTR)topDesc;
        lg.cchDescriptionTop = topDesc.GetLength();
    }
    if (!bottomDesc.IsEmpty())
    {
        // Bottom description is drawn under the top description text when there is
        // a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
        lg.mask |= LVGF_DESCRIPTIONBOTTOM;
        lg.pszDescriptionBottom = (LPWSTR)(LPCTSTR)bottomDesc;
        lg.cchDescriptionBottom = bottomDesc.GetLength();
    }
#else
    CComBSTR bstrTopDesc = topDesc;
    CComBSTR bstrBottomDesc = bottomDesc;
    if (!topDesc.IsEmpty())
    {
        lg.mask |= LVGF_DESCRIPTIONTOP;
        lg.pszDescriptionTop = bstrTopDesc;
        lg.cchDescriptionTop = bstrTopDesc.Length();
    }
    if (!bottomDesc.IsEmpty())
    {
        lg.mask |= LVGF_DESCRIPTIONBOTTOM;
        lg.pszDescriptionBottom = bstrBottomDesc;
        lg.cchDescriptionBottom = bstrBottomDesc.Length();
    }
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::OnGroupTaskClick(NMHDR* pNMHDR, LRESULT* pResult)
{
#if _WIN32_WINNT >= 0x0600
    NMLVLINK* pLinkInfo = (NMLVLINK*)pNMHDR;
    int nGroupId = pLinkInfo->iSubItem;

    if (!IsGroupStateEnabled())
        return FALSE;

    // �����
    CString TaskText;
    DWORD Size = max(sCheckGroup.GetLength(), sUnCheckGroup.GetLength()) + 1;

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TASK;
    lg.pszTask = TaskText.GetBuffer(Size);
    lg.cchTask = Size;
    GetGroupInfo(nGroupId, (PLVGROUP)&lg);
    TaskText.ReleaseBuffer();

    // ���� ������ ���� ������� �������������, �� ������� �� ��� �������� �������� �������
    if ((TaskText == sCheckGroup) || (TaskText == sUnCheckGroup))
    {
        if (TaskText == sCheckGroup)
        {
            TaskText = sUnCheckGroup;
            CheckEntireGroup(nGroupId, true);
        }
        else
        {
            TaskText = sCheckGroup;
            CheckEntireGroup(nGroupId, false);
        }

        Size = TaskText.GetLength();
        lg.pszTask = TaskText.GetBuffer(Size);
        lg.cchTask = Size;
        SetGroupInfo(nGroupId, (PLVGROUP)&lg);
    }
    else
        CheckEntireGroup(nGroupId, true);
#endif
    return FALSE;
}

void CListGroupCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CListCtrl::OnLButtonDblClk(nFlags, point);

    if (!IsGroupStateEnabled())
        return;

    int nGroupId = GroupHitTest(point);
}

BOOL CListGroupCtrl::OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLISTVIEW* pLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

    int nCol = pLV->iSubItem;
    if (m_SortCol == nCol)
    {
        m_Ascending = !m_Ascending;
    }
    else
    {
        m_SortCol = nCol;
        m_Ascending = true;
    }

    SortColumn(m_SortCol, m_Ascending);
    SetSortArrow(m_SortCol, m_Ascending);
    return FALSE;	// Let parent-dialog get chance
}

//----------------------------------------------------------------------------//
BOOL CListGroupCtrl::OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // ������������ ��������� ��������� ���� ��������� �� �������� ������
    if ((pNMLV->uChanged & LVIF_STATE) && (CListCtrl::GetExtendedStyle() & LVS_EX_CHECKBOXES))
    {
        // ��������� ����� �� �������� ��������� ��������� ��������
        bool bAllChecked(true);
        // ���� �� ������������ ������, ���� �� ��� ������������� �������
        for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
        {
            if (CListCtrl::GetCheck(Row) == BST_UNCHECKED)
            {
                bAllChecked = false;
                break;
            }
        }

        CHeaderCtrl *header = CListCtrl::GetHeaderCtrl();
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        Header_GetItem(*header, 0, &hdi);
        if (bAllChecked)
            hdi.fmt |= HDF_CHECKED;
        else
            hdi.fmt &= ~HDF_CHECKED;
        Header_SetItem(*header, 0, &hdi);
    }

    return FALSE;	// Let parent-dialog get chance
}

void CListGroupCtrl::Resort()
{
    if (m_SortCol >= 0)
        SortColumn(m_SortCol, m_Ascending);
}

void CListGroupCtrl::SetSortArrow(int colIndex, bool ascending)
{
    // ��� ������� ����������� ������ ���������� ���������� ��������� BMP ����������� � ������ � ����������������:
    // IDB_DOWNARROW ��� ���������� ���� � IDB_UPARROW ��� ���������� �����
#if defined(IDB_DOWNARROW) && defined(IDB_UPARROW)
    UINT bitmapID = m_Ascending ? IDB_DOWNARROW : IDB_UPARROW;
    for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
    {
        HDITEM hditem = { 0 };
        hditem.mask = HDI_BITMAP | HDI_FORMAT;
        VERIFY(GetHeaderCtrl()->GetItem(i, &hditem));
        if (hditem.fmt & HDF_BITMAP && hditem.fmt & HDF_BITMAP_ON_RIGHT)
        {
            if (hditem.hbm)
            {
                DeleteObject(hditem.hbm);
                hditem.hbm = NULL;
            }
            hditem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hditem));
        }
        if (i == colIndex)
        {
            hditem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
            hditem.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
            VERIFY(hditem.hbm != NULL);
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hditem));
        }
    }
#else
    if (IsThemeEnabled())
#if (_WIN32_WINNT >= 0x501)
        for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
        {
            HDITEM hditem = { 0 };
            hditem.mask = HDI_FORMAT;
            VERIFY(GetHeaderCtrl()->GetItem(i, &hditem));
            hditem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (i == colIndex)
            {
                hditem.fmt |= ascending ? HDF_SORTDOWN : HDF_SORTUP;
            }
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hditem));
        }
#endif
#endif
}

void CListGroupCtrl::PreSubclassWindow()
{
    CListCtrl::PreSubclassWindow();

    DWORD extendedListStyle = GetExtendedStyle();
    // Focus retangle is not painted properly without double-buffering
#if (_WIN32_WINNT >= 0x501)
    extendedListStyle |= LVS_EX_DOUBLEBUFFER;
#endif
    extendedListStyle |= LVS_EX_FULLROWSELECT |
        LVS_EX_HEADERDRAGDROP |
        LVS_EX_GRIDLINES |
        LVS_EX_LABELTIP;
    SetExtendedStyle(extendedListStyle);

    // Enable Vista-look if possible
    EnableWindowTheme(GetSafeHwnd(), L"ListView", L"Explorer", NULL);
    EnableWindowTheme(GetHeaderCtrl()->GetSafeHwnd(), L"ListView", L"Explorer", NULL);

    if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
    {
        HWND header = *GetHeaderCtrl();
        DWORD dwHeaderStyle = ::GetWindowLong(header, GWL_STYLE);
        dwHeaderStyle |= HDS_CHECKBOXES;
        ::SetWindowLong(header, GWL_STYLE, dwHeaderStyle);
    }
}

BOOL CListGroupCtrl::ModifyStyle(_In_ DWORD dwRemove, _In_ DWORD dwAdd, _In_opt_ UINT nFlags /*= 0*/)
{
    SetExtendedStyle(GetExtendedStyle() | dwAdd);

    if (dwAdd & LVS_EX_CHECKBOXES)
    {
        HWND header = *GetHeaderCtrl();
        DWORD dwHeaderStyle = ::GetWindowLong(header, GWL_STYLE);
        dwHeaderStyle |= HDS_CHECKBOXES;
        ::SetWindowLong(header, GWL_STYLE, dwHeaderStyle);
    }

    return CListCtrl::ModifyStyle(dwRemove, 0, nFlags);
}

void CListGroupCtrl::AutoScaleColumns()
{
    CRect Rect;
    GetClientRect(Rect);

    int CountColumns = GetHeaderCtrl()->GetItemCount();

    if (CountColumns != 0)
    {
        int Width = Rect.Width() / CountColumns;
        for (int Column = 0; Column < CountColumns; Column++)
            CListCtrl::SetColumnWidth(Column, Width);
    }
}

void CListGroupCtrl::FitToContentColumn(int nCol, bool bIncludeHeaderContent)
{
    int flag = bIncludeHeaderContent ? LVSCW_AUTOSIZE_USEHEADER : LVSCW_AUTOSIZE;
    ListView_SetColumnWidth(m_hWnd, nCol, flag);
}

void CListGroupCtrl::FindItems(_In_ CString sFindString)
{
    std::list<CString> FindVect;
    FindVect.push_back(sFindString);

    FindItems(FindVect);
}

void CListGroupCtrl::FindItems(_In_ std::list<CString> &sFindStrings, _In_opt_ bool bMatchAll /*= false*/)
{
    m_sFindStrings = sFindStrings;
    m_bMatchAll = bMatchAll;

    bool bFind;			// ���� ����, ��� � ������� ���� �������� ��������������� ������
    CString sColumnText;

    // ��������� ������ � ������� ����� ��������������� �� ��������� � �������
    m_DeletedItems.sort([](const DeletedItemsInfo & Item1, const DeletedItemsInfo & Item2) -> bool
    {
        return Item1.ItemData.iItem < Item2.ItemData.iItem;
    });

    // ��������������� ������ ��������� ��������� m_DeletedItems
    for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
    {
        if (bIsNeedToRestoreDeletedItem(it))
            it = m_DeletedItems.erase(it);
        else
            ++it;
    }

    int nTotalColumns = GetHeaderCtrl()->GetItemCount();
    // �������� �� ���� ��������� � ������� ��, ������� �� ������ ��� �������� ������
    for (int Row = CListCtrl::GetItemCount() - 1; Row >= 0; Row--)
    {
        bFind = bMatchAll;
        for (auto &it : sFindStrings)
        {
            if (bMatchAll)// ��������� ��� ������� ������� ������ ��������� �� ����� ���������� �������
                bFind &= FindItemInTable(it, Row);
            else // ��������� ��� ������� ������� ������ ��������� ������ � ����� ��������� �������
                bFind |= FindItemInTable(it, Row);
        }

        // ���� ������� �� ������� � �������������, �� ������� ��
        if (!bFind)
        {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_GROUPID | TVIF_PARAM | LVIF_STATE | LVIF_TEXT;
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.iItem = Row;
            CListCtrl::GetItem(&lvi);

            m_DeletedItems.push_back({ CListCtrl::GetCheck(Row), lvi });
            m_DeletedItems.back().ColumnsText.resize(nTotalColumns);
            for (auto Column = 0; Column < nTotalColumns; ++Column)
            {
                m_DeletedItems.back().ColumnsText[Column] = CListCtrl::GetItemText(Row, Column);
            }

            CListCtrl::DeleteItem(Row);
        }
    }

    Resort();
}

bool CListGroupCtrl::FindItemInTable(_In_ CString psSearchText, _In_ unsigned RowNumber, _In_opt_ bool bCaseSensitive /*= false*/)
{
    unsigned nTotalRows    = CListCtrl::GetItemCount();
    int nTotalColumns = CListCtrl::GetHeaderCtrl()->GetItemCount();

    if (RowNumber < nTotalRows)
    {
        for (int Column = 0; Column < nTotalColumns; ++Column)
        {
            CString sColumnText = CListCtrl::GetItemText(RowNumber, Column);
            if (!bCaseSensitive)
            {
                sColumnText  = sColumnText .MakeLower();
                psSearchText = psSearchText.MakeLower();
            }

            if (sColumnText.Find(psSearchText) != -1)
                return true;
        }
    }

    return false;
}

// BOOL CListGroupCtrl::GetDefaultItemCheck(_In_ int nRow)
// {
// 	if (GetItemData(nRow) == nRow)
// 		return CListCtrl::GetCheck(nRow);
//
// 	// ���� �� ������������ ������, ���� �� ��� ������������� �������
// 	for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
// 	{
// 		if (GetItemData(Row) == nRow)
// 			return CListCtrl::GetCheck(Row);
// 	}
//
// 	// ���� �� ��������� ���������
// 	for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
// 	{
// 		if (it->ItemData.lParam == nRow)
// 			return it->bChecked;
// 	}
//
// 	// ���� ����� ����, �� �������� ������� �������� ������
// 	return CListCtrl::GetCheck(nRow);;
// }

// BOOL CListGroupCtrl::SetDefaultItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText)
// {
// 	// ���� �� ������������ ������, ���� �� ��� ������������� �������
// 	for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
// 	{
// 		if (GetItemData(Row) == nItem)
// 			return CListCtrl::SetItemText(Row, nSubItem, lpszText);
// 	}
//
// 	// ���� �� ��������� ���������
// 	for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
// 	{
// 		if (it->ItemData.lParam == nItem)
// 		{
// 			if ((int)it->ColumnsText.size() > nSubItem)
// 			{
// 				it->ColumnsText[nSubItem] = lpszText;
// 				return TRUE;
// 			}
// 			else
// 				break;;
// 		}
// 	}
//
// 	return CListCtrl::SetItemText(nItem, nSubItem, lpszText);
// }

int CListGroupCtrl::GetCurrentRowIndex(_In_ int nRow)
{
    // ���� �� ������������ ������, ���� �� ��� ������������� �������
    for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
    {
        if (GetItemData(Row) == nRow)
            return Row;
    }

    // ���� �� ��������� ���������
    for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
    {
        if (it->ItemData.lParam == nRow)
            return it->ItemData.iItem;
    }

    return nRow;
}

int CListGroupCtrl::GetDefaultRowIndex(_In_ int nRow)
{
    return (int)GetItemData(nRow);
}

void CListGroupCtrl::CheckAllElements(bool bChecked)
{
    for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end(); ++it)
    {
        it->bChecked = bChecked;
    }

    CheckElements(bChecked);
}

void CListGroupCtrl::CheckElements(bool bChecked)
{
    // ���� �� ������������ ������, ���� �� ��� ������������� �������
    for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
    {
        CListCtrl::SetCheck(Row, bChecked);
    }

    CHeaderCtrl *header = CListCtrl::GetHeaderCtrl();
    HDITEM hdi = { 0 };
    hdi.mask = HDI_FORMAT;
    Header_GetItem(*header, 0, &hdi);
    if (bChecked)
        hdi.fmt |= HDF_CHECKED;
    else
        hdi.fmt &= ~HDF_CHECKED;
    Header_SetItem(*header, 0, &hdi);
}

void CListGroupCtrl::GetCheckedList(_Out_ std::vector<int> &CheckedList, _In_opt_ bool bCurrentIndexes /*= false*/)
{
    CheckedList.clear();
    int nItemsCount = CListCtrl::GetItemCount();

    if (bCurrentIndexes)
    {
        CheckedList.reserve(nItemsCount);

        for (int Row = 0; Row < nItemsCount; ++Row)
        {
            if (GetCheck(Row) != BST_UNCHECKED)
                CheckedList.push_back(Row);
        }
    }
    else
    {
        CheckedList.reserve(nItemsCount + m_DeletedItems.size());

        for (int Row = 0; Row < nItemsCount; ++Row)
        {
            if (GetCheck(Row) != BST_UNCHECKED)
                CheckedList.push_back((int)GetItemData(Row));
        }

        for (auto &it : m_DeletedItems)
        {
            if (it.bChecked)
                CheckedList.push_back((int)it.ItemData.lParam);
        }
    }
}

BOOL CListGroupCtrl::DeleteAllItems()
{
    m_DeletedItems.clear();
    return CListCtrl::DeleteAllItems();
}

void CListGroupCtrl::OnHdnItemStateIconClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMHEADER pNMHeader = (LPNMHEADER)pNMHDR;

    // first determine whether the click was a checkbox change
    if (pNMHeader->pitem->mask & HDI_FORMAT && pNMHeader->pitem->fmt & HDF_CHECKBOX)
    {
        // now determine whether it was checked or unchecked
        BOOL bUnChecked = pNMHeader->pitem->fmt & HDF_CHECKED;

        CheckElements(bUnChecked == FALSE);
    }

    *pResult = 0;
}

bool CListGroupCtrl::bIsNeedToRestoreDeletedItem(std::list<DeletedItemsInfo>::iterator Item)
{
    bool bDelete(false);

    // ��������� ��� ������� ������������� �������� ������
    bool bFind = m_bMatchAll;
    CString sColumnText;
    for (auto &String : m_sFindStrings)
    {
        bool bFindInColumn = false;
        for (auto &CurText : Item->ColumnsText)
        {
            if (CurText.MakeLower().Find(String.MakeLower()) != -1)
            {
                bFindInColumn = true;
                break;
            }
        }

        if (m_bMatchAll)	// ��������� ��� ������� ������� ������ ��������� �� ����� ���������� �������
            bFind &= bFindInColumn;
        else            // ��������� ��� ������� ������� ������ ��������� ������ � ����� ��������� �������
            bFind |= bFindInColumn;
    }

    // ���� ���� ������� ������������� �������� ������ �� ��������������� ���, ���� ��� �� ��������� � ����������
    if (bFind)
    {
        int n;
        for (size_t Column = 0, Column_Count = Item->ColumnsText.size(); Column < Column_Count; Column++)
        {
            if (Column == 0)
            {
                Item->ItemData.cchTextMax = Item->ColumnsText.front().GetLength();
                Item->ItemData.pszText    = Item->ColumnsText.front().GetBuffer();
                n = CListCtrl::InsertItem(&Item->ItemData);
                Item->ColumnsText.front().ReleaseBuffer();
                CListCtrl::SetCheck(n, Item->bChecked);
            }
            else
                CListCtrl::SetItemText(n, (int)Column, Item->ColumnsText[Column]);
        }

        bDelete = true;
    }

    return bDelete;
}
