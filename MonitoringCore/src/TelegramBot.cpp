#include "pch.h"

#include <filesystem>
#include <regex>

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

#include <Messages.h>
#include <DirsService.h>

#include "TelegramBot.h"
#include "Utils.h"

// ����� ������� ���� ��������� ���� ��� �����������
const CString gBotPassword_User   = L"MonitoringAuth";      // ����������� �������������
const CString gBotPassword_Admin  = L"MonitoringAuthAdmin"; // ����������� �������

// ��������� ��� ������� ������
// kKeyWord kParamType={'ReportType'} kParamChan={'chan1'}(�����������) kParamInterval={'1000000'}
namespace reportCallBack
{
    const std::string kKeyWord          = R"(/report)";     // �������� �����
    // ���������
    const std::string kParamType        = "reportType";     // ��� ������
    const std::string kParamChan        = "chan";           // ����� �� �������� ����� �����
    const std::string kParamInterval    = "interval";       // ��������
};

// ��������� ��� ������� �������� �������
namespace restartCallBack
{
    const std::string kKeyWord          = R"(/restart)";    // �������� ������
};

// ��������� ��� ������� ��������� ����������
// kKeyWord kParamid={'GUID'}
namespace resendCallBack
{
    const std::string kKeyWord          = R"(/resend)";     // �������� �����
    // ���������
    const std::string kParamid          = "errorId";        // ������������� ������ � ������ ������(m_monitoringErrors)
};

// ��������� ��� ������� ����������
// kKeyWord kParamEnable={'true'} kParamChan={'chan1'}
namespace allertCallBack
{
    const std::string kKeyWord          = R"(/allert)";     // �������� �����
    // ���������
    const std::string kParamEnable      = "enable";         // ��������� ������������/�������������
    const std::string kParamChan        = "chan";           // ����� �� �������� ����� ��������� �����������
    const std::string kValueAllChannels = "allChannels";    // �������� ������� ������������� ���������� ���������� �� ���� �������
};

// ����� �������
namespace
{
    //------------------------------------------------------------------------//
    // ������� ��������� ������� � ���������
    auto createKeyboardButton(const CString& text, const KeyboardCallback& callback)
    {
        TgBot::InlineKeyboardButton::Ptr channelButton(new TgBot::InlineKeyboardButton);

        // ����� ������ ���� � UTF-8
        channelButton->text = getUtf8Str(text);
        channelButton->callbackData = callback.buildReport();

        return channelButton;
    }
};

////////////////////////////////////////////////////////////////////////////////
// ���������� ����
CTelegramBot::CTelegramBot()
{
    // ������������� �� ������� � ���������� �����������
    EventRecipientImpl::subscribe(onCompletingMonitoringTask);
    // ������������� �� ������� � ����������� ������� � �������� ������� ����� ������
    EventRecipientImpl::subscribe(onMonitoringErrorEvent);
}

//----------------------------------------------------------------------------//
CTelegramBot::~CTelegramBot()
{
    if (m_telegramThread)
        m_telegramThread->stopTelegramThread();
}

//----------------------------------------------------------------------------//
void CTelegramBot::onAllertFromTelegram(const CString& allertMessage)
{
    send_message_to_log(LogMessageData::MessageType::eError, allertMessage);
}

//----------------------------------------------------------------------------//
void CTelegramBot::initBot(ITelegramUsersListPtr telegramUsers)
{
    m_telegramUsers = telegramUsers;
}

//----------------------------------------------------------------------------//
void CTelegramBot::setBotSettings(const TelegramBotSettings& botSettings)
{
    if (m_telegramThread)
    {
        m_telegramThread->stopTelegramThread();
        // ���� �� ������� � ���� ����� �� �������� �� �������� ����������
        // TODO ���������� �� BoostHttpOnlySslClient � dll
        //m_telegramThread.reset();
    }

    m_botSettings = botSettings;

    if (!m_botSettings.bEnable || m_botSettings.sToken.IsEmpty())
        return;

    // ��������� ����� �����������
    {
        ITelegramThreadPtr pTelegramThread;
        if (m_defaultTelegramThread)
            pTelegramThread.swap(m_defaultTelegramThread);
        else
            pTelegramThread = CreateTelegramThread(std::string(CStringA(m_botSettings.sToken)),
                                                   static_cast<ITelegramAllerter*>(this));

        m_telegramThread.swap(pTelegramThread);
    }

    // �������� ������ � ������� ����������� ��� ������ �������
    std::map<std::string, CommandFunction> commandsList;
    // ������� ����������� ��� ��������� ������ ���������
    CommandFunction onUnknownCommand;
    CommandFunction onNonCommandMessage;
    fillCommandHandlers(commandsList, onUnknownCommand, onNonCommandMessage);

    m_telegramThread->startTelegramThread(commandsList, onUnknownCommand, onNonCommandMessage);
}

//----------------------------------------------------------------------------//
void CTelegramBot::setDefaultTelegramThread(ITelegramThreadPtr& pTelegramThread)
{
    m_defaultTelegramThread.swap(pTelegramThread);
}

