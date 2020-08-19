#pragma once

/*
    Описание интерфейсов сериализуемых объектов

    Реализация базовызх классов для упрощения сериализации/десериализации данных
    По сути - попытка создать рефлексию в С++

Использование:
    DECLARE_COM_INTERNAL_CLASS_(ApplicationConfiguration, SerializableObjectsCollection)
    {
    public:
        ApplicationConfiguration() = default;

        // Объявляем все свойства которые будем сериализовать/десериализовать
        BEGIN_SERIALIZABLE_PROPERTIES(ApplicationConfiguration)
            ADD_PROPERTY(m_myValueBool)
            ADD_CONTAINER(m_listOfParams)
        END_SERIALIZABLE_PROPERTIES()

        // объявление свойства m_myPropInt и добавление его в список сериализуемых
        DECLARE_PROPERTY((int) m_myPropInt);
        // объявление свойства m_myPropLong и добавление его в список сериализуемых
        // инициализирует m_myPropLong значением после запятой (m_myPropLong = 55;)
        DECLARE_PROPERTY((long) m_myPropLong, 55);

    public:
        // сериализуемые свойства
        bool m_myValueBool;
        std::list<int> m_listOfParams;
    }
*/

#include <afx.h>
#include <algorithm>
#include <combaseapi.h>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include <COM.h>

#include "SerializationUtils.h"

#pragma region SerializationInterfaces

/// Общий интерфейс сериализуемого объекта
DECLARE_COM_INTERFACE(ISerializable, "515CB11E-2B96-4685-9555-69B28A89B830")
{
    /// <summary>Получает имя объекта.</summary>
    /// <param name="name">Имя объекта.</param>
    /// <returns>True если ообъект нужно (де)серилизовать, false - пропустить.</returns>
    virtual bool getObjectName(CString* name = nullptr) = 0;
};

/// Интерфейс сериализуемого свойства
DECLARE_COM_INTERFACE_(ISerializableProperty, ISerializable, "122BD471-1669-409A-9902-A0DAC9872C99")
{
    /// <summary>Сериализация свойства.</summary>
    /// <returns>Строковое представление свойства.</returns>
    virtual CString serializePropertyValue() = 0;
    /// <summary>Десериализация свойства.</summary>
    /// <param name="value">Строковое представление десериализуемого свойства.</param>
    virtual void    deserializePropertyValue(const CString& value) = 0;
};

/// Интерфейс коллекции сериализуемых объектов
DECLARE_COM_INTERFACE_(ISerializableCollection, ISerializable, "55F5C6DE-2E50-4516-8699-D733A02EC115")
{
    /// <summary>Получение количества объектов в коллекции.</summary>
    virtual size_t getObjectsCount() = 0;
    /// <summary>Получает объект из коллекции по индексу.</summary>
    /// <param name="index">Индекс в коллекции.</param>
    /// <returns>Сериализуемый объект.</returns>
    virtual ISerializablePtr getObject(const size_t index) = 0;

    /// <summary>Оповещает о начале сериализации коллекции.</summary>
    /// <returns>true если коллекцию можно сериализовать.</returns>
    virtual bool onStartSerializing() = 0;
    /// <summary>Оповещает об окончании сериализации коллекции.</summary>
    virtual void onEndSerializing() = 0;

    /// <summary>Оповещает о начале десериализации, передает список сохраненных при сериализации имен объектов.</summary>
    /// <param name="objNames">Имена объектов которые были сохранены при сериализации.</param>
    /// <returns>true если коллекцию можно десериализовать.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) = 0;
    /// <summary>Оповещает о конце десериализации коллекции.</summary>
    virtual void onEndDeserializing() = 0;
};

#pragma endregion SerializationInterfaces

////////////////////////////////////////////////////////////////////////////////
// Вспомогательные макросы для объявления сериализуемых объектов
// используются при наследовании от SerializableObjectsCollection
#pragma region SerializationMacros

//----------------------------------------------------------------------------//
// Объявление сериализуемых свойств через функцию базового класса

