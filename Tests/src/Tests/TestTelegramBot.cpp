#include "pch.h"

#include <fstream>
#include <regex>

#include <boost/algorithm/string.hpp>

#include <tgbot/tgbot.h>

#include <TelegramDLL/TelegramThread.h>

#include <DirsService.h>
#include <ITrendMonitoring.h>
#include <Messages.h>

#include "TestHelper.h"
#include "TestTelegramBot.h"

// ������������� ���������� ������������ ���������
const int64_t kTelegramTestChatId = 1234;

////////////////////////////////////////////////////////////////////////////////
// ������� ������ ��� ������ �������������
TgBot::InlineKeyboardButton::Ptr generateKeyBoardButton(const CString& text,
                                                        const CString& callBack)
{
    TgBot::InlineKeyboardButton::Ptr button = std::make_shared<TgBot::InlineKeyboardButton>();
    button->text = getUtf8Str(text);
    button->callbackData = getUtf8Str(callBack);
    return button;
}

////////////////////////////////////////////////////////////////////////////////
// �������� ������� ������������ ������ ����
TEST_F(TestTelegramBot, CheckCommandListeners)
{
    TgBot::EventBroadcaster& botEvents = m_pTelegramThread->getBotEvents();
    // ��������� ������� ������������
    auto& commandListeners = botEvents.getCommandListeners();
    for (const auto& command : m_commandsToUserStatus)
    {
        EXPECT_NE(commandListeners.find(CStringA(command.first).GetString()), commandListeners.end())
            << "� ������� ���� \"" + CStringA(command.first) + "\" ����������� ����������";
    }

    EXPECT_EQ(commandListeners.size(), 5) << "���������� ������������ ������ � ����� ������ �� ���������";
}

