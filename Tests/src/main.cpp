#include "pch.h"

#include <DirsService.h>

#include "TestHelper.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // �������� ������ � ������ �������� ������
    setlocale(LC_ALL, "Russian");

    EXPECT_TRUE(AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) << "�� ������� ���������������� ����������";

    // ������ ���������� � ��������� ��� �������� ����������
    auto& zetDirsService = get_service<DirsService>();
    zetDirsService.setZetSignalsDir(zetDirsService.getExeDir() + LR"(Signals\)");
    // ������ ������ ��� ������ ������� �������� �� ���������� �� ������� ��������� ���������
    zetDirsService.setZetCompressedDir(zetDirsService.getExeDir() + LR"(Signals\)");

    TestHelper& handler = get_service<TestHelper>();
    // ���� �� ����� ������ ����� ����� ����������� �������� �������, ���� ��� ���� ��������� �� � ����� �����
    const std::filesystem::path currentConfigPath(handler.getConfigFilePath());
    const std::filesystem::path copyConfigFilePath(handler.getCopyConfigFilePath());
    if (std::filesystem::is_regular_file(currentConfigPath))
        std::filesystem::rename(currentConfigPath, copyConfigFilePath);

    // ������ �� �� ����� � ������ �������� �������
    const std::filesystem::path restartFilePath(handler.getRestartFilePath());
    const std::filesystem::path copyRestartFilePath(handler.getCopyConfigFilePath());
    if (std::filesystem::is_regular_file(restartFilePath))
        std::filesystem::rename(restartFilePath, copyRestartFilePath);

    ////////////////////////////////////////////////////////////////////////////
    // ��������� �����
    int res = RUN_ALL_TESTS();
    ////////////////////////////////////////////////////////////////////////////

    // ��������������� ����������� ������ ��� ������� ��������� ������
    if (std::filesystem::is_regular_file(copyConfigFilePath))
        // ���� ��� �������� ���� ������� ������� ������ - ���������� ��� �� �����
        std::filesystem::rename(copyConfigFilePath, currentConfigPath);
    else
    {
        // ��������� ��������� ���� � �����������
        EXPECT_TRUE(std::filesystem::is_regular_file(currentConfigPath)) << "���� � ����������� ����������� �� ��� ������!";
        EXPECT_TRUE(std::filesystem::remove(currentConfigPath)) << "�� ������� ������� ����";
    }

    // ��������������� ���� ��������
    if (std::filesystem::is_regular_file(copyRestartFilePath))
        // ���� ��� �������� ���� ������� ������� ������ - ���������� ��� �� �����
        std::filesystem::rename(copyRestartFilePath, restartFilePath);
    else
    {
        // ��������� ��������� ���� ��������
        EXPECT_TRUE(std::filesystem::is_regular_file(restartFilePath)) << "���� � ����������� ����������� �� ��� ������!";
        EXPECT_TRUE(std::filesystem::remove(restartFilePath)) << "�� ������� ������� ����";
    }

    return res;
}