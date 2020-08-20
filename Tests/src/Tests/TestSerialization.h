#pragma once

#include "Serialization\ISerializable.h"

//****************************************************************************//
// �������� ����� ��� �������� ���������� ������������ ������� �����
DECLARE_COM_INTERNAL_CLASS_(TestSerializing, SerializableObjectsCollection)
{
public:
    TestSerializing() = default;
public:
    DECLARE_COLLECTION(TestSerializing);

    // ��������� ��� �������� ������� ����� �������������/���������������
    DECLARE_PROPERTY((bool) m_bValue, false);
    DECLARE_PROPERTY((int) m_iValue, 5);
    DECLARE_PROPERTY((unsigned int) m_uiValue, 15);
    DECLARE_PROPERTY((long) m_lValue, -5);
    DECLARE_PROPERTY((long long) m_llValue, 12);
    DECLARE_PROPERTY((unsigned long) m_ulValue, 15);
    DECLARE_PROPERTY((unsigned long long) m_ullValue, 14);
    DECLARE_PROPERTY((float) m_fValue, 7.f);
    DECLARE_PROPERTY((double) m_dValue, 8.);
    DECLARE_PROPERTY((CString) m_CStrValue, L"Test");
    DECLARE_PROPERTY((std::string) m_strValue, "String");
    DECLARE_PROPERTY((CTime) m_CTimeValue, CTime(2020, 05, 20, 12, 0, 0));
    DECLARE_CONTAINER((std::vector<int>) m_vectorInt, { 1, 2, 3 });
    DECLARE_CONTAINER((std::list<bool>) m_listBool, { false, true, false });
    DECLARE_CONTAINER((std::map<long, float>) m_mapLongFloat, { { 1, 100.f }, { 2, 32.f }, { 3, 42.f } });
    DECLARE_CONTAINER((std::set<long>) m_setLong, { 10, 20, 30 });
};

//----------------------------------------------------------------------------//
// �������� ������� ������ ������ �� ����������� ��������� ������� ������
inline void checkProperties(const TestSerializing& first, const TestSerializing& second)
{
    EXPECT_EQ(first.m_bValue, second.m_bValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_iValue, second.m_iValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_uiValue, second.m_uiValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_lValue, second.m_lValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_llValue, second.m_llValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_ulValue, second.m_ulValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_ullValue, second.m_ullValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_fValue, second.m_fValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_dValue, second.m_dValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_CStrValue, second.m_CStrValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_strValue, second.m_strValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_CTimeValue, second.m_CTimeValue) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_vectorInt, second.m_vectorInt) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_listBool, second.m_listBool) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_mapLongFloat, second.m_mapLongFloat) << "������ ��� ��������� TestSerializing";
    EXPECT_EQ(first.m_setLong, second.m_setLong) << "������ ��� ��������� TestSerializing";
}

//****************************************************************************//
// ������������� ��������
DECLARE_COM_INTERNAL_CLASS_(TestSerializingSubClass, SerializableObjectsCollection)
{
public:
    TestSerializingSubClass() = default;
    TestSerializingSubClass(const CString text, const int value)
        : m_text(text)
        , m_value(value)
    {}

    BEGIN_SERIALIZABLE_PROPERTIES(TestSerializingClass)
        ADD_PROPERTY(m_text)
        ADD_PROPERTY(m_value)
    END_SERIALIZABLE_PROPERTIES()

    CString m_text;
    int m_value = 0;
};

//----------------------------------------------------------------------------//
inline bool CompareSubClasses(const TestSerializingSubClass& first, const TestSerializingSubClass& second)
{
    EXPECT_EQ(first.m_text, second.m_text) << "������ ��� ��������� TestSerializingSubClass";
    EXPECT_EQ(first.m_value, second.m_value) << "������ ��� ��������� TestSerializingSubClass";

    return first.m_text == second.m_text &&
        first.m_value == second.m_value;
}

//****************************************************************************//
// �������� ����� ��� �������� ���������� ������������ ������
DECLARE_COM_INTERNAL_CLASS_(TestSerializingClass, SerializableObjectsCollection)
{
public:
    TestSerializingClass() = default;

    // ��������� �������� ������� ����� �������������/���������������
    BEGIN_SERIALIZABLE_PROPERTIES(TestSerializingClass)
        ADD_PROPERTY(m_bValue)
        ADD_CONTAINER(m_listOfUINT)
        ADD_CONTAINER(m_mapStringTofloat)
    END_SERIALIZABLE_PROPERTIES()

    void FillSerrializableValues()
    {
        m_iValue = 75;
        m_lValue = 41;

        m_vectorDoubleValues = { 0., 1., 8., 7. };

        m_mapStringTofloat[L"����"] = 1.f;
        m_mapStringTofloat[L"���"] = 2.f;
        m_mapStringTofloat[L"���"] = 3.f;

        m_listOfUINT = { 125, 542, 95 };

        m_serializingSubclass.push_back(TestSerializingSubClass::make(L"Value1", 3114));
        m_serializingSubclass.push_back(TestSerializingSubClass::make(L"Value2", 555));
    }

public:
    DECLARE_PROPERTY((int) m_iValue, 0);
    DECLARE_PROPERTY((long) m_lValue, 0);
    DECLARE_CONTAINER((std::vector<double>) m_vectorDoubleValues);
    // ��������� ������������� ��������
    DECLARE_SERIALIZABLE((TestSerializing::Ptr) m_serializingClass, TestSerializing::create());
    // ������ ����������
    DECLARE_CONTAINER((std::list<TestSerializingSubClass::Ptr>) m_serializingSubclass);

    bool m_bValue = true;
    std::list<UINT> m_listOfUINT;
    std::map<CString, float> m_mapStringTofloat;
};
