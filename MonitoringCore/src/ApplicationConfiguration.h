#pragma once

#include <list>
#include <memory>
#include <optional>

#include <COM.h>
#include "Serialization/ISerializable.h"

#include "ITrendMonitoring.h"
#include "ITelegramUsersList.h"

////////////////////////////////////////////////////////////////////////////////
// ����� � ����������� ������ ��� �����������
class ATL_NO_VTABLE ChannelParameters
    : public COMInternalClass<ChannelParameters>
    , public SerializableObjectsCollection
    , public MonitoringChannelData
{
public:
    ChannelParameters() = default;
    ChannelParameters(const CString& initChannelName);

    // ��������� ������������� ��������� � ��������
    BEGIN_SERIALIZABLE_PROPERTIES(ChannelParameters)
        ADD_PROPERTY(channelName)
        ADD_PROPERTY(bNotify)
        ADD_PROPERTY(monitoringInterval)
        ADD_PROPERTY(allarmingValue)
    END_SERIALIZABLE_PROPERTIES();

public:
    /// <summary>�������� ������ �� ������.</summary>
    const MonitoringChannelData& getMonitoringData();
    /// <summary>�������� ������ �� ������.</summary>
    void setTrendChannelData(const TrendChannelData& data);
    /// <summary>�������� ��� ������.</summary>
    /// <param name="newName">����� ��������.</param>
    /// <returns>������ � ������ ��������� ��������.</returns>
    bool changeName(const CString& newName);
    /// <summary>�������� ���� ����������� � ��������� ������.</summary>
    /// <param name="state">����� ���������.</param>
    /// <returns>������ � ������ ��������� ��������.</returns>
    bool changeNotification(const bool state);
    /// <summary>�������� �������� �� ������.</summary>
    /// <param name="newInterval">����� �������� ���������.</param>
    /// <returns>������ � ������ ��������� ��������.</returns>
    bool changeInterval(const MonitoringInterval newInterval);
    /// <summary>�������� ����������� �������� �� ������.</summary>
    /// <param name="newvalue">����� ��������.</param>
    /// <returns>������ � ������ ��������� ��������.</returns>
    bool changeAllarmingValue(const float newvalue);
    /// <summary>�������� ����������� ������ �� ������.</summary>
    void resetChannelData();

public:
    // ����� ��������� ��������� �� �������� ���� ��������� ���������
    std::optional<CTime> m_loadingParametersIntervalEnd;
};

// ������ ������� � � �����������
typedef std::list<ChannelParameters::Ptr> ChannelParametersList;
// �������� �� �����
typedef std::list<ChannelParameters::Ptr>::iterator ChannelIt;

////////////////////////////////////////////////////////////////////////////////
// ����� � ������� ������������ ���������
class ATL_NO_VTABLE TelegramUser
    : public COMInternalClass<TelegramUser>
    , public SerializableObjectsCollection
    , public TgBot::User
{
public:
    // ����� �������������� ��� Ptr ��� ���� � TgBot::User � COMInternalClass
    typedef ComPtr<TelegramUser> Ptr;

    TelegramUser() = default;
    TelegramUser(const TgBot::User& pUser);

    // ��������� ������������� ��������� � ��������
    BEGIN_SERIALIZABLE_PROPERTIES(TelegramUser)
        ADD_PROPERTY(m_chatId)
        ADD_PROPERTY(m_userStatus)
        ADD_PROPERTY(m_userNote)
        // ��������� �� TgBot::User
        ADD_PROPERTY(id)                // ������������� ������������
        ADD_PROPERTY(isBot)
        ADD_PROPERTY(firstName)
        ADD_PROPERTY(lastName)
        ADD_PROPERTY(username)
        ADD_PROPERTY(languageCode)
    END_SERIALIZABLE_PROPERTIES();

    TelegramUser& operator=(const TgBot::User& pUser);
    bool operator!=(const TgBot::User& pUser) const;
    // TODO C++20
    //auto operator<=>(const TgBot::User::Ptr&) const = default;

public:
    // ������������� ���� � �������������
    int64_t m_chatId;
    // ������ ������������
    ITelegramUsersList::UserStatus m_userStatus = ITelegramUsersList::UserStatus::eNotAuthorized;
    // ������� � ������������, ���� �� ������������
    // � ���������������� ����� ����� ����� ���� ��� �������
    CString m_userNote;
};

