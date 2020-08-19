// ������������ ��������� ������ �����������

#include "pch.h"

#include <boost/scope_exit.hpp>

#include <ITrendMonitoring.h>

#include "TestMonitoringTasks.h"

#define DONT_TEST_MONITORING_TASKS

#define GTEST_COUT(Text) std::cerr << "[          ] [ INFO ] " << Text << std::endl;

// ��� ����������� ������ �� �������� ��� ������
const CString kRandomChannelName = L"����������";

// �� ����� ���� ������ ������ �� 07.08.2020, �� �������� ������� ��������
const CTime kExistDataStartTime(2020, 6, 7, 0, 0, 0);
const CTime kExistDataStopTime(2020, 10, 7, 0, 0, 0);

#ifdef DONT_TEST_MONITORING_TASKS

////////////////////////////////////////////////////////////////////////////////
// ������������ ������� � ���������� � �������� ������ ��� �����������
TEST_F(MonitoringTasksTestClass, AddTaskList)
{
    m_testType = TestType::eTestAddTaskList;

    // ��������� ������ ������ ��� �����������
    fillTaskParams();

    // �������� ������ ��� �������
    std::list<CString> channelNames;
    for (auto& task : m_taskParams)
    {
        channelNames.emplace_back(task->channelName);
        task->startTime = kExistDataStartTime;
        task->endTime   = kExistDataStopTime;
    }

    // ��������� ������� ������� ����������
    IMonitoringTasksService* pMonitoringService = get_monitoing_tasks_service();

    // ���������� ������� �����������
    std::unique_lock<std::mutex> lock(m_resultMutex);
    m_currentTask = pMonitoringService->addTaskList(channelNames, kExistDataStartTime, kExistDataStopTime,
                                                    IMonitoringTasksService::eHigh);

    GTEST_COUT("�������� ���������� ������� ����������� (1 ������).");

    // ��� ���� ����� ���������
    ASSERT_TRUE(m_resultTaskCV.wait_for(lock, std::chrono::minutes(1)) != std::cv_status::timeout) <<
        "�� ������� �������� ������ ����������� �� 1 ������";
}

////////////////////////////////////////////////////////////////////////////////
TEST_F(MonitoringTasksTestClass, AddTaskParams)
{
    m_testType = TestType::eTestAddTaskParams;

    // ��������� ������ ������ ��� �����������
    fillTaskParams();

    // ��������� ������� ������� ����������
    IMonitoringTasksService* pMonitoringService = get_monitoing_tasks_service();

    // ���������� ������� �����������
    std::unique_lock<std::mutex> lock(m_resultMutex);
    m_currentTask = pMonitoringService->addTaskList(m_taskParams, IMonitoringTasksService::eHigh);

    GTEST_COUT("�������� ���������� ������� ����������� (1 ������).");

    // ��� ���� ����� ���������
    ASSERT_TRUE(m_resultTaskCV.wait_for(lock, std::chrono::minutes(1)) != std::cv_status::timeout) <<
        "�� ������� �������� ������ ����������� �� 1 ������";
}

////////////////////////////////////////////////////////////////////////////////
TEST_F(MonitoringTasksTestClass, RemoveTask)
{
    m_testType = TestType::eTestRemoveTask;

    // ��������� ������ ������ ��� �����������
    fillTaskParams();

    // ��������� ������� ������� ����������
    IMonitoringTasksService* pMonitoringService = get_monitoing_tasks_service();

    // ���������� ������� �����������
    std::unique_lock<std::mutex> lock(m_resultMutex);
    m_currentTask = pMonitoringService->addTaskList(m_taskParams, IMonitoringTasksService::eHigh);
    pMonitoringService->removeTask(m_currentTask);

    GTEST_COUT("�������� ���������� ������� ����������� (1 ������).");

    // ��� ���� ����� ���������
    ASSERT_TRUE(m_resultTaskCV.wait_for(lock, std::chrono::minutes(1)) == std::cv_status::timeout) <<
        "���� ��������� ���������� ������� ������� ���� ��������";
}

#endif DONT_TEST_MONITORING_TASKS

////////////////////////////////////////////////////////////////////////////////
void MonitoringTasksTestClass::SetUp()
{
    // ������������� �� ������� � ���������� ������� �����������
    EventRecipientImpl::subscribeAssynch(onCompletingMonitoringTask);
}

