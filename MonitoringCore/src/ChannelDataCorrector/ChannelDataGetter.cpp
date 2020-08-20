#include "pch.h"

#include <afx.h>
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#include "ChannelDataGetter.h"

#include "DirsService.h"

#include "pugixml.hpp"

using namespace pugi;

//****************************************************************************//
CChannelDataGetter::CChannelDataGetter(const CString& channelName,
                                       const CTime& begin, const CTime& end)
    : m_channelInfo(new ChannelInfo())
{
    m_channelInfo->SetName(channelName);
    calcChannelInfo(m_channelInfo.get(), channelName, begin, end);
}

//----------------------------------------------------------------------------//
void CChannelDataGetter::getSourceChannelData(const CString& channelName,
                                              const CTime& timeStart, const CTime& timeEnd,
                                              std::vector<float>& data)
{
    // получаем инфу по каналу
    ChannelInfo chanInfo;
    calcChannelInfo(&chanInfo, channelName, timeStart, timeEnd);

    // вычисляем сколько надо данных
    CTimeSpan tsDelta = timeEnd - timeStart;
    double freq = m_channelInfo->GetFrequency();

    int dataSize = int(tsDelta.GetTotalSeconds() * freq);

    if (tsDelta.GetTotalSeconds() <= 0)
        throw CString(L"Неверный временной интервал для загрузки исходных данных " + channelName);

    data.resize(dataSize);
    // заполним все нулями после ресайза потому что буфер мог уже использоваться и если в данных пропуски
    // то данные будут невалидны
    std::fill(data.begin(), data.end(), 0.f);

    GetSourceChannelDataByInterval(channelName, timeStart, timeEnd,
                                   dataSize, data.data(), freq);

    // заменяем наны на нули
    std::replace_if(data.begin(), data.end(),
                    [](auto val)
                    {
                        return _finite(val) == 0;
                    },
                    0.f);
}

//----------------------------------------------------------------------------//
void CChannelDataGetter::deleteEmissionsValuesFromData(std::vector<float>& data)
{
    float averageVal(0);
    // удаляем нули и наны
    averageVal = deleteInappropriateValues(data, [](const float& value) mutable
                                           {
                                               return abs(value) <= FLT_MIN;
                                           });

    size_t dataSize = data.size();
    if (dataSize > 10)
    {
        // https://ru.m.wikihow.com/%D0%B2%D1%8B%D1%87%D0%B8%D1%81%D0%BB%D0%B8%D1%82%D1%8C-%D0%B2%D1%8B%D0%B1%D1%80%D0%BE%D1%81%D1%8B

        //копируем полученные данные, т.к юужем их сортировать
        std::vector<float> copyDataVect = data;
        // соритруем
        std::sort(copyDataVect.begin(), copyDataVect.end());

        // функция вычисления медианы
        auto calcMedianFunc = [&copyDataVect](size_t startIndex, size_t endIndex) -> float
        {
            double middleIndex = double(endIndex + startIndex) / 2.;

            return (copyDataVect[(size_t)floor(middleIndex)] +
                    copyDataVect[(size_t)ceil(middleIndex)]) / 2.f;
        };

        size_t endIndex = dataSize - 1;

        // центральная медиана
        float centerMedian = calcMedianFunc(0, endIndex);

        // вычисляем нижний квартиль
        float startMedianQ1;
        {
            size_t currentIndex = (size_t)floor(double(endIndex) / 2.);
            do
            {
                startMedianQ1 = calcMedianFunc(0, currentIndex);

                // Если квартиль совпал с медианой - значит надо делать более узкую выборку
                // бьем повторно пока выборка не "обмелеет" или не будет отличаться от медианы
                currentIndex /= 2;
            } while (startMedianQ1 == centerMedian && currentIndex > 10);
        }

        // вычисляем верхний квартиль
        float endMedianQ3;
        {
            size_t currentIndex = (size_t)ceil(double(endIndex) / 2.);
            do
            {
                endMedianQ3 = calcMedianFunc(endIndex - currentIndex, endIndex);

                // Если квартиль совпал с медианой - значит надо делать более узкую выборку
                // бьем повторно пока выборка не "обмелеет" или не будет отличаться от медианы
                currentIndex /= 2;
            } while (endMedianQ3 == centerMedian && currentIndex > 10);
        }

        // межквартильный диапазон
        float diapazon = endMedianQ3 - startMedianQ1;

        // поправляем диапазон на 1.5
        diapazon *= 1.5f;

        float validIntervalStart = startMedianQ1 - diapazon;
        float validIntervalEnd = endMedianQ3 + diapazon;

        // удаляем данные не в нашем диапазоне
        deleteInappropriateValues(data,
                                  [validIntervalStart, validIntervalEnd](const float& value) mutable
                                  {
                                      return value < validIntervalStart ||
                                          value > validIntervalEnd;
                                  });
    }
    else
    {
        // удаляем выделяющиеся данные
        deleteInappropriateValues(data, [&averageVal](const float& value) mutable
                                  {
                                      return abs(value - averageVal) > 30;
                                  });
    }
}

