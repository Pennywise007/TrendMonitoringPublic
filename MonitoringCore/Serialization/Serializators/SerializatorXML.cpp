#include "pch.h"

#include "SerializatorXML.h"

//****************************************************************************//
SerializatorXML::SerializatorXML(const CString& filePath)
    : m_filePath(filePath)
{}

#pragma region Serialization

//----------------------------------------------------------------------------//
// ISerializator
// Оповещение о начале сериализации.
bool SerializatorXML::onSerializatingStart()
{
    // сбрасываем все данные в XML документе
    m_hFile.reset();

    // говорим что текущая нода это начало XML документа
    m_currentNode = m_hFile;

    return true;
}

//----------------------------------------------------------------------------//
// ISerializator
// Оповещение об окончании сериализации.
bool SerializatorXML::onSerializatingEnd()
{
    // Сохраняем сериализуемый файл
    return m_hFile.save_file(m_filePath, PUGIXML_TEXT("    "),
                             pugi::format_default, pugi::encoding_utf8);
}

//----------------------------------------------------------------------------//
// ISerializator
// Сериализовать свойство.
void SerializatorXML::serializeProperty(ISerializablePropertyPtr pProperty)
{
    CString propertyName;
    if (!pProperty->getObjectName(&propertyName) || !m_currentNode)
    {
        assert(false);
        return;
    }

    setValue(addChildNode(*m_currentNode, propertyName), pProperty->serializePropertyValue());
}

//----------------------------------------------------------------------------//
// ISerializator
// Записать начало сериализуемой коллекции.
void SerializatorXML::writeCollectionStart(ISerializableCollectionPtr pCollection)
{
    CString collectionName;
    if (!pCollection->getObjectName(&collectionName) || !m_currentNode)
    {
        assert(false);
        return;
    }

    m_currentNode = addChildNode(*m_currentNode, collectionName);
}

//----------------------------------------------------------------------------//
// ISerializator
// Записать конец сериализуемой коллекции.
void SerializatorXML::writeCollectionEnd(ISerializableCollectionPtr pCollection)
{
    if (!m_currentNode)
    {
        assert(false);
        return;
    }

    m_currentNode = getParentNode(*m_currentNode);
}

#pragma endregion Serialization

#pragma region Deserialization

//----------------------------------------------------------------------------//
// IDeserializator
// Оповещение о начале десериализации.
bool SerializatorXML::onDeserializatingStart()
{
    // Открываем загруженный файл
    pugi::xml_parse_result parseRes = m_hFile.load_file(m_filePath, pugi::parse_default, pugi::encoding_utf8);

    m_currentNode.reset();

    return parseRes && m_hFile;
}

//----------------------------------------------------------------------------//
// IDeserializator
// Оповещение об окончании десериализации.
bool SerializatorXML::onDeserializatingEnd()
{
    return true;
}

//----------------------------------------------------------------------------//
// IDeserializator
// Десериализовать свойство.
bool SerializatorXML::deserializeProperty(size_t indexAmongIdenticalNames,
                                          ISerializablePropertyPtr pProperty)
{
    CString propertyName;
    if (!pProperty->getObjectName(&propertyName))
    {
        assert(false);
        return false;
    }

    // флаг что успешно прочитали начало коллекции
    bool bChildFound = checkConsistent(propertyName);

    if (m_currentNode.has_value() && *m_currentNode)
    {
        pugi::xml_node childNode;

        // если есть нода и ещё не спозиционировались на нужном элементе - переходим к нему
        if (!bChildFound)
            bChildFound = getChildNode(*m_currentNode, propertyName,
                                       indexAmongIdenticalNames, childNode);
        else
            childNode = *m_currentNode;

        if (bChildFound)
            pProperty->deserializePropertyValue(getValue(childNode));
        else
            // нет данных?
            assert(false);
    }
    else
        // нода должна быть
        assert(false);

    return bChildFound;
}

