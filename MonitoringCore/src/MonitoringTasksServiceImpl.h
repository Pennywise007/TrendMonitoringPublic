#pragma once

/*
    ���������� ������ � ���������� ������ �� �������
*/

#include <afx.h>
#include <future>
#include <thread>
#include <list>
#include <map>

#include <Messages.h>
#include "Singleton.h"

#include <ChannelDataCorrector/ChannelDataGetter.h>

#include "IMonitoringTasksService.h"

////////////////////////////////////////////////////////////////////////////////
// ������ ��� ���������� ������� ��������� ������ ������������ ���
// ���������� ��������� ����������� ������� ����������� � ��������� ������
// ���������� ���������� ������� ���������� ����� onCompletingMonitoringTask
class MonitoringTasksServiceImpl : public IMonitoringTasksService
{
    friend class CSingleton<MonitoringTasksServiceImpl>;

public:
    MonitoringTasksServiceImpl();
    virtual ~MonitoringTasksServiceImpl();

// IMonitoringTasksService
public:
    // ��������� ������ ������� ��� �����������, ��������� �� ��� ������ ���� ������� �������������
    // @param channelNames - ������ ���� ������� ��� ������� ����� ���������� ����������
    // @param intervalStart - ������ ��������� �� �������� ���� ���������� ����������
    // @param intervalEnd - ����� ��������� �� �������� ���� ���������� ����������
    // @param priority - ��������� �������, ���������� � ����� ����������� ����� ����������� �������
    // @return ������������� �������
    TaskId addTaskList(const std::list<CString>& channelNames,
                       const CTime& intervalStart,
                       const CTime& intervalEnd,
                       const TaskPriority priority) override;

    // ��������� ������ ������� ��� �����������, ��������� �� ��� ������ ���� ������� �������������
    // @param listTaskParams - ������ ���������� ��� �������
    // @param priority - ��������� �������, ���������� � ����� ����������� ����� ����������� �������
    // @return ������������� �������
    TaskId addTaskList(const std::list<TaskParameters::Ptr>& listTaskParams,
                       const TaskPriority priority) override;

    // �������� ���������� ������� � ������� �� �������
    void removeTask(const TaskId& taskId);

private:
    // ����� � ����������� ������� ��� �����������
    class MonitoringTask;
    typedef std::shared_ptr<MonitoringTask> MonitoringTaskPtr;
    // ��������������� ����� ��� �������� ����������� �����������
    class MonitoringResultHelper;
private:
    // �����, ����������� ������� ��� �����������
    void tasksExecutorThread();
    // ��������� �������
    void executeTask(MonitoringTaskPtr pMonitoringTask);

private:
    // ���� ���������� ������ ������
    std::atomic_bool m_interruptThread = false;

    // ����� ����������� ������
    std::thread m_monitoringTasksExecutorThread;

    // ������� �������
    std::list<MonitoringTaskPtr> m_queueTasks;

    // ������� �� ������� �������, ������������ � ���� �������
    // 1. ������������� �������� ��� m_queueTasks
    // 2. ��� m_cvTasks
    std::mutex m_queueMutex;
    // ������������ ��� ������������� ���������� ������� ���-����
    std::condition_variable m_cvTasks;

    // ������ ��������������� ������� � ����������� �����������
    std::map<TaskId, MonitoringResultHelper, TaskComparer> m_resultsList;
    // ������� �� ������ �����������
    std::mutex m_resultsMutex;
};

////////////////////////////////////////////////////////////////////////////////
// ������� ��� �����������
class MonitoringTasksServiceImpl::MonitoringTask
{
public:
    MonitoringTask(const TaskParameters::Ptr& taskParameters,
                   const TaskPriority priority,
                   const TaskId& taskId)
        : m_pTaskParams(taskParameters)
        , m_priority(priority)
        , m_taskId(taskId)
    {}

public:
    // ��������� �������
    TaskParameters::Ptr m_pTaskParams;
    // ��������� �������
    TaskPriority m_priority;
    // ������������� �������
    TaskId m_taskId;
};

////////////////////////////////////////////////////////////////////////////////
// ��������������� ����� ��� �������� ����������� �����������
class MonitoringTasksServiceImpl::MonitoringResultHelper
{
public:
    MonitoringResultHelper(const size_t taskCount, const TaskId& taskId)
        : m_taskCount(taskCount)
        , m_pMonitoringResult(new MonitoringResult(taskId))
    {}

public:
    // ���������� ������� ������� ���� ��������
    size_t m_taskCount;
    // ��������� �����������
    MonitoringResult::Ptr m_pMonitoringResult;
};