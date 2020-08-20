#pragma once

#include <bitset>
#include <list>
#include <memory>
#include <optional>
#include <set>

#include "Messages.h"

// ���������� �� ��������� � ������ ����������� �������
// {BFE83474-9AC0-49CE-B26E-752657C366F3}
constexpr EventId onMonitoringListChanged =
{ 0xbfe83474, 0x9ac0, 0x49ce, { 0xb2, 0x6e, 0x75, 0x26, 0x57, 0xc3, 0x66, 0xf3 } };

// ���������� � ������ ������������� �������� ��������� � ���, �� LogMessageData � send_message_to_log
// {5B436604-D55E-4112-AC8C-B0521314BA25}
constexpr EventId onNewLogMessageEvent =
{ 0x5b436604, 0xd55e, 0x4112, { 0xac, 0x8c, 0xb0, 0x52, 0x13, 0x14, 0xba, 0x25 } };

// ���������� � ������ ��������� ������ � �����������, �� MonitoringErrorEventData
// {FB0BF8DC-0AA5-41AC-B018-4CFE11CF14BE}
constexpr EventId onMonitoringErrorEvent =
{ 0xfb0bf8dc, 0xaa5, 0x41ac, { 0xb0, 0x18, 0x4c, 0xfe, 0x11, 0xcf, 0x14, 0xbe } };

// ���������� ��� ������������ ������, �� MessageTextData
// {39A54983-53D5-4D2B-9290-23E2D85ABA5F}
constexpr EventId onReportPreparedEvent =
{ 0x39a54983, 0x53d5, 0x4d2b, { 0x92, 0x90, 0x23, 0xe2, 0xd8, 0x5a, 0xba, 0x5f } };

////////////////////////////////////////////////////////////////////////////////
// ������ ������
struct TrendChannelData
{
    // ��������� �������� � ������ ���������
    float startValue = 0.f;
    // ������� ��������
    float currentValue = 0.f;
    // ������������ �������� �� ����������� ��������
    float maxValue = 0.f;
    // ����������� �������� �� ����������� ��������
    float minValue = 0.f;
    // ����� ��� ������(�������� ������) �� ����������� ��������
    CTimeSpan emptyDataTime = 0;
    // ����� ��������� "������������" ������ �� ������
    CTime lastDataExistTime;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ��� ����������� ������
enum class MonitoringInterval
{
    eOneDay,            // ���� ����
    eOneWeek,           // ���� ������
    eTwoWeeks,          // ��� ������
    eOneMonth,          // ���� �����
    eThreeMonths,       // ��� ������
    eHalfYear,          // ��� ����
    eOneYear,           // ���� ���
    eLast
};

////////////////////////////////////////////////////////////////////////////////
// ��������� � ������� � ������
struct MonitoringChannelData
{
    // �������� ������
    CString channelName;
    // ��������� �� ���������� �������������
    bool bNotify = true;
    // ����������� ��������
    MonitoringInterval monitoringInterval = MonitoringInterval::eOneMonth;
    // ��������, ��������� ������� ���������� ����������. �� ��������� - NAN
    float allarmingValue = NAN;
    // ������ �� ������������ ������, ���� �� eDataLoaded - �� ��������/������
    TrendChannelData trendData;

    // ��������� ������
    enum ChannelState
    {
        // ������
        //eWaitingForData,           // ���� �������� ������
        eDataLoaded,                // ������ ��������� �������
        eLoadingError,              // ������ ��� �������� ������
        //eErrorOnUpdatingData,       // �������� ������ ��� ���������� ������

        // ����������
        eReportedFallenOff,         // ��������� ���������� ������������� �� ����������� �������
        eReportedExcessOfValue,     // ��������� ���������� ������������� � ���������� ����������� ��������
        eReportedALotOfEmptyData,   // ��������� ���������� ������������� � ������� ���������� ���������

