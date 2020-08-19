#pragma once

/*
    Интерфейсы и фабрика для загрузки/выгрузки/передачи данных
    В настоящий момент используется только для выгрузки зугрузки в XML
    При необходимости может быть расширен для сохранения настроек в БД/JSON/Файл
*/

#include "ISerializable.h"

#include <memory>

//****************************************************************************//
/// Интерфейс сериализатора объекта
DECLARE_INTERFACE(ISerializator)
{
    typedef std::shared_ptr<ISerializator> Ptr;

// Оповещения о начале/окончании работы сериализатора
public:
    /// <summary>Оповещение о начале сериализации.</summary>
    /// <returns>Истину в случае успешного начала сериализации.</returns>
    virtual bool onSerializatingStart() = 0;
    /// <summary>Оповещение об окончании сериализации.</summary>
    /// <returns>Истину в случае успешного окончания сериализации.</returns>
    virtual bool onSerializatingEnd() = 0;

// Свойство
public:
    /// <summary>Сериализовать свойство.</summary>
    /// <param name="pProperty">Сериализуемое свойство.</param>
    virtual void serializeProperty(ISerializablePropertyPtr pProperty) = 0;

// Коллекция
public:
    /// <summary>Записать начало сериализуемой коллекции.</summary>
    /// <param name="pCollection">Сериализуемая коллекция.</param>
    virtual void writeCollectionStart(ISerializableCollectionPtr pCollection) = 0;
    /// <summary>Записать конец сериализуемой коллекции.</summary>
    /// <param name="pCollection">Сериализуемая коллекция.</param>
    virtual void writeCollectionEnd(ISerializableCollectionPtr pCollection) = 0;
};

//****************************************************************************//
/// Интерфейс десериализатора объекта
DECLARE_INTERFACE(IDeserializator)
{
    typedef std::shared_ptr<IDeserializator> Ptr;

// Оповещения о начале/окончании работы десериализатора
public:
    /// <summary>Оповещение о начале десериализации.</summary>
    /// <returns>Истину в случае успешного начала десериализации.</returns>
    virtual bool onDeserializatingStart() = 0;
    /// <summary>Оповещение об окончании десериализации.</summary>
    /// <returns>Истину в случае успешного окончания десериализации.</returns>
    virtual bool onDeserializatingEnd() = 0;

// Свойство
public:
    /// <summary>Десериализовать свойство.</summary>
    /// <param name="indexAmongIdenticalNames">Индекс среди одинаковых имен в коллекции.</param>
    /// <param name="pProperty">Десериализуемое свойство.</param>
    /// <returns>Истину в случае успешной десериализаци.</returns>
    virtual bool deserializeProperty(size_t indexAmongIdenticalNames,
                                     ISerializablePropertyPtr pProperty) = 0;

// Коллекция
public:
    /// <summary>Прочитать начало сериализуемой коллекции.</summary>
    /// <param name="indexAmongIdenticalNames">Индекс среди одинаковых имен в коллекции.</param>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    virtual bool readCollectionStart(size_t indexAmongIdenticalNames,
                                     ISerializableCollectionPtr pCollection) = 0;
    /// <summary>Прочитать конец сериализуемой коллекции.</summary>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    virtual void readCollectionEnd(ISerializableCollectionPtr pCollection) = 0;
    /// <summary>Прочитать имена сериализованных элементов коллекции.</summary>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <param name="objectNames">Имена объектов в коллекции.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    virtual bool readCollectionObjectNames(ISerializableCollectionPtr pCollection,
                                           std::list<CString>& objectNames) = 0;
};

//****************************************************************************//
// Фабрика создания интерфейсов для (де)сериализации объектов
class SerializationFabric
{
public:
    /// <summary>Создание сериализатора для XML файла по пути к файлу.</summary>
    /// <param name="filePath">Путь к xml файлу.</param>
    /// <returns>Интерфейс сериализатора в XML.</returns>
    static ISerializator::Ptr createXMLSerializator(const CString& filePath);
    /// <summary>Создание десериализатора для XML файла по пути к файлу.</summary>
    /// <param name="filePath">Путь к xml файлу.</param>
    /// <returns>Интерфейс десериализатора в XML.</returns>
    static IDeserializator::Ptr createXMLDeserializator(const CString& filePath);
};

//****************************************************************************//
// Класс сериализующий объекты
class SerializationExecutor
{
public:
    /// <summary>Сериализация объекта.</summary>
    /// <param name="pSerializator">Сериализатор объектов.</param>
    /// <param name="pSerializableObject">Сериализуемый объект.</param>
    /// <returns>Истину в случае успешной сериализации объекта.</returns>
    static bool serializeObject(ISerializator::Ptr pSerializator,
                                ISerializablePtr pSerializableObject);

    /// <summary>Десериализация объекта.</summary>
    /// <param name="filePath">Путь к xml файлу.</param>
    /// <returns>Интерфейс сериализатора в XML.</returns>
    static bool deserializeObject(IDeserializator::Ptr pDeserializator,
                                  ISerializablePtr pSerializableObject);

private:
    // Внутренняя функция сериализации объекта, используется для сериализации вложенных объектам
    static bool serializeObjectImpl(ISerializator* pSerializator,
                                    ISerializablePtr pSerializableObject);
    // Внутренняя функция десериализации объекта, используется для десериализации вложенных объектов
    static bool deserializeObjectImpl(IDeserializator* pDeserializator,
                                      ISerializablePtr pSerializableObject);
};
