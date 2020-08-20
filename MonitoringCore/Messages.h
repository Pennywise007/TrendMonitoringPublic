#pragma once

/*
    ���������� ��������� ���������������� ��������� CMassages, ������ ��� ����������
    ����������� �������� � ��������� �������.
    ������������� ������� ������ ������������� ����� IEventRecipient(EventRecipientImpl) � �������������
    �� ������������ �� �������.

������ �������� ���������

    struct LogMessageData : public IEventData
    {
        // ����� ��������� ��� ����
        CString logMessage;
    };
    auto logMessage = std::make_shared<LogMessageData>();

    get_service<CMassages>().postMessage(onNewLogEventId, 0.f,
                                         std::static_pointer_cast<IEventData>(logMessage));

������ ��������� ���������
    // IEventRecipient
    void CLog::onEvent(const EventId& code, float eventValue,
                       std::shared_ptr<IEventData> eventData)
    {
        if (code == onNewLogEventId)
        {
            std::shared_ptr<LogMessageData> logMessageData = std::static_pointer_cast<LogMessageData>(eventData);
            ...
        }
    }
*/
#include <afx.h>
#include <afxwin.h>
#include <assert.h>
#include <functional>
#include <guiddef.h>
#include <future>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <tlhelp32.h>
#include <queue>

#include "Singleton.h"

// �������� � ��� ����� ��� ���� ����������� �� ������� �������, �� ���������� �� ���������
// ������ ��� � �������� ��������� ����� ����� � ��� �������� ���������� ����� ���� �������
// #define CHECK_SUBSCRIBERS_EXIST

////////////////////////////////////////////////////////////////////////////////
// ��������� ��� ���������������� ����, �� �������������� ���������� �������������
#define WM_EXECUTE_MESSAGE WM_USER + 1
// ������������� �������
typedef GUID EventId;
//****************************************************************************//
// ��������� ������������ � ���������
interface IEventData{ virtual ~IEventData() = default; };
//****************************************************************************//
// ��������� ���������� �������
interface IEventRecipient
{
    virtual ~IEventRecipient() = default;

    // ���������� � ������������ �������
    virtual void onEvent(const EventId& code, float eventValue,
                         std::shared_ptr<IEventData> eventData) = 0;
};
//****************************************************************************//
// ���������� ������ ����������� �������
class EventRecipientImpl : public IEventRecipient
{
public:
    EventRecipientImpl() = default;
    virtual ~EventRecipientImpl();
protected:
    void subscribe(const EventId& code);        // ������������ �� �������
    void unsubscribe(const EventId& code);      // ������� �� �������
    // �����������/���������� ������������ ���������� �������
    // ��� onEvent ����� ���������� �� ������ �������
    void subscribeAssynch(const EventId& eventID);
    void unsubscribeAssynch(const EventId& eventID);
};
//****************************************************************************//
// ����� �������� ���������
class CMassages : public IEventRecipient
{
    friend class CSingleton<CMassages>;

public:
    // ��������/������� �� �������
    void subscribe(const EventId& eventID, IEventRecipient* recipient);
    void unsubscribe(const EventId& eventID, IEventRecipient* recipient);
    // �����������/���������� ������������ ���������� �������
    // ��� onEvent ����� ���������� �� ������ �������
    void subscribeAssynch(const EventId& eventID, IEventRecipient* recipient);
    void unsubscribeAssynch(const EventId& eventID, IEventRecipient* recipient);
    // ������������ �� ��� ������� �� ����� ����������
    void unsubscribeAll(IEventRecipient* recipient);

// �������� ���������
public:
    // ���������� ���������� ����������� � ��������� �������
    void sendMessage(const EventId& code, float massageValue = 0,
                     std::shared_ptr<IEventData> eventData = nullptr);
    // ���������� ���������� ����������� � ��������� ������
    void postMessage(const EventId& code, float massageValue = 0,
                     std::shared_ptr<IEventData> eventData = nullptr);

// �������� ������� �� ���������� � �������� ������ ����������
public:
    // ����������� ���������� �������
    void call(const std::function<void(void)>& func);
    // ���������� ���������� �������
    void postCall(const std::function<void(void)>& func);

// ������
public:
    // �������� ��� �� ��������� � �������� ������
    bool isMainThread() const;

// IEventRecipient
private:
    // ���������� � ������������ �������
    void onEvent(const EventId& code, float eventValue,
                 std::shared_ptr<IEventData> eventData) override;

// ���������� �������
private:
    // ��������� � ������� ������� ������������ ����� ��������� � CMessagesWindow � � onEvent
    struct ThreadMsg
    {
        typedef std::unique_ptr<ThreadMsg> UPtr;