//----------------------------------------------------------------------------//
void CChannelDataGetter::FillChannelList(const CString& sDirectory,
                                         std::map<CString, CString>& channelList)
{
    WIN32_FIND_DATA win32_find_data;
    WCHAR wcDirectory[MAX_PATH];
    wcscpy_s(wcDirectory, MAX_PATH, sDirectory);
    wcscat_s(wcDirectory, MAX_PATH, L"*.*");

    HANDLE hFind = FindFirstFile(wcDirectory, &win32_find_data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            CString sFileName(CString(win32_find_data.cFileName));
            if (!(win32_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (sFileName.Find(L"sig0000.xml") != -1)
                {
                    sFileName = sDirectory + sFileName;
                    xml_document hFile;
                    if (hFile.load_file(sFileName, parse_default, encoding_utf8))
                    {
                        if (xml_node hRoot = hFile.child(L"CommonSignalDescriptor"))
                        {
                            xml_object_range<xml_named_node_iterator> signals = hRoot.children(L"Signal");
                            for (auto itSignal = signals.begin(), end = signals.end(); itSignal != end; ++itSignal)
                            {
                                xml_attribute nameAtr = itSignal->attribute(L"name");
                                xml_attribute conversionAtr = itSignal->attribute(L"conversion");

                                if (nameAtr && conversionAtr &&
                                    wcslen(nameAtr.value()) != 0 &&
                                    wcslen(conversionAtr.value()) != 0)
                                    channelList.try_emplace(nameAtr.value(), conversionAtr.value());
                            }

                            break;
                        }
                    }
                }
            }
            else if (sFileName != L"." && sFileName != L"..")
            {
                FillChannelList(sDirectory + sFileName + L"\\", channelList);
            }
        } while (FindNextFile(hFind, &win32_find_data));
        FindClose(hFind);
    }
}

float CChannelDataGetter::deleteInappropriateValues(std::vector<float>& datavect, Callback&& callback)
{
    datavect.erase(std::remove_if(datavect.begin(), datavect.end(), [&callback](auto val)
                                  {
                                      return callback(val);
                                  }), datavect.end());

    return std::accumulate(datavect.begin(), datavect.end(), 0.f) /
        (datavect.empty() ? 1 : datavect.size());
}

//----------------------------------------------------------------------------//
void CChannelDataGetter::calcChannelInfo(ChannelInfo* chanInfo, CString channelName,
                                         const CTime& begin, const CTime& end)
{
    CTime tBegin = CTime(begin.GetYear(), begin.GetMonth(), begin.GetDay(), begin.GetHour(), 0, 0);
    int offset = end.GetMinute() * 60 + end.GetSecond();
    CTime tEnd = offset > 0 ? (end - CTimeSpan(offset - 3600)) : end;

    // флаг что удалось вытащить информацию о канале
    bool bChannelInfoExist(false);

    // директория с сигналами
    CString sourceFilesDir = getSourceFilesDir();

    // имя файла
    CString sFileName(L"");

    CTime tTimeNext = tBegin;
    while (tTimeNext < tEnd)
    {
        sFileName.Format(L"%s%d\\%02d\\%02d\\%02d\\sig0000.xml", sourceFilesDir.GetString(), tTimeNext.GetYear(), tTimeNext.GetMonth(), tTimeNext.GetDay(), tTimeNext.GetHour());

        // пытаемся вытащить информацию о канале
        if (getChannelInfo(sFileName))
            return;

        tTimeNext += 3600;
    }

    // если не нашли информацию о канале вытаскиваем ее из файла с информацией
    if (!bChannelInfoExist)
    {
        CString reserveFileInfo = get_service<DirsService>().getExeDir() + L"sig0000.xml";
        if (!getChannelInfo(reserveFileInfo))
        {
            CString logMessage;
            logMessage.Format(L"Не удалось вытащить информацию о канале, нету исходных файлов и не подошел резервный файл, имя канала: %s\n\
Резевный файл(нужен sig0000.xml с названием канала): %s", m_channelInfo->GetName().GetString(), reserveFileInfo.GetString());

            throw logMessage;
        }
    }
}