//----------------------------------------------------------------------------//
// IDeserializator
// Прочитать начало сериализуемой коллекции.
bool SerializatorXML::readCollectionStart(size_t indexAmongIdenticalNames,
                                          ISerializableCollectionPtr pCollection)
{
    CString collectionName;
    if (!pCollection->getObjectName(&collectionName))
    {
        assert(false);
        return false;
    }

    // флаг что успешно прочитали начало коллекции
    bool bResSuccessfulRead = checkConsistent(collectionName);

    if (m_currentNode.has_value() && *m_currentNode)
    {
        // если есть нода и ещё не спозиционировались на нужном элементе - переходим к нему
        if (!bResSuccessfulRead)
            bResSuccessfulRead = getChildNode(*m_currentNode, collectionName,
                                              indexAmongIdenticalNames, *m_currentNode);
    }
    else
        // нода должна быть
        assert(false);

    // если успешно нашли нужную коллекцию сообщаем сколько будет элементов
    return bResSuccessfulRead;
}

//----------------------------------------------------------------------------//
// IDeserializator
// рочитать конец сериализуемой коллекции.
void SerializatorXML::readCollectionEnd(ISerializableCollectionPtr pCollection)
{
    CString collectionName;
    if (!isCheckCanReadObject(pCollection, collectionName))
        return;

    if (m_currentNode->name() == collectionName)
    {
        m_currentNode = getParentNode(*m_currentNode);
    }
    else
        // пытаются выйти из ноды с другим именем
        assert(false);
}

//----------------------------------------------------------------------------//
bool SerializatorXML::readCollectionObjectNames(ISerializableCollectionPtr pCollection,
                                                std::list<CString>& objectNames)
{
    CString collectionName;
    if (!isCheckCanReadObject(pCollection, collectionName))
        return false;

    // если текущая нода и есть коллекция смотрим ее детей
    if (m_currentNode->name() == collectionName)
    {
        getChildNodeNames(*m_currentNode, objectNames);
        return true;
    }
    else
        // должны были спозиционироваться внутри ноды коллекции
        assert(false);

    return false;
}

#pragma endregion Deserialization

