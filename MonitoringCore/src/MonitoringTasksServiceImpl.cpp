#include "pch.h"

#include <assert.h>
#include <memory>

#include "Logger.h"
#include "MonitoringTasksServiceImpl.h"

// ��������� ���������� �����������
#define DETAILED_LOGGING

////////////////////////////////////////////////////////////////////////////////
IMonitoringTasksService* get_monitoing_tasks_service()
{
    return &get_service<MonitoringTasksServiceImpl>();
}

//----------------------------------------------------------------------------//
MonitoringTasksServiceImpl::MonitoringTasksServiceImpl()
{
    // ������ �� ����� ����������� ����� ��� ���������� ������������������
    m_monitoringTasksExecutorThread = std::thread(std::bind(&MonitoringTasksServiceImpl::tasksExecutorThread, this));
}

//----------------------------------------------------------------------------//
MonitoringTasksServiceImpl::~MonitoringTasksServiceImpl()
{
    // ������� ���� ���������� ������
    m_interruptThread = true;
    // ��������� � ����� �������
    m_cvTasks.notify_all();
    // ���� ���� ����� ���������
    m_monitoringTasksExecutorThread.join();
}

//----------------------------------------------------------------------------//
// IMonitoringTasksService
TaskId MonitoringTasksServiceImpl::addTaskList(const std::list<CString>& channelNames,
                                               const CTime& intervalStart,
                                               const CTime& intervalEnd,
                                               const TaskPriority priority)
{
    // ������ ���������� �������
    std::list<TaskParameters::Ptr> listTaskParams;

    // ���������� ����� ��������� �� �������� ���� ��������� ������
    for (auto& channelName : channelNames)
    {
        listTaskParams.emplace_back(new TaskParameters(channelName, intervalStart, intervalEnd));
    }

    return addTaskList(listTaskParams, priority);
}

//----------------------------------------------------------------------------//
// IMonitoringTasksService
TaskId MonitoringTasksServiceImpl::addTaskList(const std::list<TaskParameters::Ptr>& listTaskParams,
                                               const TaskPriority priority)
{
#ifdef DETAILED_LOGGING
    OUTPUT_LOG_FUNC_ENTER;
#endif // DETAILED_LOGGING

    // ������� ������������� �������
    TaskId newTaskId;
    if (!SUCCEEDED(CoCreateGuid(&newTaskId)))
        assert(!"�� ������� ������� ����!");

    // �������������� � ������ �����������
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex);
        m_resultsList.try_emplace(newTaskId, listTaskParams.size(), newTaskId);
    }

    {
        // ��������� � ������� �������
        std::lock_guard<std::mutex> lock(m_queueMutex);

        // �������� ���� ����� ���������
        decltype(m_queueTasks)::iterator insertIter =
            std::find_if(m_queueTasks.begin(), m_queueTasks.end(),
                         [priority](const MonitoringTaskPtr& task)
                         {
                             return task->m_priority > priority;
                         });

        for (const auto& taskParams : listTaskParams)
        {
            insertIter = ++m_queueTasks.emplace(insertIter, new MonitoringTask(taskParams,
                                                                               priority,
                                                                               newTaskId));
        }

#ifdef DETAILED_LOGGING
        // �������� ���������� ��� �����������
        CString channelsInfo;
        for (const auto& taskParams : listTaskParams)
        {
            channelsInfo.AppendFormat(L" ����� %s, �������� %s - %s;",
                                      taskParams->channelName.GetString(),
                                      taskParams->startTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString(),
                                      taskParams->endTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString());
        }

        OUTPUT_LOG_SET_TEXT(L"�������� ������%s. TaskId = %s",
                            channelsInfo.GetString(),
                            CString(CComBSTR(newTaskId)).GetString());
#endif // DETAILED_LOGGING

    }

    m_cvTasks.notify_one();

    return newTaskId;
}

//----------------------------------------------------------------------------//
// IMonitoringTasksService
void MonitoringTasksServiceImpl::removeTask(const TaskId& taskId)
{
#ifdef DETAILED_LOGGING

    OUTPUT_LOG_FUNC_ENTER;
    OUTPUT_LOG_SET_TEXT(L"TaskId = %s", CString(CComBSTR(taskId)).GetString());

#endif // DETAILED_LOGGING

    // ������� �� ������ �����������
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex);
        m_resultsList.erase(taskId);
    }

    // ������� ��� ������� ����������� � ����� ���������������
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queueTasks.erase(std::remove_if(m_queueTasks.begin(), m_queueTasks.end(),
                                          [&taskId](const MonitoringTaskPtr& task)
                                          {
                                              return task->m_taskId == taskId;
                                          }),
                           m_queueTasks.end());
    }
}

