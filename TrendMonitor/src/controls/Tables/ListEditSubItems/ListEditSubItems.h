#pragma once
#include "afxcmn.h"

#include <map>
#include <memory>
#include <functional>

////////////////////////////////////////////////////////////////////////////////
// ��������� � ����������� ������������� ������ �����
struct LVSubItemParams
{
    // ���������� ��������� � ��������� �� ���������
    typedef std::shared_ptr<LVSubItemParams> Ptr;

    LVSubItemParams(int iItem, int iSubItem, int iGroup)
        : iItem(iItem), iSubItem(iSubItem), iGroup(iGroup) {};

    // ��������� �������������� ��������
    int iItem;
    int iSubItem;
    int iGroup;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ����������� ��� �������������� ��������� � �������
interface ISubItemEditorController;

////////////////////////////////////////////////////////////////////////////////
/*
    CListEditSubItems - ������� ��� �������� ������������ �� CListCtrl ��� �������������� �����
    �� ��������� ��� ������� ����� ��������������� ����� CEdit

    ����� ������ ��������� ����������� ��� �������������� ����� ����� setSubItemEditorController.
    ��� ������ ������������� ISubItemEditorController � �� �������� �������� ������������ �����
    ���������� ����� SubItemEditorControllerBase::getStandartEditorWndStyle()

������� ������:
    �� ���� ����� � ������ ��������� �������������� ���� � ������� ����������� �������� ������� ��� ��������������

    ���� ��� �������������� �������� ���������� ���������� ������������� ���� �� IDCANCEL(::EndDialog(GetParent()->m_hWnd, IDCANCEL);),
    �������� Ecs ��� ���� ������ ������ ������� IDCANCEL ��������� �� ����� ���������(� ������ ������������� SubItemEditorControllerBase),
    � ISubItemEditorController::onEndEditSubItem ������ bAcceptResult == false
    � ������� ���� ������ IDOK - ��������� ����� ���������(� ������ ������������� SubItemEditorControllerBase) � bAcceptResult == true
*/
template <typename CBaseList = CListCtrl>
class CListEditSubItems : public CBaseList
{
public:
    CListEditSubItems() = default;

public:
    // ������ ���������� ��� ���������� ��������������� ������
    void setSubItemEditorController(int iSubItem,
                                    std::shared_ptr<ISubItemEditorController> controller);

    // ������� ������ ������������� �� parentWindow � ����� �����
    // WS_CHILDWINDOW | WS_VISIBLE, �� SubItemEditorControllerBase::getStandartEditorWndStyle()
    // @param pList - ��������� �� ���� � ������� ���������� ��������������
    // @param parentWindow - ������������ ���� ���� �������� �������
    // @param params - ��������� � ����������� � ������ ��� ������� ��������� �������
    typedef std::function<std::shared_ptr<CWnd>(CListCtrl* pList,
                                                CWnd* parentWindow,
                                                const LVSubItemParams* pParams)> createEditorControlFunc;
    // ���������� ��� ��������� �������������� ������
    // @param pList - ��������� �� ���� � ������� ���������� ��������������
    // @param editorControl - ������������� �������
    // @param pParams - ��������� � ����������� � ������ ��� ������� ��������� �������
    // @param bAcceptResult - ������������ �������� �������������� ����������� ���������
    typedef std::function<void(CListCtrl* pList,
                               CWnd* editorControl,
                               const LVSubItemParams* pParams,
                               bool bAcceptResult)> onEndEditSubItemFunc;

    // ������� ����������� ��� �������������� ��� �������� �������
    // ����� �������� �������, ��������� ������� ��������� ����������� �������� ISubItemEditorController
    // ���� ������� �� ������ ����� �������������� ����������� ��������� �� SubItemEditorControllerBase
    // ����������� �������
    void setSubItemEditorController(int iSubItem,
                                    const createEditorControlFunc& createFunc,
                                    const onEndEditSubItemFunc& endEditFunc = nullptr);

protected://********************************************************************
    DECLARE_MESSAGE_MAP()

    virtual void PreSubclassWindow() override;

    // ���� ���� � ��������� �������� �������������� ���� ����������
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    // ����������� �� ��������� �������������� ������
    afx_msg LRESULT OnEndEditSubItem(WPARAM wParam, LPARAM lParam);

private:
    // �������� ������� �������������� ��� �������������� ������
    // ���� �� ������ ������������� ������� SubItemEditorControllerBase
    // ���� ������� ��� ���� ������ �� ������������� ������ nullptr
    std::shared_ptr<ISubItemEditorController> getSubItemEditorController(int iSubItem);

private:
    // ����������� ��� ������ �������, ���� �� ������ - ����� �������������� SubItemEditorFabricBase
    // ���� ������ ��� nullptr, �� ������� �� �������������
    std::map<int, std::shared_ptr<ISubItemEditorController>> m_subItemsEditorControllers;

    // ���� �������������� �������
    std::unique_ptr<class IEditSubItemWindow> m_editSubItemWindow;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ����������� ��� �������������� ��������� � �������
// �� SubItemEditorControllerBase
interface ISubItemEditorController
{
    virtual ~ISubItemEditorController() = default;

    // ��������� ����� �������� ������� ��� �������������� ������
    // ������� ������ ������������� �� parentWindow � ����� �����
    // WS_CHILDWINDOW | WS_VISIBLE, �� SubItemEditorControllerBase::getStandartEditorWndStyle()
    // @param pList - ��������� �� ���� � ������� ���������� ��������������
    // @param parentWindow - ������������ ���� ���� �������� �������
    // @param params - ��������� � ����������� � ������ ��� ������� ��������� �������
    virtual std::shared_ptr<CWnd> createEditorControl(CListCtrl* pList,
                                                      CWnd* parentWindow,
                                                      const LVSubItemParams* pParams) = 0;

    // ���������� ��� ��������� �������������� ������
    // @param pList - ��������� �� ���� � ������� ���������� ��������������
    // @param editorControl - ������������� �������
    // @param pParams - ��������� � ����������� � ������ ��� ������� ��������� �������
    // @param bAcceptResult - ������������ �������� �������������� ����������� ���������
    virtual void onEndEditSubItem(CListCtrl* pList,
                                  CWnd* editorControl,
                                  const LVSubItemParams* pParams,
                                  bool bAcceptResult) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// ������� ����� �������������� ��� �������������� ���������, �� ��������� ������� ���� �����
class SubItemEditorControllerBase : public ISubItemEditorController
{
public:
    SubItemEditorControllerBase() = default;
    virtual ~SubItemEditorControllerBase() = default;

// ISubItemEditorController
public:
    // ������� ������� ��� ��������������, ������� ���� ����� ��� �������������� ������
    std::shared_ptr<CWnd> createEditorControl(CListCtrl* pList,
                                              CWnd* parentWindow,
                                              const LVSubItemParams* pParams) override;

    // ���������� ��� ��������� �������������� ������
    // �������� ����� �� ���� � ����������� ��� � �������
    void onEndEditSubItem(CListCtrl* pList,
                          CWnd* editorControl,
                          const LVSubItemParams* pParams,
                          bool bAcceptResult) override;

public:
    // �������� ����� ���� ��� �������� � ��������������� �� ���������
    static DWORD getStandartEditorWndStyle()
    { return WS_CHILDWINDOW | WS_VISIBLE; }
};

// �.� ����� ��������� ������ ���������� � ������ �����
#include "ListEditSubItemsImpl.h"