//----------------------------------------------------------------------------//
// ��������� ������� �� ��������� �������������
TEST_F(TestTelegramBot, CheckCommandsAvailability)
{
    // ��������� ��������� �������� ����
    CString expectedMessage;

    // ����� ��� �������� ������� ������������
    TelegramUserMessagesChecker checker(m_pTelegramThread, &expectedMessage);

    // �� ��������� � ������������ ������ eNotAuthorized � � ���� ��� ��������� ������ ���� �� �� ������������
    expectedMessage = L"��� ������ ���� ��� ���������� ��������������.";
    for (const auto& command : m_commandsToUserStatus)
    {
        emulateBroadcastMessage(L"/" + command.first);
    }
    // ��� ����� ��������� ��������� ����� ��������� � ������������ ���� ������ ��������
    emulateBroadcastMessage(L"132123");
    emulateBroadcastMessage(L"/22");

    {
        // ���������� ������������ ��� �������� eOrdinaryUser
        expectedMessage = L"������������ ������� �����������.";
        emulateBroadcastMessage(L"MonitoringAuth");
        EXPECT_EQ(m_pUserList->getUserStatus(nullptr), ITelegramUsersList::UserStatus::eOrdinaryUser)
            << "� ������������ �� ������������� ������ ����� �����������";

        // �������� ���������� ��������� �����������
        expectedMessage = L"������������ ��� �����������.";
        emulateBroadcastMessage(L"MonitoringAuth");
        EXPECT_EQ(m_pUserList->getUserStatus(nullptr), ITelegramUsersList::UserStatus::eOrdinaryUser)
            << "� ������������ �� ������������� ������ ����� �����������";

        CString unknownMessageText = L"����������� �������. �������������� ������� ����:\n\n\n/info - �������� ������ ����.\n/report - ������������ �����.\n\n\n��� ���� ����� �� ������������ ���������� �������� �� ����� ����(����������� ������������ ���� ����� ������� �������(/)!). ��� ������ � ���� ����, ��� ������ ��������������.";

        // ��������� ��������� �������� ������������ �������
        for (const auto& command : m_commandsToUserStatus)
        {
            if (command.second.find(ITelegramUsersList::UserStatus::eOrdinaryUser) == command.second.end())
            {
                // ����������� �������
                expectedMessage = unknownMessageText;
            }
            else
            {
                // ��������� �������
                if (command.first == L"info")
                    expectedMessage = L"�������������� ������� ����:\n\n\n/info - �������� ������ ����.\n/report - ������������ �����.\n\n\n��� ���� ����� �� ������������ ���������� �������� �� ����� ����(����������� ������������ ���� ����� ������� �������(/)!). ��� ������ � ���� ����, ��� ������ ��������������.";
                else
                {
                    EXPECT_TRUE(command.first == L"report") << "����������� ����� �������: " << command.first;
                    expectedMessage = L"������ ��� ����������� �� �������";
                }
            }
            // ��������� ��������
            emulateBroadcastMessage(L"/" + command.first);
        }

        // ��������� ��������� �����
        expectedMessage = unknownMessageText;
        emulateBroadcastMessage(L"/123");
        emulateBroadcastMessage(L"123");
    }

    {
        // ���������� ������������ ��� ������ eAdmin
        expectedMessage = L"������������ ������� ����������� ��� �������������.";
        emulateBroadcastMessage(L"MonitoringAuthAdmin");
        EXPECT_EQ(m_pUserList->getUserStatus(nullptr), ITelegramUsersList::UserStatus::eAdmin)
            << "� ������������ �� ������������� ������ ����� �����������";

        // �������� ���������� ��������� ����������� �������� ������������
        expectedMessage = L"������������ �������� ��������������� �������. ����������� �� ���������.";
        emulateBroadcastMessage(L"MonitoringAuth");
        EXPECT_EQ(m_pUserList->getUserStatus(nullptr), ITelegramUsersList::UserStatus::eAdmin)
            << "� ������������ �� ������������� ������ ����� �����������";

        // �������� ���������� ��������� ����������� ������
        expectedMessage = L"������������ ��� ����������� ��� �������������.";
        emulateBroadcastMessage(L"MonitoringAuthAdmin");
        EXPECT_EQ(m_pUserList->getUserStatus(nullptr), ITelegramUsersList::UserStatus::eAdmin)
            << "� ������������ �� ������������� ������ ����� �����������";

        // ��������� ��������� �������� ������������ �������
        for (const auto& command : m_commandsToUserStatus)
        {
            EXPECT_TRUE(command.second.find(ITelegramUsersList::UserStatus::eAdmin) != command.second.end())
                << "� ������ ���� ����������� ��� ������� " << command.first;

            if (command.first == L"info")
                expectedMessage = L"�������������� ������� ����:\n\n\n/info - �������� ������ ����.\n/report - ������������ �����.\n/restart - ������������� ������� �����������.\n/allertionOn - �������� ���������� � ��������.\n/allertionOff - ��������� ���������� � ��������.\n\n\n��� ���� ����� �� ������������ ���������� �������� �� ����� ����(����������� ������������ ���� ����� ������� �������(/)!). ��� ������ � ���� ����, ��� ������ ��������������.";
            else if (command.first == L"allertionOn"     ||
                     command.first == L"allertionOff"    ||
                     command.first == L"report")
                expectedMessage = L"������ ��� ����������� �� �������";
            else
            {
                EXPECT_TRUE(command.first == L"restart") << "����������� ����� �������: " << command.first;
                expectedMessage = L"���� ��� ����������� �� ������.";
            }

            // ��������� ��������
            emulateBroadcastMessage(L"/" + command.first);
        }

        // ��������� �������� �� ��������� �����
        expectedMessage = L"����������� �������. �������������� ������� ����:\n\n\n/info - �������� ������ ����.\n/report - ������������ �����.\n/restart - ������������� ������� �����������.\n/allertionOn - �������� ���������� � ��������.\n/allertionOff - ��������� ���������� � ��������.\n\n\n��� ���� ����� �� ������������ ���������� �������� �� ����� ����(����������� ������������ ���� ����� ������� �������(/)!). ��� ������ � ���� ����, ��� ������ ��������������.";;
        emulateBroadcastMessage(L"/123");
        emulateBroadcastMessage(L"123");
    }
}

