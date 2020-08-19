#pragma once

#include <afx.h>
#include <bitset>
#include <map>
#include <memory>
#include <time.h>

#include <TelegramDLL/TelegramThread.h>

#include "IMonitoringTasksService.h"
#include "ITelegramUsersList.h"
#include "ITrendMonitoring.h"

////////////////////////////////////////////////////////////////////////////////
// ����� ���������� �������� �����
class CTelegramBot
    : public ITelegramAllerter
    , public EventRecipientImpl
{
public:
    CTelegramBot();
    virtual ~CTelegramBot();

public:
    // ������������� ����
    void initBot(ITelegramUsersListPtr telegramUsers);
    // ��������� ������������ ������ ��������� �� ���������
    void setDefaultTelegramThread(ITelegramThreadPtr& pTelegramThread);
    // ��������� �������� ����
    void setBotSettings(const TelegramBotSettings& botSettings);

    // ������� �������� ��������� ��������������� �������
    void sendMessageToAdmins(const CString& message);
    // ������� �������� ��������� ������������� �������
    void sendMessageToUsers(const CString& message);

// ITelegramAllerter
public:
    void onAllertFromTelegram(const CString& allertMessage) override;

// IEventRecipient
public:
    // ���������� � ������������ �������
    void onEvent(const EventId& code, float eventValue,
                 std::shared_ptr<IEventData> eventData) override;

// ��������� ������ ����
private:
    // �������� ������� �� �������
    void fillCommandHandlers(std::map<std::string, CommandFunction>& commandsList,
                             CommandFunction& onUnknownCommand,
                             CommandFunction& onNonCommandMessage);
    // ��������� �� ������������������� ���������/�������
    void onNonCommandMessage(const MessagePtr commandMessage);

    // �������� ������
    enum class Command
    {
        eUnknown,       // ����������� ������� ����
        eInfo,          // ������ ���������� ����
        eReport,        // ������ ������ �� ������
        eRestart,       // ���������� �����������
        eAllertionsOn,  // ���������� � �������� ������ ��������
        eAllertionsOff, // ���������� � �������� ������ ���������
        // ��������� �������
        eLastCommand
    };

    // ����������� ���������� ���� ������
    void onCommandMessage(const Command command, const MessagePtr message);
    // ��������� ������� /info
    void onCommandInfo(const MessagePtr commandMessage);
    // ��������� ������� /report
    void onCommandReport(const MessagePtr commandMessage);
    // ��������� ������� /restart
    void onCommandRestart(const MessagePtr commandMessage);
    // ��������� ������� /allert, bEnable - true ���� �������, false ���� ���������
    void onCommandAllert(const MessagePtr commandMessage, bool bEnable);

// ������� ��������
private:
    // ��������� �������
    void onCallbackQuery(const TgBot::CallbackQuery::Ptr query);

    // ��� ������������ ������
    enum class ReportType : unsigned long
    {
        eAllChannels,       // ��� ������ ��� �����������
        eSpecialChannel     // ��������� �����
    };

    // ��������� ������������ ������
    using CallBackParams = std::map<std::string, std::string>;
    // ��������� ������� ������
    void executeCallbackReport(const TgBot::CallbackQuery::Ptr query,
                               const CallBackParams& params);
    // ��������� ������� ����������������� �������
    void executeCallbackRestart(const TgBot::CallbackQuery::Ptr query,
                                const CallBackParams& params);
    // ��������� ������� ���������� ��������� ��������� �������������
    void executeCallbackResend(const TgBot::CallbackQuery::Ptr query,
                               const CallBackParams& params);
    // ��������� ������� ���������/���������� ����������
    void executeCallbackAllertion(const TgBot::CallbackQuery::Ptr query,
                                  const CallBackParams& params);

private:
    // ����� ����������� ���������
    ITelegramThreadPtr m_telegramThread;
    // �������������� �� ��������� ����� ����������
    ITelegramThreadPtr m_defaultTelegramThread;
    // ������� ��������� ����
    TelegramBotSettings m_botSettings;
    // ��������������� ����� ��� ������ � ��������� ����
    class CommandsHelper;
    std::shared_ptr<CommandsHelper> m_commandHelper = std::make_shared<CommandsHelper>();
    // ������ � ������������� ����������
    ITelegramUsersListPtr m_telegramUsers;

// ������� ������� �������� ���
private:
    // ��������� � �������������� ����������� �� �������
    struct TaskInfo
    {
        // ������������� ���� �� �������� ���� �������� �������
        int64_t chatId;
        // ������ ������������ ��������� �������
        ITelegramUsersList::UserStatus userStatus;
    };
    // ������� ������� �������� �������� ���
    std::map<TaskId, TaskInfo, TaskComparer> m_monitoringTasksInfo;

// ������ ������ � ������� �������� ���
private:
    // ��������� � ����������� �� �������
    struct ErrorInfo
    {
    public:
        ErrorInfo(const MonitoringErrorEventData* errorData)
            : errorText(errorData->errorText)
            , errorGUID(errorData->errorGUID)
        {}

    public:
        // ����� ������
        CString errorText;
        // ���� �������� ������ ������� �������������
        bool bResendToOrdinaryUsers = false;
        // ����� ������������� ������
        std::chrono::steady_clock::time_point timeOccurred = std::chrono::steady_clock::now();
        // ������������� ������
        GUID errorGUID;
    };
    // ������������ ���������� ��������� ������ �������� ����������
    const size_t kMaxErrorInfoCount = 200;
    // ������ ������� ��������� � �����������
    std::list<ErrorInfo> m_monitoringErrors;
};