#define BEGIN_SERIALIZABLE_PROPERTIES(CollectionName) \
virtual CString declareSerializableProperties() override\
{\
    CString collectionName = L#CollectionName;

// добавление (де)сериализуемого свойства простого типа
#define ADD_PROPERTY(Property) \
    addProperty<decltype(Property)>(L#Property, &Property);

// добавление (де)сериализуемого контейнера простых типов (std::vector|set|map<long> и т.п)
#define ADD_CONTAINER(Container) \
    addSTDContainer<decltype(Container)>(L#Container, &Container);

// добавление объектов наследующих ISerializableCollection/ISerializableProperty
#define ADD_SERIALIZABLE(Serializable) \
    addSerializable(Serializable.address());

// передает вызов регистрации сериализуемых объектов в базовый класс
#define CHAIN_SERIALIZABLE_PROPERTIES(BaseClass) \
    /* если не передали имя коллекции - берем у баозового класса */ \
    if (auto baseClassName = BaseClass::declareSerializableProperties();    \
        collectionName.IsEmpty()) \
        collectionName = baseClassName;

#define END_SERIALIZABLE_PROPERTIES() \
    return collectionName; \
}

//----------------------------------------------------------------------------//
// Макросы с декларацией свойств и добавлением их сразу в список сериализуемых при
// отработке конструктора. Позволяет уменьшить количество используемого кода.

// Объявление имени сериализуемой коллекции
#define DECLARE_COLLECTION_NAME(CollectionName)                 \
virtual CString declareSerializableProperties() override        \
{ return CollectionName; }

// Объявление сериализуемой коллекции
#define DECLARE_COLLECTION(Collection) DECLARE_COLLECTION_NAME(L#Collection);

// Декларация сериализуемого свойства и регистрация его в базовом классе
// Вариант использования DECLARE_PROPERTY((long) m_propName); => long m_propName = long();
// Вариант использования DECLARE_PROPERTY((long) m_propName, 105); => long m_propName = long(105);
// !!Наличие пробела после скобки с типом - обязательно
// !!Нельзя использовать конструктор у этой переменной
#define DECLARE_PROPERTY(Property, ...) \
DECLARE_OBJECT(Property) =              \
REGISTER_SERIALIZABLE_OBJECT(GET_OBJECT(Property), registerProperty, __VA_ARGS__);

// Декларация сериализуемого контейнера и регистрация его в базовом классе
// Вариант использования DECLARE_CONTAINER((std::vector<long>) m_propName); => std::vector<long> m_propName;
// !!Наличие пробела после скобки с типом - обязательно
// !!Нельзя использовать конструктор у этого контейнера
#define DECLARE_CONTAINER(Container, ...)   \
DECLARE_OBJECT(Container) =                 \
REGISTER_SERIALIZABLE_OBJECT(GET_OBJECT(Container), registerSTDContainer, __VA_ARGS__)

// Декларация сериализуемого объекта и регистрация его в базовом классе
// Вариант использования:
// DECLARE_SERIALIZABLE((TestSerializing::Ptr) m_serializingClass,
//                      TestSerializing::create()); =>
// TestSerializing::Ptr m_serializingClass = TestSerializing::create();
//
// !!Наличие пробела после скобки с типом - обязательно
// !!Необходимо использовать конструктор у этой переменной
#define DECLARE_SERIALIZABLE(Serializable, ...) \
DECLARE_OBJECT(Serializable) =                  \
registerSerializable<                           \
    extract_value_type_v<                       \
        decltype(GET_OBJECT(Serializable)) > >  \
            ([this](){ return GET_OBJECT(Serializable).address(); }, __VA_ARGS__);

#pragma endregion SerializationMacros

#pragma region SerializableClasses

////////////////////////////////////////////////////////////////////////////////
/// Базовый класс для сериализуемого свойства
template <class T>
DECLARE_COM_BASE_CLASS(SerializablePropertyBase)
    , public ISerializableProperty
{
public:
    SerializablePropertyBase(T* pProperty, const CString& propName)
        : m_pProperty(pProperty)
        , m_propName(propName)
    {}

    BEGIN_COM_MAP(SerializablePropertyBase<T>)
        COM_INTERFACE_ENTRY(ISerializable)
        COM_INTERFACE_ENTRY(ISerializableProperty)
    END_COM_MAP()

// ISerializable
protected:
    /// <summary>Получает имя объекта.</summary>
    /// <param name="propertyName">Имя объекта.</param>
    /// <returns>True если ообъект нужно (де)серилизовать, false - пропустить.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    { if (name) *name = m_propName; return true; }

// ISerializableProperty
protected:
    /// <summary>Сериализация свойства.</summary>
    /// <param name="propVersion">версия сериализуемого свойства.</param>
    /// <returns>Строковое представление свойства.</returns>
    virtual CString serializePropertyValue() override
    { return value_to_sting<T>(*m_pProperty); }
    /// <summary>Десериализация свойства.</summary>
    /// <param name="propVersion">Версия сериализуемого свойства.</param>
    /// <param name="value">Строковое представление десериализуемого свойства.</param>
    virtual void deserializePropertyValue(const CString& value) override
    { *m_pProperty = string_to_value<T>(value); }

protected:
    // Имя свойства
    CString m_propName;
    // Указатель на свойство
    T* m_pProperty;
};

////////////////////////////////////////////////////////////////////////////////
/// Реализация внутреннего класса свойства, хранит указатель на свойство
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializablePropertyImpl, SerializablePropertyBase<T>, T)
{
public:
    SerializablePropertyImpl(T* pProperty, const CString& propName)
        : SerializablePropertyBase<T>(pProperty, propName)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// Реализация внутреннего класса свойства с возможностью хранения значение свойства
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializablePropertyValue, SerializablePropertyBase<T>, T)
{
public:
    SerializablePropertyValue(const T& value, const CString& propName)
        : SerializablePropertyBase<T>(nullptr, propName)
        , m_value(value)
    { SerializablePropertyBase<T>::m_pProperty = &m_value; }

public:
    // сохраненное значение, оно будет (де)сериализоваться базовым классом
    T m_value;
};

////////////////////////////////////////////////////////////////////////////////
// Базовый класс для коллекций
template <class T>
DECLARE_COM_BASE_CLASS(SerializableCollectionBase)
    , public ISerializableCollection
{
public:
    SerializableCollectionBase(T* pCollectionContainer,
                               const CString& containerName)
        : m_pCollectionContainer(pCollectionContainer)
        , m_collectionName(containerName)
    {}

    virtual ~SerializableCollectionBase() = default;

    BEGIN_COM_MAP(SerializableCollectionBase<T>)
        COM_INTERFACE_ENTRY(ISerializable)
        COM_INTERFACE_ENTRY(ISerializableCollection)
    END_COM_MAP()

// ISerializable
protected:
    /// <summary>Получает имя объекта.</summary>
    /// <param name="name">Имя объекта.</param>
    /// <returns>True если ообъект нужно (де)серилизовать, false - пропустить.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    { if (name) *name = m_collectionName; return true; }

// ISerializableCollection
protected:
    /// <summary>Получение количества объектов в коллекции.</summary>
    virtual size_t getObjectsCount() override { return m_pCollectionContainer->size(); }
    /// <summary>Получает объект из коллекции по индексу.</summary>
    /// <param name="index">Индекс в коллекции.</param>
    /// <returns>Сериализуемый объект.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (index >= getObjectsCount())
        {
            assert(!"Выход за пределы количества элементов в контейнере");
            return nullptr;
        }

        auto it = std::next(m_pCollectionContainer->begin(), index);
        // для map контейнера возвращаем его mapped_type
        if constexpr (isMapContainer<T>::value)
        {
            if constexpr (isSerializableType<T::mapped_type>::value)
                return it->second;
        }
        // Проверяем что это коллекция сериализуемых элементов
        else if constexpr (isSerializableType<T::value_type>::value)
            return *it;

        return nullptr;
    }
    /// <summary>Оповещает о начале сериализации.</summary>
    /// <returns>true если коллекцию можно сериализовать.</returns>
    virtual bool onStartSerializing() override { return true; }
    /// <summary>Оповещает об окончании сериализации коллекции.</summary>
    virtual void onEndSerializing()  override { }

    /// <summary>Оповещает о начале десериализации, передает список сохраненных при сериализации имен объектов.</summary>
    /// <param name="objNames">Имена объектов которые были сохранены при сериализации.</param>
    /// <returns>true если коллекцию можно десериализовать.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& /*objNames*/) override { return true; }
    /// <summary>Оповещает о конце десериализации коллекции.</summary>
    virtual void onEndDeserializing()  override { }

protected:
    // Структура проверяющая что заданный тип является сериализуемым(наследуется от ISerializable)
    template <class T>
    struct isSerializableType
    {
        // 1. Сначала вычленяем тип(если передали шаблонный)
        // 2. Убираем поинтер если это указатель
        // 3. Проверяем что наследуется от ISerializable
        static constexpr bool value =
            std::is_base_of_v<ISerializable,
            std::remove_pointer_t<
            extract_value_type_v<T>>>;
    };

protected:
    // Имя коллекции
    CString m_collectionName;
    // Указатель на контейнер с коллекцией
    T* m_pCollectionContainer;
};

