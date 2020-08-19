#pragma once

/*
    ������ CTickService

    ���������� ������ � �������� � ������������ ���������� ����������� ����� �������,
    ����� ������ ��������� ��������� ��� ������� � ����������� �������������� ��� ������� �������.

    ���������� ��������������� ����� CTickHandlerImpl ������� ������ �� ��������� � �������� ������ � ����
*/

#include <assert.h>
#include <chrono>
#include <map>
#include <optional>
#include <windef.h>
#include <WinUser.h>

#include "Singleton.h"

// �������� ����, ���������� � ���������� ��� ����
typedef LONG_PTR TickParam;
// ��� ����� ������������ � �������
typedef std::chrono::steady_clock tick_clock;

////////////////////////////////////////////////////////////////////////////////
// ��������� �������������� �������� ������������ � �����, ���������� �� ���� �������
interface ITickHandler
{
    virtual ~ITickHandler() = default;

    /// <summary>���������� ��� ���� �������.</summary>
    /// <param name="tickParam">�������� ���������� �������.</param>
    /// <returns>true ���� ��� ��, false ���� ���� ���������� ���� ������.</returns>
    virtual bool onTick(TickParam tickParam) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// ���������� ������� �����
// ������� ������ �� ��� ���������� ������ � �������� ����������� ���������� � �����
// ����� ������ �������� ��� ���� (�� ����� �������� � ��������� kDefTickInterval
class CTickService
{
    friend class CSingleton<CTickService>;
public://---------------------------------------------------------------------//
    CTickService() = default;
    ~CTickService();

    // �������� ����� �������������� �� ���������
    static const std::chrono::milliseconds kDefTickInterval;

public:
    /// <summary>���������� ���������� �����.</summary>
    /// <param name="handler">��������� �� ��������� � ������������ �����.</param>
    /// <param name="tickInterval">�������� �����, ������� � ������� ���� ��������� ���������� � ����.</param>
    /// <param name="tickParam">�������� ������������ � ���������� ��� ����,
    /// ��������� ���������������� ������/���������� ���������� � ����������.</param>
    void subscribeHandler(ITickHandler* handler,
                          tick_clock::duration tickInterval = kDefTickInterval,
                          const TickParam& tickParam = 0);

    /// <summary>������� ���������� ����� �� ������.</summary>
    /// <param name="handler">��������� �� ��������� � ������������ ����� ������� ����� �������.</param>
    void unsubscribeHandler(ITickHandler* handler);
    /// <summary>������� ���������� ����� �� ������ � ����������� ����������.</summary>
    /// <param name="handler">��������� �� ��������� � ������������ ����� ������� ����� �������.</param>
    /// <param name="tickParam">�������� ����.</param>
    void unsubscribeHandler(ITickHandler* handler, const TickParam& tickParam);

    /// <summary>�������� ���� �� ������ � ����� ����������� � �������� ����������.</summary>
    /// <param name="handler">��������� �� ��������� � ������������ �����.</param>
    /// <param name="tickParam">�������� ����������� �������.</param>
    bool isTimerExist(ITickHandler* handler, const TickParam& tickParam = 0);

private:
    // ����������� ������� �������������� � �������
    static void CALLBACK timerProc(HWND hWnd, UINT uMsg, UINT_PTR uID, DWORD drTime);
    // ���������� ��� ���� ������� �� timerProc
    void onTickTimer();

    // ��������� ������
    void startTimer();
    // ���������� ������
    void stopTimer();
    // ��������� ������, ��������� ��� ���� ��� ��� � ���� ����������� �����
    // � ������� ������ ���� ��� ������������
    void checkTimer();
private:
    // ��������� � ����������� ����������� �����
    struct TickHandlerParams
    {
        // �������� ������������ ��� ����� ����
        TickParam tickParam = 0;
        // ����� ���������� ����
        tick_clock::time_point lastTickTime = tick_clock::now();
        // �������� �����
        tick_clock::duration tickInterval;
    };

    // ������ � ������������� �����
    std::multimap<ITickHandler*, TickHandlerParams> m_tickHandlers;
    // ������������� ������� ���� �� �������
    std::optional<UINT_PTR> m_timerId;
};

////////////////////////////////////////////////////////////////////////////////
// ���������� �������� ������ ����������� �����, ����� ��� ��������� ������ � ��������
class CTickHandlerImpl : public ITickHandler
{
public:
    CTickHandlerImpl() = default;
    virtual ~CTickHandlerImpl();

    /// <summary>���������� ���������� �����.</summary>
    /// <param name="tickInterval">�������� �����, ������� � ������� ���� ��������� ���������� � ����.</param>
    /// <param name="tickParam">�������� ������������ � ���������� ��� ����,
    /// ��������� ���������������� ������/���������� ���������� � ����������.</param>
    void subscribeTimer(tick_clock::duration tickInterval = CTickService::kDefTickInterval,
                        const TickParam& tickParam = 0);

    /// <summary>������� ���������� ����� �� ������.</summary>
    void unsubscribeTimer();
    /// <summary>������� ���������� ����� �� ������ � ����������� ����������.</summary>
    /// <param name="tickParam">�������� ����.</param>
    void unsubscribeTimer(const TickParam& tickParam);

