#pragma once

//**************************************************************************
// ��������
template<class T>
class CSingleton
{
public:
    static T& Instance()
    {
        // �������� ���������, ���� ��� ������� � ����������������
        static T s;
        return s;
    }

private:
    CSingleton() {};  // ����������� ����������
    ~CSingleton() {}; // � ���������� ���� ����������

    // ���������� ����� ��������� �����������
    CSingleton(CSingleton const&) = delete;
    CSingleton& operator= (CSingleton const&) = delete;
};

//**************************************************************************
// ������� ��������� ���������
template<class T>
static T& get_service()
{
    return CSingleton<T>::Instance();
}

//************************************************************************//
// ������ ������ � ����������
/*class CLoggerData
{
    friend class CSingleton<CLoggerData>;
public://-----------------------------------------------------------------//
    CLoggerData() {}
    ~CLoggerData() {}

public:
};*/