        eLast
    };
    std::bitset<ChannelState::eLast> channelState;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� �������� ����
struct TelegramBotSettings
{
    // ��������� ������ ����
    bool bEnable = false;
    // ����� ����
    CString sToken;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ������������ � ������� onNewLogMessageEvent
struct LogMessageData : public IEventData
{
    // ��� ���������
    enum class MessageType
    {
        eError,         // ��������� �� ������, � ���� ���������� ������� ������
        eOrdinary,      // ������� ���������
    };
    MessageType messageType = MessageType::eOrdinary;
    // ����� ��������� ��� ����
    CString logMessage;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ������������ � ������ ������������� ������ �����������(onMonitoringErrorEvent)
struct MonitoringErrorEventData : public IEventData
{
    // ����� ������
    CString errorText;
    // ������������� ������
    GUID errorGUID = {};
};

////////////////////////////////////////////////////////////////////////////////
// ��������� � ������� ������������ � ��������
struct MessageTextData : public IEventData
{
    // ����� ���������
    CString messageText;
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ��� ����������� ������, ������������ ��� ��������� ������ ������� � ���������� ��
interface ITrendMonitoring
{
    virtual ~ITrendMonitoring() = default;

#pragma region ����� ������� ��� ������� �������
    /// <summary>�������� ������ ��� ���� ��������� ��� ����������� �������.</summary>
    virtual std::set<CString> getNamesOfAllChannels() = 0;
    /// <summary>�������� ������ ���� ������� �� ������� ���������� ����������.</summary>
    virtual std::list<CString> getNamesOfMonitoringChannels() = 0;

    /// <summary>�������� ������ ��� ���� �������.</summary>
    virtual void updateDataForAllChannels() = 0;

    /// <summary>�������� ���������� ����������� �������.</summary>
    virtual size_t getNumberOfMonitoringChannels() = 0;
    /// <summary>�������� ������ ��� ������������ ������ �� �������.</summary>
    /// <param name="channelIndex">������ � ������ �������.</param>
    /// <returns>������ ��� ������.</returns>
    virtual const MonitoringChannelData& getMonitoringChannelData(const size_t channelIndex) = 0;
#pragma endregion ����� ������� ��� ������� �������

#pragma region ���������� ������� �������
    /// <summary>�������� ����� ��� �����������.</summary>
    /// <returns>������ ������������ ������ � ������.</returns>
    virtual size_t addMonitoingChannel() = 0;
    /// <summary>������� ����������� ����� �� ������ �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>������ ��������� ����� ��������.</returns>
    virtual size_t removeMonitoringChannelByIndex(const size_t channelIndex) = 0;

    /// <summary>�������� ���� ���������� � ������ �� ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newNotifyState">����� ��������� ����������.</param>
    virtual void changeMonitoingChannelNotify(const size_t channelIndex,
                                              const bool newNotifyState) = 0;
    /// <summary>�������� ��� ������������ ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newChannelName">����� ��� ������.</param>
    virtual void changeMonitoingChannelName(const size_t channelIndex,
                                            const CString& newChannelName) = 0;
    /// <summary>�������� �������� ����������� ������ ��� ������������ ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newInterval">����� �������� �����������.</param>
    virtual void changeMonitoingChannelInterval(const size_t channelIndex,
                                                const MonitoringInterval newInterval) = 0;
    /// <summary>�������� �������� �� ���������� �������� ����� ����������� ����������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newValue">����� �������� � �����������.</param>
    virtual void changeMonitoingChannelAllarmingValue(const size_t channelIndex,
                                                      const float newValue) = 0;


