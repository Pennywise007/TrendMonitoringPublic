#include "pch.h"

#include <functional>

#include "SerializatorFabric.h"
#include "Serialization/Serializators/SerializatorXML.h"

//****************************************************************************//
ISerializator::Ptr
SerializationFabric::createXMLSerializator(const CString& filePath)
{
    return ISerializator::Ptr(new SerializatorXML(filePath));
}

//----------------------------------------------------------------------------//
IDeserializator::Ptr
SerializationFabric::createXMLDeserializator(const CString& filePath)
{
    return IDeserializator::Ptr(new SerializatorXML(filePath));
}

//****************************************************************************//
// ����� ������ �� ��������� � ���������� �������������� �������
// ������������ ��� ���������� �������� �� ������������� ��������
class SerializableObjectsVisitorImpl //: public ISerializableObjectsVisitor
{
public:

    // ����� ������ �������������
    enum WorkMode
    {
        eSerialization = 0, // ������������
        eDeserialization    // ��������������
    };
    SerializableObjectsVisitorImpl(const WorkMode workMode);

public:
    // ������������� ������ - �������
    bool initVisitor(ISerializable* pSerializableObject);

    // ��� �������������� �������
    enum ObjectType
    {
        eCollectionStart = 0,   // ������ ���������
        eCollectionEnd,         // ����� ���������
        eProperty              // ��������
    };
    // �������� ��� �������
    ObjectType getObjectType();
    // �������� ������� ������
    ISerializable* getCurrentObject();
    // �������� ���������� ���������� ���� � ��������� ��� �������� ������
    // �� ������ ���� ������������� ��������� �������� � ����������� �������
    size_t getIndexAmongIdenticalNames(bool bCollectionStart);
    // ������� � ���������� �������������� �������
    bool goToNextObject();
    // ���������� ���� ������ � ������� � ����������
    void skipCollectionContent();
    // ���������� ������ ������� ���������
    void updateCurrentCollectionSize();

private:
    // �������� ������� ��� �������
    bool updateObjectType();
    // ������� �������� ������� �� ����������� �������������
    bool checkObject(ISerializable* object);

private:
    // ����� ������ �������������
    bool m_bSkipEmptyCollections;
    // ��� �������� ��������
    ObjectType m_currentObjectType = ObjectType::eCollectionStart;
    // ������� ������������� ������
    ISerializablePtr m_currentSerializableObject;

    // ���������� � ��������� � ������� ���� ��������
    struct CollectionInfo
    {
        // ��������� �� ���������
        ISerializableCollection* pCollection;
        // ������ ���������
        size_t sizeOfCollection;
        // ������� ������ � ���������
        size_t currentIndexInCollection;

        CollectionInfo(ISerializableCollection* pCollection_)
            : pCollection(pCollection_)
            , sizeOfCollection(pCollection_->getObjectsCount())
            , currentIndexInCollection(0L)
        {}
    };

    // ������� ������� �� ��������� ����������, ��������� ������� - ������� �������
    std::list<CollectionInfo> m_collectionsDepth;
};

//****************************************************************************//
SerializableObjectsVisitorImpl::SerializableObjectsVisitorImpl(const WorkMode workMode)
    : m_bSkipEmptyCollections(workMode == WorkMode::eSerialization)
{}

//****************************************************************************//
bool SerializableObjectsVisitorImpl::initVisitor(ISerializable* pSerializableObject)
{
    m_currentSerializableObject = pSerializableObject;
    assert(m_currentSerializableObject);

    if (!checkObject(m_currentSerializableObject))
        return false;

    updateObjectType();

    return true;
}

//----------------------------------------------------------------------------//
SerializableObjectsVisitorImpl::ObjectType SerializableObjectsVisitorImpl::getObjectType()
{
    return m_currentObjectType;
}

//----------------------------------------------------------------------------//
ISerializable* SerializableObjectsVisitorImpl::getCurrentObject()
{
    return m_currentSerializableObject;
}

//----------------------------------------------------------------------------//
size_t SerializableObjectsVisitorImpl::getIndexAmongIdenticalNames(bool bCollectionStart)
{
    size_t counter = 0;

    // ���� ���� ����������� ���������� ��������� � ���������
    if ((!bCollectionStart && !m_collectionsDepth.empty()) ||
        (bCollectionStart && m_collectionsDepth.size() >= 2))
    {
        CString curObjectName;
        if (!m_currentSerializableObject->getObjectName(&curObjectName))
            return 0;

        decltype(m_collectionsDepth)::reverse_iterator collectionInfoIt;
        collectionInfoIt = m_collectionsDepth.rbegin();

        // ��� ������ ��������� ����� �������������
        if (bCollectionStart)
            ++collectionInfoIt;

        for (size_t index = 0; index < collectionInfoIt->currentIndexInCollection; ++index)
        {
            CString objectName;
            if (ISerializablePtr object = collectionInfoIt->pCollection->getObject(index);
                object && object->getObjectName(&objectName) && objectName == curObjectName)
                ++counter;
        }
    }

    return counter;
}

