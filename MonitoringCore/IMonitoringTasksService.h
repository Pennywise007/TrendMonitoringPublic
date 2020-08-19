#pragma once

#include <afx.h>
#include <list>

#include "Messages.h"

#include "ITrendMonitoring.h"

// ������������� ������� ��������� �������� ������ ��� ������ � ������������
// {C6C55763-BC3F-41B0-95D0-D1BC52A149AE}
static const EventId onCompletingMonitoringTask =
{ 0xc6c55763, 0xbc3f, 0x41b0, { 0x95, 0xd0, 0xd1, 0xbc, 0x52, 0xa1, 0x49, 0xae } };

////////////////////////////////////////////////////////////////////////////////
// ������������� �������
typedef GUID TaskId;
// ��������� ��� ��������� ��������������� �������
struct TaskComparer
{
    bool operator()(const TaskId& Left, const TaskId& Right) const
    {
        // comparison logic goes here
        return memcmp(&Left, &Right, sizeof(Right)) < 0;
    }
};

// ��������� ������������ �������
struct TaskParameters
{
    typedef std::shared_ptr<TaskParameters> Ptr;

    TaskParameters(const CString& chanName, const CTime& start, const CTime& end)
        : channelName(chanName), startTime(start), endTime(end)
    {}

    CString channelName;        // ��� ������ �� �������� ����� ��������� ����������
    CTime startTime;            // ������ ��������� ����������� ������ ������
    CTime endTime;              // �����  ��������� ����������� ������ �������
};

////////////////////////////////////////////////////////////////////////////////
// ��������� ������� ��� ���������� ������� ��������� ������ ������������ ���
// ���������� ��������� ����������� ������� ����������� � ��������� ������
// ���������� ���������� ������� ���������� ����� onCompletingMonitoringTask
interface IMonitoringTasksService
{
    virtual ~IMonitoringTasksService() = default;

    // �������������� �������, ������ ���� ������������� � ������� ��������
    enum TaskPriority : unsigned
    {
        eHigh,     // �������
        eNormal    // �������
    };

    // ��������� ������ ������� ��� �����������, ��������� �� ��� ������ ���� ������� �������������
    // @param channelNames - ������ ���� ������� ��� ������� ����� ���������� ����������
    // @param intervalStart - ������ ��������� �� �������� ���� ���������� ����������
    // @param intervalEnd - ����� ��������� �� �������� ���� ���������� ����������
    // @param priority - ��������� �������, ���������� � ����� ����������� ����� ����������� �������
    // @return ������������� �������
    virtual TaskId addTaskList(const std::list<CString>& channelNames,
                               const CTime& intervalStart,
                               const CTime& intervalEnd,
                               const TaskPriority priority) = 0;

    // ��������� ������ ������� ��� �����������, ��������� �� ��� ������ ���� ������� �������������
    // @param listTaskParams - ������ ���������� ��� �������
    // @param priority - ��������� �������, ���������� � ����� ����������� ����� ����������� �������
    // @return ������������� �������
    virtual TaskId addTaskList(const std::list<TaskParameters::Ptr>& listTaskParams,
                               const TaskPriority priority) = 0;

    // �������� ���������� ������� � ������� ��� �� �������
    virtual void removeTask(const TaskId& taskId) = 0;
};

// ��������� ������� ������� ����������
IMonitoringTasksService* get_monitoing_tasks_service();

////////////////////////////////////////////////////////////////////////////////
// ��������� ������� �����������
class MonitoringResult
    : public IEventData
{
public:
    typedef std::shared_ptr<MonitoringResult> Ptr;

    MonitoringResult(const TaskId& taskId)
        : m_taskId(taskId) {}

public:
    // ������������� �������, �� �������� ������� ���������
    TaskId m_taskId;

    // ��������� ������� ������
    enum class Result
    {
        eSucceeded = 0, // �������� ��������
        eErrorText,     // �������� ������, � m_errorText ���� � �����
        eNoData         // ��� ������ � ����������� ���������
    };

    // ��������� � ����������� �����������
    struct ResultData : public TrendChannelData
    {
        ResultData(const TaskParameters::Ptr params) : pTaskParameters(params) {}

        // ��������� ���������� �������
        Result resultType = Result::eSucceeded;
        // ����� ��������� ������
        CString errorText;
        // ��������� ������� �� ������� ������� ���������
        TaskParameters::Ptr pTaskParameters;
    };

    typedef std::list<ResultData> ResultsList;
    typedef std::list<ResultData>::const_iterator ResultIt;

    // ������ ����������� ����������� ��� ������� ������ �� �������
    ResultsList m_taskResults;
};