////////////////////////////////////////////////////////////////////////////////
// ��������������� ���� ��� ������ � ��������� ���������
class CTelegramBot::CommandsHelper
{
public:
    // ������ ������������� ������� �������� ������������� �������
    typedef ITelegramUsersList::UserStatus AvailableStatus;
    CommandsHelper() = default;

public:
    // ���������� �������� �������
    // @param command - ������������� ����������� �������
    // @param commandtext - ����� �������
    // @param descr - �������� �������
    // @param availabilityStatuses - �������� �������� ������������� ������� �������� �������
    void addCommand(const CTelegramBot::Command command,
                    const CString& commandText, const CString& descr,
                    const std::vector<AvailableStatus>& availabilityStatuses);

    // �������� ������ ������ � ��������� ��� ������������� ������������
    CString getAvailableCommandsWithDescr(const AvailableStatus userStatus) const;

    // �������� ��� ���� �������� �� ������� ������������
    // ���� false - � messageToUser ����� ����� ������������
    bool ensureNeedAnswerOnCommand(ITelegramUsersList* usersList,
                                   const CTelegramBot::Command command,
                                   const MessagePtr commandMessage,
                                   CString& messageToUser) const;

private:
    // �������� ������� �������� ����
    struct CommandDescription
    {
        CommandDescription(const CString& text, const CString& descr)
            : m_text(text) , m_description(descr)
        {}

        // ���� ������� ���� ��������� ��� ���������� �������
        CString m_text;
        // �������� �������
        CString m_description;
        // ����������� ������� ��� ��������� ����� �������������
        std::bitset<AvailableStatus::eLastStatus> m_availabilityForUsers;
    };

    // �������� ������ ����
    //          �������            |  �������� �������
    std::map<CTelegramBot::Command, CommandDescription> m_botCommands;
};

////////////////////////////////////////////////////////////////////////////////
// ����� ��� ������������ ������� ��� ������ ���������, �� ������ UTF-8
class KeyboardCallback
{
public:
    // ����������� �����������
    KeyboardCallback(const std::string& keyWord);
    KeyboardCallback(const KeyboardCallback& reportCallback);

    // �������� ������ ��� ������� � ����� �������� - ��������, Unicode
    KeyboardCallback& addCallbackParam(const std::string& param, const CString& value);
    // �������� ������ ��� ������� � ����� �������� - ��������, Unicode
    KeyboardCallback& addCallbackParam(const std::string& param, const std::wstring& value);
    // �������� ������ ��� ������� � ����� �������� - ��������, UTF-8
    KeyboardCallback& addCallbackParam(const std::string& param, const std::string& value);
    // ������������ ������(UTF-8)
    std::string buildReport() const;

private:
    // ������ ������
    CString m_reportStr;
};
