#pragma once

/*
    Сервис CTickService

    Реализация класса с таймером и возможностью подключать обработчики тиков таймера,
    Можно задать различные интервалы для таймера и специальные идентификаторы для каждого таймера.

    Реализован вспомогательный класс CTickHandlerImpl который следит за таймерами и упрощает работу с ними
*/

#include <assert.h>
#include <chrono>
#include <map>
#include <optional>
#include <windef.h>
#include <WinUser.h>

#include "Singleton.h"

// параметр тика, передается в обработчик при тике
typedef LONG_PTR TickParam;
// тип часов используемых в сервисе
typedef std::chrono::steady_clock tick_clock;

////////////////////////////////////////////////////////////////////////////////
// Интерфейс поддерживаемый классами нуждающимися в тиках, вызывается на тике таймера
interface ITickHandler
{
    virtual ~ITickHandler() = default;

    /// <summary>Вызывается при тике таймера.</summary>
    /// <param name="tickParam">Параметр вызванного таймера.</param>
    /// <returns>true если все ок, false если надо прекратить этот таймер.</returns>
    virtual bool onTick(TickParam tickParam) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Реализация сервиса тиков
// Создает единый на все приложение таймер и посылает подписчикам информацию о тиках
// Можно задать интервал для тика (он будет работать с точностью kDefTickInterval
class CTickService
{
    friend class CSingleton<CTickService>;
public://---------------------------------------------------------------------//
    CTickService() = default;
    ~CTickService();

    // интервал тиков использующийся по умолчанию
    static const std::chrono::milliseconds kDefTickInterval;

public:
    /// <summary>Подключить обработчик тиков.</summary>
    /// <param name="handler">Указатель на интерфейс с обработчиком тиков.</param>
    /// <param name="tickInterval">Интервал тиков, частота с которой надо оповещать обработчик о тике.</param>
    /// <param name="tickParam">Параметр передаваемый в обработчик при тике,
    /// позволяет идентифицировать таймер/передавать информацию в обработчик.</param>
    void subscribeHandler(ITickHandler* handler,
                          tick_clock::duration tickInterval = kDefTickInterval,
                          const TickParam& tickParam = 0);

    /// <summary>Удалить обработчик тиков из списка.</summary>
    /// <param name="handler">Указатель на интерфейс с обработчиком тиков который нужно удалить.</param>
    void unsubscribeHandler(ITickHandler* handler);
    /// <summary>Удалить обработчик тиков из списка с совпадающим параметром.</summary>
    /// <param name="handler">Указатель на интерфейс с обработчиком тиков который нужно удалить.</param>
    /// <param name="tickParam">Параметр тика.</param>
    void unsubscribeHandler(ITickHandler* handler, const TickParam& tickParam);

    /// <summary>Проверка есть ли таймер у этого обработчика с заданным параметром.</summary>
    /// <param name="handler">Указатель на интерфейс с обработчиком тиков.</param>
    /// <param name="tickParam">Параметр обработчика таймера.</param>
    bool isTimerExist(ITickHandler* handler, const TickParam& tickParam = 0);

private:
    // статическая функция использующаяся в таймере
    static void CALLBACK timerProc(HWND hWnd, UINT uMsg, UINT_PTR uID, DWORD drTime);
    // вызывается при тике таймера из timerProc
    void onTickTimer();

    // Запустить таймер
    void startTimer();
    // Остановить таймер
    void stopTimer();
    // проверить таймер, запускает его если его нет и есть обработчики тиков
    // и убирает таймер если нет обработчиков
    void checkTimer();
private:
    // структура с параметрами обработчика тиков
    struct TickHandlerParams
    {
        // параметр передаваемый для этого тика
        TickParam tickParam = 0;
        // время последнего тика
        tick_clock::time_point lastTickTime = tick_clock::now();
        // интервал тиков
        tick_clock::duration tickInterval;
    };

    // Список с обработчиками тиков
    std::multimap<ITickHandler*, TickHandlerParams> m_tickHandlers;
    // Идентификатор таймера если он запущен
    std::optional<UINT_PTR> m_timerId;
};

////////////////////////////////////////////////////////////////////////////////
// Реализация базового класса обработчика тиков, нужен для упрощения работы с сервисом
class CTickHandlerImpl : public ITickHandler
{
public:
    CTickHandlerImpl() = default;
    virtual ~CTickHandlerImpl();

    /// <summary>Подключить обработчик тиков.</summary>
    /// <param name="tickInterval">Интервал тиков, частота с которой надо оповещать обработчик о тике.</param>
    /// <param name="tickParam">Параметр передаваемый в обработчик при тике,
    /// позволяет идентифицировать таймер/передавать информацию в обработчик.</param>
    void subscribeTimer(tick_clock::duration tickInterval = CTickService::kDefTickInterval,
                        const TickParam& tickParam = 0);

