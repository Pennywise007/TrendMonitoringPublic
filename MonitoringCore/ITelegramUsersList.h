#pragma once

// ������ ���������������� �������� ��� ������ � ����������

#include "COM.h"
#include "Messages.h"

#include <tgbot/tgbot.h>

// ���������� �� ��������� � ������ ������������� ���������
// {491F3E83-3A8B-48BF-9235-33E5CE2E9FEC}
constexpr EventId onUsersListChangedEvent =
{ 0x491f3e83, 0x3a8b, 0x48bf, { 0x92, 0x35, 0x33, 0xe5, 0xce, 0x2e, 0x9f, 0xec } };

////////////////////////////////////////////////////////////////////////////////
// ��������� ��� ���������� ������� ������������� ���������
DECLARE_COM_INTERFACE(ITelegramUsersList, "EFB00D0B-3B37-4AEC-B63A-9ECEA34C0802")
{
    // ������ ������������
    enum UserStatus
    {
        eNotAuthorized,     // �� ��������������
        eOrdinaryUser,      // ������� ������������ �������
        eAdmin,             // ������������� �������
        // ��������� ������
        eLastStatus
    };

    // ��������� ��� ������������ ����������
    virtual void ensureExist(const TgBot::User::Ptr& pUser, const int64_t chatId) = 0;

    // ��������� ������� ������������
    virtual UserStatus getUserStatus(const TgBot::User::Ptr& pUser) = 0;
    // ��������� ������� ������������
    virtual void setUserStatus(const TgBot::User::Ptr& pUser,
                               const UserStatus newStatus) = 0;
    // �������� ��� �������������� ����� ������������� � ������������ ��������
    virtual std::list<int64_t> getAllChatIdsByStatus(const UserStatus usertatus) = 0;
};