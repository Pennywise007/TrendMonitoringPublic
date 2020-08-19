#pragma once

#include <filesystem>

#include <gtest/gtest.h>

#include <ITrendMonitoring.h>

////////////////////////////////////////////////////////////////////////////////
// ����� ������ � �������� ������, ������������ ��� ������������ �����������
class MonitoringTestClass
    : public testing::Test
{
protected:
    void SetUp() override;

protected:  // �����
    // �������� ���������� ������� ��� �����������
    void testAddChannels();
    // �������� ��������� ���������� �������
    void testSetParamsToChannels();
    // �������� ���������� ������� �������
    void testChannelListManagement();
    // �������� �������� ������� �� ������
    void testDelChannels();

private:    // ���������� �������
    // �������� ��� ���������� ������ �� ������� ������� ��������� � �������� ����� ���������� �����
    void checkModelAndRealChannelsData(const std::string& testDescr);

protected:
    // ������ ������ ������� �������, �������� ������ ��������� � ��� ��� � �������
    std::list<MonitoringChannelData> m_channelsData;
    // ������ �����������
    ITrendMonitoring* m_monitoingService = get_monitoing_service();
};