    /// <summary>Удалить обработчик тиков из списка.</summary>
    void unsubscribeTimer();
    /// <summary>Удалить обработчик тиков из списка с совпадающим параметром.</summary>
    /// <param name="tickParam">Параметр тика.</param>
    void unsubscribeTimer(const TickParam& tickParam);

    /// <summary>Проверка есть ли таймер у этого обработчика с заданным параметром.</summary>
    /// <param name="tickParam">Параметр обработчика таймера.</param>
    bool isTimerExist(const TickParam& tickParam = 0);
};

////////////////////////////////////////////////////////////////////////////////
// интервал тиков использующийся в таймере и по умолчанию
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

    // проверяем необходимость включить таймер
    checkTimer();
}

//----------------------------------------------------------------------------//
// Удалить обработчик тиков из списка
inline void CTickService::unsubscribeHandler(ITickHandler* handler)
{
    m_tickHandlers.erase(handler);

    // проверяем необходимость в таймере
    checkTimer();
}

//----------------------------------------------------------------------------//
// Удалить обработчик тиков из списка с совпадающим параметром
inline void CTickService::unsubscribeHandler(ITickHandler* handler, const TickParam& tickParam)
{
    typedef decltype(m_tickHandlers)::iterator iterator;
    // получаем все данные по этом хэндлеру
    std::pair<iterator, iterator> handlers = m_tickHandlers.equal_range(handler);

    // проходим по ним и ищем с указанным параметром
    for (iterator it = handlers.first; it != handlers.second; ++it)
    {
        if (it->second.tickParam == tickParam)
            m_tickHandlers.erase(it);
    }

    // проверяем необходимость в таймере
    checkTimer();
}

//----------------------------------------------------------------------------//
inline bool CTickService::isTimerExist(ITickHandler* handler, const TickParam& tickParam)
{
    typedef decltype(m_tickHandlers)::iterator iterator;
    // получаем все данные по этом хэндлеру
    std::pair<iterator, iterator> handlers = m_tickHandlers.equal_range(handler);

    // проходим по ним и ищем с указанным параметром
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
    // для всех обработчиков тиков вызываем обработку
    for (auto handlerIt = m_tickHandlers.begin(), end = m_tickHandlers.end(); handlerIt != end;)
    {
        TickHandlerParams& handlerParams = handlerIt->second;

        // если настало время тика - сообщаем обработчику
        if (tick_clock::now() - handlerParams.lastTickTime >= handlerParams.tickInterval)
        {
            if (handlerIt->first->onTick(handlerParams.tickParam))
                // обновляем время последнего тика
                handlerParams.lastTickTime = tick_clock::now();
            else
            {
                // если надо прекратить тики - удалем информацию по этому хэндлеру
                handlerIt = m_tickHandlers.erase(handlerIt);
                continue;
            }
        }

        ++handlerIt;
    }

    // возможно отключили таймер во время тика - проверяем его необходимость в наличии
    checkTimer();
}

//----------------------------------------------------------------------------//
// Запустить таймер
inline void CTickService::startTimer()
{
    assert(!m_timerId.has_value());

    m_timerId = ::SetTimer(NULL, NULL,
                           (UINT)kDefTickInterval.count(),
                           &CTickService::timerProc);
}

//----------------------------------------------------------------------------//
// Остановить таймер
inline void CTickService::stopTimer()
{
    // отключаем таймер
    if (m_timerId.has_value())
    {
        ::KillTimer(0, *m_timerId);
        m_timerId.reset();
    }
}

//----------------------------------------------------------------------------//
// Проверить наличие таймера
inline void CTickService::checkTimer()
{
    // Наличие обработчиков должно совпасть с наличием идентификатора таймера
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
// Подключить обработчик тиков.
inline void CTickHandlerImpl::subscribeTimer(tick_clock::duration tickInterval,
                                             const TickParam& tickParam)
{
    get_service<CTickService>().subscribeHandler(this, tickInterval, tickParam);
}

//----------------------------------------------------------------------------//
// Удалить обработчик тиков из списка.
inline void CTickHandlerImpl::unsubscribeTimer()
{
    get_service<CTickService>().unsubscribeHandler(this);
}

//----------------------------------------------------------------------------//
// Удалить обработчик тиков из списка с совпадающим параметром.
inline void CTickHandlerImpl::unsubscribeTimer(const TickParam& tickParam)
{
    get_service<CTickService>().unsubscribeHandler(this, tickParam);
}

//----------------------------------------------------------------------------//
// Проверка есть ли таймер у этого обработчика с заданным параметром
inline bool CTickHandlerImpl::isTimerExist(const TickParam& tickParam)
{
    return get_service<CTickService>().isTimerExist(this, tickParam);
}