        // ���� �������
        EventId eventID = { };
        // ������������ ��������
        float massageValue = 0.f;
        // ������������ ������
        std::shared_ptr<IEventData> eventData;
        // ������ ����������� ���������
        std::set<IEventRecipient*> recipients;
    };

    // ��������� ���������
    void processMessage(ThreadMsg::UPtr& threadMessage, const bool bSynch,
                        const std::set<IEventRecipient*>* assynchReciepients = nullptr);

private://----------------------------------------------------------------------
    // ������ ������������ ��� ������ �������
    struct CallFuncData : public IEventData
    {
        // ���������� �������
        std::function<void(void)> callFunc;
        // ���������� ������� ����� ���������� �� ����� ���������� �������
        std::exception_ptr pExc;
    };
    // ��������� ��������� �������
    struct EventComparer
    {
        bool operator()(const EventId& Left, const EventId& Right) const
        {
            // comparison logic goes here
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };

    // ���������� �������
    struct EventRecipients
    {
        // ���������� ���������� �������(� �������� ������)
        std::set<IEventRecipient*> syncRecipients;
        // ���������� ������� ������� ����� �������� ������� ����������
        std::set<IEventRecipient*> assyncRecipients;
    };
    // ��������� �������� �������������� ������� � �������� �����������
    std::map<EventId, EventRecipients, EventComparer> m_messageRecipients;
    // ������� �� �������
    std::mutex m_recipientsMutex;

private:
    // ������� ��������������� ���� ������� ������ ��� ������������� � �������� �������, ��� UI
    class CMessagesWindow : public CWnd
    {
    public:
        CMessagesWindow();
        ~CMessagesWindow();

    private:
        // ������� ��������� �������������� ��������� ������
        DWORD getMainThreadId();

