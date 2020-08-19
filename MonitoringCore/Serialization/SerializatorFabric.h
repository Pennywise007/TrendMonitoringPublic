#pragma once

/*
    ���������� � ������� ��� ��������/��������/�������� ������
    � ��������� ������ ������������ ������ ��� �������� �������� � XML
    ��� ������������� ����� ���� �������� ��� ���������� �������� � ��/JSON/����
*/

#include "ISerializable.h"

#include <memory>

//****************************************************************************//
/// ��������� ������������� �������
DECLARE_INTERFACE(ISerializator)
{
    typedef std::shared_ptr<ISerializator> Ptr;

// ���������� � ������/��������� ������ �������������
public:
    /// <summary>���������� � ������ ������������.</summary>
    /// <returns>������ � ������ ��������� ������ ������������.</returns>
    virtual bool onSerializatingStart() = 0;
    /// <summary>���������� �� ��������� ������������.</summary>
    /// <returns>������ � ������ ��������� ��������� ������������.</returns>
    virtual bool onSerializatingEnd() = 0;

// ��������
public:
    /// <summary>������������� ��������.</summary>
    /// <param name="pProperty">������������� ��������.</param>
    virtual void serializeProperty(ISerializablePropertyPtr pProperty) = 0;

// ���������
public:
    /// <summary>�������� ������ ������������� ���������.</summary>
    /// <param name="pCollection">������������� ���������.</param>
    virtual void writeCollectionStart(ISerializableCollectionPtr pCollection) = 0;
    /// <summary>�������� ����� ������������� ���������.</summary>
    /// <param name="pCollection">������������� ���������.</param>
    virtual void writeCollectionEnd(ISerializableCollectionPtr pCollection) = 0;
};

//****************************************************************************//
/// ��������� ��������������� �������
DECLARE_INTERFACE(IDeserializator)
{
    typedef std::shared_ptr<IDeserializator> Ptr;

// ���������� � ������/��������� ������ ���������������
public:
    /// <summary>���������� � ������ ��������������.</summary>
    /// <returns>������ � ������ ��������� ������ ��������������.</returns>
    virtual bool onDeserializatingStart() = 0;
    /// <summary>���������� �� ��������� ��������������.</summary>
    /// <returns>������ � ������ ��������� ��������� ��������������.</returns>
    virtual bool onDeserializatingEnd() = 0;

// ��������
public:
    /// <summary>��������������� ��������.</summary>
    /// <param name="indexAmongIdenticalNames">������ ����� ���������� ���� � ���������.</param>
    /// <param name="pProperty">��������������� ��������.</param>
    /// <returns>������ � ������ �������� �������������.</returns>
    virtual bool deserializeProperty(size_t indexAmongIdenticalNames,
                                     ISerializablePropertyPtr pProperty) = 0;

// ���������
public:
    /// <summary>��������� ������ ������������� ���������.</summary>
    /// <param name="indexAmongIdenticalNames">������ ����� ���������� ���� � ���������.</param>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    virtual bool readCollectionStart(size_t indexAmongIdenticalNames,
                                     ISerializableCollectionPtr pCollection) = 0;
    /// <summary>��������� ����� ������������� ���������.</summary>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    virtual void readCollectionEnd(ISerializableCollectionPtr pCollection) = 0;
    /// <summary>��������� ����� ��������������� ��������� ���������.</summary>
    /// <param name="pCollection">��������������� ���������.</param>
    /// <param name="objectNames">����� �������� � ���������.</param>
    /// <returns>������ � ������ ��������� ������.</returns>
    virtual bool readCollectionObjectNames(ISerializableCollectionPtr pCollection,
                                           std::list<CString>& objectNames) = 0;
};

//****************************************************************************//
// ������� �������� ����������� ��� (��)������������ ��������
class SerializationFabric
{
public:
    /// <summary>�������� ������������� ��� XML ����� �� ���� � �����.</summary>
    /// <param name="filePath">���� � xml �����.</param>
    /// <returns>��������� ������������� � XML.</returns>
    static ISerializator::Ptr createXMLSerializator(const CString& filePath);
    /// <summary>�������� ��������������� ��� XML ����� �� ���� � �����.</summary>
    /// <param name="filePath">���� � xml �����.</param>
    /// <returns>��������� ��������������� � XML.</returns>
    static IDeserializator::Ptr createXMLDeserializator(const CString& filePath);
};

//****************************************************************************//
// ����� ������������� �������
class SerializationExecutor
{
public:
    /// <summary>������������ �������.</summary>
    /// <param name="pSerializator">������������ ��������.</param>
    /// <param name="pSerializableObject">������������� ������.</param>
    /// <returns>������ � ������ �������� ������������ �������.</returns>
    static bool serializeObject(ISerializator::Ptr pSerializator,
                                ISerializablePtr pSerializableObject);

    /// <summary>�������������� �������.</summary>
    /// <param name="filePath">���� � xml �����.</param>
    /// <returns>��������� ������������� � XML.</returns>
    static bool deserializeObject(IDeserializator::Ptr pDeserializator,
                                  ISerializablePtr pSerializableObject);

private:
    // ���������� ������� ������������ �������, ������������ ��� ������������ ��������� ��������
    static bool serializeObjectImpl(ISerializator* pSerializator,
                                    ISerializablePtr pSerializableObject);
    // ���������� ������� �������������� �������, ������������ ��� �������������� ��������� ��������
    static bool deserializeObjectImpl(IDeserializator* pDeserializator,
                                      ISerializablePtr pSerializableObject);
};
