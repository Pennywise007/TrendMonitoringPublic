#pragma once

#include <optional>

#include <pugixml.hpp>

#include "Serialization/SerializatorFabric.h"

////////////////////////////////////////////////////////////////////////////////
// Сериализатор объектов в XML
class SerializatorXML
    : public ISerializator
    , public IDeserializator
{
public:
    SerializatorXML(const CString& filePath);

// ISerializator
public:
    /// <summary>Оповещение о начале сериализации.</summary>
    /// <returns>Истину в случае успешного начала сериализации.</returns>
    bool onSerializatingStart() override;
    /// <summary>Оповещение об окончании сериализации.</summary>
    /// <returns>Истину в случае успешного окончания сериализации.</returns>
    bool onSerializatingEnd() override;
    /// <summary>Сериализовать свойство.</summary>
    /// <param name="pProperty">Сериализуемое свойство.</param>
    void serializeProperty(ISerializablePropertyPtr pProperty) override;
    /// <summary>Записать начало сериализуемой коллекции.</summary>
    /// <param name="pCollection">Сериализуемая коллекция.</param>
    void writeCollectionStart(ISerializableCollectionPtr pCollection) override;
    /// <summary>Записать конец сериализуемой коллекции.</summary>
    /// <param name="pCollection">Сериализуемая коллекция.</param>
    void writeCollectionEnd(ISerializableCollectionPtr pCollection) override;

// IDeserializator
public:
    /// <summary>Оповещение о начале десериализации.</summary>
    /// <returns>Истину в случае успешного начала десериализации.</returns>
    bool onDeserializatingStart() override;
    /// <summary>Оповещение об окончании десериализации.</summary>
    /// <returns>Истину в случае успешного окончания десериализации.</returns>
    bool onDeserializatingEnd() override;
    /// <summary>Десериализовать свойство.</summary>
    /// <param name="indexAmongIdenticalNames">Индекс среди одинаковых имен в коллекции.</param>
    /// <param name="pProperty">Десериализуемое свойство.</param>
    /// <returns>Истину в случае успешной десериализаци.</returns>
    bool deserializeProperty(size_t indexAmongIdenticalNames,
                             ISerializablePropertyPtr pProperty) override;
    /// <summary>Прочитать начало сериализуемой коллекции.</summary>
    /// <param name="indexAmongIdenticalNames">Индекс среди одинаковых имен в коллекции.</param>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    bool readCollectionStart(size_t indexAmongIdenticalNames,
                             ISerializableCollectionPtr pCollection) override;
    /// <summary>Прочитать конец сериализуемой коллекции.</summary>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    void readCollectionEnd(ISerializableCollectionPtr pCollection) override;
    /// <summary>Прочитать имена сериализованных элементов коллекции.</summary>
    /// <param name="pCollection">Десериализуемая коллекция.</param>
    /// <param name="objectNames">Имена объектов в коллекции.</param>
    /// <returns>Истину в случае успешного чтения.</returns>
    bool readCollectionObjectNames(ISerializableCollectionPtr pCollection,
                                   std::list<CString>& objectNames) override;

private:
    // Проверка возможности вычитать объект
    bool isCheckCanReadObject(ISerializablePtr obj, CString& objName);

    // проверка на случай если ещё не было позиционирования внутри вычитанного файла
    // проверяем что первая нода в файле и есть сериализуемый объект
    bool checkConsistent(const CString& objName);

private:
    // Получить родительскую ноду
    pugi::xml_node getParentNode(const pugi::xml_node& xmlNode) const;
    // получить имена дочерних нод
    void getChildNodeNames(const pugi::xml_node& xmlNode,
                           std::list<CString>& childNames);

    // Добавление дочерней ноды
    pugi::xml_node addChildNode(pugi::xml_node xmlNode, const CString& nodeName);
    // Поиск дочерней ноды по имени
    bool getChildNode(const pugi::xml_node& xmlNode,
                      const CString& childName,
                      const size_t childIndex,
                      pugi::xml_node& childNode) const;

    // Установить значение в ноду
    void setValue(pugi::xml_node xmlNode, const CString& value);
    // Получения значение из ноды
    CString getValue(const pugi::xml_node& xmlNode) const;

    // Добавление значения атрибута в ноду по имени
    void setAttributeValue(pugi::xml_node xmlNode,
                           const CString& attributeName,
                           const CString& attributeValue);
    // Получение значения атрибута из ноды
    bool getAttributeValue(pugi::xml_node xmlNode,
                           const CString& attributeName,
                           CString& attributeValue) const;

private:
    // Путь к XML файлу куда будет происходить сохранение/чтение данных
    CString m_filePath;

    // XML документ в котором происходит сериализация/десериализация
    pugi::xml_document m_hFile;

    // Текущая нода
    std::optional<pugi::xml_node> m_currentNode;
};