    /// <summary>����������� ����� �� ������ ����� �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>����� ������ ������.</returns>
    virtual size_t moveUpMonitoingChannelByIndex(const size_t channelIndex) = 0;
    /// <summary>����������� ���� �� ������ ����� �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>����� ������ ������.</returns>
    virtual size_t moveDownMonitoingChannelByIndex(const size_t channelIndex) = 0;
#pragma endregion ���������� ������� �������

#pragma region ���������� �����
    // �������� ��������� ���� ���������
    virtual const TelegramBotSettings& getBotSettings() = 0;
    // ���������� ��������� ���� ���������
    virtual void setBotSettings(const TelegramBotSettings& newSettings) = 0;
#pragma endregion ���������� �����
};

// ��������� ������� ��� �����������
ITrendMonitoring* get_monitoing_service();

////////////////////////////////////////////////////////////////////////////////
// ����������� ��������� ����������� � �����
inline CString monitoring_interval_to_string(const MonitoringInterval interval)
{
    switch (interval)
    {
    default:
        assert(!"����������� �������� �����������");
        [[fallthrough]];
    case MonitoringInterval::eOneDay:
        return L"����";
    case MonitoringInterval::eOneWeek:
        return L"������";
    case MonitoringInterval::eTwoWeeks:
        return L"��� ������";
    case MonitoringInterval::eOneMonth:
        return L"�����";
    case MonitoringInterval::eThreeMonths:
        return L"��� ������";
    case MonitoringInterval::eHalfYear:
        return L"��� ����";
    case MonitoringInterval::eOneYear:
        return L"���";
    }
}

//------------------------------------------------------------------------//
// ����������� ��������� ����������� � ���������� �������
inline CTimeSpan monitoring_interval_to_timespan(const MonitoringInterval interval)
{
    switch (interval)
    {
    default:
        assert(!"����������� �������� �����������");
        [[fallthrough]];
    case MonitoringInterval::eOneDay:
        return CTimeSpan(1, 0, 0, 0);
    case MonitoringInterval::eOneWeek:
        return CTimeSpan(7, 0, 0, 0);
    case MonitoringInterval::eTwoWeeks:
        return CTimeSpan(14, 0, 0, 0);
    case MonitoringInterval::eOneMonth:
        return CTimeSpan(30, 0, 0, 0);
    case MonitoringInterval::eThreeMonths:
        return CTimeSpan(91, 0, 0, 0);
    case MonitoringInterval::eHalfYear:
        return CTimeSpan(183, 0, 0, 0);
    case MonitoringInterval::eOneYear:
        return CTimeSpan(365, 0, 0, 0);
    }
}

//------------------------------------------------------------------------//
// ����������� ���������� ���������� � �����
inline CString time_span_to_string(const CTimeSpan& value)
{
    CString res;

    if (auto countDays = value.GetDays(); countDays > 0)
    {
        LONGLONG countHours = (value - CTimeSpan((LONG)countDays, 0, 0, 0)).GetTotalHours();
        if (countHours == 0)
            res.Format(L"%lld ����", countDays);
        else
            res.Format(L"%lld ���� %lld �����", countDays, countHours);
    }
    else if (auto countHours = value.GetTotalHours(); countHours > 0)
    {
        LONGLONG countMinutes = (value - CTimeSpan(0, (LONG)countHours, 0, 0)).GetTotalMinutes();
        if (countMinutes == 0)
            res.Format(L"%lld �����", countHours);
        else
            res.Format(L"%lld ����� %lld �����", countHours, countMinutes);
    }
    else if (auto countMinutes = value.GetTotalMinutes(); countMinutes > 0)
    {
        LONGLONG countSeconds = (value - CTimeSpan(0, 0, (LONG)countMinutes, 0)).GetTotalSeconds();
        if (countSeconds == 0)
            res.Format(L"%lld �����", countMinutes);
        else
            res.Format(L"%lld ����� %lld ������", countMinutes, countSeconds);
    }
    else if (auto countSeconds = value.GetTotalSeconds(); countSeconds > 0)
        res.Format(L"%lld ������", countSeconds);
    else
        res = L"��� ���������";

    assert(!res.IsEmpty());

    return res;
}

//----------------------------------------------------------------------------//
template <typename... Args>
inline void send_message_to_log(const LogMessageData::MessageType type, Args&&... textFormat)
{
    // ��������� � ��������� ������
    auto logMessage = std::make_shared<LogMessageData>();
    logMessage->messageType = type;
    logMessage->logMessage.Format(textFormat...);

    get_service<CMassages>().postMessage(onNewLogMessageEvent, 0,
                                         std::static_pointer_cast<IEventData>(logMessage));
}