////////////////////////////////////////////////////////////////////////////////
/// Реализация внутреннего класса коллекции
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializableCollectionImpl, SerializableCollectionBase<T>, T)
{
public:
    SerializableCollectionImpl(T* pCollectionContainer, const CString& containerName)
        : SerializableCollectionBase<T>(pCollectionContainer, containerName)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// Реализация внутреннего класса коллекции, в котором можно использовать stl контейнеры
/// Такие как map/vector/list стандартных типов
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializableSTLContainer, SerializableCollectionBase<T>, T)
{
public:
    typedef SerializableCollectionBase<T> BaseClass;
    SerializableSTLContainer(T* pCollectionContainer, const CString& containerName)
        : SerializableCollectionBase<T>(pCollectionContainer, containerName)
    {}

// ISerializableCollection
protected:
    /// <summary>Получает объект из коллекции по индексу.</summary>
    /// <param name="index">Индекс в коллекции.</param>
    /// <returns>Сериализуемый объект.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (index >= BaseClass::getObjectsCount())
        {
            assert(!"Выход за пределы количества элементов в контейнере");
            return nullptr;
        }

        auto it = std::next(BaseClass::m_pCollectionContainer->begin(), index);

        if constexpr (isMapContainer<T>::value)
        {
            // для map контейнера возвращаем его mapped_type
            if constexpr (BaseClass::isSerializableType<T::mapped_type>::value)
                // в таком случае с контейнером идет работа как с массивом
                return it->second;
            else
                // для контейнера map делаем спец обработку, у него ключ это имя ключа
                return SerializablePropertyImpl<T::mapped_type>::make(
                    &it->second, m_kNotSerializableAppendText + value_to_sting<T::key_type>(it->first));
        }
        // Проверяем что это коллекция сериализуемых элементов
        else if constexpr (BaseClass::isSerializableType<T::value_type>::value)
            // Если объект уже наследуется от ISerializable - возвращаем его
            return *it;
        else
        {
            if constexpr (std::is_same_v<std::set<T::value_type>, T> ||
                          std::is_same_v<std::multiset<T::value_type>, T>)
                // у std::set значение и ключ совпадают
                return SerializablePropertyValue<T::value_type>::make(
                    *it, m_kNotSerializableAppendText + value_to_sting(*it));
            else
                // Если объект обычного типа - оборачиваем в SerializablePropertyImpl
                return SerializablePropertyImpl<T::value_type>::make(
                    &*it, m_kNotSerializableAppendText + value_to_sting(index));
        }
    }

    /// <summary>Оповещает о начале десериализации, передает список сохраненных при сериализации имен объектов.</summary>
    /// <param name="objNames">Имена объектов которые были сохранены при сериализации.</param>
    /// <returns>true если коллекцию можно десериализовать.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) override
    {
        BaseClass::m_pCollectionContainer->clear();
        // готовим коллекцию с десериализации
        if constexpr (std::is_same_v<std::vector<T::value_type>, T> ||
                      std::is_same_v<std::list<T::value_type>, T>)
        {
            // если это контейнеры типа vector/list ресайзим их до количества объектов
            BaseClass::m_pCollectionContainer->resize(objNames.size());

            if constexpr (BaseClass::isSerializableType<T::value_type>::value)
            {
                // контейнер типа std::vector<ISerializable::Ptr>
                // создаем каждый объект
                for (auto& item : *BaseClass::m_pCollectionContainer)
                    // у объектов должен быть конструктор без параметров, иначе используйте
                    // отдельный класс SerializableCollectionBase
                    item = extract_value_type_v<T::value_type>::create();
            }
        }
        // если это set контейнеры
        else if constexpr (std::is_same_v<std::set<T::value_type>, T> ||
                           std::is_same_v<std::multiset<T::value_type>, T>)
        {
            // контейнер типа std::set<ISerializable>
            static_assert(!BaseClass::isSerializableType<T::value_type>::value,
                          "Container std::set<ISerializable> not implementeed, use SerializableCollectionBase");

            // у std::set objName это ключ, вставляем в коллекцию элементы
            for (auto& objName : objNames)
            {
                if (objName.Find(m_kNotSerializableAppendText) == 0)
                {
                    // получаем имя
                    CString setKeyTextVal = objName.Mid(m_kNotSerializableAppendText.GetLength());
                    BaseClass::m_pCollectionContainer->emplace(string_to_value<T::key_type>(setKeyTextVal));
                }
                else
                    assert(!"В начале имени у ключа std::set отсутсвует m_kNotSerializableAppendText");
            }
        }
        // если это map контейнеры
        else if constexpr (isMapContainer<T>::value)
        {
            // у std::map objName это ключ, вставляем в мапу пустое значение с нужным ключом
            for (auto& objName : objNames)
            {
                if (objName.Find(m_kNotSerializableAppendText) == 0)
                {
                    // получаем имя
                    CString mapKeyTextVal = objName.Mid(m_kNotSerializableAppendText.GetLength());

                    if constexpr (BaseClass::isSerializableType<T::mapped_type>::value)
                        // контейнер типа std::map<ValueType, ISerializable>,
                        // у объектов должен быть конструктор без параметров, иначе используйте
                        // отдельный класс SerializableCollectionBase
                        BaseClass::m_pCollectionContainer->try_emplace(
                            string_to_value<T::key_type>(mapKeyTextVal),
                            extract_value_type_v<T::value_type>::create());
                    else
                        BaseClass::m_pCollectionContainer->try_emplace(
                            string_to_value<T::key_type>(mapKeyTextVal),
                            T::mapped_type());
                }
                else
                    assert(!"В начале имени у ключа std::map отсутсвует m_kNotSerializableAppendText");
            }
        }
        else
        {
            static_assert(false, "Not implemented for this container type");
            return false;
        }

        return true;
    }

