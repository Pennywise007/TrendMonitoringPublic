#include "pch.h"

#include <ctime>

#include <DirsService.h>

#include "ChannelDataCorrector/ChannelDataGetter.h"
#include "Serialization/SerializatorFabric.h"
#include "TrendMonitoring.h"
#include "Utils.h"

// �������� ���������� ������
const std::chrono::minutes kUpdateDataInterval(5);

// ����� � ������� ����� ���������� ����� ������ ���� ���� + ������ (20:00)
const std::pair<int, int> kReportDataTime = std::make_pair(20, 00);

////////////////////////////////////////////////////////////////////////////////
ITrendMonitoring* get_monitoing_service()
{
    return &get_service<TrendMonitoring>();
}

////////////////////////////////////////////////////////////////////////////////
// ���������� ������� ��� ����������� �������
TrendMonitoring::TrendMonitoring()
{
    // ������������� �� ������� � ���������� �����������
    EventRecipientImpl::subscribe(onCompletingMonitoringTask);
    // ������������� �� ������� ��������� � ������ ������������� ����������
    EventRecipientImpl::subscribe(onUsersListChangedEvent);

    // ��������� ������������ �� �����
    loadConfiguration();

    // �������������� ���� ����� �������� ������������
    m_telegramBot.initBot(m_appConfig->getTelegramUsers());
    // �������� ���� ��������� �� ��������
    m_telegramBot.setBotSettings(getBotSettings());

    // �������� ������� ��� �����������, ������ ���������� ��������� ��� �����
    // ����� ��������� ������ ��� ����������
    for (auto& channel : m_appConfig->m_chanelParameters)
        addMonitoringTaskForChannel(channel, TaskInfo::TaskType::eIntervalInfo);

    // ���������� ������ ���������� ������
    CTickHandlerImpl::subscribeTimer(kUpdateDataInterval, TimerType::eUpdatingData);

    // ���������� ������ ������
    {
        // ������������ ����� ��������� ������
        time_t nextReportTime_t = time(NULL);

        tm nextReportTm;
        localtime_s(&nextReportTm, &nextReportTime_t);

        // ��������� ��� � ���� ���� ��� �� ������� ����� ��� ������
        bool needReportToday = false;
        if (nextReportTm.tm_hour < kReportDataTime.first ||
            (nextReportTm.tm_hour == kReportDataTime.first &&
             nextReportTm.tm_min < kReportDataTime.second))
            needReportToday = true;

        // ������������ ����� ���� ������
        nextReportTm.tm_hour = kReportDataTime.first;
        nextReportTm.tm_min  = kReportDataTime.second;
        nextReportTm.tm_sec  = 0;

        // ���� �� ������� - ������ ���� ����� ����� ��� ������
        if (!needReportToday)
            ++nextReportTm.tm_mday;

        // ������������ � std::chrono
        std::chrono::system_clock::time_point nextReportTime =
            std::chrono::system_clock::from_time_t(mktime(&nextReportTm));

        // ���������� ������ � ���������� �� ���� ������
        CTickHandlerImpl::subscribeTimer(nextReportTime - std::chrono::system_clock::now(),
                                         TimerType::eEveryDayReporting);
    }
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
std::set<CString> TrendMonitoring::getNamesOfAllChannels()
{
    // �������� ��� ������ �� �������
    std::map<CString, CString> channelsWithConversion;
    // ���� ������ � ���������� � ������� ��������� ��� ��� ������ ����������� � ����� ��� �������
    CChannelDataGetter::FillChannelList(get_service<DirsService>().getZetCompressedDir(), channelsWithConversion);

    // ��������� ������������� ������ �������
    std::set<CString> allChannelsNames;
    for (const auto& channelInfo : channelsWithConversion)
        allChannelsNames.emplace(channelInfo.first);

    return allChannelsNames;
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
std::list<CString> TrendMonitoring::getNamesOfMonitoringChannels()
{
    // ��������� ������������� ������ �������
    std::list<CString> allChannelsNames;
    for (const auto& channel :  m_appConfig->m_chanelParameters)
        allChannelsNames.emplace_back(channel->channelName);

    return allChannelsNames;
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
void TrendMonitoring::updateDataForAllChannels()
{
    // ������� ������ ��� ���� ������� � ��������� ������� �� ����������
    for (auto& channel : m_appConfig->m_chanelParameters)
    {
        // ������� ������� ����������� ��� ������
        delMonitoringTaskForChannel(channel);
        // ������� ������
        channel->resetChannelData();
        // ��������� ����� �������, ������ �� ������ ����� ���� �������� ����������
        addMonitoringTaskForChannel(channel, TaskInfo::TaskType::eIntervalInfo);
    }

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
size_t TrendMonitoring::getNumberOfMonitoringChannels()
{
    return m_appConfig->m_chanelParameters.size();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
const MonitoringChannelData& TrendMonitoring::getMonitoringChannelData(const size_t channelIndex)
{
    assert(channelIndex < m_appConfig->m_chanelParameters.size() && "���������� ������� ������ ��� ������ ������");
    return (*std::next(m_appConfig->m_chanelParameters.begin(), channelIndex))->getMonitoringData();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
size_t TrendMonitoring::addMonitoingChannel()
{
    const auto& channelsList = getNamesOfAllChannels();

    if (channelsList.empty())
    {
        ::MessageBox(NULL, L"������ ��� ����������� �� �������", L"���������� �������� ����� ��� �����������", MB_OK | MB_ICONERROR);
        return 0;
    }

    m_appConfig->m_chanelParameters.push_back(ChannelParameters::make(*channelsList.begin()));

    // �������� ������� ������
    addMonitoringTaskForChannel(m_appConfig->m_chanelParameters.back(),
                                TaskInfo::TaskType::eIntervalInfo);

    // �������� �� ��������� � ������ ������� ���������, ����� � ������ ������� ����� ��������� �����
    onMonitoringChannelsListChanged(false);

    return m_appConfig->m_chanelParameters.size() - 1;
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
size_t TrendMonitoring::removeMonitoringChannelByIndex(const size_t channelIndex)
{
    auto& channelsList = m_appConfig->m_chanelParameters;

    if (channelIndex >= channelsList.size())
    {
        assert(!"���������� ������� ������ ��� ������ ���������� ������");
        return channelIndex;
    }

    // �������� �������� �� �����
    ChannelIt channelIt = std::next(channelsList.begin(), channelIndex);

    // ���������� ������� ����������� ��� ������
    delMonitoringTaskForChannel(*channelIt);

    // ������� �� ����� �������
    channelIt = channelsList.erase(channelIt);
    if (channelIt == channelsList.end() && !channelsList.empty())
        --channelIt;

    size_t result = std::distance(channelsList.begin(), channelIt);

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();

    return result;
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
void TrendMonitoring::changeMonitoingChannelNotify(const size_t channelIndex,
                                                   const bool newNotifyState)
{
    if (channelIndex >= m_appConfig->m_chanelParameters.size())
    {
        ::MessageBox(NULL, L"������ ��� � ������", L"���������� �������� ����������� ������", MB_OK | MB_ICONERROR);
        return;
    }

    // �������� ��������� ������ �� �������� ������ ���
    ChannelParameters::Ptr channelParams = *std::next(m_appConfig->m_chanelParameters.begin(),
                                                      channelIndex);
    if (!channelParams->changeNotification(newNotifyState))
        return;

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged(true);
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
void TrendMonitoring::changeMonitoingChannelName(const size_t channelIndex,
                                                 const CString& newChannelName)
{
    if (channelIndex >= m_appConfig->m_chanelParameters.size())
    {
        ::MessageBox(NULL, L"������ ��� � ������", L"���������� �������� ��� ������", MB_OK | MB_ICONERROR);
        return;
    }

    // �������� ��������� ������ �� �������� ������ ���
    ChannelParameters::Ptr channelParams = *std::next(m_appConfig->m_chanelParameters.begin(), channelIndex);
    if (!channelParams->changeName(newChannelName))
        return;

    // ���� ��� ���������� ������� ��������� ��������� ������� �� ������
    delMonitoringTaskForChannel(channelParams);
    // ��������� ����� ������� ��� �����������
    addMonitoringTaskForChannel(channelParams, TaskInfo::TaskType::eIntervalInfo);

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
void TrendMonitoring::changeMonitoingChannelInterval(const size_t channelIndex,
                                                     const MonitoringInterval newInterval)
{
    if (channelIndex >= m_appConfig->m_chanelParameters.size())
    {
        ::MessageBox(NULL, L"������ ��� � ������", L"���������� �������� �������� ����������", MB_OK | MB_ICONERROR);
        return;
    }

    ChannelParameters::Ptr channelParams = *std::next(m_appConfig->m_chanelParameters.begin(), channelIndex);
    if (!channelParams->changeInterval(newInterval))
        return;

    // ���� ��� ���������� ������� ��������� ��������� ������� �� ������
    delMonitoringTaskForChannel(channelParams);
    // ��������� ����� ������� ��� �����������
    addMonitoringTaskForChannel(channelParams, TaskInfo::TaskType::eIntervalInfo);

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
void TrendMonitoring::changeMonitoingChannelAllarmingValue(const size_t channelIndex,
                                                           const float newValue)
{
    if (channelIndex >= m_appConfig->m_chanelParameters.size())
    {
        ::MessageBox(NULL, L"������ ��� � ������", L"���������� �������� �������� ����������", MB_OK | MB_ICONERROR);
        return;
    }

    ChannelParameters::Ptr channelParams = *std::next(m_appConfig->m_chanelParameters.begin(), channelIndex);
    if (!channelParams->changeAllarmingValue(newValue))
        return;

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
size_t TrendMonitoring::moveUpMonitoingChannelByIndex(const size_t channelIndex)
{
    auto& channelsList = m_appConfig->m_chanelParameters;

    if (channelsList.size() < 2)
        return channelIndex;

    ChannelIt movingIt = std::next(channelsList.begin(), channelIndex);

    // ������ ������� ������ ����� �����������
    size_t resultIndex;

    if (movingIt == channelsList.begin())
    {
        // ������� � �����
        channelsList.splice(channelsList.end(), channelsList, movingIt);
        resultIndex = channelsList.size() - 1;
    }
    else
    {
        // ������ � ���������� �������
        std::iter_swap(movingIt, std::prev(movingIt));
        resultIndex = channelIndex - 1;
    }

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();

    return resultIndex;
}

//----------------------------------------------------------------------------//
// ITrendMonitoring
size_t TrendMonitoring::moveDownMonitoingChannelByIndex(const size_t channelIndex)
{
    auto& channelsList = m_appConfig->m_chanelParameters;

    if (channelsList.size() < 2)
        return channelIndex;

    ChannelIt movingIt = std::next(channelsList.begin(), channelIndex);

    // ������ ������� ������ ����� �����������
    size_t resultIndex;

    if (movingIt == --channelsList.end())
    {
        // ������� � ������
        channelsList.splice(channelsList.begin(), channelsList, movingIt);
        resultIndex = 0;
    }
    else
    {
        // ������ �� ��������� �������
        std::iter_swap(movingIt, std::next(movingIt));
        resultIndex = channelIndex + 1;
    }

    // �������� �� ��������� � ������ �������
    onMonitoringChannelsListChanged();

    return resultIndex;
}

//----------------------------------------------------------------------------//
const TelegramBotSettings& TrendMonitoring::getBotSettings()
{
    return m_appConfig->getTelegramSettings();
}

//----------------------------------------------------------------------------//
void TrendMonitoring::setBotSettings(const TelegramBotSettings& newSettings)
{
    // ��������� ���������
    m_appConfig->setTelegramSettings(newSettings);

    // ��������� ���������
    saveConfiguration();

    // �������������� ���� ����� �������
    m_telegramBot.setBotSettings(getBotSettings());
}

//----------------------------------------------------------------------------//
bool TrendMonitoring::handleIntervalInfoResult(const MonitoringResult::ResultData& monitoringResult,
                                               ChannelParameters* channelParameters,
                                               CString& allertText)
{
    switch (monitoringResult.resultType)
    {
    case MonitoringResult::Result::eSucceeded:  // ������ ������� ��������
        {
            // ����������� ����� ������ �� �������
            channelParameters->setTrendChannelData(monitoringResult);

            channelParameters->channelState[MonitoringChannelData::eDataLoaded] = true;
        }
        break;
    case MonitoringResult::Result::eNoData:     // � ���������� ��������� ��� ������
    case MonitoringResult::Result::eErrorText:  // �������� ������
        {
            // ��������� � ��������� ������
            if (monitoringResult.resultType == MonitoringResult::Result::eErrorText &&
                !monitoringResult.errorText.IsEmpty())
                allertText = monitoringResult.errorText;
            else
                allertText = L"��� ������ � ����������� ���������.";

            // ��������� ����� ��� ������
            channelParameters->trendData.emptyDataTime = monitoringResult.emptyDataTime;
            // �������� ��� �������� ������ ��������
            channelParameters->channelState[MonitoringChannelData::eLoadingError] = true;
        }
        break;
    default:
        assert(!"�� ��������� ��� ����������");
        break;
    }

    // ������ ���� ��� ����������
    return true;
}

//----------------------------------------------------------------------------//
bool TrendMonitoring::handleUpdatingResult(const MonitoringResult::ResultData& monitoringResult,
                                           ChannelParameters* channelParameters,
                                           CString& allertText)
{
    bool bDataChanged = false;
    switch (monitoringResult.resultType)
    {
    case MonitoringResult::Result::eSucceeded:  // ������ ������� ��������
        {
            // ���� ������ ��� �� ���� ��������
            if (!channelParameters->channelState[MonitoringChannelData::eDataLoaded])
                // ����������� ����� ������ �� �������
                channelParameters->setTrendChannelData(monitoringResult);
            else
            {
                // ������ ������ � ����� ������
                {
                    channelParameters->trendData.currentValue = monitoringResult.currentValue;

                    if (channelParameters->trendData.maxValue < monitoringResult.maxValue)
                        channelParameters->trendData.maxValue = monitoringResult.maxValue;
                    if (channelParameters->trendData.minValue > monitoringResult.minValue)
                        channelParameters->trendData.minValue = monitoringResult.minValue;

                    channelParameters->trendData.emptyDataTime += monitoringResult.emptyDataTime;
                    channelParameters->trendData.lastDataExistTime = monitoringResult.lastDataExistTime;
                }
            }

            // ����������� ����� ������
            {
                // ���� ����������� ���������� � �� �������� �� ���� �������� ������
                if (_finite(channelParameters->allarmingValue) != 0)
                {
                    // ���� �� �������� ��� �������� ����� �� ����������
                    if ((channelParameters->allarmingValue >= 0 &&
                         monitoringResult.minValue >= channelParameters->allarmingValue) ||
                         (channelParameters->allarmingValue < 0 &&
                          monitoringResult.maxValue <= channelParameters->allarmingValue))
                    {
                        allertText.AppendFormat(L"���������� ���������� ��������. ���������� �������� %.02f, �������� [%.02f..%.02f].",
                                                channelParameters->allarmingValue,
                                                monitoringResult.minValue, monitoringResult.maxValue);
                        channelParameters->channelState[MonitoringChannelData::eReportedExcessOfValue] = true;
                    }
                }

                // ��������� ���������� ���������, ��������� ���� ������ ��� ������ ������ ��� �������� ������� ���������� ���������
                if (auto emptySeconds = monitoringResult.emptyDataTime.GetTotalMinutes();
                    emptySeconds > kUpdateDataInterval.count() / 2)
                {
                    allertText.Append(CString(allertText.IsEmpty() ? L"" : L" ") + L"����� ��������� ������.");

                    channelParameters->channelState[MonitoringChannelData::eReportedALotOfEmptyData] = true;
                }
            }

            channelParameters->channelState[MonitoringChannelData::eDataLoaded] = true;
            if (channelParameters->channelState[MonitoringChannelData::eLoadingError])
            {
                if (channelParameters->channelState[MonitoringChannelData::eReportedFallenOff])
                {
                    // ���� ������������ ������� ��� ������ ��� � �� �������� - �������� ��������� �������.
                    allertText.Append(CString(allertText.IsEmpty() ? L"" : L" ") + L"������ ��������.");
                    channelParameters->channelState[MonitoringChannelData::eReportedFallenOff] = false;
                }
                else
                    // �.�. ������ � ��� ��� ������ �� ������� �������� - ������� ��� ������ ��������
                    send_message_to_log(LogMessageData::MessageType::eOrdinary,
                                        L"����� \"%s\": ������ ��������.",
                                        channelParameters->channelName.GetString());

                // ���� ���� ������ ��� �������� ������ ������� ���� ������ � �������� ��� ������ ���������
                channelParameters->channelState[MonitoringChannelData::eLoadingError] = false;
            }

            // ��������� �� ��������� � ������ ��� �����������
            bDataChanged = true;
        }
        break;
    case MonitoringResult::Result::eNoData:     // � ���������� ��������� ��� ������
    case MonitoringResult::Result::eErrorText:  // �������� ������
        {
            // ��������� ����� ��� ������
            channelParameters->trendData.emptyDataTime += monitoringResult.emptyDataTime;

            // �������� ��� �������� ������ ��������
            if (!channelParameters->channelState[MonitoringChannelData::eLoadingError])
            {
                send_message_to_log(LogMessageData::MessageType::eOrdinary,
                                    L"����� \"%s\": �� ������� �������� ������.",
                                    channelParameters->channelName.GetString());

                channelParameters->channelState[MonitoringChannelData::eLoadingError] = true;
            }

            // ��������� ��������� ������ ���� ��� ���� ��������� ������
            if (channelParameters->channelState[MonitoringChannelData::eDataLoaded])
                // ��������� �� ��������� � ������ ��� �����������
                bDataChanged = true;

            // ���� �� ������ ���� ��������� ������, � ������ ��������� �� ����������
            // ������ ��������� ���������� - ��������� ���� �� ��������� �� ����
            if (channelParameters->channelState[MonitoringChannelData::eDataLoaded] &&
                !channelParameters->channelState[MonitoringChannelData::eReportedFallenOff])
            {
                // ���� ������ 3 ���������� ������ � ������� ��������� ������ �� ������, � ������ ��� ��� ��� - ������ ���������
                if ((CTime::GetCurrentTime() - channelParameters->trendData.lastDataExistTime).GetTotalMinutes() >=
                    3 * kUpdateDataInterval.count())
                {
                    allertText = L"������� ������ �� ������.";
                    channelParameters->channelState[MonitoringChannelData::eReportedFallenOff] = true;
                }
            }
        }
        break;
    default:
        assert(!"�� ��������� ��� ����������");
        break;
    }

    return bDataChanged;
}

//----------------------------------------------------------------------------//
bool TrendMonitoring::handleEveryDayReportResult(const MonitoringResult::ResultData& monitoringResult,
                                                 ChannelParameters* channelParameters,
                                                 CString& allertText)
{
    // ������ �����������
    const MonitoringChannelData& monitoringData = channelParameters->getMonitoringData();

    switch (monitoringResult.resultType)
    {
    case MonitoringResult::Result::eSucceeded:  // ������ ������� ��������
        {
            // ���� ����������� ���������� ��� ���������� ��������
            if (_finite(monitoringData.allarmingValue) != 0)
            {
                // ���� �� �������� ���� �� �������� ����� �� ����������
                if ((monitoringData.allarmingValue >= 0 &&
                     monitoringResult.maxValue >= monitoringData.allarmingValue) ||
                     (monitoringData.allarmingValue < 0 &&
                      monitoringResult.minValue <= monitoringData.allarmingValue))
                {
                    allertText.AppendFormat(L"���������� ������� ��� ��������. ���������� �������� %.02f, �������� �� ���� [%.02f..%.02f].",
                                            monitoringData.allarmingValue,
                                            monitoringResult.minValue, monitoringResult.maxValue);
                }
            }

            // ���� ����� ��������� ������
            if (monitoringResult.emptyDataTime.GetTotalHours() > 2)
                allertText.AppendFormat(L"����� ��������� ������ (%lld �).",
                                        monitoringResult.emptyDataTime.GetTotalHours());
        }
        break;
    case MonitoringResult::Result::eNoData:     // � ���������� ��������� ��� ������
    case MonitoringResult::Result::eErrorText:  // �������� ������
        // �������� ��� ������ ���
        allertText = L"��� ������.";
        break;
    default:
        assert(!"�� ��������� ��� ����������");
        break;
    }

    // ������ ����������� ��� ��������� ������ �����������
    return false;
}

//----------------------------------------------------------------------------//
// IEventRecipient
void TrendMonitoring::onEvent(const EventId& code, float eventValue,
                              std::shared_ptr<IEventData> eventData)
{
    if (code == onCompletingMonitoringTask)
    {
        MonitoringResult::Ptr monitoringResult = std::static_pointer_cast<MonitoringResult>(eventData);

        auto it = m_monitoringTasksInfo.find(monitoringResult->m_taskId);
        if (it == m_monitoringTasksInfo.end())
            // ��������� �� ���� �������
            return;

        assert(!monitoringResult->m_taskResults.empty() && !it->second.channelParameters.empty());

        // ��������� �� ���������� �������
        ChannelIt channelIt  = it->second.channelParameters.begin();
        ChannelIt channelEnd = it->second.channelParameters.end();

        // ��������� �� ����������� �������
        MonitoringResult::ResultIt resultIt  = monitoringResult->m_taskResults.begin();
        MonitoringResult::ResultIt resultEnd = monitoringResult->m_taskResults.end();

        // ���� ��� � ������ ����������� ���� ���������
        bool bMonitoringListChanged = false;
        // ����� ������ �� ���� �������
        CString reportText;

        // ��� ������� ������ ����������� ��� ��������� �������
        for (; channelIt != channelEnd && resultIt != resultEnd; ++channelIt, ++resultIt)
        {
            assert((*channelIt)->channelName == resultIt->pTaskParameters->channelName &&
                   "�������� ���������� ��� ������� ������!");

            CString channelError;

            switch (it->second.taskType)
            {
            case TaskInfo::TaskType::eIntervalInfo:
                bMonitoringListChanged |= handleIntervalInfoResult(*resultIt, *channelIt, channelError);
                break;
            case TaskInfo::TaskType::eUpdatingInfo:
                bMonitoringListChanged |= handleUpdatingResult(*resultIt, *channelIt, channelError);
                break;
            case TaskInfo::TaskType::eEveryDayReport:
                bMonitoringListChanged |= handleEveryDayReportResult(*resultIt, *channelIt, channelError);
                break;
            }

            // ���� �������� ������ ��� ��������� ������ � ����� � ��� ���������
            if (!channelError.IsEmpty() && (*channelIt)->bNotify)
            {
                reportText.AppendFormat(L"����� \"%s\": %s\n",
                                        (*channelIt)->channelName.GetString(),
                                        channelError.GetString());
            }
        }

        reportText = reportText.Trim();

        // ���� �������� ������ ������������ �� �� ������� ��� ������� ���� �������
        if (!reportText.IsEmpty() || it->second.taskType == TaskInfo::TaskType::eEveryDayReport)
        {
            switch (it->second.taskType)
            {
            case TaskInfo::TaskType::eIntervalInfo:
            case TaskInfo::TaskType::eUpdatingInfo:
                {
                    // �������� � ��� ��� �������� ��������
                    send_message_to_log(LogMessageData::MessageType::eError, reportText);

                    // ��������� � ��������� ������
                    auto errorMessage = std::make_shared<MonitoringErrorEventData>();
                    errorMessage->errorText = std::move(reportText);
                    // ������� ������������� ������
                    if (!SUCCEEDED(CoCreateGuid(&errorMessage->errorGUID)))
                        assert(!"�� ������� ������� ����!");
                    get_service<CMassages>().postMessage(onMonitoringErrorEvent, 0,
                                                         std::static_pointer_cast<IEventData>(errorMessage));
                }
                break;
            case TaskInfo::TaskType::eEveryDayReport:
                {
                    // ���� �� � ��� �������� ������� ��� ��� ��
                    if (reportText.IsEmpty())
                        reportText = L"������ � �������.";

                    CString reportDelimer(L'*', 25);

                    // ������� ��������� �� ������
                    auto reportMessage = std::make_shared<MessageTextData>();
                    reportMessage->messageText.Format(L"%s\n\n���������� ����� �� %s\n\n%s\n%s",
                                                      reportDelimer.GetString(),
                                                      CTime::GetCurrentTime().Format(L"%d.%m.%Y").GetString(),
                                                      reportText.GetString(),
                                                      reportDelimer.GetString());

                    // ��������� � ������� ������
                    get_service<CMassages>().postMessage(onReportPreparedEvent, 0,
                                                         std::static_pointer_cast<IEventData>(reportMessage));

                    // �������� ������������� ���������
                    m_telegramBot.sendMessageToAdmins(reportMessage->messageText);
                }
                break;
            }
        }

        if (bMonitoringListChanged)
            // ��������� �� ��������� � ������ ��� �����������
            get_service<CMassages>().postMessage(onMonitoringListChanged);

        // ������� ������� �� ������
        m_monitoringTasksInfo.erase(it);
    }
    else if (code == onUsersListChangedEvent)
    {
        // ��������� ��������� � ������
        saveConfiguration();
    }
    else
        assert(!"����������� �������");
}

//----------------------------------------------------------------------------//
bool TrendMonitoring::onTick(TickParam tickParam)
{
    switch (TimerType(tickParam))
    {
    case TimerType::eUpdatingData:
        {
            CTime currentTime = CTime::GetCurrentTime();

            // �.�. ������ ����� ����������� � ������ ����� ����� ��� ������� ������ ������
            // ���� ������� � ������������ ����������, ��������� ������ ���������� �������
            std::list<TaskParameters::Ptr> listTaskParams;
            // ������ ����������� �������
            ChannelParametersList updatingDataChannels;

            // �������� �� ���� ������� � ������� �� ����� ���� �������� ������
            for (auto& channelParameters : m_appConfig->m_chanelParameters)
            {
                // ������ ��� �� ���������
                if (!channelParameters->channelState[MonitoringChannelData::eDataLoaded] &&
                    !channelParameters->channelState[MonitoringChannelData::eLoadingError])
                {
                    // ��������� ��� ����� �� ���
                    if (channelParameters->m_loadingParametersIntervalEnd.has_value() &&
                        (currentTime - *channelParameters->m_loadingParametersIntervalEnd).GetTotalMinutes() > 10)
                        send_message_to_log(LogMessageData::MessageType::eError,
                                            L"������ �� ������ %s �������� ������ 10 �����",
                                            channelParameters->channelName.GetString());

                    continue;
                }

                // ��� ������ ���� ������� ������ � ���������
                assert(channelParameters->m_loadingParametersIntervalEnd.has_value());

                // ���� � ������� �������� ������ �� ���������� �������(������ ��������� �� ��������)
                if (!channelParameters->m_loadingParametersIntervalEnd.has_value() ||
                    (currentTime - *channelParameters->m_loadingParametersIntervalEnd).GetTotalMinutes() <
                    kUpdateDataInterval.count() - 1)
                    continue;

                // ��������� ����� � ������ �����������
                updatingDataChannels.emplace_back(channelParameters);
                listTaskParams.emplace_back(new TaskParameters(channelParameters->channelName,
                                                               *channelParameters->m_loadingParametersIntervalEnd,
                                                               currentTime));
            }

            if (!listTaskParams.empty())
                addMonitoringTaskForChannels(updatingDataChannels, listTaskParams,
                                             TaskInfo::TaskType::eUpdatingInfo);
        }
        break;
    case TimerType::eEveryDayReporting:
        {
            // ���������� ������ � ���������� �� ���� ������
            CTickHandlerImpl::subscribeTimer(std::chrono::hours(24),
                                             TimerType::eEveryDayReporting);

            if (!m_appConfig->m_chanelParameters.empty())
            {
                // ����� ������� ������� �� ������� ��������� ����������
                ChannelParametersList channelsCopy;
                for (const auto& currentChannel : m_appConfig->m_chanelParameters)
                {
                    channelsCopy.push_back(ChannelParameters::make(currentChannel->channelName));
                    channelsCopy.back()->allarmingValue = currentChannel->allarmingValue;
                }

                // ��������� ������� ������������ ������ �� ��������� ����
                addMonitoringTaskForChannels(channelsCopy,
                                             TaskInfo::TaskType::eEveryDayReport,
                                             CTimeSpan(1, 0, 0, 0));
            }
        }
        // ����������� ���� ��� ������ �.�. �� ��������� �����
        return false;
    default:
        assert(!"����������� ������!");
        break;
    }

    return true;
}

//----------------------------------------------------------------------------//
void TrendMonitoring::saveConfiguration()
{
    ISerializator::Ptr serializator =
        SerializationFabric::createXMLSerializator(getConfigurationXMLFilePath());

    SerializationExecutor serializationExecutor;
    serializationExecutor.serializeObject(serializator, m_appConfig);
}

//----------------------------------------------------------------------------//
void TrendMonitoring::loadConfiguration()
{
    IDeserializator::Ptr deserializator =
        SerializationFabric::createXMLDeserializator(getConfigurationXMLFilePath());

    SerializationExecutor serializationExecutor;
    serializationExecutor.deserializeObject(deserializator, m_appConfig);
}

//----------------------------------------------------------------------------//
CString TrendMonitoring::getConfigurationXMLFilePath()
{
    return get_service<DirsService>().getExeDir() + kConfigFileName;
}

//----------------------------------------------------------------------------//
void TrendMonitoring::onMonitoringChannelsListChanged(bool bAsynchNotify /*= true*/)
{
    // ��������� ����� ������ �����������
    saveConfiguration();

    // ��������� �� ��������� � ������ ��� �����������
    if (bAsynchNotify)
        get_service<CMassages>().postMessage(onMonitoringListChanged);
    else
        get_service<CMassages>().sendMessage(onMonitoringListChanged);
}

//----------------------------------------------------------------------------//
void TrendMonitoring::addMonitoringTaskForChannel(ChannelParameters::Ptr& channelParams,
                                                  const TaskInfo::TaskType taskType,
                                                  CTimeSpan monitoringInterval /* = -1*/)
{
    if (monitoringInterval == -1)
        monitoringInterval =
            monitoring_interval_to_timespan(channelParams->monitoringInterval);

    addMonitoringTaskForChannels({ channelParams }, taskType, monitoringInterval);
}

//----------------------------------------------------------------------------//
void TrendMonitoring::addMonitoringTaskForChannels(const ChannelParametersList& channelList,
                                                   const TaskInfo::TaskType taskType,
                                                   CTimeSpan monitoringInterval)
{
    // ��������� ��������� �� ������� ����� ��������� �������
    CTime stopTime = CTime::GetCurrentTime();
    CTime startTime = stopTime - monitoringInterval;

    // ������ ���������� �������
    std::list<TaskParameters::Ptr> listTaskParams;
    for (auto& channelParams : channelList)
    {
        listTaskParams.emplace_back(new TaskParameters(channelParams->channelName,
                                                       startTime, stopTime));
    }

    addMonitoringTaskForChannels(channelList, listTaskParams, taskType);
}

//----------------------------------------------------------------------------//
void TrendMonitoring::addMonitoringTaskForChannels(const ChannelParametersList& channelList,
                                                   const std::list<TaskParameters::Ptr>& taskParams,
                                                   const TaskInfo::TaskType taskType)
{
    if (channelList.size() != taskParams.size())
    {
        assert(!"����������� ������ ������� � �������!");
        return;
    }

    if (taskParams.empty())
    {
        assert(!"�������� ������ ������ �������!");
        return;
    }

    if (taskType != TaskInfo::TaskType::eEveryDayReport)
    {
        // ��� ������� ������� ������ ���������� ����� ������ ����������� ����������
        auto channelsIt = channelList.begin(), channelsItEnd = channelList.end();
        auto taskIt = taskParams.cbegin(), taskItEnd = taskParams.cend();
        for (; taskIt != taskItEnd && channelsIt != channelsItEnd; ++taskIt, ++channelsIt)
        {
            // ���������� �������� ��������� ��������
            (*channelsIt)->m_loadingParametersIntervalEnd = (*taskIt)->endTime;
        }
    }

    TaskInfo taskInfo;
    taskInfo.taskType = taskType;
    taskInfo.channelParameters = channelList;

    // ��������� ������� ���������� ������
    m_monitoringTasksInfo.try_emplace(
        get_monitoing_tasks_service()->addTaskList(taskParams, IMonitoringTasksService::eNormal),
        taskInfo);
}

//----------------------------------------------------------------------------//
void TrendMonitoring::delMonitoringTaskForChannel(const ChannelParameters::Ptr& channelParams)
{
    // �������� �� ���� ��������
    for (auto monitoringTaskIt = m_monitoringTasksInfo.begin(), end = m_monitoringTasksInfo.end();
         monitoringTaskIt != end;)
    {
        switch (monitoringTaskIt->second.taskType)
        {
        case TaskInfo::TaskType::eIntervalInfo:
        case TaskInfo::TaskType::eUpdatingInfo:
            {
                // ������ ������� �� ������� �������� �������
                auto& taskChannels = monitoringTaskIt->second.channelParameters;

                // ���� � ������ ������� ��� �����
                auto it = std::find(taskChannels.begin(), taskChannels.end(), channelParams);
                if (it != taskChannels.end())
                {
                    // �������� ��������� ����� �� �������� ���������
                    *it = nullptr;

                    // ��������� ��� � ������� �������� �� ������ ������
                    if (std::all_of(taskChannels.begin(), taskChannels.end(),
                                    [](const auto& el)
                                    {
                                        return el == nullptr;
                                    }))
                    {
                        // �� ������ ������� �� �������� - ����� ������� �������

                        get_monitoing_tasks_service()->removeTask(monitoringTaskIt->first);
                        monitoringTaskIt = m_monitoringTasksInfo.erase(monitoringTaskIt);

                        break;
                    }
                }

                ++monitoringTaskIt;
            }
            break;
        default:
            ++monitoringTaskIt;
            break;
        }
    }
}