//----------------------------------------------------------------------------//
bool SerializatorXML::isCheckCanReadObject(ISerializablePtr obj, CString& objName)
{
    if (!m_currentNode.has_value() || !*m_currentNode || !obj->getObjectName(&objName))
    {
        assert(false);
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------//
bool SerializatorXML::checkConsistent(const CString& objName)
{
    if (!m_currentNode.has_value())
    {
        // если успешно загрузили - говорим что текущая нода это начало XML документа
        m_currentNode = m_hFile.document_element();

        assert(*m_currentNode);
        if (!*m_currentNode)
            // файл пуст
            return false;

        // возможно начало файла и есть наша коллекция
        return m_currentNode->name() == objName;
    }

    return false;
}


/*
//----------------------------------------------------------------------------//
// ISerializationFabric
bool SerializationXML::loadConfiguration(ISerializablePtr& serializableObj)
{
    CString xmlFilaPath;
    if (!getXMLFilePath(serializableObj, xmlFilaPath))
        return false;

    // Функция получения значение из ноды
    auto fGetChildValueFromNode = [](pugi::xml_node xmlNode,
                                     const CString &childName,
                                     auto *childVal) -> bool
    {
        bool bRes(childVal != nullptr);
        pugi::xml_node childNode = xmlNode.child(childName).first_child();
        bRes &= (childNode != nullptr);
        if (bRes)
            *childVal =
            string_to_value<std::remove_pointer<decltype(childVal)>::type>(childNode.value());

        return bRes;
    };

    // Функция получения значения атрибута из ноды
    auto fGetAttributeFromNode = [](pugi::xml_node xmlNode,
                                    const CString &attributeName,
                                    auto *attributeVal) -> bool
    {
        bool bRes(attributeVal != nullptr);
        pugi::xml_attribute attributeNode = xmlNode.attribute(attributeName);
        bRes &= (attributeNode != nullptr);
        if (bRes)
            *attributeVal =
            atribute_to_value<std::remove_pointer<decltype(attributeVal)>::type>(attributeNode);

        return bRes;
    };

    return true;
}

//----------------------------------------------------------------------------//
// ISerializationFabric
bool SerializationXML::saveConfiguration(const ISerializablePtr serializableObj)
{
    CString xmlFilaPath;
    if (!getXMLFilePath(serializableObj, xmlFilaPath))
        return false;

    pugi::xml_document hFile;
    pugi::xml_node MainNode = hFile.append_child(L"Config");

    // Функция добавления дочернего элемента в ноду
    auto fAddChildToNode = [](pugi::xml_node xmlNode,
                              const CString &childName,
                              const auto &childVal)
    {
        xmlNode.append_child(childName)
            .append_child(pugi::node_pcdata)
            .set_value(value_to_sting(childVal).c_str());
    };

    // Функция добавления атрибута
    auto fAddAttributeToChild = [](pugi::xml_node xmlNode,
                                   const CString &attributeName,
                                   const auto &attributeVal)
    {
        xmlNode.append_attribute(attributeName)
            .set_value(value_to_sting(attributeVal).c_str());
    };


    return true;
}
*/

//----------------------------------------------------------------------------//
pugi::xml_node SerializatorXML::getParentNode(const pugi::xml_node& xmlNode) const
{
    return xmlNode.parent();
}

//----------------------------------------------------------------------------//
void SerializatorXML::getChildNodeNames(const pugi::xml_node& xmlNode,
                                        std::list<CString>& childNames)
{
    pugi::xml_object_range<pugi::xml_node_iterator> childs = xmlNode.children();

    childNames.clear();

    for (auto it = childs.begin(), end = childs.end(); it != end; ++it)
        childNames.emplace_back(it->name());
}

//----------------------------------------------------------------------------//
pugi::xml_node SerializatorXML::addChildNode(pugi::xml_node xmlNode,
                                              const CString& nodeName)
{
    return xmlNode.append_child(nodeName);
}

//----------------------------------------------------------------------------//
bool SerializatorXML::getChildNode(const pugi::xml_node& xmlNode,
                                   const CString& childName,
                                   const size_t childIndex,
                                   pugi::xml_node& childNode) const
{
    pugi::xml_object_range<pugi::xml_named_node_iterator>
        childenNodes = xmlNode.children(childName);

    pugi::xml_node child;

    if (childIndex < (size_t)std::distance(childenNodes.begin(), childenNodes.end()))
        child = *std::next(childenNodes.begin(), childIndex);
    else
        return false;

    if (child == nullptr)
        return false;

    childNode = child;
    return true;
}

//----------------------------------------------------------------------------//
void SerializatorXML::setValue(pugi::xml_node xmlNode, const CString& value)
{
    xmlNode.append_child(pugi::node_pcdata).set_value(value);
}

//----------------------------------------------------------------------------//
CString SerializatorXML::getValue(const pugi::xml_node& xmlNode) const
{
    return xmlNode.child_value();
}

//----------------------------------------------------------------------------//
void SerializatorXML::setAttributeValue(pugi::xml_node xmlNode,
                                         const CString& attributeName,
                                         const CString& attributeValue)
{
    xmlNode.append_attribute(attributeName)
        .set_value(attributeValue);
}

//----------------------------------------------------------------------------//
bool SerializatorXML::getAttributeValue(pugi::xml_node xmlNode,
                                         const CString& attributeName,
                                         CString& attributeValue) const
{
    pugi::xml_attribute xmlAttribute = xmlNode.attribute(attributeName);
    bool bAttributeExist = (xmlAttribute != nullptr);
    if (bAttributeExist)
        attributeValue = xmlAttribute.as_string();

    return bAttributeExist;
}