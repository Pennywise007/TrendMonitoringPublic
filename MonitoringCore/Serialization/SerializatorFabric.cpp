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
// Класс визитёр по свойствам и коллекциям сериализуемого объекта
// Используется для унификации итерации по сериализуемым объектам
class SerializableObjectsVisitorImpl //: public ISerializableObjectsVisitor
{
public:

    // Режим работы сериализатора
    enum WorkMode
    {
        eSerialization = 0, // сериализация
        eDeserialization    // десериализация
    };
    SerializableObjectsVisitorImpl(const WorkMode workMode);

public:
    // Инициализация класса - визитёра
    bool initVisitor(ISerializable* pSerializableObject);

    // Тип сериализуемого объекта
    enum ObjectType
    {
        eCollectionStart = 0,   // начало коллекции
        eCollectionEnd,         // конец коллекции
        eProperty              // свойства
    };
    // Получить тип объекта
    ObjectType getObjectType();
    // Получить текущий объект
    ISerializable* getCurrentObject();
    // Получить количество идентичных имен в коллекции для текущего объкта
    // на случай если сериализовали несколько обьектов с одинаковыми именами
    size_t getIndexAmongIdenticalNames(bool bCollectionStart);
    // Перейти к следующему сериализуемому объекту
    bool goToNextObject();
    // Пропустить весь объект и перейти к следующему
    void skipCollectionContent();
    // Перечитать размер текущей коллекции
    void updateCurrentCollectionSize();

private:
    // обновить текущий тип объекта
    bool updateObjectType();
    // фукнция проверки объекта на возможность сериализациии
    bool checkObject(ISerializable* object);

private:
    // Режим работы сериализатора
    bool m_bSkipEmptyCollections;
    // тип текущего свойства
    ObjectType m_currentObjectType = ObjectType::eCollectionStart;
    // текущий сериализуемый объект
    ISerializablePtr m_currentSerializableObject;

    // информация о коллекции в которой идет итерация
    struct CollectionInfo
    {
        // Указатель на коллекцию
        ISerializableCollection* pCollection;
        // Рамзер коллекции
        size_t sizeOfCollection;
        // Текущий индекс в коллекции
        size_t currentIndexInCollection;

        CollectionInfo(ISerializableCollection* pCollection_)
            : pCollection(pCollection_)
            , sizeOfCollection(pCollection_->getObjectsCount())
            , currentIndexInCollection(0L)
        {}
    };

    // текущая грубина во вложенных коллекциях, последний элемент - текущий уровень
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

    // если есть необходимое количество элементов в коллекции
    if ((!bCollectionStart && !m_collectionsDepth.empty()) ||
        (bCollectionStart && m_collectionsDepth.size() >= 2))
    {
        CString curObjectName;
        if (!m_currentSerializableObject->getObjectName(&curObjectName))
            return 0;

        decltype(m_collectionsDepth)::reverse_iterator collectionInfoIt;
        collectionInfoIt = m_collectionsDepth.rbegin();

        // для начала коллекции берем предпоследний
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
            // если коллекция пустая - говорим что дошли до её конца
            m_currentObjectType = eCollectionEnd;
            return true;
        }

        m_currentSerializableObject = pCollection->getObject(0);

        if (!updateObjectType())
        {
            // переходим к след свойству если не смогли определить тип
            m_currentObjectType = ObjectType::eProperty;
            return goToNextObject();
        }

        // проверяем что объект сериализуется и все хорошо
        if (!checkObject(m_currentSerializableObject))
            // если он не сериализуется - переходим к следующему объекту
            return goToNextObject();
    }
    break;
    case ObjectType::eCollectionEnd:
    {
        // выходим из предыдущего уровня коллекции
        if (!m_collectionsDepth.empty())
            m_collectionsDepth.pop_back();

        // коллекции закончились
        if (m_collectionsDepth.empty())
        {
            m_currentSerializableObject = nullptr;
            return false;
        }

        // переходим к след свойству в коллекции
        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        m_currentSerializableObject =
            collectionInfo.pCollection->getObject(collectionInfo.currentIndexInCollection);
        m_currentObjectType = ObjectType::eProperty;
        return goToNextObject();
    }
    break;
    case ObjectType::eProperty:
    {
        // случай когда сериализуется всего одно свойство у объекта
        if (m_collectionsDepth.empty())
            return false;

        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        // проеряем что в коллекции есть подходящий для сериализации объект
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
            // обновляем тип свойства
            if (!updateObjectType())
            {
                // переходим к след свойству если не смогли определить тип
                m_currentObjectType = ObjectType::eProperty;
                return goToNextObject();
            }
        }
        else
        {
            // дошли до конца коллекции
            m_currentObjectType = ObjectType::eCollectionEnd;
            m_currentSerializableObject = collectionInfo.pCollection;
        }
    }
    break;
    default:
        assert(!"Не известный тип сериализуемого объекта");
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------//
void SerializableObjectsVisitorImpl::skipCollectionContent()
{
    // говорим что мы в конце коллекции
    m_currentObjectType = ObjectType::eCollectionEnd;
}