//----------------------------------------------------------------------------//
void CTelegramBot::sendMessageToAdmins(const CString& message)
{
    if (!m_telegramThread)
        return;

    m_telegramThread->sendMessage(
        m_telegramUsers->getAllChatIdsByStatus(ITelegramUsersList::UserStatus::eAdmin),
        message);
}

//----------------------------------------------------------------------------//
void CTelegramBot::sendMessageToUsers(const CString& message)
{
    if (!m_telegramThread)
        return;

    m_telegramThread->sendMessage(
        m_telegramUsers->getAllChatIdsByStatus(ITelegramUsersList::UserStatus::eOrdinaryUser),
        message);
}

//----------------------------------------------------------------------------//
// IEventRecipient
void CTelegramBot::onEvent(const EventId& code, float eventValue,
                           std::shared_ptr<IEventData> eventData)
{
    if (code == onCompletingMonitoringTask)
    {
        MonitoringResult::Ptr monitoringResult =
            std::static_pointer_cast<MonitoringResult>(eventData);

        // ��������� ��� ��� ���� �������
        auto it = m_monitoringTasksInfo.find(monitoringResult->m_taskId);
        if (it == m_monitoringTasksInfo.end())
            return;

        // ���� ��� ����� ��������� �����
        bool bDetailedInfo = it->second.userStatus == ITelegramUsersList::UserStatus::eAdmin;
        // ���������� ������������� ����
        int64_t chatId = it->second.chatId;

        // ������� ������� �� ������
        m_monitoringTasksInfo.erase(it);

        // ��������� �����
        CString reportText;
        reportText.Format(L"����� �� %s - %s\n\n",
                          monitoringResult->m_taskResults.front().pTaskParameters->startTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString(),
                          monitoringResult->m_taskResults.front().pTaskParameters->endTime.Format(L"%d.%m.%Y  %H:%M:%S").GetString());

        // ��������� ����� �� ���� �������
        for (auto& channelResData : monitoringResult->m_taskResults)
        {
            // ���������� ���������� ��������� ������ - ������� ����� �� ���������
            CTimeSpan permissibleEmptyDataTime =
                (channelResData.pTaskParameters->endTime - channelResData.pTaskParameters->startTime).GetTotalSeconds() / 30;

            reportText.AppendFormat(L"����� \"%s\":", channelResData.pTaskParameters->channelName);

            switch (channelResData.resultType)
            {
            case MonitoringResult::Result::eSucceeded:  // ������ ������� ��������
                {
                    if (bDetailedInfo)
                    {
                        // ��������, ��������� ������� ���������� ����������. �� ��������� - NAN
                        float allarmingValue = NAN;

                        // ���� ����� ��������� - ���� ����� �������������� �������� � ������
                        auto pMonitoringService = get_monitoing_service();
                        for (size_t i = 0, count = pMonitoringService->getNumberOfMonitoringChannels();
                             i < count; ++i)
                        {
                            const MonitoringChannelData& channelData = pMonitoringService->getMonitoringChannelData(i);
                            if (channelData.channelName == channelResData.pTaskParameters->channelName)
                            {
                                allarmingValue = channelData.allarmingValue;
                                break;
                            }
                        }

                        // ���� �� ����� �������� ��� ������� ����� ��������� - ��������� ���������� ����� ��������
                        if (_finite(allarmingValue) != 0)
                        {
                            // ���� �� �������� ���� �� �������� ����� �� ����������
                            if ((allarmingValue >= 0 && channelResData.maxValue >= allarmingValue) ||
                                (allarmingValue < 0 && channelResData.minValue <= allarmingValue))
                                reportText.AppendFormat(L"���������� ������� %.02f ��� ��������, ",
                                                        allarmingValue);
                        }
                    }

                    reportText.AppendFormat(L"�������� �� �������� [%.02f..%.02f], ��������� ��������� - %.02f.",
                                            channelResData.minValue, channelResData.maxValue,
                                            channelResData.currentValue);

                    // ���� ����� ��������� ������
                    if (bDetailedInfo && channelResData.emptyDataTime > permissibleEmptyDataTime)
                        reportText.AppendFormat(L" ����� ��������� ������ (%s).",
                                                time_span_to_string(channelResData.emptyDataTime).GetString());
                }
                break;
            case MonitoringResult::Result::eNoData:     // � ���������� ��������� ��� ������
            case MonitoringResult::Result::eErrorText:  // �������� ������
                {
                    // ��������� � ��������� ������
                    if (!channelResData.errorText.IsEmpty())
                        reportText.Append(channelResData.errorText);
                    else
                        reportText.Append(L"��� ������ � ����������� ���������.");
                }
                break;
            default:
                assert(!"�� ��������� ��� ����������");
                break;
            }

            reportText += L"\n";
        }

        // ���������� ������� ����� ������
        m_telegramThread->sendMessage(chatId, reportText);
    }
    else if (code == onMonitoringErrorEvent)
    {
        std::shared_ptr<MonitoringErrorEventData> errorData =
            std::static_pointer_cast<MonitoringErrorEventData>(eventData);
        assert(!errorData->errorText.IsEmpty());

        // �������� ������ ��������� �����
        auto adminsChats = m_telegramUsers->getAllChatIdsByStatus(ITelegramUsersList::UserStatus::eAdmin);
        if (adminsChats.empty() || errorData->errorText.IsEmpty())
            return;

        // ��������� ������ �������� ��� ���� ������
        TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

        // ������ �� ���������� �����������
        // kKeyWord
        auto buttonRestart = createKeyboardButton(L"������������� �������",
                                                  KeyboardCallback(restartCallBack::kKeyWord));

        // ��������� ����� ������ � ������ � ���������� � �������������
        m_monitoringErrors.emplace_back(errorData.get());

        if (m_monitoringErrors.size() > kMaxErrorInfoCount)
            m_monitoringErrors.pop_front();

        // ������ �� �������� ����� ��������� ������� �������������
        // kKeyWord kParamid={'GUID'}
        auto buttonResendToOrdinary = createKeyboardButton(L"���������� ������� �������������",
            KeyboardCallback(resendCallBack::kKeyWord).
                addCallbackParam(resendCallBack::kParamid, CString(CComBSTR(errorData->errorGUID))));

        keyboard->inlineKeyboard.push_back({ buttonRestart, buttonResendToOrdinary });

        // ���������� ���� ������� ����� ������ � ���������� ��� ������� �������
        m_telegramThread->sendMessage(adminsChats, errorData->errorText, false, 0, keyboard);
    }
    else
        assert(!"����������� �������");
}