////////////////////////////////////////////////////////////////////////////////
// ����� �������� ������ ������������� ���������
// ��������� �������� �� ������� ������������� �� ���������� �������
class ATL_NO_VTABLE TelegramUsersList
    : public COMInternalClass<TelegramUsersList>
    , public SerializableObjectsCollection
    , public ITelegramUsersList
{
public:
    TelegramUsersList() = default;

    BEGIN_COM_MAP(TelegramUsersList)
        COM_INTERFACE_ENTRY(ITelegramUsersList)
        COM_INTERFACE_ENTRY_CHAIN(SerializableObjectsCollection)
    END_COM_MAP()

// ISerializableCollection
public:
    /// <summary>��������� � ������ ������������ ���������.</summary>
    /// <returns>true ���� ��������� ����� �������������.</returns>
    virtual bool onStartSerializing() override;
    /// <summary>��������� �� ��������� ������������ ���������.</summary>
    virtual void onEndSerializing() override;

    /// <summary>��������� � ������ ��������������, �������� ������ ����������� ��� ������������ ���� ��������.</summary>
    /// <param name="objNames">����� �������� ������� ���� ��������� ��� ������������.</param>
    /// <returns>true ���� ��������� ����� ���������������.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) override;
    /// <summary>��������� � ����� �������������� ���������.</summary>
    virtual void onEndDeserializing() override;

// ITelegramUsersList
public:
    // ��������� ��� ������������ ����������
    void ensureExist(const TgBot::User::Ptr& pUser, const int64_t chatId) override;
    // ��������� ������� ������������
    UserStatus getUserStatus(const TgBot::User::Ptr& pUser) override;
    // ��������� ������� ������������
    void setUserStatus(const TgBot::User::Ptr& pUser, const UserStatus newStatus) override;
    // �������� ��� �������������� ����� ������������� � ������������ ��������
    std::list<int64_t> getAllChatIdsByStatus(const UserStatus userStatus) override;

private:
    // ��������� �������� �������� �� ������������
    std::list<TelegramUser::Ptr>::iterator getUserIterator(const TgBot::User::Ptr& pUser);
    // �������� �������� �� ������������ ��� �������, ���� ��� ���
    std::list<TelegramUser::Ptr>::iterator getOrCreateUsertIterator(const TgBot::User::Ptr& pUser);
    // ���������� ��� ��������� ������ � ������ �������������
    void onUsersListChanged();

private:
    DECLARE_COLLECTION(TelegramUsersList);
    // ������ ������������� ���������
    DECLARE_CONTAINER((std::list<TelegramUser::Ptr>) m_telegramUsers);

    // ������� �� ���������, ��������� ���������� � ��� �� ������ �������
    std::mutex m_usersMutex;
};

////////////////////////////////////////////////////////////////////////////////
// ����� � ����������� ������ ��� �����������
class ATL_NO_VTABLE TelegramParameters
    : public COMInternalClass<TelegramParameters>
    , public SerializableObjectsCollection
    , public TelegramBotSettings
{
public:
    TelegramParameters() = default;

    // ��������� ������������� ��������� � ��������
    BEGIN_SERIALIZABLE_PROPERTIES(TelegramParameters)
        ADD_PROPERTY(bEnable)
        ADD_PROPERTY(sToken)
        ADD_SERIALIZABLE(m_telegramUsers)
    END_SERIALIZABLE_PROPERTIES();

    TelegramParameters& operator=(const TelegramBotSettings& botSettings);

public:
    // ������ ������������� ���������
    TelegramUsersList::Ptr m_telegramUsers = TelegramUsersList::create();
};

////////////////////////////////////////////////////////////////////////////////
/*
    �����, �������� � ���� ��������� ����������

    ������:

    1. ���������� � �������
        - ��������� ������ ��� ����������� � ��������� ��������� �����������

    2. ���������� � ������������� �������� ����
        - ������ ������������ � ��� ������
*/
class ATL_NO_VTABLE ApplicationConfiguration
    : public COMInternalClass<ApplicationConfiguration>
    , public SerializableObjectsCollection
{
public:
    ApplicationConfiguration() = default;

    // ��������� ������ ������������� ���������
    ITelegramUsersListPtr getTelegramUsers();
    // ��������� �������� �������� ����
    const TelegramBotSettings& getTelegramSettings();
    // ��������� �������� �������� ����
    void setTelegramSettings(const TelegramBotSettings& newSettings);

public:
    // ��������� ������������� ���������
    DECLARE_COLLECTION(ApplicationConfiguration);

    // ������ �������� ��� ������� ������
    DECLARE_CONTAINER((ChannelParametersList) m_chanelParameters);

    // ��������� �������� ����
    DECLARE_SERIALIZABLE((TelegramParameters::Ptr) m_telegramParameters,
                         TelegramParameters::create());
};
