#pragma once

#include <assert.h>

#include "ListEditSubItems.h"

////////////////////////////////////////////////////////////////////////////////
// ����������� ����� ��� ���� �������������� ������
class IEditSubItemWindow : public CDialogEx
{
public:
    // ��������� ����� �������� ���� ��� �������������� ������
    static std::unique_ptr<IEditSubItemWindow> createWindow(
        const CRect& rect, LVSubItemParams::Ptr subitemParams);

public:
    // ������������� ���������� ������� �������������� ��� ��������������
    virtual void setInternalControl(std::shared_ptr<CWnd> pControlWnd) = 0;
    // �������� ���������� ������� �������������� ��� ��������������
    virtual CWnd* getInternalControl() = 0;
    // �������� ���������� �� ������������� ������
    virtual LVSubItemParams* getSubItemParams() = 0;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� �� ��������� �������������� ������
// lParam - ��������� �������� ���� ������������� ������ IDOK/IDCANCEL
#define WM_END_EDIT_SUB_ITEM WM_USER + 200

////////////////////////////////////////////////////////////////////////////////
// ����� � ������������ ������������� ������ ������
BEGIN_TEMPLATE_MESSAGE_MAP(CListEditSubItems, CBaseList, CBaseList)
    ON_WM_LBUTTONDBLCLK()
    ON_MESSAGE(WM_END_EDIT_SUB_ITEM, &CListEditSubItems::OnEndEditSubItem)
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
template <typename CBaseList>
void CListEditSubItems<CBaseList>::
setSubItemEditorController(int iSubItem, std::shared_ptr<ISubItemEditorController> controller)
{
    if (controller)
        m_subItemsEditorControllers[iSubItem] = controller;
    else
    {
        auto existedIt = m_subItemsEditorControllers.find(iSubItem);
        if (existedIt != m_subItemsEditorControllers.end())
            m_subItemsEditorControllers.erase(existedIt);
    }
}

//----------------------------------------------------------------------------//
template<typename CBaseList>
inline std::shared_ptr<ISubItemEditorController>
CListEditSubItems<CBaseList>::getSubItemEditorController(int iSubItem)
{
    std::shared_ptr<ISubItemEditorController> editorController;
    // ��������� ����� �� ���������� ��� ���� �������
    auto editorControllerIt = m_subItemsEditorControllers.find(iSubItem);
    if (editorControllerIt != m_subItemsEditorControllers.end())
    {
        // ���� ������� ��� ������� �� ������������� - �������
        if (!editorControllerIt->second)
            return nullptr;

        editorController = editorControllerIt->second;
    }
    else
        // ���������� ������� �� ���������
        editorController = std::make_shared<SubItemEditorControllerBase>();

    assert(editorController.get()); // �� ���������?

    return editorController;
}

//----------------------------------------------------------------------------//
template<typename CBaseList>
inline void CListEditSubItems<CBaseList>::setSubItemEditorController(int iSubItem,
    const createEditorControlFunc& createFunc, const onEndEditSubItemFunc& endEditFunc)
{
    class SubItemEditorControllerCustom : public SubItemEditorControllerBase
    {
    public:
        SubItemEditorControllerCustom(const createEditorControlFunc& createFunc, const onEndEditSubItemFunc& endEditFunc)
            : m_createFunc(createFunc), m_endEditFunc(endEditFunc)
        {}

    // ISubItemEditorController
    public:
        // ������� ������� ��� ��������������, ������� ���� ����� ��� �������������� ������
        std::shared_ptr<CWnd> createEditorControl(CListCtrl* pList,
                                                  CWnd* parentWindow,
                                                  const LVSubItemParams* pParams) override
        {
            if (m_createFunc)
                return m_createFunc(pList, parentWindow, pParams);
            else
                return SubItemEditorControllerBase::createEditorControl(pList, parentWindow, pParams);
        }

        // ���������� ��� ��������� �������������� ������
        void onEndEditSubItem(CListCtrl* pList,
                              CWnd* editorControl,
                              const LVSubItemParams* pParams,
                              bool bAcceptResult) override
        {
            if (m_endEditFunc)
                m_endEditFunc(pList, editorControl, pParams, bAcceptResult);
            else
                SubItemEditorControllerBase::onEndEditSubItem(pList, editorControl, pParams, bAcceptResult);
        }

    public:
        createEditorControlFunc m_createFunc = nullptr;
        onEndEditSubItemFunc m_endEditFunc = nullptr;
    };

    setSubItemEditorController(iSubItem, std::make_shared<SubItemEditorControllerCustom>(createFunc, endEditFunc));
}

//----------------------------------------------------------------------------//
template <typename CBaseList>
void CListEditSubItems<CBaseList>::PreSubclassWindow()
{
    CBaseList::PreSubclassWindow();

    // ������� �������� �������������� ����� ������ ��� �� �������� ������ ��� ������ �������
    CBaseList::ModifyStyle(LVS_EDITLABELS, 0);

    DWORD extendedListStyle = CBaseList::GetExtendedStyle();
    // Focus retangle is not painted properly without double-buffering
#if (_WIN32_WINNT >= 0x501)
    extendedListStyle |= LVS_EX_DOUBLEBUFFER;
#endif
    extendedListStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP;
    CBaseList::SetExtendedStyle(extendedListStyle);
}

//----------------------------------------------------------------------------//
template <typename CBaseList>
afx_msg void CListEditSubItems<CBaseList>::OnLButtonDblClk(UINT nFlags,
                                                           CPoint point)
{
    CBaseList::OnLButtonDblClk(nFlags, point);
    // CMT: Select the item the user clicked on.
    UINT uFlags;
    int nItem = CBaseList::HitTest(point, &uFlags);

    // CMT: If the click has been made on some valid item
    if (-1 != nItem)
    {
        DWORD_PTR dwData = CBaseList::GetItemData(nItem);

        if (uFlags & LVHT_ONITEMLABEL)
        {
            LVHITTESTINFO hti;
            hti.pt = point;
            hti.flags = LVM_SUBITEMHITTEST;

            CBaseList::SubItemHitTest(&hti);
            int nSubItem = hti.iSubItem;

            LVITEMINDEX htItemIndex;
            htItemIndex.iItem = hti.iItem;
            htItemIndex.iGroup = hti.iGroup;

            // �������� ������ ������
            CRect htItemRect;
            if (!CBaseList::GetItemIndexRect(&htItemIndex, hti.iSubItem,
                                             LVIR_LABEL, htItemRect))
            {
                assert(false);  // �� ������ �������� ������� ���� ��� ����
                return;
            }
            CBaseList::ClientToScreen(htItemRect);

            if (m_editSubItemWindow)
            {
                assert(false);   // ���������� ������ ���� ��� ����������
                m_editSubItemWindow.reset();
            }

            // �������� ������� ��� ������
            std::shared_ptr<ISubItemEditorController> editorController =
                getSubItemEditorController(hti.iSubItem);
            if (!editorController)
                return;

            LVSubItemParams::Ptr params =
                std::make_shared<LVSubItemParams>(hti.iItem, hti.iSubItem, hti.iGroup);

            // ������� ���� �������������� ���������
            m_editSubItemWindow.swap(IEditSubItemWindow::createWindow(htItemRect, params));
            if (!::IsWindow(m_editSubItemWindow->m_hWnd))
            {
                assert(false);
                m_editSubItemWindow.reset();
                return;
            }

            // ������� �������
            std::shared_ptr<CWnd> editorControl =
                editorController->createEditorControl(this,
                                                      m_editSubItemWindow.get(),
                                                      params.get());
            if (!editorControl || !::IsWindow(editorControl->m_hWnd))
            {
                m_editSubItemWindow.reset();
                return;
            }

            // ����������� ��������� ����� ��������� ��� ���
            m_editSubItemWindow->SetOwner(this);

            // ����������� ����� �� �������
            editorControl->SetFont(CBaseList::GetFont());

            // ������ ������� �� �����
            CRect editControlRect;
            m_editSubItemWindow->GetClientRect(editControlRect);
            editorControl->MoveWindow(editControlRect);

            // �������� ���� �����
            ::PostMessage(editorControl->m_hWnd, EM_SETSEL, 0, editorControl->GetWindowTextLengthW());

            // �������� ������ ���� �������������� � ����������� � ���� ��������
            m_editSubItemWindow->setInternalControl(editorControl);
        }
    }
}

//----------------------------------------------------------------------------//
template <typename CBaseList>
afx_msg LRESULT CListEditSubItems<CBaseList>::OnEndEditSubItem(WPARAM wParam,
                                                               LPARAM lParam)
{
    // �� ����� �������� ���� ����� OnOk ��� OnCancel ����� ������ ���������� ��� ��� �� OnActivate
    // ��-����� ����� ���� ���� �� ������������� ����� ������� � ������� ���� ���� ��� ���
    decltype(m_editSubItemWindow) editWindow;
    editWindow.swap(m_editSubItemWindow);
    if (!editWindow)
        return -1;

    bool bAcceptResult = false;
    switch (wParam)
    {
        case IDOK:
            bAcceptResult = true;
            break;
        case IDCANCEL:
            break;
        default:
            assert(false);  // ����������� ����
            break;
    }

    CWnd* internalControl = editWindow->getInternalControl();
    LVSubItemParams* subItemParams = editWindow->getSubItemParams();
    if (internalControl && subItemParams)
    {
        // �������� ������� �� ��������� ��������������
        getSubItemEditorController(subItemParams->iSubItem)->onEndEditSubItem(
            this, internalControl, subItemParams, bAcceptResult);
    }
    else
        assert(false);

    editWindow->DestroyWindow();

    return 0;
}