//----------------------------------------------------------------------------//
// ��������� ������ ������
TEST_F(TestTelegramBot, CheckReportCommandCallbacks)
{
    // ��������� ��������� �������� ����
    CString expectedMessage;
    // ����� � ����������
    TgBot::GenericReply::Ptr expectedReply = std::make_shared<TgBot::GenericReply>();

    // ����� ��� �������� ������� ������������
    TelegramUserMessagesChecker checker(m_pTelegramThread, &expectedMessage, &expectedReply);

    // ������� �������� ������ �������, ������ ������
    expectedMessage = L"������������ ������� ����������� ��� �������������.";
    emulateBroadcastMessage(L"MonitoringAuthAdmin");

    // ������ �������� ������� ����������� ����� � ���� ��������
    ITrendMonitoring* trendMonitoring = get_monitoing_service();
    std::set<CString> allChannels = trendMonitoring->getNamesOfAllChannels();
    for (auto& chan : allChannels)
    {
        size_t chanInd = trendMonitoring->addMonitoingChannel();
        trendMonitoring->changeMonitoingChannelName(chanInd, chan);
    }

    // ��������� ������������ �������
    // kKeyWord kParamType={'ReportType'} kParamChan={'chan1'}(�����������) kParamInterval={'1000000'}
    expectedMessage = L"�� ����� ������� ������������ �����?";
    TgBot::InlineKeyboardMarkup::Ptr expectedKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    expectedKeyboard->inlineKeyboard = { { generateKeyBoardButton(L"��� ������", L"/report reportType={\\\'0\\\'}") },
                                         { generateKeyBoardButton(L"������������ �����", L"/report reportType={\\\'1\\\'}") } };
    expectedReply = expectedKeyboard;
    emulateBroadcastMessage(L"/report");

    // ��������� ����� �� ���� �������
    {
        expectedMessage = L"�������� �������� ������� �� ������� ����� �������� �����";
        expectedKeyboard->inlineKeyboard.resize((size_t)MonitoringInterval::eLast);
        for (int i = (int)MonitoringInterval::eLast - 1; i >= 0; --i)
        {
            CString callBackText;
            callBackText.Format(L"/report reportType={\\\'0\\\'} interval={\\\'%d\\\'}", i);

            expectedKeyboard->inlineKeyboard[i] = { generateKeyBoardButton(monitoring_interval_to_string(MonitoringInterval(i)), callBackText) };
        }
        // ��������� ����� �� ���� �������
        // kKeyWord kParamType={'0'} kParamInterval={'1000000'}
        emulateBroadcastCallbackQuery(L"/report reportType={'0'}");

        expectedMessage = L"����������� ������ ������, ��� ����� ������ ��������� �����.";
        expectedReply = std::make_shared<TgBot::GenericReply>();
        // ��������� ��� ���������
        for (int i = (int)MonitoringInterval::eLast - 1; i >= 0; --i)
        {
            CString text;
            text.Format(L"/report reportType={'0'} interval={'%d'}", i);

            // kKeyWord kParamType={'ReportType'} kParamChan={'chan1'}(�����������) kParamInterval={'1000000'}
            emulateBroadcastCallbackQuery(CStringA(text).GetString());
        }
    }

    // ��������� ����� �� ���������� ������
    {
        expectedMessage = L"�������� �����";
        expectedKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
        expectedReply = expectedKeyboard;
        for (auto& chan : allChannels)
        {
            CString callBackText;
            callBackText.Format(L"/report reportType={\\\'1\\\'} chan={\\\'%s\\\'}", chan.GetString());

            expectedKeyboard->inlineKeyboard.push_back({ generateKeyBoardButton(chan, callBackText) });
        }
        // ��������� ����� �� ������������� ������
        emulateBroadcastCallbackQuery(L"/report reportType={'1'}");

        expectedKeyboard->inlineKeyboard.resize((size_t)MonitoringInterval::eLast);
        // ��������� ��� ��� ������� ������ ����� ����������� �����
        for (auto& chan : allChannels)
        {
            expectedMessage = L"�������� �������� ������� �� ������� ����� �������� �����";
            expectedReply = expectedKeyboard;

            // ��� ������ ��� ������� ��������� �����������
            for (int i = (int)MonitoringInterval::eLast - 1; i >= 0; --i)
            {
                CString callBackText;
                callBackText.Format(L"/report reportType={\\\'1\\\'} chan={\\\'%s\\\'} interval={\\\'%d\\\'}", chan.GetString(), i);

                expectedKeyboard->inlineKeyboard[i] = { generateKeyBoardButton(monitoring_interval_to_string(MonitoringInterval(i)), callBackText) };
            }

            CString text;
            text.Format(L"/report reportType={'1'} chan={'%s'}", chan.GetString());
            // ����������� ���� �� ������ chan
            emulateBroadcastCallbackQuery(text);

            // ����������� ������ ��� ������� ���������
            expectedMessage = L"����������� ������ ������, ��� ����� ������ ��������� �����.";
            expectedReply = std::make_shared<TgBot::GenericReply>();
            // ��������� ��� ���������
            for (int i = (int)MonitoringInterval::eLast - 1; i >= 0; --i)
            {
                CString text;
                text.Format(L"/report reportType={'1'} chan={'%s'} interval={'%d'}", chan.GetString(), i);
                emulateBroadcastCallbackQuery(text);
            }
        }
    }
}