private:
    // Текст, дописываемый в начале строкового представления индексов коллекции
    // Использутся для того чтоб не было записей типо <0>Значение</0> могут быть ошибки при парсинге
    const CString m_kNotSerializableAppendText = L"ind_";
};

////////////////////////////////////////////////////////////////////////////////
// Класс который хранит адрес объекта реализуюшего ISerializableProperty и
// проксирующий все вызовы от интерфейсов в него
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE(SerializablePropertyProxy, T)
    , public ISerializableProperty
{
public:
    SerializablePropertyProxy(T** pPropertyAddress)
        : m_pPropertyAddress(pPropertyAddress)
    {}

    BEGIN_COM_MAP(SerializablePropertyProxy<T>)
        COM_INTERFACE_ENTRY(ISerializable)
        COM_INTERFACE_ENTRY(ISerializableProperty)
    END_COM_MAP()

// ISerializable
protected:
    /// <summary>Получает имя объекта.</summary>
    /// <param name="propertyName">Имя объекта.</param>
    /// <returns>True если ообъект нужно (де)серилизовать, false - пропустить.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    {
        if (ISerializablePtr pSerializableObj = *m_pPropertyAddress)
            return pSerializableObj->getObjectName(name);

        assert(!"Нет сериализуемого объекта!");
        return false;
    }