//----------------------------------------------------------------------------//
ChannelsFromFileByYear CChannelDataGetter::GetSourceFilesInfoByInterval(const CString& channelName,
                                                                        const CTime& Begin,
                                                                        const CTime& End)
{
    // перечень всех файлов которые должны быть
    ChannelsFromFileByYear filesByDate;

    CTime tBegin = CTime(Begin.GetYear(), Begin.GetMonth(), Begin.GetDay(), Begin.GetHour(), 0, 0);
    int offset = End.GetMinute() * 60 + End.GetSecond();
    CTime tEnd = offset > 0 ? (End - CTimeSpan(offset - 3600)) : End;

    // флаг что удалось вытащить информацию о канале
    bool bChannelInfoExist(false);

    // директория с сигналами
    CString sourceFilesDir = getSourceFilesDir();
    // имя файла
    CString sFileName;

    CTime tTimeNext = tBegin;
    while (tTimeNext < tEnd)
    {
        sFileName.Format(L"%s%d\\%02d\\%02d\\%02d\\sig0000.xml", sourceFilesDir.GetString(), tTimeNext.GetYear(), tTimeNext.GetMonth(), tTimeNext.GetDay(), tTimeNext.GetHour());

        // пытаемся вытащить информацию о файлах с данными
        filesByDate[tTimeNext.GetYear()].ChannelsFromFileByMonth[tTimeNext.GetMonth()].ChannelsFromFileByDay[tTimeNext.GetDay()].ChannelsFromFileByHour[tTimeNext.GetHour()] =
            getFilesInfo(channelName, sFileName);

        tTimeNext += 3600;
    }

    return filesByDate;
}

//----------------------------------------------------------------------------//
bool CChannelDataGetter::getChannelInfo(const CString& xmlFileName)
{
    bool bRes(false);
    xml_document hFile;
    if (hFile.load_file(xmlFileName, parse_default, encoding_utf8))
    {
        xml_node hRoot = hFile.child(L"CommonSignalDescriptor");
        if (hRoot != NULL)
        {
            xml_node hSignal = hRoot.find_child_by_attribute(L"name", m_channelInfo->GetName());
            bRes = hSignal != NULL;
            if (bRes)
            {
                //m_channelInfo->m_sName = m_channelName;
                m_channelInfo->m_sComment = hSignal.attribute(L"comment").value();
                m_channelInfo->m_dFrequency = hSignal.attribute(L"frequency").as_double();
                m_channelInfo->m_fMinlevel = hSignal.attribute(L"minlevel").as_float();
                m_channelInfo->m_fMaxlevel = hSignal.attribute(L"maxlevel").as_float();
                m_channelInfo->m_fSense = hSignal.attribute(L"sense").as_float();
                m_channelInfo->m_fReference = hSignal.attribute(L"reference").as_float();
                m_channelInfo->m_fShift = hSignal.attribute(L"shift").as_float();
                m_channelInfo->m_sConversion = hSignal.attribute(L"conversion").value();
                m_channelInfo->m_iTypeAdc = hSignal.attribute(L"typeADC").as_int();
                m_channelInfo->m_iNumberDSP = hSignal.attribute(L"numberDSP").as_int();
                m_channelInfo->m_iChannel = hSignal.attribute(L"channel").as_int();

                GUID guid = { 0 };
                //запрашиваем CLSID компонента Список переменных
                HRESULT hRes = CLSIDFromString(hSignal.attribute(L"id").value(), &guid);
                assert(hRes == S_OK);// все оч плохо

                m_channelInfo->m_ID = guid;
                m_channelInfo->m_iType = hSignal.attribute(L"type").as_int();
                m_channelInfo->m_sGroupName = hSignal.attribute(L"groupname").value();
            }
        }
    }

    return bRes;
}

//----------------------------------------------------------------------------//
std::shared_ptr<ChannelInfo> CChannelDataGetter::getChannelInfo()
{
    return m_channelInfo;
}