//----------------------------------------------------------------------------//
// ��������� ������ �����������
TEST_F(TestTelegramBot, CheckRestartCommandCallbacks)
{
    // ��������� ��������� �������� ����
    CString expectedMessage;

    // ����� ��� �������� ������� ������������
    TelegramUserMessagesChecker checker(m_pTelegramThread, &expectedMessage);

    // ������� �������� ������ �������, ������ ������
    expectedMessage = L"������������ ������� ����������� ��� �������������.";
    emulateBroadcastMessage(L"MonitoringAuthAdmin");

    // ������� ������ ���� ������� ��� �������� ��������
    std::ofstream ofs(get_service<TestHelper>().getRestartFilePath());
    ofs.close();

    expectedMessage = L"���������� ������� ��������������.";
    emulateBroadcastCallbackQuery(L"/restart");
}

//----------------------------------------------------------------------------//
// ��������� ������ �����������
TEST_F(TestTelegramBot, CheckAllertCommandCallbacks)
{
    // ��������� ��������� �������� ����
    CString expectedMessage;
    // ����� � ����������
    TgBot::GenericReply::Ptr expectedReply = std::make_shared<TgBot::GenericReply>();

    // ����� ��� �������� ������� ������������
    TelegramUserMessagesChecker checker(m_pTelegramThread, &expectedMessage, &expectedReply);

    // ������� �������� ������ �������, ������ ������
    expectedMessage = L"������������ ������� ����������� ��� �������������.";
    emulateBroadcastMessage(L"MonitoringAuthAdmin");

    // ������ �������� ������� ����������� ����� �� ��������� ����������
    ITrendMonitoring* trendMonitoring = get_monitoing_service();
    std::set<CString> allChannels = trendMonitoring->getNamesOfAllChannels();
    for (auto& chan : allChannels)
    {
        size_t chanInd = trendMonitoring->addMonitoingChannel();
        trendMonitoring->changeMonitoingChannelName(chanInd, chan);
    }

    TgBot::InlineKeyboardMarkup::Ptr expectedKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    auto checkAllert = [&](bool allertOn)
    {
        expectedReply = expectedKeyboard;

        // ��������� ���������/���������� ����������
        // kKeyWord kParamEnable={'true'} kParamChan={'chan1'}
        expectedMessage.Format(L"�������� ����� ��� %s ����������.", allertOn ? L"���������" : L"����������");

        CString defCallBackText;
        defCallBackText.Format(L"/allert enable={\\\'%s\\\'}", allertOn ? L"true" : L"false");

        // ��������� ��� ������
        expectedKeyboard->inlineKeyboard.clear();
        for (auto& chan : allChannels)
        {
            CString callBack;
            callBack.Format(L"%s chan={\\\'%s\\\'}", defCallBackText.GetString(), chan.GetString());
            expectedKeyboard->inlineKeyboard.push_back({ generateKeyBoardButton(chan, callBack) });
        }
        // ��������� ������ ���� �������
        defCallBackText.Append(L" chan={\\\'allChannels\\\'}");
        expectedKeyboard->inlineKeyboard.push_back({ generateKeyBoardButton(L"��� ������", defCallBackText) });

        // ��������� ������� ���������/����������
        emulateBroadcastMessage(allertOn ? L"/allertionOn" : L"/allertionOff");

        expectedReply = std::make_shared<TgBot::GenericReply>();
        // ��������� ��� ������ ������ ��� ����
        for (size_t ind = 0, count = expectedKeyboard->inlineKeyboard.size(); ind < count; ++ind)
        {
            const std::string& callBackStr = expectedKeyboard->inlineKeyboard[ind].front()->callbackData;

            // ���� �������� ��� \\\' �� ������ '
            CString callBackData = std::regex_replace(getUNICODEString(callBackStr).GetString(),
                                                      std::wregex(LR"(\\')"),
                                                      L"'").c_str();

            // ��������� ��� ��� ��������� ������ �� ����� ��������
            if (ind == count - 1)
            {
                // ������ �������� ��������� � ����������
                for (size_t i = 0, count = allChannels.size(); i < count; ++i)
                {
                    trendMonitoring->changeMonitoingChannelNotify(i, !allertOn);
                }

                expectedMessage.Format(L"���������� ��� ���� ������� %s",
                                       allertOn ? L"��������" : L"���������");
                emulateBroadcastCallbackQuery(callBackData);

                // ��������� ��� ��������� ���������� ����������
                for (size_t i = 0, count = allChannels.size(); i < count; ++i)
                {
                    EXPECT_EQ(trendMonitoring->getMonitoringChannelData(i).bNotify, allertOn)
                        << "����� ���������� ���������� ������������ ��������� ���������� ����������!";
                }
            }
            else
            {
                // ������ �������� ���������������� ��������� ���������� � ������
                trendMonitoring->changeMonitoingChannelNotify(ind, !allertOn);

                expectedMessage.Format(L"���������� ��� ������ %s %s",
                                       std::next(allChannels.begin(), ind)->GetString(),
                                       allertOn ? L"��������" : L"���������");
                emulateBroadcastCallbackQuery(callBackData);

                // ��������� ��� ��������� ���������� ����������
                EXPECT_EQ(trendMonitoring->getMonitoringChannelData(ind).bNotify, allertOn)
                    << "����� ���������� ���������� ������������ ��������� ���������� ����������!";
            }
        }
    };

    // ��������� ��������� ����������
    checkAllert(true);

    // ��������� ���������� ����������
    checkAllert(false);
}

