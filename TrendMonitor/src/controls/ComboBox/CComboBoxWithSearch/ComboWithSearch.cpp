#include "stdafx.h"

#include <assert.h>
#include <regex>
#include <algorithm>

#include "ComboWithSearch.h"

////////////////////////////////////////////////////////////////////////////////
// ���������
// ������������� ������� �������
const UINT_PTR kTextChangeTimerId = 0;

// ����� ����� ������� ������ ��������� ������ �������, ��
const UINT kSearchTime = 200;

BEGIN_MESSAGE_MAP(ComboWithSearch, CComboBox)
    ON_CONTROL_REFLECT_EX(CBN_KILLFOCUS,       &ComboWithSearch::OnCbnKillfocus)
    ON_CONTROL_REFLECT_EX(CBN_SELENDOK,        &ComboWithSearch::OnCbnSelendok)
    ON_CONTROL_REFLECT_EX(CBN_SELENDCANCEL,    &ComboWithSearch::OnCbnSelendcancel)
    ON_CONTROL_REFLECT_EX(CBN_EDITCHANGE,      &ComboWithSearch::OnCbnEditchange)
    ON_WM_TIMER()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// ����� ����������� ������� ���������� �������
class CurSelSaver
{
public:
    CurSelSaver(ISelectionSaver* pSaverClass);
    ~CurSelSaver();

private:
    // ��������� �� ����� � �����������
    ISelectionSaver* m_saverClass;
    // ����������� ���������
    int m_savedRealSel;
};
//----------------------------------------------------------------------------//
LRESULT ComboWithSearch::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case CB_SETCURSEL:
        {
            auto res = BaseCtrl::WindowProc(message, wParam, lParam);

            updateRealCurSelFromCurrent();

            return res;
        }
    }

    return BaseCtrl::WindowProc(message, wParam, lParam);
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnEditchange()
{
    BaseCtrl::KillTimer(kTextChangeTimerId);
    BaseCtrl::SetTimer(kTextChangeTimerId, kSearchTime, nullptr);

    return FALSE;
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnSelendok()
{
    // ���� ��� ��������� ������, �� ���� ������ � ������
    // ������ ��������� �� ������ ������������
    if (BaseCtrl::GetCurSel() == -1 && BaseCtrl::GetCount() > 0)
        BaseCtrl::SetCurSel(1);

    updateRealCurSelFromCurrent();

    // ������� ������, ���������� ��������������� ������ � �������
    resetSearch();

    return FALSE;
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnSelendcancel()
{
    // ������� ������, ���������� ��������������� ������ � �������
    resetSearch();

    // ��������������� ���������� ���������� �������, ������ �������������
    // ������ ��� � ���� ����� ���� ����� �� ��������������� ���������(������ ��������)
    BaseCtrl::SetCurSel(m_realSel);

    return FALSE;
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnKillfocus()
{
    // ������� ������, ���������� ��������������� ������ � �������
    resetSearch();

    return FALSE;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // �� ������� ������� ����� � ������� �����
    if (nChar == VK_ESCAPE && nFlags == 0)
    {
        // ���������� ������ ��������� ���������� ������
        bool dropShow = !!BaseCtrl::GetDroppedState();

        // �������� ������� �����
        CString curWindowText;
        BaseCtrl::GetWindowText(curWindowText);

        // ���� � ��� ������ ���������� ������ � � ��� ���� �����
        if (dropShow && !curWindowText.IsEmpty())
        {
            // ������� ������, ���������� ��������������� ������ � �������
            if (resetSearch())
                adjustComboBoxToContent();

            // ������� �����
            BaseCtrl::SetWindowText(CString());

            return;
        }
    }
    BaseCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

//----------------------------------------------------------------------------//
void ComboWithSearch::executeSearch()
{
    ::SetCursor(LoadCursor(NULL, IDC_WAIT));

    // ���������� ������ ��������� ���������� ������
    bool dropShow = !!BaseCtrl::GetDroppedState();

    // �������� ������� �����
    CString curWindowText;
    BaseCtrl::GetWindowText(curWindowText);

    // �������� ������� ���������
    DWORD curSel = BaseCtrl::GetEditSel();

    bool bChangeInStrings = !curWindowText.IsEmpty();
    {
        // ��������� ��������� ���� ��������
        CurSelSaver selSaver(static_cast<ISelectionSaver*>(this));

        if (!curWindowText.IsEmpty())
        {
            try
            {
                // ���������� ���� ������� �������
                const std::wregex re_regexEscape(_T("[.^$|()\\[\\]{}*+?\\\\]"));
                const std::wstring rep(_T("\\\\&"));
                CString regexStr = std::regex_replace(curWindowText.GetString(),
                                                      re_regexEscape,
                                                      rep,
                                                      std::regex_constants::format_sed |
                                                      std::regex_constants::match_default).c_str();
                // ��������� ������ ������ ��� �������
                regexStr.Replace(L" ", L".*?");

                const std::wregex xRegEx(regexStr, std::regex::icase);

                int existElementsIndex = 0;
                auto currentFilteredElementIt = m_filteredItems.begin();
                // �������� �� ���� ������� � ��������� � ��� ������
                for (int i = 0, count = BaseCtrl::GetCount() + (int)m_filteredItems.size(); i < count; ++i)
                {
                    std::wsmatch xResults;

                    // ���� ������� ������ ��������� � ������ ��������� ���������
                    if (currentFilteredElementIt != m_filteredItems.end() &&
                        i == currentFilteredElementIt->first)
                    {
                        // ��������� ���� �� �������� ��� ������
                        // ���� � ����� �������� ������ ������������� ������ - �������������� �
                        if (std::regex_search(currentFilteredElementIt->second,
                            xResults, xRegEx))
                        {
                            // ���������� ������ � �������
                            BaseCtrl::InsertString(existElementsIndex,
                                                   currentFilteredElementIt->second.c_str());
                            bChangeInStrings = true;
                            // ������� �� �� ������ ���������������
                            currentFilteredElementIt = m_filteredItems.erase(currentFilteredElementIt);

                            existElementsIndex++;
                        }
                        else
                            // ��������� � ���� ��������
                            ++currentFilteredElementIt;
                    }
                    else
                    {
                        // ��������� ������ �� ����������
                        std::wstring str;
                        str.resize(BaseCtrl::GetLBTextLen(existElementsIndex) + 1);
                        BaseCtrl::GetLBText(existElementsIndex,
                                            const_cast<WCHAR*>(str.c_str()));

                        // ���� ��� �� ������������� ������� ���������� �
                        if (!std::regex_search(str, xResults, xRegEx))
                        {
                            // ��������� � ������ ��������
                            assert(m_filteredItems.find(i) == m_filteredItems.end());
                            m_filteredItems[i] = str;

                            // ������� ��������������� ������
                            BaseCtrl::DeleteString(existElementsIndex);

                            bChangeInStrings = true;
                        }
                        else
                            ++existElementsIndex;
                    }
                }
            }
            catch (const std::regex_error& err)
            {
                ::MessageBox(0, CString(err.what()),
                             L"�������� ����������!", MB_ICONERROR | MB_OK);
            }
        }
        else
            // ������� ������, ���������� ��������������� ������ � �������
            bChangeInStrings |= resetSearch();
    }

    if (!dropShow)
    {
        if (BaseCtrl::GetCount())
            // ���� ��� ������ ���������� ������ � �� �� ������ - ���������� ���
            BaseCtrl::ShowDropDown(true);
    }
    else
    {
        // ���� ���� ��������� - ���� ����������� ������ ����������� ������
        if (bChangeInStrings)
            adjustComboBoxToContent();
    }

    // ��������������� ����� � ���������
    BaseCtrl::SetWindowText(curWindowText);
    BaseCtrl::SetEditSel(LOWORD(curSel), HIWORD(curSel));

    ::SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//----------------------------------------------------------------------------//
void ComboWithSearch::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kTextChangeTimerId)
    {
        KillTimer(kTextChangeTimerId);

        executeSearch();
    }

    BaseCtrl::OnTimer(nIDEvent);
}

//----------------------------------------------------------------------------//
bool ComboWithSearch::resetSearch()
{
    if (!m_filteredItems.empty())
    {
        // ��������� ��������� ���� ��������
        CurSelSaver selSaver(static_cast<ISelectionSaver*>(this));

        // ������� ������, ���������� ��������������� ������ � �������
        for (auto &it : m_filteredItems)
        {
            BaseCtrl::InsertString(it.first, it.second.c_str());
        }

        m_filteredItems.clear();

        return true;
    }
    else
        return false;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::updateRealCurSelFromCurrent()
{
    // �������� ������� �������� ���������
    m_realSel = convertToRealIndex(BaseCtrl::GetCurSel());
}

//----------------------------------------------------------------------------//
void ComboWithSearch::adjustComboBoxToContent()
{
    if(!BaseCtrl::GetSafeHwnd() || !::IsWindow(BaseCtrl::GetSafeHwnd()))
    {
        ASSERT(FALSE);
        return;
    }

    // Make sure the drop rect for this combobox is at least tall enough to
    // show 3 items in the dropdown list.
    int nHeight = 0;
    int nItemsToShow = std::min<int>(10, BaseCtrl::GetCount());
    for (int i = 0; i < nItemsToShow; i++)
    {
        int nItemH = BaseCtrl::GetItemHeight(i);
        nHeight += nItemH;
    }

    nHeight += (4 * ::GetSystemMetrics(SM_CYEDGE));

    // Set the height if necessary -- save current size first
    COMBOBOXINFO cmbxInfo;
    cmbxInfo.cbSize = sizeof(COMBOBOXINFO);
    if (BaseCtrl::GetComboBoxInfo(&cmbxInfo))
    {
        CRect rcListBox;
        ::GetWindowRect(cmbxInfo.hwndList, &rcListBox);

        if (rcListBox.Height() != nHeight)
            ::SetWindowPos(cmbxInfo.hwndList, 0, 0, 0, rcListBox.Width(),
                           nHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
    }
}

//----------------------------------------------------------------------------//
int ComboWithSearch::convertToRealIndex(int index)
{
    if (index != -1 && !m_filteredItems.empty())
    {
        // ������������ ������ � ������ ��������� �����
        // ����� ����������� �� ������ ��������� ������ �� �������� �������
        for (const auto &it : m_filteredItems)
        {
            if (it.first <= index)
                ++index;
            else
                break;
        }
    }

    return index;
}

//----------------------------------------------------------------------------//
int ComboWithSearch::convertFromRealIndex(int index)
{
    if (index != -1 && !m_filteredItems.empty())
    {
        int currentElementsIndex = index;
        // ���� ��� ��������� ������ � ������ ��������� �����
        // ����� ��������� �� ������ ��������� ������ �� �������� �������
        for (const auto &it : m_filteredItems)
        {
            if (it.first <= index)
                --currentElementsIndex;
            else
                break;
        }

        return currentElementsIndex;
    }

    return index;
}

//----------------------------------------------------------------------------//
int ComboWithSearch::getRealCurSel()
{
    return m_realSel;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::setRealCurSel(int selection)
{
    BaseCtrl::SetCurSel(convertFromRealIndex(selection));
}

//----------------------------------------------------------------------------//
CurSelSaver::CurSelSaver(ISelectionSaver* pSaverClass)
    : m_saverClass(pSaverClass)
    , m_savedRealSel(m_saverClass->getRealCurSel())
{}

//----------------------------------------------------------------------------//
CurSelSaver::~CurSelSaver()
{
    m_saverClass->setRealCurSel(m_savedRealSel);
}