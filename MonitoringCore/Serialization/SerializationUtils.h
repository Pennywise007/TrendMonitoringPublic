#pragma once

/*
    ��������������� ������� ��� ������������ ��������.
    ��������������� ����������� ����� � ����� � �������
*/

#include <afx.h>
#include <map>
#include <string>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
// ��������������� �������

// ���������� ���� �������, ComPtr<ISerializable> => ISerializable
template<typename U>
struct extract_value_type { typedef U value_type; };
template<template<typename> class X, typename U>
struct extract_value_type<X<U>> { typedef U value_type; };
template<class U>
using extract_value_type_v = typename extract_value_type<U>::value_type;

// �������� �� std::map/multimap ���������
template <class U>
struct isMapContainer { static constexpr bool value = false; };
template<class Key, class Value>
struct isMapContainer<std::map<Key, Value>> { static constexpr bool value = true; };
template<class Key, class Value>
struct isMapContainer<std::multimap<Key, Value>> { static constexpr bool value = true; };

#pragma region ValuesConvertation

////////////////////////////////////////////////////////////////////////////////
/// ����������� ������ � ��������, ����� ��������� ����������, � ������ �������������
/// �������������/��������������� ���-�� ��������� ���� �������� ����� ������������� �������

template<class T>
inline T string_to_value(const CString& str);

template<>
inline bool string_to_value<bool>(const CString& str) { return str == L"true"; }
template<>
inline int string_to_value<int>(const CString& str) { return _wtoi(str); }
template<>
inline unsigned int string_to_value<unsigned int>(const CString& str) { return _wtoi(str); }
template<>
inline long string_to_value<long>(const CString& str) { return (long)_wtoll(str); }
template<>
inline long long string_to_value<long long>(const CString& str) { return _wtoll(str); }
template<>
inline unsigned long string_to_value<unsigned long>(const CString& str) { return (unsigned long)_wtoll(str); }
template<>
inline unsigned long long string_to_value<unsigned long long>(const CString& str) { return (unsigned long long)_wtoll(str); }
template<>
inline float string_to_value<float>(const CString& str) { return (float)_wtof(str); }
template<>
inline double string_to_value<double>(const CString& str) { return _wtof(str); }
template<>
inline CString string_to_value<CString>(const CString& str) { return str; }
template<>
inline std::string string_to_value<std::string>(const CString& atr) { return std::string(CStringA(atr)); }
template<>
inline CTime string_to_value<CTime>(const CString& str)
{
    SYSTEMTIME st = { 0 };
    swscanf_s(str.GetString(), L"%hd.%hd.%hd %hd:%hd:%hd",
              &st.wDay, &st.wMonth, &st.wYear,
              &st.wHour, &st.wMinute, &st.wSecond);
    return st;
}
template<class T>
inline T string_to_value(const CString& str)
{
    if constexpr (std::is_enum_v<T>)
        return T(string_to_value<long>(str));
    else
        static_assert(false, __FUNCTION__ ": Need implement for this type");
}

////////////////////////////////////////////////////////////////////////////////
/// ����������� �������� � ������, ����� ��������� ����������, � ������ �������������
/// �������������/��������������� ���-�� ��������� ���� �������� ����� ������������� �������

template<class T>
inline CString value_to_sting(const T& val);

/// ��������������� ������ ��� ��������������������
#define CONVERT_TO_STRING(specifier, val) \
CString tempStr, Format((CString)L'%' + L#specifier); \
tempStr.Format(Format, val); \
return tempStr;

// ����������� �������� � ������
template<>
inline CString value_to_sting(const bool& val) { return val ? L"true" : L"false"; }
template<>
inline CString value_to_sting(const int& val) { CONVERT_TO_STRING(d, val) }
template<>
inline CString value_to_sting(const unsigned int& val) { CONVERT_TO_STRING(u, val) }
template<>
inline CString value_to_sting(const long& val) { CONVERT_TO_STRING(ld, val) }
template<>
inline CString value_to_sting(const long long& val) { CONVERT_TO_STRING(lld, val) }
template<>
inline CString value_to_sting(const unsigned long& val) { CONVERT_TO_STRING(ld, val) }
template<>
inline CString value_to_sting(const unsigned long long& val) { CONVERT_TO_STRING(llu, val) }
template<>
inline CString value_to_sting(const float& val) { CONVERT_TO_STRING(f, val) }
template<>
inline CString value_to_sting(const double& val) { CONVERT_TO_STRING(lf, val) }
template<>
inline CString value_to_sting(const CString& val) { return val; }
template<>
inline CString value_to_sting(const std::string& val) { return CString(val.c_str()); }
template<>
inline CString value_to_sting(const CTime& val) { return val.Format(L"%d.%m.%Y %H:%M:%S"); }
template<class T>
inline CString value_to_sting(const T& val)
{
    if constexpr (std::is_enum_v<T>)
        return value_to_sting<long>((long)val);
    else
        static_assert(false, __FUNCTION__ ": Need implement for this type");
}
// �� ������ ����������� ��������� ������
#undef CONVERT_TO_STRING

#pragma endregion ValuesConvertation

////////////////////////////////////////////////////////////////////////////////
// ��������������� �������, ������������ ��� ������������
#pragma region HelpMacros

// ��������������� ������� ��� ���������� ��������
#define REM(...) __VA_ARGS__
#define TYPE_AND_OBJECT(x) REM x
// ��������� ��������.      DECLARE_OBJECT((int) x) => int x
#define DECLARE_OBJECT(object) TYPE_AND_OBJECT(object)

// ��������������� ������� ��� ��������� ����� ��������
#define EAT(...)
#define REMOVE_TYPE(x) EAT x
// ��������� ��� ��������.  GET_OBJECT((int) x) => x
#define GET_OBJECT(object) REMOVE_TYPE(object)

// ������ ��� ��������� ����� �������� � ��������� ���� GET_PROPERTY_NAME((int) x) => L"x"
#define GET_OBJECT_NAME_IMPL(...) L#__VA_ARGS__
#define GET_OBJECT_NAME(...) CString(GET_OBJECT_NAME_IMPL(__VA_ARGS__)).Trim()

/*
���������� �������������� ������� �������� ������
����� ������ ���������� this � ����� ��� ���������� ��������� �� ������������� ����������
��������� ������ � �������������� ������������ � __VA_ARGS__
�������������� ������� ���������� ���� ������
*/
// ������ ��� ����������� �������������� ������� � ������� ���������
#define REGISTER_SERIALIZABLE_OBJECT(object, registerFunc, ...) \
registerFunc<decltype(object)>                                  \
(                                                               \
    GET_OBJECT_NAME(object),                                    \
    [this]()                                                    \
    {                                                           \
        return &object;                                         \
    },                                                          \
    decltype(object)(__VA_ARGS__)                               \
);

#pragma endregion HelpMacros
