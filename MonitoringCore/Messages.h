#pragma once

/*
    Реализация событийно ориентированного синглтона CMassages, служит для оповещения
    подписанных объектов о возникшем событии.
    Подписавшиеся объекты должны реализовывать класс IEventRecipient(EventRecipientImpl) и подписываться
    на интересующие их события.

Пример отправки сообщения

    struct LogMessageData : public IEventData
    {
        // текст сообщения для лога
        CString logMessage;
    };
    auto logMessage = std::make_shared<LogMessageData>();

    get_service<CMassages>().postMessage(onNewLogEventId, 0.f,
                                         std::static_pointer_cast<IEventData>(logMessage));

Пример получения сообщения
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

// Проверка в ран тайме что есть подписанные на события объекты, не включается по умолчанию
// потому что у сервисов различное время жизни и при закрытии приложения могут быть ассерты
// #define CHECK_SUBSCRIBERS_EXIST

////////////////////////////////////////////////////////////////////////////////
// Сообщение для вспомогательного окна, об необходжимости оповестить подписавшихся
#define WM_EXECUTE_MESSAGE WM_USER + 1
// идентификатор события
typedef GUID EventId;
//****************************************************************************//
// структура передаваемая в сообщении
interface IEventData{ virtual ~IEventData() = default; };
//****************************************************************************//
// интерфейс получающий события
interface IEventRecipient
{
    virtual ~IEventRecipient() = default;

    // оповещение о произошедшем событии
    virtual void onEvent(const EventId& code, float eventValue,
                         std::shared_ptr<IEventData> eventData) = 0;
};
//****************************************************************************//
// Реализация класса получающего события
class EventRecipientImpl : public IEventRecipient
{
public:
    EventRecipientImpl() = default;
    virtual ~EventRecipientImpl();
protected:
    void subscribe(const EventId& code);        // подписывание на события
    void unsubscribe(const EventId& code);      // отписка от события
    // Подписываем/отписываем асинхронного получателя событий
    // его onEvent может вызываться из других потоков
    void subscribeAssynch(const EventId& eventID);
    void unsubscribeAssynch(const EventId& eventID);
};
//****************************************************************************//
// класс отправки сообщений
class CMassages : public IEventRecipient
{
    friend class CSingleton<CMassages>;

public:
    // подписка/отписка на события
    void subscribe(const EventId& eventID, IEventRecipient* recipient);
    void unsubscribe(const EventId& eventID, IEventRecipient* recipient);
    // Подписываем/отписываем асинхронного получателя событий
    // его onEvent может вызываться из других потоков
    void subscribeAssynch(const EventId& eventID, IEventRecipient* recipient);
    void unsubscribeAssynch(const EventId& eventID, IEventRecipient* recipient);
    // отписываемся на все события от этого получателя
    void unsubscribeAll(IEventRecipient* recipient);

// отправка сообщений
public:
    // синхронное оповещение подписчиков о возникшем событии
    void sendMessage(const EventId& code, float massageValue = 0,
                     std::shared_ptr<IEventData> eventData = nullptr);
    // отложенное оповещение подписчиков о возникшем собыии
    void postMessage(const EventId& code, float massageValue = 0,
                     std::shared_ptr<IEventData> eventData = nullptr);

// передача функции на выполнение в основном потоке приложения
public:
    // немедленное выполнение функции
    void call(const std::function<void(void)>& func);
    // отложенное выполнение функции
    void postCall(const std::function<void(void)>& func);

// прочее
public:
    // проверка что мы находимся в основном потоке
    bool isMainThread() const;

// IEventRecipient
private:
    // оповещение о произошедшем событии
    void onEvent(const EventId& code, float eventValue,
                 std::shared_ptr<IEventData> eventData) override;

// внутренние функции
private:
    // структура с данными которые отправляются через сообщения в CMessagesWindow и в onEvent
    struct ThreadMsg
    {
        typedef std::unique_ptr<ThreadMsg> UPtr;

        // гуид события
        EventId eventID = { };
        // пересылаемое значение
        float massageValue = 0.f;
        // передаваемые данные
        std::shared_ptr<IEventData> eventData;
        // массив получателей сообщения
        std::set<IEventRecipient*> recipients;
    };

    // обработка сообщений
    void processMessage(ThreadMsg::UPtr& threadMessage, const bool bSynch,
                        const std::set<IEventRecipient*>* assynchReciepients = nullptr);

private://----------------------------------------------------------------------
    // данные передаваемые для вызова функции
    struct CallFuncData : public IEventData
    {
        // вызываемая функция
        std::function<void(void)> callFunc;
        // исключение которое могло возникнуть во время выполнения функции
        std::exception_ptr pExc;
    };
    // структура сравнения событий
    struct EventComparer
    {
        bool operator()(const EventId& Left, const EventId& Right) const
        {
            // comparison logic goes here
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };

