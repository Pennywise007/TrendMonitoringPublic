// ������ ��� ������ � �������� �����������, �������� �������� ���������������� ����

#pragma once

#include <filesystem>

#include <ITrendMonitoring.h>
#include <Singleton.h>

////////////////////////////////////////////////////////////////////////////////
// ������ ������ � ������������ ������, ������������ ��� ���� ����� � ������� ��� ���������� ���������
// � �� �� ������� ��������� ��� ����� �� ������������ ����������,
// � ��� �� ��� ���������� ��������� ����������������� �����
class TestHelper
{
    friend class CSingleton<TestHelper>;

public:
    TestHelper() = default;

public:
    // ����� �������� ������� �����������
    void resetMonitoringService();

// ���� � ������
public:
    // ��������� ���� � �������� ��������������� ����� ������������
    std::filesystem::path getConfigFilePath() const;
    // ��������� ���� � ���������� ����� ��������� ����� ������������
    std::filesystem::path getCopyConfigFilePath() const;

    // �������� ���� � ��������� ������� � ������������ �������
    std::filesystem::path getRestartFilePath() const;
    // �������� ���� � ����� ������� � ������������ �������
    std::filesystem::path getCopyRestartFilePath() const;
};
