#pragma once

#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "ChannelsHelpHeader.h"

////////////////////////////////////////////////////////////////////////////////
// ����� ��������� ������ �� ������������� ������ �� �������� ���������� �������
class CChannelDataGetter
{
public:
    CChannelDataGetter(const CString& channelName,
                       const CTime& begin, const CTime& end);
    virtual ~CChannelDataGetter() = default;

    // �������� ������ �� ������
    void getSourceChannelData(const CString& channelName,
                              const CTime& begin, const CTime& end,
                              std::vector<float>& data);

    // ������� ������� �� ������
    void deleteEmissionsValuesFromData(std::vector<float>& data);

    // ��������� ������ �������
    static void FillChannelList(const CString& sDirectory,
                                std::map<CString, CString>& channelList);

    std::shared_ptr<ChannelInfo> getChannelInfo();

protected:
    // ������� �������� ������������ ��� ������ ������
    typedef std::function<bool(const float& value)> Callback;
    float deleteInappropriateValues(std::vector<float>& datavect, Callback&& callback);

private:
    // ��������� ������
    void calcChannelInfo(ChannelInfo* chanInfo, CString channelName,
                         const CTime& begin, const CTime& end);

    // ��������� ���������� � ������
    bool getChannelInfo(const CString& xmlFileName);
    // �������� ���������� � ������
    ChannelFromFile getFilesInfo(const CString& channelName, const CString& xmlFileName);

    // �������� ���������� � ������ ������� ���������� ����������
    ChannelsFromFileByYear GetSourceFilesInfoByInterval(const CString& channelName,
                                                        const CTime& Begin,
                                                        const CTime& End);

    // ������� �������� ������ ������ �� �������� ���������
    void GetSourceChannelDataByInterval(const CString& channelName,
                                        const CTime& tStart, const CTime& tEnd,
                                        long uiLength, float* pData, double freq);
private:
    // ��������� ����������
    CString getSourceFilesDir();
    CString getZetCompressedDir();

private:
    std::shared_ptr<ChannelInfo> m_channelInfo;
};