//----------------------------------------------------------------------------//
bool SerializableObjectsVisitorImpl::goToNextObject()
{
    switch (getObjectType())
    {
    case ObjectType::eCollectionStart:
    {
        ISerializableCollectionPtr pCollection = m_currentSerializableObject;
        assert(pCollection);

        if (pCollection->getObjectsCount() == 0)
        {
            // ���� ��������� ������ - ������� ��� ����� �� � �����
            m_currentObjectType = eCollectionEnd;
            return true;
        }

        m_currentSerializableObject = pCollection->getObject(0);

        if (!updateObjectType())
        {
            // ��������� � ���� �������� ���� �� ������ ���������� ���
            m_currentObjectType = ObjectType::eProperty;
            return goToNextObject();
        }

        // ��������� ��� ������ ������������� � ��� ������
        if (!checkObject(m_currentSerializableObject))
            // ���� �� �� ������������� - ��������� � ���������� �������
            return goToNextObject();
    }
    break;
    case ObjectType::eCollectionEnd:
    {
        // ������� �� ����������� ������ ���������
        if (!m_collectionsDepth.empty())
            m_collectionsDepth.pop_back();

        // ��������� �����������
        if (m_collectionsDepth.empty())
        {
            m_currentSerializableObject = nullptr;
            return false;
        }

        // ��������� � ���� �������� � ���������
        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        m_currentSerializableObject =
            collectionInfo.pCollection->getObject(collectionInfo.currentIndexInCollection);
        m_currentObjectType = ObjectType::eProperty;
        return goToNextObject();
    }
    break;
    case ObjectType::eProperty:
    {
        // ������ ����� ������������� ����� ���� �������� � �������
        if (m_collectionsDepth.empty())
            return false;

        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        // �������� ��� � ��������� ���� ���������� ��� ������������ ������
        bool bGotSerializableObject = false;
        while (++collectionInfo.currentIndexInCollection < collectionInfo.sizeOfCollection)
        {
            m_currentSerializableObject = collectionInfo.pCollection->
                getObject(collectionInfo.currentIndexInCollection);
            if (checkObject(m_currentSerializableObject))
            {
                bGotSerializableObject = true;
                break;
            }
        }

        if (bGotSerializableObject)
        {
            // ��������� ��� ��������
            if (!updateObjectType())
            {
                // ��������� � ���� �������� ���� �� ������ ���������� ���
                m_currentObjectType = ObjectType::eProperty;
                return goToNextObject();
            }
        }
        else
        {
            // ����� �� ����� ���������
            m_currentObjectType = ObjectType::eCollectionEnd;
            m_currentSerializableObject = collectionInfo.pCollection;
        }
    }
    break;
    default:
        assert(!"�� ��������� ��� �������������� �������");
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------//
void SerializableObjectsVisitorImpl::skipCollectionContent()
{
    // ������� ��� �� � ����� ���������
    m_currentObjectType = ObjectType::eCollectionEnd;
}

//----------------------------------------------------------------------------//
void SerializableObjectsVisitorImpl::updateCurrentCollectionSize()
{
    if (m_collectionsDepth.empty())
        assert(!"����������� ������ �� ������������ ���������!");

    CollectionInfo& collectionInfo = m_collectionsDepth.back();
    collectionInfo.sizeOfCollection = collectionInfo.pCollection->getObjectsCount();
}

//----------------------------------------------------------------------------//
bool SerializableObjectsVisitorImpl::updateObjectType()
{
    if (ISerializablePropertyPtr(m_currentSerializableObject))
        m_currentObjectType = ObjectType::eProperty;
    else
    {
        ISerializableCollectionPtr pCollection = m_currentSerializableObject;
        if (pCollection)
        {
            m_currentObjectType = ObjectType::eCollectionStart;

            // ��������� � ������ ��������� ����� �������
            m_collectionsDepth.emplace_back(pCollection);
        }
        else
        {
            assert(!"����������� ��� �������������� �������");
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------//
bool SerializableObjectsVisitorImpl::checkObject(ISerializable* object)
{
    // ���������� �� ������������� �������
    if (object && object->getObjectName())
    {
        if (m_bSkipEmptyCollections)
        {
            // ���� ���������� ���������� ������ ���������
            ISerializableCollectionPtr pCollection = object;
            if (pCollection)
            {
                // ���� ��� ��������� - ��������� ��� � ��� ���� ������������� �������
                for (size_t i = 0, count = pCollection->getObjectsCount(); i < count; ++i)
                {
                    if (checkObject(pCollection->getObject(i)))
                        return true;
                }

                return false;
            }
        }

        return true;
    }
    else
        return false;
}

//****************************************************************************//
bool SerializationExecutor::serializeObject(ISerializator::Ptr pSerializator,
                                            ISerializablePtr pSerializableObject)
{
    if (!pSerializator || !pSerializableObject)
    {
        // �� �������� ���/��� �������������
        assert(false);
        return false;
    }

    // �������� ������������
    if (!pSerializator->onSerializatingStart() ||
        !serializeObjectImpl(pSerializator.get(), pSerializableObject))
    {
        // ��� �������� ��� ������������
        assert(false);
        return false;
    }

    // ��������� ������������
    return pSerializator->onSerializatingEnd();
}

//----------------------------------------------------------------------------//
bool SerializationExecutor::deserializeObject(IDeserializator::Ptr pDeserializator,
                                              ISerializablePtr pSerializableObject)
{
    if (!pDeserializator || !pSerializableObject)
    {
        // �� �������� ���/��� ���������������
        assert(false);
        return false;
    }

    // �������� ��������������
    if (!pDeserializator->onDeserializatingStart())
        return false;

    if (!deserializeObjectImpl(pDeserializator.get(), pSerializableObject))
    {
        // ��� �������� ��� ��������������
        assert(false);
        return false;
    }

    // ��������� ��������������
    return pDeserializator->onDeserializatingEnd();
}

//----------------------------------------------------------------------------//
bool SerializationExecutor::serializeObjectImpl(ISerializator* pSerializator,
                                                ISerializablePtr pSerializableObject)
{
    SerializableObjectsVisitorImpl objectsVisitor(
        SerializableObjectsVisitorImpl::WorkMode::eSerialization);
    if (!objectsVisitor.initVisitor(pSerializableObject))
        // ��� �������� ��� ������������
        return false;

    do
    {
        // �������� ��������������� ������� ������������� � ����������� �� �������
        switch (objectsVisitor.getObjectType())
        {
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionStart:
                {
                    ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();
                    if (pCollection->onStartSerializing())
                        pSerializator->writeCollectionStart(pCollection);
                    else
                        // ���������� ��� ���������
                        objectsVisitor.skipCollectionContent();
                }
                break;
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionEnd:
                {
                    ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();

                    pSerializator->writeCollectionEnd(pCollection);

                    pCollection->onEndSerializing();
                }
                break;
            case SerializableObjectsVisitorImpl::ObjectType::eProperty:
                pSerializator->serializeProperty(objectsVisitor.getCurrentObject());
                break;
            default:
                assert(!"������� ������������� � XML ������ ������������ ����");
                break;
        }

    } while (objectsVisitor.goToNextObject());

    return true;
}

//----------------------------------------------------------------------------//
bool SerializationExecutor::deserializeObjectImpl(IDeserializator* pDeserializator,
                                                  ISerializablePtr pSerializableObject)
{
    SerializableObjectsVisitorImpl objectsVisitor(
        SerializableObjectsVisitorImpl::WorkMode::eDeserialization);
    if (!objectsVisitor.initVisitor(pSerializableObject))
        // ��� ���������� ��� �������������� ��������
        return false;

    do
    {
        // �������� ��������������� ������� ������������� � ����������� �� �������
        switch (objectsVisitor.getObjectType())
        {
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionStart:
            {
                ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();
                if (!pDeserializator->readCollectionStart(objectsVisitor.getIndexAmongIdenticalNames(true),
                                                          pCollection))
                {
                    // ���� ��������� ������ ������ ������ ����� �� ����
                    //assert(false);
                    // ���������� ��� ���������
                    objectsVisitor.skipCollectionContent();
                }
                else
                {
                    // �������� ������ ��������������� �������� � ���������
                    std::list<CString> objNames;
                    if (pDeserializator->readCollectionObjectNames(pCollection, objNames))
                    {
                        if (pCollection->onStartDeserializing(objNames))
                            // ��������� ������ ���������, �� ��� ���������� ����� onStartDeserializing
                            objectsVisitor.updateCurrentCollectionSize();
                        else
                            // ���������� ��� ���������
                            objectsVisitor.skipCollectionContent();
                    }
                    else
                        // ���������� ��� ���������
                        objectsVisitor.skipCollectionContent();
                }
            }
            break;
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionEnd:
                {
                    ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();
                    pDeserializator->readCollectionEnd(pCollection);
                    pCollection->onEndDeserializing();
                }
                break;
            case SerializableObjectsVisitorImpl::ObjectType::eProperty:
                pDeserializator->deserializeProperty(objectsVisitor.getIndexAmongIdenticalNames(false),
                                                     objectsVisitor.getCurrentObject());
                break;
            default:
                assert(!"������� ������������� � XML ������ ������������ ����");
                break;
        }

    } while (objectsVisitor.goToNextObject());

    return true;
}
