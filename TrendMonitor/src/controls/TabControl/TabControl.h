#pragma once

#include <afxcmn.h>
#include <map>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// –асширение дл€ контрола с табами позвол€ет вставл€ть диалоги во вкладки
// и переключатьс€ между ними / раставл€ть их на места и сайзировать вместе с контролом
class CTabControl : public CTabCtrl
{
public:
    CTabControl() = default;

public:
    /// ¬ставка таба с окном прив€занным к нему, при переключении вкладок
    /// будет включатьс€ окно прив€занное к вкладке.
    ///
    /// ¬нимание! ¬ставл€ть необходимо по очереди (по возрастанию индексов)
    /// ¬ставка между существующими табами может привести к проблемам(не реализована)
    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem,
                   _In_ std::shared_ptr<CWnd> tabWindow);

    /// ¬ставка диалога в таб, если у диалога нет окна - будет вызван
    /// CDialog::Create с заданным идентификатором nIDTemplate и родителем будет проставлен таб контрол
    ///
    /// ¬нимание! ¬ставл€ть необходимо по очереди (по возрастанию индексов)
    /// ¬ставка между существующими табами может привести к проблемам(не реализована)
    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem,
                   _In_ std::shared_ptr<CDialog> tabDialog, UINT nIDTemplate);

public:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);

private:
    // располагает текущее активное окно в клиентской области таба
    void layoutCurrentWindow();

private:
    // диалоги дл€ каждой вкладки
    std::map<LONG, std::shared_ptr<CWnd>> m_tabWindows;
};