//----------------------------------------------------------------------------//
void CTelegramBot::fillCommandHandlers(std::map<std::string, CommandFunction>& commandsList,
                                       CommandFunction& onUnknownCommand,
                                       CommandFunction& onNonCommandMessage)
{
    m_commandHelper = std::make_shared<CommandsHelper>();

    // ������ ������������� ������� �������� ��� ������� �� ���������
    const std::vector<ITelegramUsersList::UserStatus> kDefaultAvailability =
    {   ITelegramUsersList::eAdmin, ITelegramUsersList::eOrdinaryUser };

    static_assert(ITelegramUsersList::UserStatus::eLastStatus == 3,
                  "������ ����������� ���������, �������� ����� ������������ ����������� ������");

    // ����������� ������� � ������
    auto addCommand = [&]
        (const Command command, const CString& commandText, const CString& descr,
         const std::vector<ITelegramUsersList::UserStatus>& availabilityStatuses)
    {
        m_commandHelper ->addCommand(command, commandText,
                                     descr, availabilityStatuses);
        commandsList.try_emplace(getUtf8Str(commandText),
                                 [this, command](const auto message)
                                 {
                                     this->onCommandMessage(command, message);
                                 });
    };

    static_assert(Command::eLastCommand == (Command)6,
                  "���������� ������ ����������, ���� �������� ���������� � ����!");

    // !��� ������� ����� ��������� � �������� ������ ����� ����� �� ��������� �������������
    addCommand(Command::eInfo,    L"info",    L"�������� ������ ����.", kDefaultAvailability);
    addCommand(Command::eReport,  L"report",  L"������������ �����.",   kDefaultAvailability);
    addCommand(Command::eRestart, L"restart", L"������������� ������� �����������.",
               { ITelegramUsersList::eAdmin });
    addCommand(Command::eAllertionsOn,  L"allertionOn",  L"�������� ���������� � ��������.",
               { ITelegramUsersList::eAdmin });
    addCommand(Command::eAllertionsOff, L"allertionOff", L"��������� ���������� � ��������.",
               { ITelegramUsersList::eAdmin });

    // ������� ����������� ��� ��������� ������ ���������
    onUnknownCommand = onNonCommandMessage =
        [this](const auto message)
        {
            get_service<CMassages>().call([this, &message]() { this->onNonCommandMessage(message); });
        };
    // ��������� �������� �� ������� ����������
    m_telegramThread->getBotEvents().onCallbackQuery(
        [this](const auto param)
        {
            get_service<CMassages>().call([this, &param]() { this->onCallbackQuery(param); });
        });
}