// ISerializableProperty
protected:
    /// <summary>Сериализация свойства.</summary>
    /// <param name="propVersion">версия сериализуемого свойства.</param>
    /// <returns>Строковое представление свойства.</returns>
    virtual CString serializePropertyValue() override
    {
        if (ISerializablePropertyPtr pPropertyObj = *m_pPropertyAddress)
            return pPropertyObj->serializePropertyValue();

        assert(!"Нет коллекции!");
        return CString();
    }
    /// <summary>Десериализация свойства.</summary>
    /// <param name="propVersion">Версия сериализуемого свойства.</param>
    /// <param name="value">Строковое представление десериализуемого свойства.</param>
    virtual void deserializePropertyValue(const CString& value) override
    {
        if (ISerializablePropertyPtr pPropertyObj = *m_pPropertyAddress)
            pPropertyObj->deserializePropertyValue(value);
        else
            assert(!"Нет свойства!");
    }

protected:
    // Адресс свойства
    T** m_pPropertyAddress;
};

////////////////////////////////////////////////////////////////////////////////
// Класс который хранит адрес объекта реализуюшего ISerializableCollection и
// проксирующий все вызовы от интерфейсов в него
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE(SerializableCollectionProxy, T)
    , public ISerializableCollection
{
public:
    SerializableCollectionProxy(T** pCollectionAddress)
        : m_pCollectionAddress(pCollectionAddress)
    {}

    BEGIN_COM_MAP(SerializableCollectionProxy<T>)
        COM_INTERFACE_ENTRY(ISerializable)
        COM_INTERFACE_ENTRY(ISerializableCollection)
    END_COM_MAP()

// ISerializable
protected:
    /// <summary>Получает имя объекта.</summary>
    /// <param name="propertyName">Имя объекта.</param>
    /// <returns>True если ообъект нужно (де)серилизовать, false - пропустить.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    {
        if (ISerializablePtr pSerializableObj = *m_pCollectionAddress)
            return pSerializableObj->getObjectName(name);

        assert(!"Нет сериализуемого объекта!");
        return false;
    }