    public:
        // ������������� ������ � ������� �������� ���� ���������
        DWORD m_windowThreadId;
    } m_messagesWindow;
};

//----------------------------------------------------------------------------//
inline void CMassages::subscribe(const EventId& eventID, IEventRecipient* recipient)
{
    std::lock_guard<std::mutex> lock(m_recipientsMutex);

    m_messageRecipients[eventID].syncRecipients.insert(recipient);
}

//----------------------------------------------------------------------------//
inline void CMassages::unsubscribe(const EventId& eventID, IEventRecipient* recipient)
{
    std::lock_guard<std::mutex> lock(m_recipientsMutex);

    auto it = m_messageRecipients.find(eventID);
    if (it != m_messageRecipients.end())
    {
        it->second.syncRecipients.erase(recipient);
        if (it->second.syncRecipients.empty() && it->second.assyncRecipients.empty())
            m_messageRecipients.erase(it);
    }
    else
        assert(false);
}

//----------------------------------------------------------------------------//
inline void CMassages::subscribeAssynch(const EventId& eventID, IEventRecipient* recipient)
{
    std::lock_guard<std::mutex> lock(m_recipientsMutex);

    m_messageRecipients[eventID].assyncRecipients.insert(recipient);
}

//----------------------------------------------------------------------------//
inline void CMassages::unsubscribeAssynch(const EventId& eventID, IEventRecipient* recipient)
{
    std::lock_guard<std::mutex> lock(m_recipientsMutex);

    auto it = m_messageRecipients.find(eventID);
    if (it != m_messageRecipients.end())
    {
        it->second.assyncRecipients.erase(recipient);
        if (it->second.syncRecipients.empty() && it->second.assyncRecipients.empty())
            m_messageRecipients.erase(it);
    }
    else
        assert(false);
}

//----------------------------------------------------------------------------//
inline void CMassages::unsubscribeAll(IEventRecipient* recipient)
{
    std::lock_guard<std::mutex> lock(m_recipientsMutex);

    // ���������� ��� �� ���� ������� �� ������� ���������
    for (auto messageIt = m_messageRecipients.begin(), end = m_messageRecipients.end();
         messageIt != end;)
    {
        EventRecipients& recipients = messageIt->second;
        auto it = recipients.syncRecipients.find(recipient);
        if (it != recipients.syncRecipients.end())
            recipients.syncRecipients.erase(recipient);

        it = recipients.assyncRecipients.find(recipient);
        if (it != recipients.assyncRecipients.end())
            recipients.assyncRecipients.erase(recipient);

        if (recipients.syncRecipients.empty() && recipients.assyncRecipients.empty())
            messageIt = m_messageRecipients.erase(messageIt);
        else
            ++messageIt;
    }
}

//----------------------------------------------------------------------------//
inline void CMassages::sendMessage(const EventId& eventID, float massageValue /*= 0*/,
                                   std::shared_ptr<IEventData> eventData /*= nullptr*/)
{
    m_recipientsMutex.lock();
    if (auto messageRecipients = m_messageRecipients.find(eventID);
        messageRecipients != m_messageRecipients.end())
    {
        ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
        newThreadMsg->eventID = eventID;
        newThreadMsg->massageValue = massageValue;
        newThreadMsg->eventData = eventData;
        newThreadMsg->recipients = messageRecipients->second.syncRecipients;

        // �������� �������� �����������
        std::set<IEventRecipient*> assyncRecipients = messageRecipients->second.assyncRecipients;

        // ���������� ������ � ������������
        m_recipientsMutex.unlock();

        // ������������ ���������
        processMessage(newThreadMsg, true, &assyncRecipients);
    }
    else
    {
        m_recipientsMutex.unlock();

#ifdef CHECK_SUBSCRIBERS_EXIST
        assert(false);
#endif // CHECK_SUBSCRIBERS_EXIST
    }
}

//----------------------------------------------------------------------------//
inline void CMassages::postMessage(const EventId& eventID, float massageValue /*= 0*/,
                                   std::shared_ptr<IEventData> eventData /*= nullptr*/)
{
    m_recipientsMutex.lock();
    if (auto messageRecipients = m_messageRecipients.find(eventID);
        messageRecipients != m_messageRecipients.end())
    {
        ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
        newThreadMsg->eventID = eventID;
        newThreadMsg->massageValue = massageValue;
        newThreadMsg->eventData = eventData;
        newThreadMsg->recipients = messageRecipients->second.syncRecipients;

        // �������� �������� �����������
        std::set<IEventRecipient*> assyncRecipients = messageRecipients->second.assyncRecipients;

        // ���������� ������ � ������������
        m_recipientsMutex.unlock();

        // ������������ ���������
        processMessage(newThreadMsg, false, &assyncRecipients);
    }
    else
    {
        m_recipientsMutex.unlock();

#ifdef CHECK_SUBSCRIBERS_EXIST
        assert(false);
#endif // CHECK_SUBSCRIBERS_EXIST
    }
}

//----------------------------------------------------------------------------//
inline void CMassages::call(const std::function<void(void)>& func)
{
    if (isMainThread())
        // ���� ������� �� ��������� ������ - ����� ���������
        func();
    else
    {
        // ���� ������� �� ���������������� ������ - ���������� ��������� � ����������� �������� � �������� �����
        auto callFunc = std::make_shared<CallFuncData>();
        callFunc->callFunc = func;

        ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
        newThreadMsg->eventData = std::static_pointer_cast<IEventData>(callFunc);
        newThreadMsg->recipients = { this };

        // ���������� ���������
        processMessage(newThreadMsg, true);

        // ���� �� ����� ���������� ���� �������������� ���������� ����������� ��� ���
        if (callFunc->pExc)
            std::rethrow_exception(callFunc->pExc);
    }
}

//----------------------------------------------------------------------------//
inline void CMassages::postCall(const std::function<void(void)>& func)
{
    // ���� ������� �� ���������������� ������ - ���������� ��������� � ����������� �������� � �������� �����
    auto callFunc = std::make_shared<CallFuncData>();
    callFunc->callFunc = func;

    ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
    newThreadMsg->eventData = std::static_pointer_cast<IEventData>(callFunc);
    newThreadMsg->recipients = { this };

    // ���������� ���������
    processMessage(newThreadMsg, false);
}

//----------------------------------------------------------------------------//
inline bool CMassages::isMainThread() const
{
    return m_messagesWindow.m_windowThreadId == ::GetCurrentProcessId();
}

//----------------------------------------------------------------------------//
// IEventRecipient
inline void CMassages::onEvent(const EventId& /*code*/,
                               float /*eventValue*/,
                               std::shared_ptr<IEventData> eventData)
{
    auto pCallFuncData = std::static_pointer_cast<CallFuncData>(eventData);
    if (pCallFuncData)
    {
        try
        {
            pCallFuncData->callFunc();
        }
        catch (...)
        {
            pCallFuncData->pExc = std::current_exception();
        }
        return;
    }
    else
        assert(!"����� ���� ������� ������ ��� ��������� ������ �������!");
}

//----------------------------------------------------------------------------//
inline void CMassages::processMessage(ThreadMsg::UPtr& threadMessage, const bool bSynch,
                                      const std::set<IEventRecipient*>* assynchReciepients /*= nullptr*/)
{
    if (assynchReciepients)
    {
        // ��������� � ���� ����������� ����������� �����
        for (auto &recipient : *assynchReciepients)
        {
            // TODO bSynch = false - ���������� �� Std::assync
            recipient->onEvent(threadMessage->eventID,
                               threadMessage->massageValue,
                               threadMessage->eventData);
        }
    }

    // ���� ��� ���������� ����������� - ������ ������ ������
    if (threadMessage->recipients.empty())
        return;

    assert(::IsWindow(m_messagesWindow.m_hWnd));

    if (bSynch)
    {
        if (isMainThread())
        {
            // ���� �� � �������� ������ - ��������� �����
            for (auto &recipient : threadMessage->recipients)
            {
                recipient->onEvent(threadMessage->eventID,
                                   threadMessage->massageValue,
                                   threadMessage->eventData);
            }
        }
        else
            // ���������� ��������� ���� ����� � �������� ������ ���������� ���������
            SendMessage(m_messagesWindow.m_hWnd, WM_EXECUTE_MESSAGE, 0, (LPARAM)threadMessage.release());
    }
    else
        // ���������� ��������� ���� ����� � �������� ������ ���������� ���������
        PostMessage(m_messagesWindow.m_hWnd, WM_EXECUTE_MESSAGE, 0, (LPARAM)threadMessage.release());
}

//----------------------------------------------------------------------------//
inline EventRecipientImpl::~EventRecipientImpl()
{
    get_service<CMassages>().unsubscribeAll(this);
}

//----------------------------------------------------------------------------//
inline void EventRecipientImpl::subscribe(const EventId& code)
{
    get_service<CMassages>().subscribe(code, this);
}

//----------------------------------------------------------------------------//
inline void EventRecipientImpl::unsubscribe(const EventId& code)
{
    get_service<CMassages>().unsubscribe(code, this);
}

//----------------------------------------------------------------------------//
inline void EventRecipientImpl::subscribeAssynch(const EventId& code)
{
    get_service<CMassages>().subscribeAssynch(code, this);
}

//----------------------------------------------------------------------------//
inline void EventRecipientImpl::unsubscribeAssynch(const EventId& code)
{
    get_service<CMassages>().unsubscribeAssynch(code, this);
}

////////////////////////////////////////////////////////////////////////////////
inline CMassages::CMessagesWindow::CMessagesWindow()
{
    m_windowThreadId = ::GetCurrentThreadId();

    assert(m_windowThreadId == getMainThreadId() &&
           "���� ��������� ������� �� � �������� ������ ���������!");

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
        wndClass.lpfnWndProc = [](HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            switch (Message)
            {
            case WM_EXECUTE_MESSAGE:
                std::shared_ptr<ThreadMsg> msgData((ThreadMsg*)lparam);

                for (auto &recipient : msgData->recipients)
                {
                    recipient->onEvent(msgData->eventID,
                                       msgData->massageValue,
                                       msgData->eventData);
                }

                return 0;
            }

            return ::DefWindowProc(hwnd, Message, wparam, lparam);
        };

        wndClass.hInstance = instance;
        wndClass.lpszClassName = editLabelClassName;

        if (!RegisterClassEx(&wndClass))
        {
            ::MessageBox(NULL, L"Can`t register class", L"Error", MB_OK);
            return;
        }
    }

    if (CWnd::CreateEx(0, CString(typeid(*this).name()), L"",
                       0, 0, 0, 0, 0,
                       NULL, nullptr, nullptr) == FALSE)
    {
        assert(false);
        return;
    }
}

//----------------------------------------------------------------------------//
inline CMassages::CMessagesWindow::~CMessagesWindow()
{
    if (::IsWindow(m_hWnd))
        ::DestroyWindow(m_hWnd);
}

//----------------------------------------------------------------------------//
inline DWORD CMassages::CMessagesWindow::getMainThreadId()
{
    DWORD result = 0;

    const std::shared_ptr<void> hThreadSnapshot(
        CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);
    if (hThreadSnapshot.get() != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 tEntry;
        tEntry.dwSize = sizeof(THREADENTRY32);
        DWORD currentPID = GetCurrentProcessId();
        for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
             !result && success && GetLastError() != ERROR_NO_MORE_FILES;
             success = Thread32Next(hThreadSnapshot.get(), &tEntry))
        {
            if (tEntry.th32OwnerProcessID == currentPID)
                result = tEntry.th32ThreadID;
        }
    }
    else
        assert(!"GetMainThreadId failed");

    return result;
}