//----------------------------------------------------------------------------//
void CTelegramBot::onNonCommandMessage(const MessagePtr commandMessage)
{
    // ������������ ����������� ���������
    TgBot::User::Ptr pUser = commandMessage->from;
    // ����� ��������� ���������� ���������
    CString messageText = getUNICODEString(commandMessage->text.c_str()).Trim();

    // ���������� ��� ���� ����� ������������
    m_telegramUsers->ensureExist(pUser, commandMessage->chat->id);

    // ��������� ������� ����� ���������� ������������ � �����
    CString messageToUser;

    if (messageText.CompareNoCase(gBotPassword_User) == 0)
    {
        // ���� ������ ��� �������� ������������
        switch (m_telegramUsers->getUserStatus(pUser))
        {
        case ITelegramUsersList::UserStatus::eAdmin:
            messageToUser = L"������������ �������� ��������������� �������. ����������� �� ���������.";
            break;
        case ITelegramUsersList::UserStatus::eOrdinaryUser:
            messageToUser = L"������������ ��� �����������.";
            break;
        default:
            assert(!"�� ��������� ��� ������������.");
            [[fallthrough]];
        case ITelegramUsersList::UserStatus::eNotAuthorized:
            m_telegramUsers->setUserStatus(pUser, ITelegramUsersList::UserStatus::eOrdinaryUser);
            messageToUser = L"������������ ������� �����������.";
            break;
        }
    }
    else if (messageText.CompareNoCase(gBotPassword_Admin) == 0)
    {
        // ���� ������ ��� ��������������
        switch (m_telegramUsers->getUserStatus(pUser))
        {
        case ITelegramUsersList::UserStatus::eAdmin:
            messageToUser = L"������������ ��� ����������� ��� �������������.";
            break;
        default:
            assert(!"�� ��������� ��� ������������.");
            [[fallthrough]];
        case ITelegramUsersList::UserStatus::eOrdinaryUser:
        case ITelegramUsersList::UserStatus::eNotAuthorized:
            m_telegramUsers->setUserStatus(pUser, ITelegramUsersList::UserStatus::eAdmin);
            messageToUser = L"������������ ������� ����������� ��� �������������.";
            break;
        }
    }
    else
    {
        // ����� ���������� �� � ���, ������� �� eUnknown ������������ ����� ������
        m_commandHelper->ensureNeedAnswerOnCommand(m_telegramUsers, Command::eUnknown,
                                                   commandMessage, messageToUser);
    }

    if (!messageToUser.IsEmpty())
        m_telegramThread->sendMessage(commandMessage->chat->id, messageToUser);
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCommandMessage(const Command command, const MessagePtr message)
{
    // �.�. ������� ����� ������ �� ������� ������ �� ����� �� ������ ��������������
    // ������������� ������������ ��� � �������� �����
    get_service<CMassages>().call(
        [this, &command, &message]()
        {
            CString messageToUser;
            // ��������� ��� ���� ������������� �������� �� ������� ����� ������������
            if (m_commandHelper->ensureNeedAnswerOnCommand(m_telegramUsers, command,
                                                           message, messageToUser))
            {
                switch (command)
                {
                default:
                    assert(!"����������� �������!.");
                    [[fallthrough]];
                case Command::eInfo:
                    onCommandInfo(message);
                    break;
                case Command::eReport:
                    onCommandReport(message);
                    break;
                case Command::eRestart:
                    onCommandRestart(message);
                    break;
                case Command::eAllertionsOn:
                case Command::eAllertionsOff:
                    onCommandAllert(message, command == Command::eAllertionsOn);
                    break;
                }
            }
            else if (!messageToUser.IsEmpty())
                // ���� ������������ ������� �� �������� ���������� ��� ���������� �� �����
                m_telegramThread->sendMessage(message->chat->id, messageToUser);
            else
                assert(!"������ ���� ����� ��������� ������������");
        });
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCommandInfo(const MessagePtr commandMessage)
{
    ITelegramUsersList::UserStatus userStatus =
        m_telegramUsers->getUserStatus(commandMessage->from);

    CString messageToUser = m_commandHelper->getAvailableCommandsWithDescr(userStatus);
    if (!messageToUser.IsEmpty())
        m_telegramThread->sendMessage(commandMessage->chat->id, messageToUser);
    else
        assert(!"������ ���� ��������� � �����!");
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCommandReport(const MessagePtr commandMessage)
{
    // �������� ������ �������
    std::list<CString> monitoringChannels = get_monitoing_service()->getNamesOfMonitoringChannels();
    if (monitoringChannels.empty())
    {
        m_telegramThread->sendMessage(commandMessage->chat->id, L"������ ��� ����������� �� �������");
        return;
    }

    // ���������� ������������ ������ � ������� ������ �� �������� ����� �����
    TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    // �������� ������ � ��������, text ���������� ��� ������ ����� ����� ������������� � UTF-8, ����� ������ �� �����
    auto addButton = [&keyboard](const CString& text, const ReportType reportType)
    {
        // ������ �� ������ ������ ������ ���� ����
        // kKeyWord kParamType={'ReportType'}
        auto button =
            createKeyboardButton(
                text,
                KeyboardCallback(reportCallBack::kKeyWord).
                    addCallbackParam(reportCallBack::kParamType, std::to_wstring((unsigned long)reportType)));

        keyboard->inlineKeyboard.push_back({ button });
    };

    // ��������� ������
    addButton(L"��� ������",         ReportType::eAllChannels);
    addButton(L"������������ �����", ReportType::eSpecialChannel);

    m_telegramThread->sendMessage(commandMessage->chat->id,
                                  L"�� ����� ������� ������������ �����?",
                                  false, 0, keyboard);
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCommandRestart(const MessagePtr commandMessage)
{
    // ���������� ������� ������ ����� ������ ��� ��� ���� ����� ����� ������� ��� ������ ������
    CString batFullPath = get_service<DirsService>().getExeDir() + kRestartSystemFileName;

    // ��������� ������������
    CString messageToUser;
    if (std::filesystem::is_regular_file(batFullPath.GetString()))
    {
        m_telegramThread->sendMessage(commandMessage->chat->id, L"���������� ������� ��������������.");

        // ��������� ������
        STARTUPINFO cif = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION m_ProcInfo = { 0 };
        if (FALSE != CreateProcess(batFullPath.GetBuffer(),     // ��� ������������ ������
                                   nullptr,	                    // ��������� ������
                                   NULL,                        // ��������� �� ��������� SECURITY_ATTRIBUTES
                                   NULL,                        // ��������� �� ��������� SECURITY_ATTRIBUTES
                                   0,                           // ���� ������������ �������� ��������
                                   NULL,                        // ����� �������� �������� ��������
                                   NULL,                        // ��������� �� ���� �����
                                   NULL,                        // ������� ���� ��� �������
                                   &cif,                        // ��������� �� ��������� STARTUPINFO
                                   &m_ProcInfo))                // ��������� �� ��������� PROCESS_INFORMATION)
        {	// ������������� ������ �� �����
            CloseHandle(m_ProcInfo.hThread);
            CloseHandle(m_ProcInfo.hProcess);
        }
    }
    else
        m_telegramThread->sendMessage(commandMessage->chat->id, L"���� ��� ����������� �� ������.");
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCommandAllert(const MessagePtr commandMessage, bool bEnable)
{
    // �������� ������ �������
    std::list<CString> monitoringChannels = get_monitoing_service()->getNamesOfMonitoringChannels();
    if (monitoringChannels.empty())
    {
        m_telegramThread->sendMessage(commandMessage->chat->id, L"������ ��� ����������� �� �������");
        return;
    }

    // ���������� ������������ ������ � ������� ������ �� �������� ����� �����
    TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    // ������ ������� ������ ���� � ������ ������
    KeyboardCallback defaultCallBack(allertCallBack::kKeyWord);
    defaultCallBack.addCallbackParam(allertCallBack::kParamEnable, CString(bEnable ? L"true" : L"false"));

    // �������� ������ � ��������, text ���������� ��� ������ ����� ����� ������������� � UTF-8, ����� ������ �� �����
    auto addButtonWithChannel = [&keyboard, &defaultCallBack](const CString& channelName)
    {
        // ������ �� ������ ������ ������ ���� ����
        // kKeyWord kParamEnable={'true'} kParamChan={'chan1'}
        auto button =
            createKeyboardButton(
                channelName,
                KeyboardCallback(defaultCallBack).
                addCallbackParam(allertCallBack::kParamChan, channelName));

        keyboard->inlineKeyboard.push_back({ button });
    };

    // ��������� ������ ��� ������� ������
    for (const auto& channel : monitoringChannels)
    {
        addButtonWithChannel(channel);
    }

    // ��������� ������ �� ����� ��������
    if (monitoringChannels.size() > 1)
        keyboard->inlineKeyboard.push_back({
            createKeyboardButton(L"��� ������",
                                 KeyboardCallback(defaultCallBack).
                                    addCallbackParam(allertCallBack::kParamChan, allertCallBack::kValueAllChannels)) });

    CString text;
    text.Format(L"�������� ����� ��� %s ����������.", bEnable ? L"���������" : L"����������");
    m_telegramThread->sendMessage(commandMessage->chat->id, text, false, 0, keyboard);
}

////////////////////////////////////////////////////////////////////////////////
// ������� ��������
namespace CallbackParser
{
    using namespace boost::spirit::x3;

    namespace
    {
        template <typename T>
        struct as_type
        {
            template <typename Expr>
            auto operator[](Expr expr) const
            {
                return rule<struct _, T>{"as"} = expr;
            }
        };

        template <typename T>
        static const as_type<T> as = {};
    }
    auto quoted = [](char q)
    {
        return lexeme[q >> *(q >> char_(q) | '\\' >> char_ | char_ - q) >> q];
    };

    auto value  = quoted('\'') | quoted('"');
    auto key    = lexeme[+alpha];
    auto pair   = key >> "={" >> value >> '}';
    auto parser = skip(space) [ * as<std::pair<std::string, std::string>>[pair] ];
}

//----------------------------------------------------------------------------//
void CTelegramBot::onCallbackQuery(const TgBot::CallbackQuery::Ptr query)
{
    try
    {
        // ������ ������ � ��������� ����� ���������� �� �������
        CallBackParams callBackParams;
        if (boost::spirit::x3::parse(query->data.begin(), query->data.end(),
                                     reportCallBack::kKeyWord >> CallbackParser::parser, callBackParams))
            executeCallbackReport(query, callBackParams);
        else if (boost::spirit::x3::parse(query->data.begin(), query->data.end(),
                                          restartCallBack::kKeyWord >> CallbackParser::parser, callBackParams))
            executeCallbackRestart(query, callBackParams);
        else if (boost::spirit::x3::parse(query->data.begin(), query->data.end(),
                                          resendCallBack::kKeyWord >> CallbackParser::parser, callBackParams))
            executeCallbackResend(query, callBackParams);
        else if (boost::spirit::x3::parse(query->data.begin(), query->data.end(),
                                          allertCallBack::kKeyWord >> CallbackParser::parser, callBackParams))
            executeCallbackAllertion(query, callBackParams);
        else
            throw std::runtime_error("������ ������� �������");
    }
    catch (const std::exception& exc)
    {
        assert(false);

        CString errorStr;
        // ��������� ������ ������� �������
        errorStr.Format(L"%s. ���������� � �������������� �������, ����� ������� \"%s\"",
                        CString(exc.what()).GetString(), getUNICODEString(query->data).GetString());

        // �������� ���� ���-�� ������������
        m_telegramThread->sendMessage(query->message->chat->id, errorStr);
        // ��������� �� ������
        onAllertFromTelegram(errorStr);
    }
}

//----------------------------------------------------------------------------//
void CTelegramBot::executeCallbackReport(const TgBot::CallbackQuery::Ptr query,
                                         const CallBackParams& reportParams)
{
    // � ������� ������ ������ ���� ������������ ���������, �������� ������ ������ ���� ����
    // kKeyWord kParamType={'ReportType'} kParamChan={'chan1'}(�����������) kParamInterval={'1000000'}

    // ��� ������
    auto reportTypeIt = reportParams.find(reportCallBack::kParamType);
    // ��������� ����� ��� ������� ������ � ����� ���������� �� �������
    if (reportTypeIt == reportParams.end())
        throw std::runtime_error("�� ��������� ������.");

    // �������� ������ �������
    std::list<CString> monitoringChannels = get_monitoing_service()->getNamesOfMonitoringChannels();
    if (monitoringChannels.empty())
        throw std::runtime_error("�� ������� �������� ������ �������, ���������� ��������� �������");

    // ��� ������ �� �������� ������ �����
    auto channelIt = reportParams.find(reportCallBack::kParamChan);

    // ��� ������
    ReportType reportType = (ReportType)std::stoul(reportTypeIt->second);

    // ������ ������ ������� "kKeyWord kParamType={'ReportType'}" � ��������� ����� ���� �������� �� ������ ������ ����� �����
    switch (reportType)
    {
    default:
        assert(!"�� ��������� ��� ������.");
        [[fallthrough]];
    case ReportType::eSpecialChannel:
        {
            // ���� ����� �� ������ ���� ��� ���������
            if (channelIt == reportParams.end())
            {
                // ��������� ������
                KeyboardCallback defCallBack = KeyboardCallback(reportCallBack::kKeyWord).
                    addCallbackParam(reportTypeIt->first, reportTypeIt->second);

                // ���������� ������������ ������ � ������� ������ �� �������� ����� �����
                TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
                keyboard->inlineKeyboard.reserve(monitoringChannels.size());

                for (const auto& channel : monitoringChannels)
                {
                    auto channelButton = createKeyboardButton(channel,
                                                              KeyboardCallback(defCallBack).
                                                              addCallbackParam(reportCallBack::kParamChan, channel));

                    keyboard->inlineKeyboard.push_back({ channelButton });
                }

                m_telegramThread->sendMessage(query->message->chat->id,
                                              L"�������� �����",
                                              false, 0, keyboard);
                return;
            }
        }
        break;

    case ReportType::eAllChannels:
        break;

    }

    auto timeIntervalIt = reportParams.find(reportCallBack::kParamInterval);
    // ��������� ��� kParamInterval �����
    if (timeIntervalIt == reportParams.end())
    {
        // ��������� ������
        KeyboardCallback defCallBack(reportCallBack::kKeyWord);
        defCallBack.addCallbackParam(reportTypeIt->first, reportTypeIt->second);

        // ���� ������� ��� - ��������� ���
        if (channelIt != reportParams.end())
            defCallBack.addCallbackParam(channelIt->first, channelIt->second);

        // ������ ������������ ������ ��������
        TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
        keyboard->inlineKeyboard.resize((size_t)MonitoringInterval::eLast);

        // ��������� ������ �� ����� �����������
        for (int i = (int)MonitoringInterval::eLast - 1; i >= 0; --i)
        {
            auto intervalButton = createKeyboardButton(
                monitoring_interval_to_string(MonitoringInterval(i)),
                KeyboardCallback(defCallBack).
                addCallbackParam(reportCallBack::kParamInterval, std::to_wstring(i)));

            keyboard->inlineKeyboard[i] = { intervalButton };
        }

        m_telegramThread->sendMessage(query->message->chat->id,
                                      L"�������� �������� ������� �� ������� ����� �������� �����",
                                      false, 0, keyboard);
    }
    else
    {
        // �������� ��� ����������� ���������, ��������� ������� �����������
        // ������ ������� ��� �����������
        std::list<CString> channelsForTask;

        switch (reportType)
        {
        default:
            assert(!"�� ��������� ��� ������.");
            [[fallthrough]];
        case ReportType::eAllChannels:
            channelsForTask = std::move(monitoringChannels);
            break;

        case ReportType::eSpecialChannel:
            if (channelIt == reportParams.end())
                throw std::runtime_error("�� ������� ���������� ��� ������, ���������� ��������� �������");

            channelsForTask.emplace_back(getUNICODEString(channelIt->second));
            break;
        }

        CTime stopTime = CTime::GetCurrentTime();
        CTime startTime = stopTime -
            monitoring_interval_to_timespan((MonitoringInterval)std::stoi(timeIntervalIt->second));

        TaskInfo taskInfo;
        taskInfo.chatId = query->message->chat->id;
        taskInfo.userStatus = m_telegramUsers->getUserStatus(query->from);

        // �������� ������������ ��� ������ �����������, ����������� ����� �����
        // ������������ ����� ���������� ��� ������ �� ����������
        m_telegramThread->sendMessage(query->message->chat->id,
                                      L"����������� ������ ������, ��� ����� ������ ��������� �����.");

        m_monitoringTasksInfo.try_emplace(
            get_monitoing_tasks_service()->addTaskList(channelsForTask, startTime, stopTime,
                                                       IMonitoringTasksService::eHigh),
            std::move(taskInfo));
    }
}

//----------------------------------------------------------------------------//
void CTelegramBot::executeCallbackRestart(const TgBot::CallbackQuery::Ptr query,
                                          const CallBackParams& params)
{
    assert(params.empty() && "���������� �� �������������");

    CString messageToUser;
    if (!m_commandHelper->ensureNeedAnswerOnCommand(m_telegramUsers, Command::eRestart,
                                                    query->message, messageToUser))
    {
        assert(!"������������ �������� ���������� ��� ���������� �� ���������� ������ ��������.");
        m_telegramThread->sendMessage(query->message->chat->id,
                                      L"� ��� ��� ���������� �� ���������� �������, ���������� � ��������������!");
    }
    else
        // ��������� ��� ������������ �������� ������ ��������
        onCommandRestart(query->message);
}

//----------------------------------------------------------------------------//
void CTelegramBot::executeCallbackResend(const TgBot::CallbackQuery::Ptr query,
                                         const CallBackParams& params)
{
    auto errorIdIt = params.find(resendCallBack::kParamid);
    if (errorIdIt == params.end())
        throw std::runtime_error("��� ������������ ��������� � ������� ��������� ���������.");

    // ����������� ���� �� ������
    GUID errorGUID;
    CLSIDFromString(CComBSTR(errorIdIt->second.c_str()), &errorGUID);

    auto errorIt = std::find_if(m_monitoringErrors.begin(), m_monitoringErrors.end(),
                                [&errorGUID](const ErrorInfo& errorInfo)
                                {
                                    return IsEqualGUID(errorGUID, errorInfo.errorGUID);
                                });
    if (errorIt == m_monitoringErrors.end())
    {
        CString text;
        text.Format(L"������������ ������ ��� � ������, �������� ������ �������� ���������� (�������� ��������� %u ������) ��� ��������� ���� ������������.",
                    kMaxErrorInfoCount);
        m_telegramThread->sendMessage(query->message->chat->id, text);
        return;
    }

    if (errorIt->bResendToOrdinaryUsers)
    {
        m_telegramThread->sendMessage(query->message->chat->id,
                                      L"������ ��� ���� ���������.");
        return;
    }

    // ���������� ������ ������� �������������
    auto ordinaryUsersChatList = m_telegramUsers->getAllChatIdsByStatus(ITelegramUsersList::UserStatus::eOrdinaryUser);
    m_telegramThread->sendMessage(ordinaryUsersChatList, errorIt->errorText);

    errorIt->bResendToOrdinaryUsers = true;

    m_telegramThread->sendMessage(query->message->chat->id,
                                  L"������ ���� ������� ��������� ������� �������������.");
}

//----------------------------------------------------------------------------//
void CTelegramBot::executeCallbackAllertion(const TgBot::CallbackQuery::Ptr query,
                                            const CallBackParams& params)
{
    // ������ ������� kKeyWord kParamEnable={'true'} kParamChan={'chan1'}
    auto enableIt = params.find(allertCallBack::kParamEnable);
    auto channelIt = params.find(allertCallBack::kParamChan);

    if (enableIt == params.end() || channelIt == params.end())
        throw std::runtime_error("��� ������������ ��������� � ������� ���������� ������������.");
    // ������������/������������� ����������
    bool bEnableAllertion = enableIt->second == "true";
    // ������ �����������
    auto monitoringService = get_monitoing_service();
    // ��������� � ����� ������������
    CString messageText;
    if (channelIt->second == allertCallBack::kValueAllChannels)
    {
        // ����������� ���������� ��� ���� �������
        size_t channelsCount = monitoringService->getNumberOfMonitoringChannels();
        if (channelsCount == 0)
            throw std::runtime_error("��� ��������� ��� ����������� �������, ���������� � ��������������");

        for (size_t channelInd = 0; channelInd < channelsCount; ++channelInd)
        {
            monitoringService->changeMonitoingChannelNotify(channelInd, bEnableAllertion);
        }

        messageText.Format(L"���������� ��� ���� ������� %s", bEnableAllertion ? L"��������" : L"���������");
    }
    else
    {
        // �������� ������ �������
        std::list<CString> monitoringChannels = monitoringService->getNamesOfMonitoringChannels();
        if (monitoringChannels.empty())
            throw std::runtime_error("��� ��������� ��� ����������� �������, ���������� � ��������������");

        // ��� ������ �� �������
        CString callBackChannel = getUNICODEString(channelIt->second);
        // ������� ��� � ������ ����������� ������ �� ������ �� ����������� ����� ��� �����
        auto channelIt = std::find_if(monitoringChannels.begin(), monitoringChannels.end(),
                                      [&callBackChannel](const auto& channelName)
                                      {
                                          return callBackChannel == channelName;
                                      });

        if (channelIt == monitoringChannels.end())
            throw std::runtime_error("� ������ ������ � ������ ����������� ��� ���������� ���� ������.");

        monitoringService->changeMonitoingChannelNotify(std::distance(monitoringChannels.begin(), channelIt),
                                                        bEnableAllertion);

        messageText.Format(L"���������� ��� ������ %s %s", callBackChannel, bEnableAllertion ? L"��������" : L"���������");
    }

    assert(!messageText.IsEmpty() && "��������� ������������ ������.");
    m_telegramThread->sendMessage(query->message->chat->id, messageText);
}

////////////////////////////////////////////////////////////////////////////////
// ������������ �������
KeyboardCallback::KeyboardCallback(const std::string& keyWord)
    : m_reportStr(keyWord.c_str())
{}

//----------------------------------------------------------------------------//
KeyboardCallback::KeyboardCallback(const KeyboardCallback& reportCallback)
    : m_reportStr(reportCallback.m_reportStr)
{}

//----------------------------------------------------------------------------//
KeyboardCallback& KeyboardCallback::addCallbackParam(const std::string& param,
                                                     const CString& value)
{
    // ������ ������ ���� ����
    // /%COMMAND% %PARAM_1%={'%VALUE_1%'} %PARAM_2%={'%VALUE_2%'} ...
    // ��������� ���� %PARAM_N%={'%VALUE_N'}
    m_reportStr.AppendFormat(L" %s={\'%s\'}",
                             getUNICODEString(param).GetString(), value.GetString());

    return *this;
}

//----------------------------------------------------------------------------//
KeyboardCallback& KeyboardCallback::addCallbackParam(const std::string& param,
                                                     const std::wstring& value)
{
    return addCallbackParam(param, CString(value.c_str()));
}

//----------------------------------------------------------------------------//
KeyboardCallback& KeyboardCallback::addCallbackParam(const std::string& param,
                                                     const std::string& value)
{
    return addCallbackParam(param, getUNICODEString(value));
}

//----------------------------------------------------------------------------//
std::string KeyboardCallback::buildReport() const
{
    // ������ ������������ ��������, ���� �� ������������ - �������� ��������
    const std::wregex escapedCharacters(LR"(['])");
    const std::wstring rep(LR"(\$&)");

    // ��-�� ���� ��� TgTypeParser::appendToJson ��� �� ��������� '}'
    // ���� ��������� �������� �������� '}' ��������� � ����� ������
    CString resultReport = std::regex_replace((m_reportStr + " ").GetString(),
                                              escapedCharacters,
                                              rep).c_str();

    // ������������ ����������� �� ������ ������� ��������� 65 ��������
    //constexpr int maxReportSize = 65;
    //assert(resultReport.GetLength() <= maxReportSize);

    return getUtf8Str(resultReport);
}

////////////////////////////////////////////////////////////////////////////////
// ��������������� ���� ��� ������ � ��������� ���������
void CTelegramBot::CommandsHelper::
addCommand(const CTelegramBot::Command command,
           const CString& commandText, const CString& descr,
           const std::vector<AvailableStatus>& availabilityStatuses)
{
    auto commandDescrIt = m_botCommands.try_emplace(command, commandText, descr);
    assert(commandDescrIt.second && "������� ��� ���������.");

    CommandDescription& commandDescr = commandDescrIt.first->second;
    for (auto& status : availabilityStatuses)
        commandDescr.m_availabilityForUsers[status] = true;
}

//----------------------------------------------------------------------------//
CString CTelegramBot::CommandsHelper::
getAvailableCommandsWithDescr(const AvailableStatus userStatus) const
{
    CString message;
    for (auto& command : m_botCommands)
    {
        if (command.second.m_availabilityForUsers[userStatus])
            message += L"/" + command.second.m_text + L" - " + command.second.m_description + L"\n";
    }

    if (!message.IsEmpty())
        message = L"�������������� ������� ����:\n\n\n" + message + L"\n\n��� ���� ����� �� ������������ ���������� �������� �� ����� ����(����������� ������������ ���� ����� ������� �������(/)!). ��� ������ � ���� ����, ��� ������ ��������������.";
    return message;
}

//----------------------------------------------------------------------------//
bool CTelegramBot::CommandsHelper::
ensureNeedAnswerOnCommand(ITelegramUsersList* usersList,
                          const CTelegramBot::Command command,
                          const MessagePtr commandMessage,
                          CString& messageToUser) const
{

    // ������������ ����������� ���������
    const TgBot::User::Ptr& pUser = commandMessage->from;

    // ���������� ��� ���� ����� ������������
    usersList->ensureExist(pUser, commandMessage->chat->id);
    // �������� ������ ������������
    auto userStatus = usersList->getUserStatus(pUser);
    // ��������� ��� ������������ ����� ������������ �������
    auto commandIt = m_botCommands.find(command);
    if (commandIt == m_botCommands.end() ||
        !commandIt->second.m_availabilityForUsers[userStatus])
    {
        // ��������� ����� ������������ �� ������� ��������� ��� ������
        CString availableCommands = getAvailableCommandsWithDescr(userStatus);
        if (availableCommands.IsEmpty())
        {
            if (userStatus == ITelegramUsersList::eNotAuthorized)
                messageToUser = L"��� ������ ���� ��� ���������� ��������������.";
            else
                messageToUser = L"����������� �������. � ��� ��� ��������� ������, ���������� � ��������������";
        }
        else
            messageToUser = L"����������� �������. " + std::move(availableCommands);
        assert(!messageToUser.IsEmpty());

        return false;
    }

    return true;
}