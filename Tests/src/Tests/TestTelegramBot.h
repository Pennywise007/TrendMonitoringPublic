#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>

#include <gtest/gtest.h>

#include <COM.h>
#include <TelegramDLL/TelegramThread.h>

#include "ITelegramUsersList.h"
#include "src/TelegramBot.h"

////////////////////////////////////////////////////////////////////////////////
// ����� �������� ������ ������������� ���������
// ��������� �������� �� ������� ������������� �� ���������� �������
DECLARE_COM_INTERNAL_CLASS(TestTelegramUsersList)
    , public ITelegramUsersList
{
public:
    TestTelegramUsersList() = default;

    BEGIN_COM_MAP(TestTelegramUsersList)
        COM_INTERFACE_ENTRY(ITelegramUsersList)
    END_COM_MAP()

public:
    // �������� ������������� ���� � �������������� � ������ ������
    void addUserChatidStatus(const int64_t chatId, const UserStatus userStatus)
    {
        m_chatIdsToUserStatusMap[userStatus].push_back(chatId);
    }

// ITelegramUsersList
public:
    // ��������� ��� ������������ ����������
    void ensureExist(const TgBot::User::Ptr& /*pUser*/, const int64_t /*chatId*/) override
    {
    }

    // ��������� ������� ������������
    UserStatus getUserStatus(const TgBot::User::Ptr& pUser) override
    {
        return m_curStatus;
    }
    // ��������� ������� ������������
    void setUserStatus(const TgBot::User::Ptr& pUser, const UserStatus newStatus) override
    {
        m_curStatus = newStatus;
    }

    // �������� ��� �������������� ����� ������������� � ������������ ��������
    std::list<int64_t> getAllChatIdsByStatus(const UserStatus userStatus) override
    {
        return m_chatIdsToUserStatusMap[userStatus];
    }

private:
    // ������� ������ (����� ��� ����)
    UserStatus m_curStatus = UserStatus::eNotAuthorized;
    // ���� ������������ ��������������� ����� � �������� �������������
    std::map<UserStatus, std::list<int64_t>> m_chatIdsToUserStatusMap;
};

////////////////////////////////////////////////////////////////////////////////
// �������� �������� ���
class TestTelegramBot
    : public testing::Test
{
protected:
    // Sets up the test fixture.
    void SetUp() override;

// �������� ������ �� ���������
protected:
    // �������� �������� ���������
    void emulateBroadcastMessage(const CString& text) const;
    // �������� �������� �� ��������� (������� �� ������)
    void emulateBroadcastCallbackQuery(const CString& text) const;

private:
    // ������� ��������� ���������
    TgBot::Message::Ptr generateMessage(const CString& text) const;

protected:
    // �������� �������� ���
    CTelegramBot m_testTelegramBot;

    // ������ �������������
    TestTelegramUsersList::Ptr m_pUserList;

    // �������� ������ � �������� ������� ���������
    class TestTelegramThread* m_pTelegramThread;

    // ������ ������ � ����������� ��� ��������� �������������
    std::map<CString, std::set<ITelegramUsersList::UserStatus>> m_commandsToUserStatus;
};