//----------------------------------------------------------------------------//
// ��������� ������ �����������
TEST_F(TestTelegramBot, TestProcessingMonitoringError)
{
    const CString errorMsg = L"�������� ��������� 111235�������1��asd 41234%$#@$$%6sfda";

    // ��������� ��������� �������� ����, �������� ��������� �� ������
    CString expectedMessage = errorMsg;
    CString expectedMessageToReciepients = errorMsg;

    // ����� � ����������
    TgBot::GenericReply::Ptr expectedReply = std::make_shared<TgBot::GenericReply>();
    // ��������� ������ �����������
    std::list<int64_t>* expectedReciepientsChats = nullptr;

    // ����� ��� �������� ������� ������������
    TelegramUserMessagesChecker checker(m_pTelegramThread, &expectedMessage, &expectedReply, &expectedReciepientsChats, &expectedMessageToReciepients);

    // �������� ����� ������� ������������� � �������
    std::list<int64_t> userChats;
    std::list<int64_t> adminChats;
    // ������������ ������ ����� � ��������
    {
        auto registerChat = [&](const int64_t chatId, const ITelegramUsersList::UserStatus userStatus)
        {
            switch (userStatus)
            {
            case ITelegramUsersList::UserStatus::eAdmin:
                adminChats.push_back(chatId);
                break;
            case ITelegramUsersList::UserStatus::eOrdinaryUser:
                userChats.push_back(chatId);
                break;
            default:
                ASSERT_FALSE("����������� ������������ � �� ������������ ��������");
                break;
            }

            m_pUserList->addUserChatidStatus(chatId, userStatus);
        };

        for (int i = 0; i < 5; ++i)
        {
            registerChat(std::rand() % 15, ITelegramUsersList::UserStatus::eAdmin);
            registerChat(std::rand() % 15, ITelegramUsersList::UserStatus::eOrdinaryUser);
        }

        // ������������� ������ ���� ������������� ������ ������ ��� �����
        registerChat(std::rand() % 15, ITelegramUsersList::UserStatus::eOrdinaryUser);
        registerChat(std::rand() % 15, ITelegramUsersList::UserStatus::eOrdinaryUser);
    }

    // ������� �������� ������
    auto errorMessage = std::make_shared<MonitoringErrorEventData>();
    errorMessage->errorText = errorMsg;
    // ������� ������������� ������
    if (!SUCCEEDED(CoCreateGuid(&errorMessage->errorGUID)))
        assert(!"�� ������� ������� ����!");

    // ���������� ��������� ������������
    TgBot::InlineKeyboardMarkup::Ptr expectedKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    expectedKeyboard->inlineKeyboard.push_back({ generateKeyBoardButton(L"������������� �������", L"/restart"),
                                                 generateKeyBoardButton(L"���������� ������� �������������",
                                                                        L"/resend errorId={\\\'" + CString(CComBSTR(errorMessage->errorGUID)) + L"\\\'}") });

    expectedReply = expectedKeyboard;
    expectedReciepientsChats = &adminChats;

    // ��������� ������������� ������
    get_service<CMassages>().sendMessage(onMonitoringErrorEvent, 0,
                                         std::static_pointer_cast<IEventData>(errorMessage));

    // ������ ��� �������
    m_pUserList->setUserStatus(nullptr, ITelegramUsersList::UserStatus::eAdmin);

    // ��������� �������

    // ������� ������ ���� ������� ��� �������� ��������
    std::ofstream ofs(get_service<TestHelper>().getRestartFilePath());
    ofs.close();

    expectedReply = std::make_shared<TgBot::GenericReply>();
    expectedMessage = L"���������� ������� ��������������.";
    emulateBroadcastCallbackQuery(L"/restart");

    // ��������� ��������� ���������
    expectedReciepientsChats = &userChats;

    // ���������� ������ ���������
    expectedMessage = L"������������ ������ ��� � ������, �������� ������ �������� ���������� (�������� ��������� 200 ������) ��� ��������� ���� ������������.";
    GUID testGuid = { 123 };
    emulateBroadcastCallbackQuery(L"/resend errorId={'" + CString(CComBSTR(testGuid)) + L"'}");

    // ���������� ��������
    expectedMessage = L"������ ���� ������� ��������� ������� �������������.";
    expectedMessageToReciepients = errorMsg;
    emulateBroadcastCallbackQuery(L"/resend errorId={'" + CString(CComBSTR(errorMessage->errorGUID)) + L"'}");

    // ��������� �� ������� ���������
    expectedMessage = L"������ ��� ���� ���������.";
    emulateBroadcastCallbackQuery(L"/resend errorId={'" + CString(CComBSTR(errorMessage->errorGUID)) + L"'}");
}