//----------------------------------------------------------------------------//
void MonitoringTasksServiceImpl::tasksExecutorThread()
{
    for (; !m_interruptThread; )
    {
        MonitoringTaskPtr pMonitoringTask = nullptr;
        {
            // ���� ���� ����� ��������� ��������� � �������
            std::unique_lock<std::mutex> lk(m_queueMutex);

            // ���� ���� �� �������� ���� ���� �������
            while (m_queueTasks.empty())
            {
                m_cvTasks.wait(lk);

                // ���� ��� ��������� ��� ���������� - �����������
                if (m_interruptThread)
                    return;
            }

            // �������� ���� ������� ��� ����������� �� �������
            pMonitoringTask = std::move(m_queueTasks.front());
            // ������� ��� �� ������� �������
            m_queueTasks.pop_front();
        }

        // ��������� �������
        executeTask(pMonitoringTask);
    }
}

//----------------------------------------------------------------------------//
void MonitoringTasksServiceImpl::executeTask(MonitoringTaskPtr pMonitoringTask)
{
#ifdef DETAILED_LOGGING
    OUTPUT_LOG_FUNC_ENTER;
#endif // DETAILED_LOGGING

    assert(pMonitoringTask);
    if (!pMonitoringTask)
        return;

    MonitoringResult::ResultData taskResult(pMonitoringTask->m_pTaskParams);

    // ��������� �������
    const CString& channelName = pMonitoringTask->m_pTaskParams->channelName;
    const CTime& startTime  = pMonitoringTask->m_pTaskParams->startTime;
    const CTime& endTime    = pMonitoringTask->m_pTaskParams->endTime;

#ifdef DETAILED_LOGGING
    OUTPUT_LOG_CALCTIME_ENABLE(true);
    OUTPUT_LOG_TEXT(L"�������� ������ �� ������ %s, �������� %s - %s.",
                    channelName.GetString(),
                    startTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString(),
                    endTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString());
#endif // DETAILED_LOGGING

    // ��������� �� ������
    CString exceptionMessage;
    try
    {
        // ������� ����� ��� ��������� ������ �� ������
        CChannelDataGetter channelDataGetter(channelName, startTime, endTime);
        // ������� ������, ������� ��� ��� ����������
        double frequency = channelDataGetter.getChannelInfo()->GetFrequency();

        // ����� ������� ��������, �������� �� 10 000 000 �����
        // ��� ������ � �������� 10 ��� 11,5 ����, ��� ~40 �� ������
        const double maxPartSize = 1e7;

        bool bDataLoaded = false;

        // ������ �� ������
        std::vector<float> channelData;

        // ��������� ����������� ����� �� ������
        CTime lastLoadedTime = startTime;
        while (lastLoadedTime != endTime && !m_interruptThread)
        {
            // ��������� ���������� ������� �������� ���������
            CTimeSpan allTimeSpan = endTime - lastLoadedTime;

            // ������������ ���������� ������ ������� ����� ���������
            LONGLONG loadingSeconds = std::min<LONGLONG>(allTimeSpan.GetTotalSeconds(),
                                                            LONGLONG(maxPartSize / frequency));

            try
            {
                // ��������� ������ �������� ������
                channelDataGetter.getSourceChannelData(channelName, lastLoadedTime,
                                                        lastLoadedTime + loadingSeconds,
                                                        channelData);

                // ���������� ������ ��������
                LONGLONG emptyValuesCount = 0;

                // ������ � ������ � ������ ��������� �� ������� ������
                LONGLONG index = 0, lastNotEmptyValueIndex = -1;

                // ����������� ������
                // ������ ����� : ������ ��� ������ ����� � ������ � ��� ����� [] ����� �������� ����� �������
                for (const auto& value : channelData)
                {
                    // ����������� ���� ����� ��������
                    if (m_interruptThread)
                        return;

                    // ���������� ���� ������� ��������� ������
                    if (abs(value) > FLT_MIN)
                    {
                        if (!bDataLoaded)
                        {
                            // �������������� ����������
                            taskResult.startValue = value;
                            taskResult.minValue = value;
                            taskResult.maxValue = value;

                            bDataLoaded = true;
                        }
                        else
                        {
                            if (taskResult.minValue > value)
                                taskResult.minValue = value;
                            if (taskResult.maxValue < value)
                                taskResult.maxValue = value;
                        }

                        // ���������� ������ ��������� �� ������� ������
                        lastNotEmptyValueIndex = index;
                    }
                    else
                        emptyValuesCount += 1;

                    ++index; // �������� ������� ������� ������ � ������
                }

#ifdef DETAILED_LOGGING
                OUTPUT_LOG_TEXT(L"������ ������ %s(������� %.02lf) ��������� � ���������� %s - %s. ������� ���������� ��������� %lld - �����  %lld. ����� ������ %u, ������ %u.",
                                channelName.GetString(), frequency,
                                lastLoadedTime.Format(L"%d.%m.%Y").GetString(),
                                (lastLoadedTime + loadingSeconds).Format(L"%d.%m.%Y").GetString(),
                                taskResult.emptyDataTime,
                                emptyValuesCount,
                                channelData.size(),
                                std::count_if(channelData.begin(), channelData.end(),
                                              [](const auto& val)
                                              {
                                                  return abs(val) <= FLT_MIN;
                                              }));
#endif // DETAILED_LOGGING

                // ���� ���� �� ������ ������
                if (lastNotEmptyValueIndex != -1)
                {
                    // ���������� ��������� �� ������ ��������
                    taskResult.currentValue = channelData[(UINT)lastNotEmptyValueIndex];

                    // ������� ����� ���������� �� ������� ��������
                    taskResult.lastDataExistTime =
                        lastLoadedTime + CTimeSpan(__time64_t(lastNotEmptyValueIndex / frequency));
                }

                // ������������ ���������� ������ ��� ������
                taskResult.emptyDataTime += LONGLONG(emptyValuesCount / frequency);
            }
            catch (const CString& logMessage)
            {
                // � �����-�� ���������� ������ ����� ������ �� ����, �������� �� �������
                taskResult.emptyDataTime += loadingSeconds;

#ifdef DETAILED_LOGGING
                OUTPUT_LOG_TEXT(L"�������� ������ ��� ��������� ������. ������� ���������� ��������� %lld. ������: %s",
                                taskResult.emptyDataTime,
                                logMessage.GetString());
#endif // DETAILED_LOGGING
            }

            // ��������� ��������� ����������� �����
            lastLoadedTime += loadingSeconds;
        }

        // ���� �� ���� ��������� ������ - �������� �� ����
        if (!bDataLoaded)
            taskResult.resultType = MonitoringResult::Result::eNoData;
    }
    catch (CException* e)
    {
        TCHAR logMessage[MAX_PATH];
        e->GetErrorMessage(logMessage, MAX_PATH);
        exceptionMessage = logMessage;
    }
    catch (const CString& logMessage)
    {
        exceptionMessage = logMessage;
    }

    // ���� ���� ��������� �� ������ - ���������� ���
    if (!exceptionMessage.IsEmpty())
    {
        taskResult.errorText.
            Format(L"�������� ������ ��� ��������� ������ ������ \"%s\" � ���������� %s - %s.\n%s",
                   channelName.GetString(),
                   startTime.Format(L"%d.%m.%Y").GetString(),
                   endTime.Format(L"%d.%m.%Y").GetString(),
                   exceptionMessage.GetString());

        // �������� ��� ���� ����� ������
        taskResult.resultType = MonitoringResult::Result::eErrorText;

        // ���� �������� ��������� ������ ������ ������ - ������� ��� ���� �������� ������
        taskResult.emptyDataTime = endTime - startTime;
    }

#ifdef DETAILED_LOGGING
    CString loadingResult;
    switch (taskResult.resultType)
    {
    case MonitoringResult::Result::eSucceeded:
        loadingResult.Format(L"������ ��������, curVal = %.02f minVal = %.02f maxVal = %.02f, emptyDataSeconds = %lld",
                             taskResult.currentValue, taskResult.minValue, taskResult.maxValue,
                             taskResult.emptyDataTime.GetTotalSeconds());
        break;
    case MonitoringResult::Result::eErrorText:
        loadingResult = taskResult.errorText;
        break;
    case MonitoringResult::Result::eNoData:
        loadingResult = L"������ ���";
        break;
    default:
        loadingResult = L"�� ��������� ��� ���������!";
        assert(false);
        break;
    }

    OUTPUT_LOG_ADD_COMMENT(L"�������� ������ �� ������ %s ���������. ��������� : %s",
                           channelName.GetString(), loadingResult.GetString());
#endif // DETAILED_LOGGING

    // ������� ��������� ������� � ������ �����������
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex);

        auto resultIt = m_resultsList.find(pMonitoringTask->m_taskId);
        // ��������� ��� ������� �� ������� ���� �� ��� ���������
        if (resultIt != m_resultsList.end())
        {
            MonitoringResult::ResultsList& resultsList = resultIt->second.m_pMonitoringResult->m_taskResults;
            // ��������� ���������
            resultsList.emplace_back(std::move(taskResult));
            // ���� ��������� ��� ������� ������� ������ ��������� �� ����
            if (resultsList.size() == resultIt->second.m_taskCount)
            {
                // ��������� � ���������� �������
                get_service<CMassages>().postMessage(onCompletingMonitoringTask, 0.f,
                                                     std::static_pointer_cast<IEventData>(resultIt->second.m_pMonitoringResult));

                // ������� ������� �� ���������� ����� �������
                m_resultsList.erase(resultIt);
            }
        }
    }
}