//----------------------------------------------------------------------------//
void SerializableObjectsVisitorImpl::updateCurrentCollectionSize()
{
    if (m_collectionsDepth.empty())
        assert(!"Обновляется размер не существующей коллекции!");

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

            // добавляем в список коллекций новый элемент
            m_collectionsDepth.emplace_back(pCollection);
        }
        else
        {
            assert(!"Неизвестный тип сериализуемого объекта");
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------//
bool SerializableObjectsVisitorImpl::checkObject(ISerializable* object)
{
    // пропускаем не сериализуемые объекты
    if (object && object->getObjectName())
    {
        if (m_bSkipEmptyCollections)
        {
            // если необходимо пропускать пустые коллекции
            ISerializableCollectionPtr pCollection = object;
            if (pCollection)
            {
                // если это коллекция - проверяем что в ней есть сериализуемые объекты
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
        // не передали чем/что сериализовать
        assert(false);
        return false;
    }

    // Начинаем сериализацию
    if (!pSerializator->onSerializatingStart() ||
        !serializeObjectImpl(pSerializator.get(), pSerializableObject))
    {
        // нет объектов для сериализации
        assert(false);
        return false;
    }

    // Завершаем сериализацию
    return pSerializator->onSerializatingEnd();
}

//----------------------------------------------------------------------------//
bool SerializationExecutor::deserializeObject(IDeserializator::Ptr pDeserializator,
                                              ISerializablePtr pSerializableObject)
{
    if (!pDeserializator || !pSerializableObject)
    {
        // не передали чем/что десериализовать
        assert(false);
        return false;
    }

    // Начинаем десериализацию
    if (!pDeserializator->onDeserializatingStart())
        return false;

    if (!deserializeObjectImpl(pDeserializator.get(), pSerializableObject))
    {
        // нет объектов для десериализации
        assert(false);
        return false;
    }

    // Завершаем десериализацию
    return pDeserializator->onDeserializatingEnd();
}

//----------------------------------------------------------------------------//
bool SerializationExecutor::serializeObjectImpl(ISerializator* pSerializator,
                                                ISerializablePtr pSerializableObject)
{
    SerializableObjectsVisitorImpl objectsVisitor(
        SerializableObjectsVisitorImpl::WorkMode::eSerialization);
    if (!objectsVisitor.initVisitor(pSerializableObject))
        // нет объектов для сериализации
        return false;

    do
    {
        // Вызываем соответствующую функцию сериализатора в зависимости от объекта
        switch (objectsVisitor.getObjectType())
        {
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionStart:
                {
                    ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();
                    if (pCollection->onStartSerializing())
                        pSerializator->writeCollectionStart(pCollection);
                    else
                        // пропускаем всю коллекцию
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
                assert(!"Попытка сериализовать в XML объект неизвестного типа");
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
        // нет подходящих для десериализации объектов
        return false;

    do
    {
        // Вызываем соответствующую функцию сериализатора в зависимости от объекта
        switch (objectsVisitor.getObjectType())
        {
            case SerializableObjectsVisitorImpl::ObjectType::eCollectionStart:
            {
                ISerializableCollectionPtr pCollection = objectsVisitor.getCurrentObject();
                if (!pDeserializator->readCollectionStart(objectsVisitor.getIndexAmongIdenticalNames(true),
                                                          pCollection))
                {
                    // если коллекция пустая значит данных могло не быть
                    //assert(false);
                    // пропускаем всю коллекцию
                    objectsVisitor.skipCollectionContent();
                }
                else
                {
                    // получаем список сериализованных объектов в коллекции
                    std::list<CString> objNames;
                    if (pDeserializator->readCollectionObjectNames(pCollection, objNames))
                    {
                        if (pCollection->onStartDeserializing(objNames))
                            // обновляем размер коллекции, он мог измениться после onStartDeserializing
                            objectsVisitor.updateCurrentCollectionSize();
                        else
                            // пропускаем всю коллекцию
                            objectsVisitor.skipCollectionContent();
                    }
                    else
                        // пропускаем всю коллекцию
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
                assert(!"Попытка сериализовать в XML объект неизвестного типа");
                break;
        }

    } while (objectsVisitor.goToNextObject());

    return true;
}