//----------------------------------------------------------------------------//
ChannelFromFile CChannelDataGetter::getFilesInfo(const CString& channelName, const CString& xmlFileName)
{
    ChannelFromFile files;
    files.sDataFileName.Empty();
    files.sDescriptorFileName.Empty();

    xml_document hFile;
    if (hFile.load_file(xmlFileName, parse_default, encoding_utf8))
    {
        xml_node hRoot = hFile.child(L"CommonSignalDescriptor");
        if (hRoot != NULL)
        {
            xml_node hSignal = hRoot.find_child_by_attribute(L"name", channelName);
            if (hSignal != NULL)
            {
                files.sDataFileName = hSignal.attribute(L"data_file").value();
                files.sDescriptorFileName = hSignal.attribute(L"descriptor_file").value();
            }
        }
    }

    return files;
}

//----------------------------------------------------------------------------//
CString CChannelDataGetter::getSourceFilesDir()
{
    return get_service<DirsService>().getZetSignalsDir();
}

//----------------------------------------------------------------------------//
CString CChannelDataGetter::getZetCompressedDir()
{
    return get_service<DirsService>().getZetCompressedDir();
}

//----------------------------------------------------------------------------//
CString GetAnotherPath(const CString& sSoursePath, const CString& sPath,
                       unsigned int depthLevel)
{
    ++depthLevel;
    std::list<CString> vPath;
    int iCurPos(0);
    CString sRes = sPath.Tokenize(_T("\\"), iCurPos);
    while (!sRes.IsEmpty())
    {
        vPath.push_back(sRes);
        sRes = sPath.Tokenize(_T("\\"), iCurPos);
    };

    CString sRet(L"");
    if (vPath.size() > depthLevel)
    {
        sRet = sSoursePath;
        sRet.TrimRight(L"\\");

        std::for_each(std::prev(vPath.end(), depthLevel), vPath.end(), [&](CString &s)
                      {
                          sRet += L"\\";
                          sRet += s;
                      });
    }

    return sRet;
}

//----------------------------------------------------------------------------//
void Resampling(float* pSource, long lSourceSize, float* pDestination, long lDestinationSize)
{
    if (lSourceSize > lDestinationSize)
    {
        for (long i = 0; i < lDestinationSize; i += 2)
        {
            long lIndex = (long)(round(double(i) / double(lDestinationSize) * double(lSourceSize)));
            long lIndex2 = (long)(round(double(i + 2.) / double(lDestinationSize) * double(lSourceSize)));
            if (lIndex > lSourceSize - 1)
                lIndex = lSourceSize - 1;
            if (lIndex2 > lSourceSize - 1)
                lIndex2 = lSourceSize - 1;

            float fMin(NAN), fMax(NAN);
            int iMin(lIndex), iMax(lIndex);

            bool notNANFound = false;
            for (long j = lIndex; j < lIndex2; ++j)
            {
                if (_finite(pSource[j]) == 0)
                {
                    // если нан - ничего сделать нельзя
                    fMin = fMax = NAN;
                    iMin = iMax = lIndex;
                    break;
                }
                else
                {
                    if (!notNANFound)
                    {
                        fMin = fMax = pSource[j];
                        iMin = iMax = j;
                    }
                    else
                    {
                        if (fMin > pSource[j])
                            fMin = pSource[j];
                        if (fMax < pSource[j])
                            fMax = pSource[j];

                        notNANFound = true;
                    }
                }
            }

            if (iMin > iMax)
            {
                pDestination[i] = fMax;
                pDestination[i + 1] = fMin;
                //TRACE(L"From %d to %d: %d = %f, %d = %f\n", lIndex, lIndex2, i, fMax, i + 1, fMin);
            }
            else
            {
                pDestination[i + 1] = fMax;
                pDestination[i] = fMin;
                //TRACE(L"From %d to %d: %d = %f, %d = %f\n", lIndex, lIndex2, i, fMin, i + 1, fMax);
            }
        }
    }
    else
    {
        for (int i = 0; i < lDestinationSize; ++i)
        {
            double index = double(i) / double(lDestinationSize) * double(lSourceSize);
            long lIndex = (long)(floor(index));

            double deltaIndex = index - lIndex;
            if (lIndex + 1 == lSourceSize)
                pDestination[i] = pSource[lIndex];
            else
            {
                if (abs(pSource[lIndex]) < FLT_MIN && abs(pSource[lIndex + 1]) >= FLT_MIN)
                    pDestination[i] = pSource[lIndex + 1];
                else if (abs(pSource[lIndex]) >= FLT_MIN && abs(pSource[lIndex + 1]) < FLT_MIN)
                    pDestination[i] = pSource[lIndex];
                else
                    pDestination[i] = (float)(pSource[lIndex] * (1. - deltaIndex) + pSource[lIndex + 1] * deltaIndex);
            }
        }
    }
}

