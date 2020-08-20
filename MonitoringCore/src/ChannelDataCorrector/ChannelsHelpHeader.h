#pragma once

#include <afxwin.h>
#include <algorithm>
#include <map>


class ChannelInfo
{
public:
    ChannelInfo() {}

    const CString& GetName() const { return m_sName; };
    void SetName(const CString& sName) { m_sName = sName; };

    const CString& GetComment() const { return m_sComment; };
    void SetComment(const CString& sComment) { m_sComment = sComment; };

    double GetFrequency() const { return m_dFrequency; };
    void SetFrequency(double dFrequency) { m_dFrequency = dFrequency; };

    float GetSubLevel() const { return m_fSublevel; };
    void SetSubLevel(float fSublevel) { m_fSublevel = fSublevel; };

    float GetMinLevel() const { return m_fMinlevel; };
    void SetMinLevel(float fMinlevel) { m_fMinlevel = fMinlevel; };

    float GetMaxLevel() const { return m_fMaxlevel; };
    void SetMaxLevel(float fMaxlevel) { m_fMaxlevel = fMaxlevel; };

    float GetSense() const { return m_fSense; };
    void SetSense(float fSense) { m_fSense = fSense; };

    float GetReference() const { return m_fReference; };
    void SetReference(float fReference) { m_fReference = fReference; };

    float GetShift() const { return m_fShift; };
    void SetShift(float fShift) { m_fShift = fShift; };

    const CString& GetConversion() const { return m_sConversion; };
    void SetConversion(const CString& sConversion) { m_sConversion = sConversion; };

    int GetNumber() const { return m_iChannel; };
    void SetNumber(int iChannel) { m_iChannel = iChannel; };

    int GetTypeADC() const { return m_iTypeAdc; };
    void SetTypeADC(int iTypeAdc) { m_iTypeAdc = iTypeAdc; };

    int GetNumberDSP() const { return m_iNumberDSP; };
    void SetNumberDSP(int iNumberDSP) { m_iNumberDSP = iNumberDSP; };

    const GUID& GetID() const { return m_ID; };
    void SetID(GUID ID) { m_ID = ID; };

    int GetType() const { return m_iType; };
    void SetType(int iType) { m_iType = iType; };

    const CString& GetGroupName() const { return m_sGroupName; };
    void SetGroupName(const CString& sGroupName) { m_sGroupName = sGroupName; };

public :
    CString m_sName;
    CString m_sComment;
    double m_dFrequency = 0.;
    float m_fMinlevel = 0.f;
    float m_fMaxlevel = 0.f;
    float m_fSense = 0.f;
    float m_fReference = 0.f;
    float m_fShift = 0.f;
    CString m_sConversion;
    int m_iChannel = 0;
    int m_iTypeAdc  = 0;
    int m_iNumberDSP  = 0;
    GUID m_ID = {};
    int m_iType = 0;
    CString m_sGroupName;
    long m_lStatus = 0l;
    float m_fSublevel = 0.f;
};

//*****************************************************************************
struct TimeInterval
{
    CTime Begin, End;

    TimeInterval() : Begin(0), End(0) {}
    TimeInterval(CTime begin, CTime end) : Begin(begin), End(end) {}
    CTimeSpan Length() const { return End - Begin; }
    bool IsEmpty() const { return Begin >= End; }

    bool operator ==(const TimeInterval &interval) const { return (Begin == interval.Begin) && (End == interval.End); }
    bool operator !=(const TimeInterval &interval) const { return (Begin != interval.Begin) || (End != interval.End); }

    bool Contains(const CTime &time) const { return (time >= Begin) && (time <= End); }
    TimeInterval Intersection(const TimeInterval &interval) const { return TimeInterval(std::max<CTime>(Begin, interval.Begin),
                                                                                        std::min<CTime>(End, interval.End)); }
    bool IsIntersecting(const TimeInterval &interval) const { return !Intersection(interval).IsEmpty(); }
};

//*****************************************************************************
struct ChannelFromFile
{
    CString sDataFileName;
    CString sDescriptorFileName;
};

struct ChannelMapFromFileByHour
{
    std::map<unsigned char, ChannelFromFile> ChannelsFromFileByHour;
};
struct ChannelMapFromFileByDay
{
    std::map<unsigned char, ChannelMapFromFileByHour> ChannelsFromFileByDay;
};
struct ChannelMapFromFileByMonth
{
    std::map<unsigned char, ChannelMapFromFileByDay> ChannelsFromFileByMonth;
};
typedef std::map<long, ChannelMapFromFileByMonth> ChannelsFromFileByYear;

typedef std::pair<UINT, std::pair<double, double>> IntervalLength;