    /// <summary>�������� ���� �� ������ � ����� ����������� � �������� ����������.</summary>
    /// <param name="tickParam">�������� ����������� �������.</param>
    bool isTimerExist(const TickParam& tickParam = 0);
};

////////////////////////////////////////////////////////////////////////////////
// �������� ����� �������������� � ������� � �� ���������
const std::chrono::milliseconds CTickService::kDefTickInterval(200);

//----------------------------------------------------------------------------//
inline CTickService::~CTickService()
{
    stopTimer();
}

//----------------------------------------------------------------------------//
inline void CTickService::subscribeHandler(ITickHandler* handler,
                                           tick_clock::duration tickInterval /*= kDefTickInterval*/,
                                           const TickParam& tickParam /*= nullptr*/)
{
    TickHandlerParams handlerParams;
    handlerParams.tickInterval = tickInterval;
    handlerParams.tickParam = tickParam;

    m_tickHandlers.emplace(handler, handlerParams);

    // ��������� ������������� �������� ������
    checkTimer();
}

//----------------------------------------------------------------------------//
// ������� ���������� ����� �� ������
inline void CTickService::unsubscribeHandler(ITickHandler* handler)
{
    m_tickHandlers.erase(handler);

    // ��������� ������������� � �������
    checkTimer();
}

//----------------------------------------------------------------------------//
// ������� ���������� ����� �� ������ � ����������� ����������
inline void CTickService::unsubscribeHandler(ITickHandler* handler, const TickParam& tickParam)
{
    typedef decltype(m_tickHandlers)::iterator iterator;
    // �������� ��� ������ �� ���� ��������
    std::pair<iterator, iterator> handlers = m_tickHandlers.equal_range(handler);

    // �������� �� ��� � ���� � ��������� ����������
    for (iterator it = handlers.first; it != handlers.second; ++it)
    {
        if (it->second.tickParam == tickParam)
            m_tickHandlers.erase(it);
    }

    // ��������� ������������� � �������
    checkTimer();
}

//----------------------------------------------------------------------------//
inline bool CTickService::isTimerExist(ITickHandler* handler, const TickParam& tickParam)
{
    typedef decltype(m_tickHandlers)::iterator iterator;
    // �������� ��� ������ �� ���� ��������
    std::pair<iterator, iterator> handlers = m_tickHandlers.equal_range(handler);

    // �������� �� ��� � ���� � ��������� ����������
    for (iterator it = handlers.first; it != handlers.second; ++it)
    {
        if (it->second.tickParam == tickParam)
            return true;
    }

    return false;
}

//----------------------------------------------------------------------------//
inline void CALLBACK CTickService::timerProc(HWND hWnd, UINT uMsg, UINT_PTR uID, DWORD drTime)
{
    get_service<CTickService>().onTickTimer();
}

//----------------------------------------------------------------------------//
inline void CTickService::onTickTimer()
{
    // ��� ���� ������������ ����� �������� ���������
    for (auto handlerIt = m_tickHandlers.begin(), end = m_tickHandlers.end(); handlerIt != end;)
    {
        TickHandlerParams& handlerParams = handlerIt->second;

        // ���� ������� ����� ���� - �������� �����������
        if (tick_clock::now() - handlerParams.lastTickTime >= handlerParams.tickInterval)
        {
            if (handlerIt->first->onTick(handlerParams.tickParam))
                // ��������� ����� ���������� ����
                handlerParams.lastTickTime = tick_clock::now();
            else
            {
                // ���� ���� ���������� ���� - ������ ���������� �� ����� ��������
                handlerIt = m_tickHandlers.erase(handlerIt);
                continue;
            }
        }

        ++handlerIt;
    }

    // �������� ��������� ������ �� ����� ���� - ��������� ��� ������������� � �������
    checkTimer();
}

//----------------------------------------------------------------------------//
// ��������� ������
inline void CTickService::startTimer()
{
    assert(!m_timerId.has_value());

    m_timerId = ::SetTimer(NULL, NULL,
                           (UINT)kDefTickInterval.count(),
                           &CTickService::timerProc);
}

//----------------------------------------------------------------------------//
// ���������� ������
inline void CTickService::stopTimer()
{
    // ��������� ������
    if (m_timerId.has_value())
    {
        ::KillTimer(0, *m_timerId);
        m_timerId.reset();
    }
}

//----------------------------------------------------------------------------//
// ��������� ������� �������
inline void CTickService::checkTimer()
{
    // ������� ������������ ������ �������� � �������� �������������� �������
    if (m_tickHandlers.empty() == m_timerId.has_value())
    {
        if (m_tickHandlers.empty())
            stopTimer();
        else
            startTimer();
    }
}

////////////////////////////////////////////////////////////////////////////////
inline CTickHandlerImpl::~CTickHandlerImpl()
{
    unsubscribeTimer();
}

//----------------------------------------------------------------------------//
// ���������� ���������� �����.
inline void CTickHandlerImpl::subscribeTimer(tick_clock::duration tickInterval,
                                             const TickParam& tickParam)
{
    get_service<CTickService>().subscribeHandler(this, tickInterval, tickParam);
}

//----------------------------------------------------------------------------//
// ������� ���������� ����� �� ������.
inline void CTickHandlerImpl::unsubscribeTimer()
{
    get_service<CTickService>().unsubscribeHandler(this);
}

//----------------------------------------------------------------------------//
// ������� ���������� ����� �� ������ � ����������� ����������.
inline void CTickHandlerImpl::unsubscribeTimer(const TickParam& tickParam)
{
    get_service<CTickService>().unsubscribeHandler(this, tickParam);
}

//----------------------------------------------------------------------------//
// �������� ���� �� ������ � ����� ����������� � �������� ����������
inline bool CTickHandlerImpl::isTimerExist(const TickParam& tickParam)
{
    return get_service<CTickService>().isTimerExist(this, tickParam);
}
