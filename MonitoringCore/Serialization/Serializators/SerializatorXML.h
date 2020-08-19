#pragma once

#include <optional>

#include <pugixml.hpp>

#include "Serialization/SerializatorFabric.h"

////////////////////////////////////////////////////////////////////////////////
// ������������ �������� � XML
class SerializatorXML
    : public ISerializator
    , public IDeserializator
{
public:
    SerializatorXML(const CString& filePath);

// ISerializator
public:
    /// <summary>���������� � ������ ������������.</summary>
    /// <returns>������ � ������ ��������� ������ ������������.</returns>
    bool onSerializatingStart() override;
    /// <summary>���������� �� ��������� ������������.</summary>
    /// <returns>������ � ������ ��������� ��������� ������������.</returns>
    bool onSerializatingEnd() override;
    /// <summary>������������� ��������.</summary>
    /// <param name="pProperty">������������� ��������.</param>
    void serializeProperty(ISerializablePropertyPtr pProperty) override;
    /// <summary>�������� ������ ������������� ���������.</summary>
    /// <param name="pCollection">������������� ���������.</param>
    void writeCollectionStart(ISerializableCollectionPtr pCollection) override;
    /// <summary>�������� ����� ������������� ���������.</summary>
    /// <param name="pCollection">������������� ���������.</param>
    void writeCollectionEnd(ISerializableCollectionPtr pCollection) override;

// IDeserializator
public:
    /// <summary>���������� � ������ ��������������.</summary>
    /// <returns>������ � ������ ��������� ������ ��������������.</returns>
    bool onDeserializatingStart() override;
    /// <summary>���������� �� ��������� ��������������.</summary>
    /// <returns>������ � ������ ��������� ��������� ��������������.</returns>
    bool onDeserializatingEnd() override;
    /// <summary>��������������� ��������.</summary>
    /// <param name="indexAmongIdenticalNames">������ ����� ���������� ���� � ���������.</param>
    /// <param name="pProperty">��������������� ��������.</param>
    /// <returns>������ � ������ �������� �������������.</returns>
    bool deserializeProperty(size_t indexAmongIdenticalNames,
                             ISerializablePropertyPtr pProperty) override;
    /// <summary>��������� ������ ������������� ���������.</summary>
    /// <param name="indexAmongIdenticalNames">������ ����� ���������� ���� � ���������.</param>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    bool readCollectionStart(size_t indexAmongIdenticalNames,
                             ISerializableCollectionPtr pCollection) override;
    /// <summary>��������� ����� ������������� ���������.</summary>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    void readCollectionEnd(ISerializableCollectionPtr pCollection) override;
    /// <summary>��������� ����� ��������������� ��������� ���������.</summary>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <param name="objectNames">����� �������� � ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    bool readCollectionObjectNames(ISerializableCollectionPtr pCollection,
                                   std::list<CString>& objectNames) override;

private:
    // �������� ����������� �������� ������
    bool isCheckCanReadObject(ISerializablePtr obj, CString& objName);

    // �������� �� ������ ���� ��� �� ���� ���������������� ������ ����������� �����
    // ��������� ��� ������ ���� � ����� � ���� ������������� ������
    bool checkConsistent(const CString& objName);

private:
    // �������� ������������ ����
    pugi::xml_node getParentNode(const pugi::xml_node& xmlNode) const;
    // �������� ����� �������� ���
    void getChildNodeNames(const pugi::xml_node& xmlNode,
                           std::list<CString>& childNames);

    // ���������� �������� ����
    pugi::xml_node addChildNode(pugi::xml_node xmlNode, const CString& nodeName);
    // ����� �������� ���� �� �����
    bool getChildNode(const pugi::xml_node& xmlNode,
                      const CString& childName,
                      const size_t childIndex,
                      pugi::xml_node& childNode) const;

    // ���������� �������� � ����
    void setValue(pugi::xml_node xmlNode, const CString& value);
    // ��������� �������� �� ����
    CString getValue(const pugi::xml_node& xmlNode) const;

    // ���������� �������� �������� � ���� �� �����
    void setAttributeValue(pugi::xml_node xmlNode,
                           const CString& attributeName,
                           const CString& attributeValue);
    // ��������� �������� �������� �� ����
    bool getAttributeValue(pugi::xml_node xmlNode,
                           const CString& attributeName,
                           CString& attributeValue) const;

private:
    // ���� � XML ����� ���� ����� ����������� ����������/������ ������
    CString m_filePath;

    // XML �������� � ������� ���������� ������������/��������������
    pugi::xml_document m_hFile;

    // ������� ����
    std::optional<pugi::xml_node> m_currentNode;
};