////////////////////////////////////////////////////////////////////////////////
void TestTelegramBot::SetUp()
{
    // �������������� ������ �������������
    m_pUserList = TestTelegramUsersList::create();;

    m_pTelegramThread = new TestTelegramThread();

    // ������� �������� ����� ���������� � ���������� ��� ���������
    ITelegramThreadPtr pTelegramThread(m_pTelegramThread);
    // ������� ��������� ����
    TelegramBotSettings botSettings;
    botSettings.bEnable = true;
    botSettings.sToken = L"Testing";

    // �������������� ������ �������������
    m_testTelegramBot.initBot(m_pUserList);
    // ������������� ��������� ������ ��� ��������� ������ ���������, ��������� ��� � ��� unique_ptr
    m_testTelegramBot.setDefaultTelegramThread(pTelegramThread);
    // ������� ���������
    m_testTelegramBot.setBotSettings(botSettings);

    // ��������� �������� ������ � ����������� � ��� ��������� �������������
    m_commandsToUserStatus["info"]         = { ITelegramUsersList::eAdmin, ITelegramUsersList::eOrdinaryUser };
    m_commandsToUserStatus["report"]       = { ITelegramUsersList::eAdmin, ITelegramUsersList::eOrdinaryUser };
    m_commandsToUserStatus["restart"]      = { ITelegramUsersList::eAdmin };
    m_commandsToUserStatus["allertionOn"]  = { ITelegramUsersList::eAdmin };
    m_commandsToUserStatus["allertionOff"] = { ITelegramUsersList::eAdmin };

    static_assert(ITelegramUsersList::eLastStatus == 3,
                  "������ ����������� ���������, �������� ����� ������������ ����������� ������");

    get_service<TestHelper>().resetMonitoringService();
}

