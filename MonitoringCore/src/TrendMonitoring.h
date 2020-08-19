#pragma once

#include "Messages.h"
#include "Singleton.h"
#include "TickService.h"

#include "ApplicationConfiguration.h"
#include "IMonitoringTasksService.h"
#include "ITrendMonitoring.h"

#include "TelegramBot.h"

////////////////////////////////////////////////////////////////////////////////
// ���������� ������� ��� ����������� �������
// ����������� ���������� ������� ����������� �������, ����������� ������� ��� �����������
class TrendMonitoring
    : public ITrendMonitoring
    , public EventRecipientImpl
    , public CTickHandlerImpl
{
    friend class CSingleton<TrendMonitoring>;

public:
    TrendMonitoring();

// ITrendMonitoring
public:
    /// <summary>�������� ������ ���� ���� �������� �������.</summary>
    std::set<CString> getNamesOfAllChannels() override;
    /// <summary>�������� ������ ���� ������� �� ������� ���������� ����������.</summary>
    std::list<CString> getNamesOfMonitoringChannels() override;

    /// <summary>�������� ������ ��� ���� �������.</summary>
    void updateDataForAllChannels() override;

    /// <summary>�������� ���������� ����������� �������.</summary>
    size_t getNumberOfMonitoringChannels() override;
    /// <summary>�������� ������ ��� ������������ ������ �� �������.</summary>
    /// <param name="channelIndex">������ � ������ �������.</param>
    /// <returns>������ ��� ������.</returns>
    const MonitoringChannelData& getMonitoringChannelData(const size_t channelIndex) override;

    /// <summary>�������� ����� ��� �����������.</summary>
    /// <returns>������ ������������ ������ � ������.</returns>
    size_t addMonitoingChannel() override;
    /// <summary>������� ����������� ����� �� ������ �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>������ ��������� ����� ��������.</returns>
    size_t removeMonitoringChannelByIndex(const size_t channelIndex) override;

    /// <summary>�������� ���� ���������� � ������ �� ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newNotifyState">����� ��������� ����������.</param>
    void changeMonitoingChannelNotify(const size_t channelIndex,
                                      const bool newNotifyState) override;
    /// <summary>�������� ��� ������������ ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newChannelName">����� ��� ������.</param>
    void changeMonitoingChannelName(const size_t channelIndex,
                                    const CString& newChannelName) override;
    /// <summary>�������� �������� ����������� ������ ��� ������������ ������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newInterval">����� �������� �����������.</param>
    void changeMonitoingChannelInterval(const size_t channelIndex,
                                        const MonitoringInterval newInterval) override;
    /// <summary>�������� �������� �� ���������� �������� ����� ����������� ����������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <param name="newValue">����� �������� � �����������.</param>
    void changeMonitoingChannelAllarmingValue(const size_t channelIndex,
                                              const float newValue) override;

    /// <summary>����������� ����� �� ������ ����� �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>����� ������ ������.</returns>
    size_t moveUpMonitoingChannelByIndex(const size_t channelIndex) override;
    /// <summary>����������� ���� �� ������ ����� �� �������.</summary>
    /// <param name="channelIndex">������ ������ � ������ �������.</param>
    /// <returns>����� ������ ������.</returns>
    size_t moveDownMonitoingChannelByIndex(const size_t channelIndex) override;

    // �������� ��������� ���� ���������
    const TelegramBotSettings& getBotSettings() override;
    // ���������� ��������� ���� ���������
    void setBotSettings(const TelegramBotSettings& newSettings) override;

// IEventRecipient
public:
    // ���������� � ������������ �������
    void onEvent(const EventId& code, float eventValue,
                 std::shared_ptr<IEventData> eventData) override;

// ITickHandler
public:
    // ���� �������
    enum TimerType
    {
        eUpdatingData = 0,      // ������ ���������� ������� ������
        eEveryDayReporting      // ������ � ���������� �������
    };

    /// <summary>���������� ��� ���� �������.</summary>
    /// <param name="tickParam">�������� ���������� �������.</param>
    /// <returns>true ���� ��� ��, false ���� ���� ���������� ���� ������.</returns>
    bool onTick(TickParam tickParam) override;

// ������ �� ������� ������� � �����������
private:
    // ���������� ������� �������� ���������
    void saveConfiguration();
    // �������� �������� ���������
    void loadConfiguration();
    // �������� ���� � ����� XML � ����������� ����������
    CString getConfigurationXMLFilePath();
    // ���������� ��� ��������� � ������ ����������� �������
    // bAsynchNotify - ���� ��� ���� ���������� �� ��������� � ������ ������� ����������
    void onMonitoringChannelsListChanged(bool bAsynchNotify = true);

// ������ � ��������� �����������
private:
    // ��������� � ����������� �������
    struct TaskInfo
    {
        // ��� ������������ �������
        enum class TaskType
        {
            eIntervalInfo = 0,      // ������ ������ �� ��������� ��������(� �����������)
            eUpdatingInfo,          // ������ ����� ������(���������� ������������ ����������)
                                    // ���������� �� ������� ��� � 5 �����
            eEveryDayReport         // ������ ������ ��� ����������� ������
        } taskType = TaskType::eIntervalInfo;

        // ��������� ������� �� ������� ������������ �������
        ChannelParametersList channelParameters;
    };

    // �������� ������� ����������� ��� ������
    // @param channelParams - ��������� ������ �� �������� ���� ��������� ������� �����������
    // @param taskType - ��� ������������ �������
    // @param monitoringInterval - �������� ������� � ���������� ������� �� ������ �����������
    // ���� -1 - ������������ channelParams->monitoringInterval
    void addMonitoringTaskForChannel(ChannelParameters::Ptr& channelParams,
                                     const TaskInfo::TaskType taskType,
                                     CTimeSpan monitoringInterval = -1);
    // �������� ������� ����������� ��� ������ �������
    void addMonitoringTaskForChannels(const ChannelParametersList& channelList,
                                      const TaskInfo::TaskType taskType,
                                      CTimeSpan monitoringInterval);
    // �������� ������� �����������, ������� ����� ������������� �����
    void addMonitoringTaskForChannels(const ChannelParametersList& channelList,
                                      const std::list<TaskParameters::Ptr>& taskParams,
                                      const TaskInfo::TaskType taskType);

    // ������� ������� ������� ����� ������ ����������� ��� ���������� ������
    void delMonitoringTaskForChannel(const ChannelParameters::Ptr& channelParams);


    // ��������� ����������� ����������� ��� ������
    // @param monitoringResult - ��������� �����������
    // @param channelParameters - ��������� ������
    // @param allertText - ����� � ������� ����� ��������
    // @return true - � ������ ������������� �������� ������ �� �������
    bool handleIntervalInfoResult(const MonitoringResult::ResultData& monitoringResult,
                                  ChannelParameters* channelParameters,
                                  CString& allertText);   // ������ ������ �� ��������
    bool handleUpdatingResult(const MonitoringResult::ResultData& monitoringResult,
                              ChannelParameters* channelParameters,
                              CString& allertText);       // ���������� ������
    bool handleEveryDayReportResult(const MonitoringResult::ResultData& monitoringResult,
                                    ChannelParameters* channelParameters,
                                    CString& allertText); // ���������� �����

private:
    // ���� � ������������� �������������� ������� � ����������� �������
    std::map<TaskId, TaskInfo, TaskComparer> m_monitoringTasksInfo;

private:
    // ��� ��� ������ � ����������
    CTelegramBot m_telegramBot;

private:
    // ������ ����������
    ApplicationConfiguration::Ptr m_appConfig = ApplicationConfiguration::create();
};