    // Получатели событий
    struct EventRecipients
    {
        // синхроныне получатели событий(в основном потоке)
        std::set<IEventRecipient*> syncRecipients;
        // получатели событий которые будут получать события асинхронно
        std::set<IEventRecipient*> assyncRecipients;
    };
    // контейнер хранящий идентификаторы событий и перечень получателей
    std::map<EventId, EventRecipients, EventComparer> m_messageRecipients;
    // мьютекс на события
    std::mutex m_recipientsMutex;

private:
    // скрытое вспомогательное окно которое служит для синхроназиции с основным потоком, для UI
    class CMessagesWindow : public CWnd
    {
    public:
        CMessagesWindow();
        ~CMessagesWindow();

    private:
        // функция получения идентификатора основного потока
        DWORD getMainThreadId();

    public:
        // идентификатор потока в котором работает окно сообщений
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

    // отписываем нас от всех событий на которые подписаны
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

        // копируем перечень получателей
        std::set<IEventRecipient*> assyncRecipients = messageRecipients->second.assyncRecipients;

        // прекращаем работу с получателями
        m_recipientsMutex.unlock();

        // обрабатываем сообщение
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

        // копируем перечень получателей
        std::set<IEventRecipient*> assyncRecipients = messageRecipients->second.assyncRecipients;

        // прекращаем работу с получателями
        m_recipientsMutex.unlock();

        // обрабатываем сообщение
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
        // если вызвали из основного потока - сразу выполняем
        func();
    else
    {
        // если вызвали из вспомогательного потока - отправляем сообщение с необходимой функцией в основной поток
        auto callFunc = std::make_shared<CallFuncData>();
        callFunc->callFunc = func;

        ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
        newThreadMsg->eventData = std::static_pointer_cast<IEventData>(callFunc);
        newThreadMsg->recipients = { this };

        // отправляем сообщение
        processMessage(newThreadMsg, true);

        // если во время выполнения было необработанное исключение прокидываем его тут
        if (callFunc->pExc)
            std::rethrow_exception(callFunc->pExc);
    }
}

//----------------------------------------------------------------------------//
inline void CMassages::postCall(const std::function<void(void)>& func)
{
    // если вызвали из вспомогательного потока - отправляем сообщение с необходимой функцией в основной поток
    auto callFunc = std::make_shared<CallFuncData>();
    callFunc->callFunc = func;

    ThreadMsg::UPtr newThreadMsg = std::make_unique<ThreadMsg>();
    newThreadMsg->eventData = std::static_pointer_cast<IEventData>(callFunc);
    newThreadMsg->recipients = { this };

    // отправляем сообщение
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
        assert(!"Может быть вызвано только для отработки вызова функции!");
}

//----------------------------------------------------------------------------//
inline void CMassages::processMessage(ThreadMsg::UPtr& threadMessage, const bool bSynch,
                                      const std::set<IEventRecipient*>* assynchReciepients /*= nullptr*/)
{
    if (assynchReciepients)
    {
        // выполняем у всех асинхронных получателей сразу
        for (auto &recipient : *assynchReciepients)
        {
            // TODO bSynch = false - переделать на Std::assync
            recipient->onEvent(threadMessage->eventID,
                               threadMessage->massageValue,
                               threadMessage->eventData);
        }
    }

    // если нет синхронных получателей - больше делать нечего
    if (threadMessage->recipients.empty())
        return;

    assert(::IsWindow(m_messagesWindow.m_hWnd));

    if (bSynch)
    {
        if (isMainThread())
        {
            // если мы в основном потоке - выполняем сразу
            for (auto &recipient : threadMessage->recipients)
            {
                recipient->onEvent(threadMessage->eventID,
                                   threadMessage->massageValue,
                                   threadMessage->eventData);
            }
        }
        else
            // отправляем сообщение окну чтобы в основном потоке отработать сообщение
            SendMessage(m_messagesWindow.m_hWnd, WM_EXECUTE_MESSAGE, 0, (LPARAM)threadMessage.release());
    }
    else
        // отправляем сообщение окну чтобы в основном потоке отработать сообщение
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
           "Окно сообщений создано не в основном потоке программы!");

    HINSTANCE instance = AfxGetInstanceHandle();
    CString editLabelClassName(typeid(*this).name());

    // регистрируем наш клас
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, editLabelClassName, &wndClass))
    {
        // Регистрация класса окна которое используется для редактирования ячеек
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

