#pragma once

//**************************************************************************
// синглтон
template<class T>
class CSingleton
{
public:
    static T& Instance()
    {
        // согласно стандарту, этот код ленивый и потокобезопасный
        static T s;
        return s;
    }

private:
    CSingleton() {};  // конструктор недоступен
    ~CSingleton() {}; // и деструктор тоже недоступен

    // необходимо также запретить копирование
    CSingleton(CSingleton const&) = delete;
    CSingleton& operator= (CSingleton const&) = delete;
};

//**************************************************************************
// функция получения синглтона
template<class T>
static T& get_service()
{
    return CSingleton<T>::Instance();
}

//************************************************************************//
// пример класса с синглтоном
/*class CLoggerData
{
    friend class CSingleton<CLoggerData>;
public://-----------------------------------------------------------------//
    CLoggerData() {}
    ~CLoggerData() {}

public:
};*/