//----------------------------------------------------------------------------//
void MonitoringTasksTestClass::onEvent(const EventId& code, float eventValue,
                                       std::shared_ptr<IEventData> eventData)
{
    if (code == onCompletingMonitoringTask)
    {
        MonitoringResult::Ptr monitoringResult = std::static_pointer_cast<MonitoringResult>(eventData);
        if (monitoringResult->m_taskId != m_currentTask)
            return;

        BOOST_SCOPE_EXIT(&m_resultTaskCV)
        {
            m_resultTaskCV.notify_one();
        } BOOST_SCOPE_EXIT_END;

        ASSERT_TRUE(m_testType != TestType::eTestRemoveTask) << "������ ��������� ����� � ��������� ������� �����������";

        MonitoringResult::ResultsList& taskResults = monitoringResult->m_taskResults;

        // ��������� ��� � ������� ��������� ��������� � ����������
        ASSERT_EQ(m_taskParams.size(), taskResults.size()) << "���������� ����������� �� ������������� ���������� ���������� ��� ������� �������";
        auto taskParamit = m_taskParams.begin(), taskParamitEnd = m_taskParams.end();
        auto taskResultit = taskResults.begin(), taskResultEnd = taskResults.end();
        for (; taskParamit != taskParamitEnd && taskResultit != taskResultEnd; ++taskParamit, ++taskResultit)
        {
            EXPECT_EQ((*taskParamit)->channelName, taskResultit->pTaskParameters->channelName);
            EXPECT_EQ((*taskParamit)->startTime, taskResultit->pTaskParameters->startTime);
            EXPECT_EQ((*taskParamit)->endTime, taskResultit->pTaskParameters->endTime);
        }

        // ��������� ���������� �����������
        for (auto& channelResult : taskResults)
        {
            if (channelResult.pTaskParameters->channelName == kRandomChannelName)
            {
                EXPECT_EQ(channelResult.resultType, MonitoringResult::Result::eErrorText) << "�������� ���������� ��� �� ������������� ������";
                continue;
            }

            EXPECT_EQ(channelResult.resultType, MonitoringResult::Result::eSucceeded) << "�������� ������ ��� ��������� ������";

            // ������� ��� ������������ ������ ��� ������
            CStringA curChannelName = channelResult.pTaskParameters->channelName;

            // ��-�� ���� ��� � ������ ����� ���� ������ ���� ������ ������� ������� ���� � �������
            CTimeSpan idialEmptyDataTime(0, 23, 56, 44);
            idialEmptyDataTime += channelResult.pTaskParameters->endTime - channelResult.pTaskParameters->startTime -
                (kExistDataStopTime - kExistDataStartTime);

            // � ���� ���������� ����� ������
            EXPECT_EQ(channelResult.emptyDataTime.GetHours(),   idialEmptyDataTime.GetHours()) << curChannelName;
            EXPECT_EQ(channelResult.emptyDataTime.GetMinutes(), idialEmptyDataTime.GetMinutes()) << curChannelName;
            EXPECT_EQ(channelResult.emptyDataTime.GetSeconds(), idialEmptyDataTime.GetSeconds()) << curChannelName;

            if (channelResult.pTaskParameters->channelName == L"����������� �1")
            {
                EXPECT_NEAR(channelResult.startValue, -79.19, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.currentValue, -130.95, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.maxValue, 241.9, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.minValue, -317.45, 0.01) << curChannelName;

                EXPECT_EQ(channelResult.lastDataExistTime.Format(L"%d.%m.%Y %H:%M:%S"), L"07.08.2020 15:56:35") << curChannelName;
            }
            else if (channelResult.pTaskParameters->channelName == L"����������� �2")
            {
                EXPECT_NEAR(channelResult.startValue, 10.76, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.currentValue, -8.63, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.maxValue, 39.8, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.minValue, -35.76, 0.01) << curChannelName;

                EXPECT_EQ(channelResult.lastDataExistTime.Format(L"%d.%m.%Y %H:%M:%S"), L"07.08.2020 15:56:35") << curChannelName;
            }
            else
            {
                EXPECT_EQ(channelResult.pTaskParameters->channelName, L"����������� �3") <<
                    "����������� ��� ������";

                EXPECT_NEAR(channelResult.startValue, -0.97, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.currentValue, -0.7, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.maxValue, 0.99, 0.01) << curChannelName;
                EXPECT_NEAR(channelResult.minValue, -1, 0.01) << curChannelName;

                EXPECT_EQ(channelResult.lastDataExistTime.Format(L"%d.%m.%Y %H:%M:%S"), L"07.08.2020 15:56:35") << curChannelName;
            }
        }
    }
}

//----------------------------------------------------------------------------//
void MonitoringTasksTestClass::fillTaskParams()
{
    ASSERT_TRUE(m_taskParams.empty()) << "������ ���������� �� ����";

    for (auto& channel : get_monitoing_service()->getNamesOfAllChannels())
    {
        m_taskParams.emplace_back(new TaskParameters(channel,
                                                     kExistDataStartTime - CTimeSpan(0, 1 * std::rand() % 10, 0, 0),
                                                     kExistDataStopTime + CTimeSpan(0, 1 * std::rand() % 10, 0, 0)));
    }

    m_taskParams.emplace_back(new TaskParameters(kRandomChannelName,
                                                 kExistDataStartTime,
                                                 kExistDataStopTime));
}