// ISerializableCollection
protected:
    /// <summary>Получение количества объектов в коллекции.</summary>
    virtual size_t getObjectsCount() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->getObjectsCount();

        assert(!"Нет коллекции!");
        return 0;
    }
    /// <summary>Получает объект из коллекции по индексу.</summary>
    /// <param name="index">Индекс в коллекции.</param>
    /// <returns>Сериализуемый объект.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->getObject(index);

        assert(!"Нет коллекции!");
        return nullptr;
    }

    /// <summary>Оповещает о начале сериализации коллекции.</summary>
    /// <returns>true если коллекцию можно сериализовать.</returns>
    virtual bool onStartSerializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->onStartSerializing();

        assert(!"Нет коллекции!");
        return false;
    }

    /// <summary>Оповещает об окончании сериализации коллекции.</summary>
    virtual void onEndSerializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            pCollection->onEndSerializing();
        else
            assert(!"Нет коллекции!");
    }

    /// <summary>Оповещает о начале десериализации, передает список сохраненных при сериализации имен объектов.</summary>
    /// <param name="objNames">Имена объектов которые были сохранены при сериализации.</param>
    /// <returns>true если коллекцию можно десериализовать.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->onStartDeserializing(objNames);

        assert(!"Нет коллекции!");
        return false;
    }

    /// <summary>Оповещает о конце десериализации коллекции.</summary>
    virtual void onEndDeserializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            pCollection->onEndDeserializing();
        else
            assert(!"Нет коллекции!");
    }

protected:
    // Адресс свойства
    T** m_pCollectionAddress;
};