//----------------------------------------------------------------------------//
void CChannelDataGetter::GetSourceChannelDataByInterval(const CString& channelName,
                                                        const CTime& tStart, const CTime& tEnd,
                                                        long uiLength, float* pData,
                                                        double freq)
{
    ChannelsFromFileByYear channelMap = GetSourceFilesInfoByInterval(channelName, tStart, tEnd);

    long lLength(uiLength);


    double dLength = (1 / freq) * uiLength;
    //округленное в меньшую сторону до целых секунд время начала
    //CZetTime tBegin(tStart);
    //double begin = double(tBegin.GetNanoseconds()) / 1.e9;
    //tBegin.RoundSecondsDown();
    CTime tIntervalBegin = tStart;

    double begin = 0;

    //округленное в большую сторону до целых секунд время конца
    //CZetTime tFinish(tEnd);
    //double end = 1. - double(tFinish.GetNanoseconds()) / 1.e9;
    //tFinish.RoundSecondsUp();
    CTime tIntervalEnd = tEnd;
    double end = 1;

    //время начала часовой записи
    CTime timeBegin = CTime(tIntervalBegin.GetYear(), tIntervalBegin.GetMonth(), tIntervalBegin.GetDay(), tIntervalBegin.GetHour(), 0, 0);
    CString sBegin(timeBegin.Format(L"%Y-%m-%d-%H-%M-%S"));

    //время окончания часовой записи
    int offset = tIntervalEnd.GetMinute() * 60 + tIntervalEnd.GetSecond();
    CTime timeEnd = offset > 0 ? (tIntervalEnd - CTimeSpan(offset - 3600)) : tIntervalEnd;
    CString sEnd(timeEnd.Format(L"%Y-%m-%d-%H-%M-%S"));

    SYSTEM_INFO sinf;
    GetSystemInfo(&sinf);

    DWORD dwAllocationGranularity;								//гранулярность для начального адреса, в котором может быть выделена виртуальная память
    dwAllocationGranularity = sinf.dwAllocationGranularity;

    CTime tTimeNext = timeBegin;
    CTime tTimeCurrent = tIntervalBegin;
    CTime tCurrentBegin = CTime(tStart);
    double currentBegin = begin;
    double currentEnd = 0.;
    long lPosition(0);
    while (tTimeNext < timeEnd)
    {
        //текущее время начала часовой записи
        timeBegin = tTimeNext;
        //время начала следующей часовой записи
        tTimeNext += 3600;

        if (tTimeNext > tIntervalEnd)
        {
            tTimeNext = tIntervalEnd;
            timeEnd = tIntervalEnd;
            currentEnd = end;
        }
        //CString sTimeCurrent(tTimeCurrent.Format(L"%Y-%m-%d-%H-%M-%S"));
        //CString sTimeNext(tTimeNext.Format(L"%Y-%m-%d-%H-%M-%S"));

        CString sFileName = channelMap[tTimeCurrent.GetYear()].ChannelsFromFileByMonth[tTimeCurrent.GetMonth()].ChannelsFromFileByDay[tTimeCurrent.GetDay()].ChannelsFromFileByHour[tTimeCurrent.GetHour()].sDataFileName;

        struct __stat64 st;
        int iStatRes = _tstat64(sFileName, &st);
        if (iStatRes != 0)
        {
            sFileName = GetAnotherPath(getSourceFilesDir(), sFileName, 4);
            iStatRes = _tstat64(sFileName, &st);
        }
        if (iStatRes == 0)
        {
            if (st.st_size > 0)
            {
                DWORD dwFileSize = DWORD(st.st_size / sizeof(float));
                double dFileDelta = 3600. / double(dwFileSize);
                HANDLE hFile(CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL));
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    HANDLE hMap(CreateFileMapping(hFile, NULL, PAGE_READONLY, 0x00000000, 0x00000000, NULL));
                    if (hMap)
                    {
                        //смещение в файле исходных данных (которое нужно)
                        long long llFileOffset = long long((double(CTimeSpan(tCurrentBegin - timeBegin).GetTotalSeconds()) + currentBegin) / dFileDelta) * sizeof(float);
                        long long llTail = dwFileSize * sizeof(float);
                        //смещение в файле сжатых данных (приведенное к гранулярности)
                        long long llFileOffsetReal = llFileOffset / dwAllocationGranularity * dwAllocationGranularity;
                        long lDeltaFileOffset(long(llFileOffset - llFileOffsetReal));
                        //количество точек
                        double dFactor = freq * dFileDelta;
                        long lCount = long(lLength * dFactor);
                        if (llTail - llFileOffset < (long long)lCount * sizeof(float))
                            lCount = (long)((llTail - llFileOffset) / sizeof(float));

                        unsigned long ulSize = unsigned long(sizeof(float)* lCount + lDeltaFileOffset);
                        float* pView = reinterpret_cast<float*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, DWORD(llFileOffsetReal), ulSize));
                        if (pView != NULL)
                        {
                            float* pSrcData = &pView[lDeltaFileOffset / sizeof(float)];
                            float* pDestData = pData + lPosition;

                            // проверяем что при загрузке данных не будет выхода за пределы диапазона
                            if (lPosition + lCount > uiLength)
                                lCount = uiLength - lPosition;
                            if (lCount < 0)
                                break;

                            if (dFactor == 1.)
                            {
                                memcpy_s(pDestData, size_t(sizeof(float)* lCount),
                                         pSrcData,  size_t(sizeof(float)* lCount));
                                //TRACE("Fill %d points by source\n", lCount);
                            }
                            else
                            {
                                //Прореживание исходных данных
                                if (dFactor > 1.)
                                {
                                    if (dFactor - floor(dFactor) > 0)
                                    {
                                        long lQuantity = long(double(lCount) / dFactor);
                                        if (lPosition + lQuantity > uiLength)
                                            lQuantity = uiLength - lPosition;
                                        Resampling(pSrcData, lCount, pDestData, lQuantity);
                                        //TRACE("Fill %d points by resampling\n", lQuantity);
                                    }
                                    else
                                    {
                                        long iIndex(0);
                                        for (long i = 0; i < lCount; i += long(dFactor) * 2)
                                        {
                                            float fMin, fMax;
                                            fMin = fMax = pSrcData[i];
                                            /*for (int j = 0; j < int(dFactor); ++j)
                                            {
                                                CheckMinimum(fMin, pSrcData[i + j]);
                                                CheckMaximum(fMax, pSrcData[i + j]);
                                            }*/
                                            if (lPosition + iIndex < uiLength)
                                            {
                                                pDestData[iIndex] = fMin;
                                                ++iIndex;
                                                if (lPosition + iIndex < uiLength)
                                                {
                                                    pDestData[iIndex] = fMax;
                                                    ++iIndex;
                                                }
                                            }
                                        }
                                        //TRACE("Fill %d points by decimation\n", iIndex);
                                    }
                                }
                                //Передискретизация исходных данных в более высокую частоту дискретизации
                                else
                                {
                                    long lQuantity = long(double(lCount) / dFactor);
                                    if (lPosition + lQuantity > uiLength)
                                        lQuantity = uiLength - lPosition;
                                    Resampling(pSrcData, lCount, pDestData, lQuantity);
                                    //TRACE("Fill %d points by resampling\n", lQuantity);
                                }
                            }
                            UnmapViewOfFile(pView);
                        }
                        else
                        {

                        }
                        lPosition += long(lCount / dFactor);
                        lLength -= long(lCount / dFactor);
                        CloseHandle(hMap);
                    }
                    CloseHandle(hFile);
                }
            }
        }
        else
        {
            //CString sTimeCurrent(tTimeCurrent.Format(L"%Y-%m-%d-%H-%M-%S"));
            //CString sTimeNext(tTimeNext.Format(L"%Y-%m-%d-%H-%M-%S"));
            long lCount = long(CTimeSpan(tTimeNext - tTimeCurrent).GetTotalSeconds() * freq);
            lPosition += lCount;
            lLength -= lCount;
            //TRACE("Fill %d points by empty\n", lCount);
        }
        tTimeCurrent = tTimeNext;
        tCurrentBegin = tTimeCurrent;
        currentBegin = 0.;
    }
}