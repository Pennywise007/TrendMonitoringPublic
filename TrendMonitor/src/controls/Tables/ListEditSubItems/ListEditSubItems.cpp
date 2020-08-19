#include "stdafx.h"

#include <cassert>

#include "ListEditSubItems.h"

////////////////////////////////////////////////////////////////////////////////
// ���� ��� �������������� ������ ������
class CEditSubItemWindow : public IEditSubItemWindow
{
public:
    CEditSubItemWindow(const CRect& rect, LVSubItemParams::Ptr subitemParams);

// IEditSubItemWindow
public:
    // ������������� ���������� ������� �������������� ��� ��������������
    void setInternalControl(std::shared_ptr<CWnd> pControlWnd) override;
    // �������� ���������� ������� �������������� ��� ��������������
    CWnd* getInternalControl() override;
    // �������� ���������� �� ������������� ������
    LVSubItemParams* getSubItemParams() override;

protected:
    DECLARE_MESSAGE_MAP()

    virtual void OnOK();
    virtual void OnCancel();
protected:
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnDestroy();

private:
    // ������� ����
    void createWindow(const CRect& rect);

private:
    // ����������� � ���� �������
    std::shared_ptr<CWnd> m_internalControl;
    LVSubItemParams::Ptr m_subitemParams;
};

////////////////////////////////////////////////////////////////////////////////
// ������� ���� ��� �������������� ������ ������
std::unique_ptr<IEditSubItemWindow> IEditSubItemWindow::createWindow(
    const CRect& rect, LVSubItemParams::Ptr subitemParams)
{
    return std::make_unique<CEditSubItemWindow>(rect, subitemParams);
}

BEGIN_MESSAGE_MAP(CEditSubItemWindow, CDialogEx)
    ON_WM_ACTIVATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
CEditSubItemWindow::CEditSubItemWindow(const CRect& rect, LVSubItemParams::Ptr subitemParams)
    : m_subitemParams(subitemParams)
{
    HINSTANCE instance = AfxGetInstanceHandle();
    CString editLabelClassName(typeid(*this).name());

    // ������������ ��� ����
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, editLabelClassName, &wndClass))
    {
        // ����������� ������ ���� ������� ������������ ��� �������������� �����
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = editLabelClassName;

        if (!RegisterClassEx(&wndClass))
            ::MessageBox(NULL, L"Can`t register class", L"Error", MB_OK);
    }

    // ������� ����
    createWindow(rect);
}

//----------------------------------------------------------------------------//
void CEditSubItemWindow::createWindow(const CRect& rect)
{
    // ������� ����
    if (CDialogEx::CreateEx(0, CString(typeid(*this).name()), L"",
                            WS_VISIBLE,
                            0, 0, 0, 0,
                            NULL, nullptr, nullptr) == FALSE)
    {
        assert(false);
        return;
    }

    // ������� ���������
    CDialogEx::ModifyStyle(WS_CAPTION, 0, SWP_FRAMECHANGED);

    // ����� �������� ��������� ����������� ����� ��������� ����
    CDialogEx::MoveWindow(rect);
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
void CEditSubItemWindow::setInternalControl(std::shared_ptr<CWnd> pControlWnd)
{
    m_internalControl = pControlWnd;

    ::SetFocus(m_internalControl->m_hWnd);
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
CWnd* CEditSubItemWindow::getInternalControl()
{
    return m_internalControl.get();
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
LVSubItemParams* CEditSubItemWindow::getSubItemParams()
{
    return m_subitemParams.get();
}

//----------------------------------------------------------------------------//
void CEditSubItemWindow::OnOK()
{
    CWnd* ownerWnd = CDialogEx::GetOwner();
    if (ownerWnd && ::IsWindow(ownerWnd->m_hWnd))
        ::SendMessage(ownerWnd->m_hWnd, WM_END_EDIT_SUB_ITEM, IDOK, 0);

    if (::IsWindow(m_hWnd))
        CDialogEx::OnOK();
}

//----------------------------------------------------------------------------//
void CEditSubItemWindow::OnCancel()
{
    CWnd* ownerWnd = CDialogEx::GetOwner();
    if (ownerWnd && ::IsWindow(ownerWnd->m_hWnd))
        ::SendMessage(ownerWnd->m_hWnd, WM_END_EDIT_SUB_ITEM, IDCANCEL, 0);

    if (::IsWindow(m_hWnd))
        CDialogEx::OnCancel();
}

//----------------------------------------------------------------------------//
void CEditSubItemWindow::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CDialogEx::OnActivate(nState, pWndOther, bMinimized);

    // ���� ���� �������� ���������� - �����������
    if (nState == 0)
        OnOK();
}

//----------------------------------------------------------------------------//
void CEditSubItemWindow::OnDestroy()
{
    CDialogEx::OnDestroy();

    if (m_internalControl && ::IsWindow(m_internalControl->m_hWnd))
        m_internalControl->DestroyWindow();
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<CWnd>
SubItemEditorControllerBase::createEditorControl(CListCtrl* pList,
                                                 CWnd* parentWindow,
                                                 const LVSubItemParams* pParams)
{
    CEdit* editWnd = new CEdit;
    editWnd->Create(getStandartEditorWndStyle() | ES_AUTOHSCROLL,
                    CRect(), parentWindow, 0);
    // ������ ���������� ������� ����� ��� ������ � �������
    SetWindowLongPtr(editWnd->m_hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE);

    // ������ �����
    editWnd->SetWindowTextW(pList->GetItemText(pParams->iItem, pParams->iSubItem));

    return std::shared_ptr<CWnd>(editWnd);
}

//----------------------------------------------------------------------------//
void SubItemEditorControllerBase::onEndEditSubItem(CListCtrl* pList, CWnd* editorControl,
                                                   const LVSubItemParams* pParams,
                                                   bool bAcceptResult)
{
    if (!bAcceptResult)
        return;

    CString currentEditorText;
    editorControl->GetWindowTextW(currentEditorText);

    pList->SetItemText(pParams->iItem, pParams->iSubItem, currentEditorText);
}
