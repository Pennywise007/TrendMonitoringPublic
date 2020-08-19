// �������� ��������������� ��������

#include "pch.h"
#include "resource.h"

#include <afx.h>
#include <filesystem>

#include <vector>
#include <list>
#include <map>

#include <COM.h>
#include "Serialization\SerializatorFabric.h"

#include "TestSerialization.h"
#include "SamplesHelper.h"

//****************************************************************************//
TEST(Serialization, TestXMLFileValid)
{
    // ���� XML
    CString fileXML = L"TestXMLFileValid.xml";

    // ������������

    // C����������� � XML
    ISerializator::Ptr pXMLSerializator = SerializationFabric::createXMLSerializator(fileXML);
    ASSERT_TRUE(pXMLSerializator) << "�� ������� ������� ������������";

    // ������������� �����
    TestSerializing::Ptr pSerializableClass = TestSerializing::create();
    ASSERT_TRUE(pSerializableClass) << "�� ������� ������� ������������� �����";

    // ����������� ����� � XML
    ASSERT_TRUE(SerializationExecutor::serializeObject(pXMLSerializator, pSerializableClass)) <<
        "�� ������� ������������� ������";

    // ��������� ��� ����� ������������ ��������� ���� ��������� � ����������� ��������
    compareWithResourceFile(fileXML, IDR_TESTXMLFILEVALID, L"TestXMLFileValid");

    // ������� ��������� ����� ������������ ����
    ASSERT_TRUE(std::filesystem::remove(std::filesystem::path(fileXML.GetString()))) <<
        "�� ������� ������� ���� ����� ������������";
}

//****************************************************************************//
TEST(Serialization, TestXMLSerialization)
{
    // ���� XML
    CString fileXML = L"TestXMLSerialization.xml";

    // ������������

    // C����������� � XML
    ISerializator::Ptr pXMLSerializator = SerializationFabric::createXMLSerializator(fileXML);
    ASSERT_TRUE(pXMLSerializator) << "�� ������� ������� ������������";

    // ������������� �����
    TestSerializingClass::Ptr pSerializableClass = TestSerializingClass::create();
    ASSERT_TRUE(pSerializableClass) << "�� ������� ������� ������������� �����";
    // ��������� ����� ����������
    pSerializableClass->FillSerrializableValues();

    // ����������� ����� � XML
    ASSERT_TRUE(SerializationExecutor::serializeObject(pXMLSerializator, pSerializableClass)) <<
        "�� ������� ������������� ������";

    // ��������� ��� ����� ������������ ��������� ���� ��������� � ����������� ��������
    compareWithResourceFile(fileXML, IDR_TESTXMLSERIALIZATION, L"TestXMLSerialization");

    // ��������������

    // C����������� � XML
    IDeserializator::Ptr pXMLDeserializator = SerializationFabric::createXMLDeserializator(fileXML);
    ASSERT_TRUE(pXMLDeserializator) << "�� ������� ������� ��������������";

    // ��������������� �����
    TestSerializingClass::Ptr pDeserializableClass = TestSerializingClass::create();
    ASSERT_TRUE(pDeserializableClass) << "�� ������� ������� ��������������� �����";

    // ������������� ����� �� XML
    ASSERT_TRUE(SerializationExecutor::deserializeObject(pXMLDeserializator, pDeserializableClass)) <<
                "�� ������� ��������������� ������";

    // ��������� ������� �� ������������ ���� �����
    EXPECT_EQ(pSerializableClass->m_iValue, pDeserializableClass->m_iValue) << "����� �������������� ������ �� �������!";
    EXPECT_EQ(pSerializableClass->m_lValue, pDeserializableClass->m_lValue) << "����� �������������� ������ �� �������!";
    EXPECT_EQ(pSerializableClass->m_bValue, pDeserializableClass->m_bValue) << "����� �������������� ������ �� �������!";
    EXPECT_EQ(pSerializableClass->m_listOfUINT, pDeserializableClass->m_listOfUINT) << "����� �������������� ������ �� �������!";
    EXPECT_EQ(pSerializableClass->m_vectorDoubleValues, pDeserializableClass->m_vectorDoubleValues) << "����� �������������� ������ �� �������!";
    EXPECT_EQ(pSerializableClass->m_mapStringTofloat, pDeserializableClass->m_mapStringTofloat) << "����� �������������� ������ �� �������!";

    // ��������� ������������ ����������� ������ m_serializingClass TestSerializing::Ptr
    checkProperties(*pSerializableClass->m_serializingClass, *pDeserializableClass->m_serializingClass);

    // ��������� ��� ������ ������������� ������� ��������� ����������������
    auto& serializableSubClass = pSerializableClass->m_serializingSubclass;
    auto& deserializableSubClass = pDeserializableClass->m_serializingSubclass;
    EXPECT_EQ(serializableSubClass.size(), deserializableSubClass.size()) <<
        "����� �������������� ������� �������� � ����������� �� �������!";

    auto serializableIt = serializableSubClass.begin();
    auto deserializableIt = deserializableSubClass.begin();
    for (size_t i = 0, size = std::min<size_t>(serializableSubClass.size(), deserializableSubClass.size());
         i < size; ++i, ++serializableIt, ++deserializableIt)
    {
        CStringA errorText;
        errorText.Format("������ � ������� ���������� �������� �� ������� %u", i);
        EXPECT_TRUE(CompareSubClasses(**serializableIt, **deserializableIt)) << errorText;
    }

    // ������� ��������� ����� ������������ ����
    ASSERT_TRUE(std::filesystem::remove(std::filesystem::path(fileXML.GetString()))) <<
        "�� ������� ������� ���� ����� ������������";
}