//----------------------------------------------------------------------------//
void TestTelegramBot::emulateBroadcastMessage(const CString& text) const
{
    TgBot::Update::Ptr pUpdate = std::make_shared<TgBot::Update>();
    pUpdate->message = generateMessage(text);

    HandleTgUpdate(m_pTelegramThread->m_botApi->getEventHandler(), pUpdate);
}

//----------------------------------------------------------------------------//
void TestTelegramBot::emulateBroadcastCallbackQuery(const CString& text) const
{
    TgBot::Message::Ptr pMessage = generateMessage(text);

    TgBot::Update::Ptr pUpdate = std::make_shared<TgBot::Update>();
    pUpdate->callbackQuery = std::make_shared<TgBot::CallbackQuery>();
    pUpdate->callbackQuery->from = pMessage->from;
    pUpdate->callbackQuery->message = pMessage;
    pUpdate->callbackQuery->data = getUtf8Str(text);

    HandleTgUpdate(m_pTelegramThread->m_botApi->getEventHandler(), pUpdate);
}

//----------------------------------------------------------------------------//
TgBot::Message::Ptr TestTelegramBot::generateMessage(const CString& text) const
{
    TgBot::Message::Ptr pMessage = std::make_shared<TgBot::Message>();
    pMessage->from = std::make_shared<TgBot::User>();
    pMessage->from->id = kTelegramTestChatId;
    pMessage->from->firstName = "Bot";
    pMessage->from->lastName = "Test";
    pMessage->from->username = "TestBot";
    pMessage->from->languageCode = "Ru";

    pMessage->chat = std::make_shared<TgBot::Chat>();
    pMessage->chat->id = kTelegramTestChatId;
    pMessage->chat->type = TgBot::Chat::Type::Private;
    pMessage->chat->title = "Test conversation";
    pMessage->chat->firstName = "Bot";
    pMessage->chat->lastName = "Test";
    pMessage->chat->username = "TestBot";

    pMessage->text = getUtf8Str(text);

    return pMessage;
}