////////////////////////////////////////////////////////////////////////////////
class TestTelegramThread
    : public ITelegramThread
{
public:
    // �������� ������ ��� "�������������" � �������� ����
    class TestClient
        : public TgBot::HttpClient
    {

    public:
        TestClient() = default;

        // Inherited via HttpClient
        virtual std::string makeRequest(const TgBot::Url& url, const std::vector<TgBot::HttpReqArg>& args) const override
        { return std::string(); }
    };

    TestTelegramThread()
        : m_botApi(CreateTelegramBot("", m_testClient))
    {}

public:
    typedef std::function<void(int64_t, const CString&, bool,
                               int32_t, TgBot::GenericReply::Ptr,
                               const std::string&, bool)> onSendMessageCallback;
    typedef std::function<void(const std::list<int64_t>&, const CString&, bool,
                               int32_t, TgBot::GenericReply::Ptr,
                               const std::string&, bool)> onSendMessageToChatsCallback;

    // ������ �� �������� ���������
    void onSendMessage(const onSendMessageCallback& callback)
    {
        m_sendMessageCallback = callback;
    }
    // ������ �� �������� ���������
    void onSendMessageToChats(const onSendMessageToChatsCallback& callback)
    {
        m_sendMessageToChatsCallback = callback;
    }

// ITelegramThread
public:
    // ������ ������
    void startTelegramThread(const std::map<std::string, CommandFunction>& commandsList,
                             const CommandFunction& onUnknownCommand = nullptr,
                             const CommandFunction& onNonCommandMessage = nullptr) override
    {
        TgBot::EventBroadcaster& eventBroadCaster = getBotEvents();

        for (auto& command : commandsList)
        {
            eventBroadCaster.onCommand(command.first, command.second);
        }

        eventBroadCaster.onUnknownCommand(onUnknownCommand);
        eventBroadCaster.onNonCommandMessage(onNonCommandMessage);
    }

    // ��������� ������
    void stopTelegramThread() override
    {}

    // ������� �������� ��������� � ����
    void sendMessage(const std::list<int64_t>& chatIds, const CString& msg,
                     bool disableWebPagePreview = false, int32_t replyToMessageId = 0,
                     TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                     const std::string& parseMode = "", bool disableNotification = false) override
    {
        ASSERT_TRUE(m_sendMessageToChatsCallback) << "����������� ��������� ��� ������� " + CStringA(msg);

        m_sendMessageToChatsCallback(chatIds, msg, disableWebPagePreview, replyToMessageId,
                                     replyMarkup, parseMode, disableNotification);
    }

    // ������� �������� ��������� � ���
    void sendMessage(int64_t chatId, const CString& msg, bool disableWebPagePreview = false,
                     int32_t replyToMessageId = 0,
                     TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                     const std::string& parseMode = "", bool disableNotification = false) override
    {
        ASSERT_TRUE(m_sendMessageCallback) << "����������� ��������� ��� ������� " + CStringA(msg);

        m_sendMessageCallback(chatId, msg, disableWebPagePreview, replyToMessageId,
                              replyMarkup, parseMode, disableNotification);
    }

    // ���������� ������� ���� ����� ������ ��� ������������
    TgBot::EventBroadcaster& getBotEvents() override
    {
        return m_botApi->getEvents();
    }

    // ��������� ��� ����
    const TgBot::Api& getBotApi() override
    {
        return m_botApi->getApi();
    }

public:
    // ��� ����
    std::unique_ptr<TgBot::Bot> m_botApi;
    // �������� ������ ��� ������
    TestClient m_testClient;
    // ������ �� �������� ���������
    onSendMessageCallback m_sendMessageCallback;
    // ������ �� �������� ��������� � ��������� �����
    onSendMessageToChatsCallback m_sendMessageToChatsCallback;
};

////////////////////////////////////////////////////////////////////////////////
// ����� ��� �������� �������� ������ �������� ����, ��������� ����� ���������
// ������������ ������������ � ���������� �� � ��������
class TelegramUserMessagesChecker
{
public:
    // pTelegramThread - ��������� ����� ��������� ������� ��������� �������� ���������
    // pExpectedUserMessage - ��������� ������� ������ ���� ���������� ������������ ����� onSendMessage
    // pExpectedReply - ����� ������� ������ �������� ������������(��� ������� ��� ���������� � �������� ������)
    // pExpectedReciepientsChats - ������ ����� ������� ��������������� ���������
    // pExpectedMessageToReciepients - ��������� ������� ������ ���� ���������� �� ��������� ������ �����
    TelegramUserMessagesChecker(TestTelegramThread* pTelegramThread,
                                CString* pExpectedUserMessage,
                                TgBot::GenericReply::Ptr* pExpectedReply = nullptr,
                                std::list<int64_t>** pExpectedReciepientsChats = nullptr,
                                CString* pExpectedMessageToReciepients = nullptr);
};