////////////////////////////////////////////////////////////////////////////////
/// Коллекция сериализуемых объектов, должна наследоваться COMInternal классом
/// Необходимо переопределеить declareSerializableProperties, см макросы
/// BEGIN_SERIALIZABLE_PROPERTIES / ADD_PROPERTY / ADD_COLLECTION ...
class ATL_NO_VTABLE SerializableObjectsCollection
    : public SerializableCollectionBase<std::list<ISerializablePtr>>
{
public:
    typedef SerializableCollectionBase<std::list<ISerializablePtr>> BaseClass;

    SerializableObjectsCollection(const CString& collectionName = CString())
        : BaseClass(&m_collectionSerializableProperties, collectionName)
    {}
    virtual ~SerializableObjectsCollection() = default;

    HRESULT FinalConstruct()
    {
        auto res = BaseClass::FinalConstruct();
        if (SUCCEEDED(BaseClass::FinalConstruct()))
        {
            if (CString res = declareSerializableProperties();
                BaseClass::m_collectionName.IsEmpty())
                BaseClass::m_collectionName = res;

            assert(!BaseClass::m_collectionName.IsEmpty() &&
                   "Пустое имя у сериализуемого объекта!");
        }

        return res;
    }

protected:
    /// <summary>Добавление свойства в коллекцию, используется в макросе ADD_PROPERTY.</summary>
    /// <param name="propName">Имя (де)сериализуемого свойства.</param>
    /// <param name="pProperty">Указатель на свойство.</param>
    template<class T>
    void addProperty(const CString& propName, T* pProperty)
    {
        m_collectionSerializableProperties.emplace_back(
            SerializablePropertyImpl<T>::make(pProperty, propName));
    }

    /// <summary>Добавление сериализуемого свойства в коллекцию с получением значения через функцию
    /// используется в макросе DECLARE_PROPERTY.</summary>
    /// <param name="propName">Имя (де)сериализуемого свойства.</param>
    /// <param name="getPropPointerFunc">Указатель на свойство.</param>
    /// <param name="value">Значение свойства которое будет возвращаться функцией.</param>
    /// <returns>Свойство которое передали в value.</returns>
    template<class T>
    T registerProperty(const CString& propName,
                       const std::function<T*()>& getPropPointerFunc,
                       const T& value)
    {
        addProperty<T>(propName, getPropPointerFunc());
        return value;
    }

    /// <summary>Добавление сериализуемого контейнера (например std::vector/list/map/set)
    /// используется в макросе ADD_CONTAINER.</summary>
    /// <param name="containerName">Имя контейнера (де)сериализуемого контейнера.</param>
    /// <param name="pСontainer">Указатель на контейнер.</param>
    template<class T>
    void addSTDContainer(const CString& containerName, T* pСontainer)
    {
        m_collectionSerializableProperties.emplace_back(
            SerializableSTLContainer<T>::make(pСontainer, containerName));
    }

    /// <summary>Регистрация сериализуемого контейнера (например std::vector/list/map/set)
    /// используется в макросе DECLARE_CONTAINER.</summary>
    /// <param name="containerName">Имя контейнера (де)сериализуемого контейнера.</param>
    /// <param name="getContainerPointerFunc">Функция получения указателя на контейнер.</param>
    /// <param name="value">Контейнер который будет возвращаться функцией.</param>
    /// <returns>Контейнер который передали в value.</returns>
    template<class T>
    T registerSTDContainer(const CString& containerName,
                           const std::function<T*()>& getContainerPointerFunc,
                           const T& value)
    {
        addSTDContainer<T>(containerName, getContainerPointerFunc());
        return value;
    }

    /// <summary>Добавление сериализуемого объекта в коллекцию
    /// используется в макросе ADD_SERIALIZABLE.</summary>
    /// <param name="pSerializableObjectAddress">Адресс сериализуемого объекта.</param>
    template <class T>
    void addSerializable(T** pSerializableObjectAddress)
    {
        // Будем оборачивать объект в Proxy классы т.к. на момент добавления
        // объект может ещё не существовать и нам надо сохранить его адрес
        // чтобы когда его создадут/изменят обращаться к нему
        if constexpr (std::is_base_of_v<ISerializableProperty, T>)
            m_collectionSerializableProperties.emplace_back(
                SerializablePropertyProxy<T>::make(pSerializableObjectAddress));
        else if constexpr (std::is_base_of_v<ISerializableCollection, T>)
            m_collectionSerializableProperties.emplace_back(
                SerializableCollectionProxy<T>::make(pSerializableObjectAddress));
        else
            static_assert(false, "ISerializableProperty and ISerializableCollection not implemented");
    }

    /// <summary>Добавление сериализуемого объекта в коллекцию
    /// используется в макросе DECLARE_SERIALIZABLE.</summary>
    /// <param name="getSerializableAddressFunc">Функция получения адресса сериализуемого объекта.</param>
    /// <param name="value">Объъект который будет возвращаться функцией.</param>
    /// <returns>Значение которое передали в value.</returns>
    template <class T, class U>
    U registerSerializable(const std::function<T**()>& getSerializableAddressFunc,
                           const U& value)
    {
        addSerializable<T>(getSerializableAddressFunc());
        return value;
    }

protected:
    /// <summary>Функция для объявления сериализуемых свойств
    /// см DECLARE_COLLECTION/DECLARE_COLLECTION_NAME и BEGIN_SERIALIZABLE_PROPERTIES.</summary>
    /// <returns>Имя сериализуемой коллекции.</returns>
    virtual CString declareSerializableProperties() = 0;

protected:
    // Список серилизуемых свойств
    std::list<ISerializablePtr> m_collectionSerializableProperties;
};

#pragma endregion SerializableClasses