////////////////////////////////////////////////////////////////////////////////
TelegramUserMessagesChecker::TelegramUserMessagesChecker(TestTelegramThread* pTelegramThread,
                                                         CString* pExpectedMessage,
                                                         TgBot::GenericReply::Ptr* pExpectedReply /*= nullptr*/,
                                                         std::list<int64_t>** pExpectedReciepientsChats /*= nullptr*/,
                                                         CString* pExpectedMessageToReciepients /*= nullptr*/)
{
    // ��������� ���� �������(��������� ������������)
    auto compareReply = [](const TgBot::GenericReply::Ptr first,
                           const TgBot::GenericReply::Ptr second) -> bool
    {
        if (first == second)
            return true;

        if (!first || !second)
            return false;

        TgBot::InlineKeyboardMarkup::Ptr keyBoardMarkupFirst =
            std::dynamic_pointer_cast<TgBot::InlineKeyboardMarkup>(first);
        TgBot::InlineKeyboardMarkup::Ptr keyBoardMarkupSecond =
            std::dynamic_pointer_cast<TgBot::InlineKeyboardMarkup>(second);

        if ((bool)keyBoardMarkupFirst != (bool)keyBoardMarkupSecond)
        {
            EXPECT_FALSE("���� ������� �� ������������� ���� �����.");
            return false;
        }

        if (keyBoardMarkupFirst && keyBoardMarkupSecond)
        {
            // ���������� ��� InlineKeyboardMarkup
            std::vector<std::vector<TgBot::InlineKeyboardButton::Ptr>> keyboardFirst =
                keyBoardMarkupFirst->inlineKeyboard;
            std::vector<std::vector<TgBot::InlineKeyboardButton::Ptr>> keyboardSecond =
                keyBoardMarkupSecond->inlineKeyboard;

            if (keyboardFirst.size() != keyboardSecond.size())
            {
                EXPECT_FALSE("���������� ��� ������ �����������");
                return false;
            }

            for (size_t row = 0, countRows = keyboardFirst.size(); row < countRows; ++row)
            {
                if (keyboardFirst[row].size() != keyboardSecond[row].size())
                {
                    EXPECT_FALSE("���������� ��� ������ �����������");
                    return false;
                }

                for (size_t col = 0, countColumns = keyboardFirst[row].size(); col < countColumns; ++col)
                {
                    TgBot::InlineKeyboardButton::Ptr firstKey = keyboardFirst[row][col];
                    TgBot::InlineKeyboardButton::Ptr secondKey = keyboardSecond[row][col];

                    // ���������� ������
                    if (firstKey->text != secondKey->text)
                    {
                        EXPECT_FALSE("���������� ��� ������ �����������");
                        return false;
                    }

                    boost::trim(firstKey->callbackData);
                    boost::trim(secondKey->callbackData);
                    if (firstKey->callbackData != secondKey->callbackData)
                    {
                        EXPECT_FALSE("���������� ��� ������ �����������");
                        return false;
                    }
                }
            }

            return true;
        }

        return true;
    };

    pTelegramThread->onSendMessage(
        [pExpectedMessage, pExpectedReply, &compareReply]
        (int64_t chatId, const CString& msg, bool disableWebPagePreview,
         int32_t replyToMessageId, TgBot::GenericReply::Ptr replyMarkup,
         const std::string& parseMode, bool disableNotification)
        {
            ASSERT_TRUE(pExpectedMessage) << "�� �������� ���������.";

            EXPECT_EQ(chatId, kTelegramTestChatId) << "������������� ���� � ����������� ��������� �� ���������";
            EXPECT_TRUE(msg == *pExpectedMessage) << "������ ��������� ������������ �� ����������: " + CStringA(msg);

            if (pExpectedReply)
                compareReply(replyMarkup, *pExpectedReply);
        });

    pTelegramThread->onSendMessageToChats(
        [pExpectedReciepientsChats, pExpectedReply, pExpectedMessageToReciepients, &compareReply]
         (const std::list<int64_t>& chatIds, const CString& msg, bool disableWebPagePreview,
         int32_t replyToMessageId, TgBot::GenericReply::Ptr replyMarkup,
         const std::string& parseMode, bool disableNotification)
        {
            ASSERT_TRUE(pExpectedReciepientsChats) << "�� �������� ������ �����.";
            ASSERT_TRUE(pExpectedMessageToReciepients) << "�� �������� ��������� ��� �����������.";

            EXPECT_EQ(chatIds.size(), (*pExpectedReciepientsChats)->size()) << "��������� ���� � ����������� �� �������.";
            auto chatId = chatIds.begin();
            auto expectedId = (*pExpectedReciepientsChats)->begin();
            for (size_t i = 0, count = std::min<size_t>(chatIds.size(), (*pExpectedReciepientsChats)->size());
                 i < count; ++i, ++chatId, ++expectedId)
            {
                EXPECT_EQ(*chatId, *expectedId) << "��������� ���� � ����������� �� �������.";;
            }

            EXPECT_TRUE(msg == *pExpectedMessageToReciepients) << "������ ��������� ������������ �� ����������: " + CStringA(msg);

            if (pExpectedReply)
                compareReply(replyMarkup, *pExpectedReply);
        });
}