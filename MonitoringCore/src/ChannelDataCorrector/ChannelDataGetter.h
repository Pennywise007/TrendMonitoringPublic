#pragma once

#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "ChannelsHelpHeader.h"

////////////////////////////////////////////////////////////////////////////////
// класс получения данных по определенному каналу за заданный промежуток времени
class CChannelDataGetter
{
public:
    CChannelDataGetter(const CString& channelName,
                       const CTime& begin, const CTime& end);
    virtual ~CChannelDataGetter() = default;

    // получить данные по каналу
    void getSourceChannelData(const CString& channelName,
                              const CTime& begin, const CTime& end,
                              std::vector<float>& data);

    // удаляем выбросы из данных
    void deleteEmissionsValuesFromData(std::vector<float>& data);

    // запонение списка каналов
    static void FillChannelList(const CString& sDirectory,
                                std::map<CString, CString>& channelList);

    std::shared_ptr<ChannelInfo> getChannelInfo();

protected:
    // функция удаления неподходящих под колбэк данных
    typedef std::function<bool(const float& value)> Callback;
    float deleteInappropriateValues(std::vector<float>& datavect, Callback&& callback);

private:
    // получение данных
    void calcChannelInfo(ChannelInfo* chanInfo, CString channelName,
                         const CTime& begin, const CTime& end);

    // получение информации о канале
    bool getChannelInfo(const CString& xmlFileName);
    // получаем информацию о файлах
    ChannelFromFile getFilesInfo(const CString& channelName, const CString& xmlFileName);

    // получаем информацию о файлах которые необходимо переписать
    ChannelsFromFileByYear GetSourceFilesInfoByInterval(const CString& channelName,
                                                        const CTime& Begin,
                                                        const CTime& End);

    // плучить исходные данные канала на заданном интервале
    void GetSourceChannelDataByInterval(const CString& channelName,
                                        const CTime& tStart, const CTime& tEnd,
                                        long uiLength, float* pData, double freq);
private:
    // получение директорий
    CString getSourceFilesDir();
    CString getZetCompressedDir();

private:
    std::shared_ptr<ChannelInfo> m_channelInfo;
};
