#pragma once

/*
    �������� ����������� ������������� ��������

    ���������� �������� ������� ��� ��������� ������������/�������������� ������
    �� ���� - ������� ������� ��������� � �++

�������������:
    DECLARE_COM_INTERNAL_CLASS_(ApplicationConfiguration, SerializableObjectsCollection)
    {
    public:
        ApplicationConfiguration() = default;

        // ��������� ��� �������� ������� ����� �������������/���������������
        BEGIN_SERIALIZABLE_PROPERTIES(ApplicationConfiguration)
            ADD_PROPERTY(m_myValueBool)
            ADD_CONTAINER(m_listOfParams)
        END_SERIALIZABLE_PROPERTIES()

        // ���������� �������� m_myPropInt � ���������� ��� � ������ �������������
        DECLARE_PROPERTY((int) m_myPropInt);
        // ���������� �������� m_myPropLong � ���������� ��� � ������ �������������
        // �������������� m_myPropLong ��������� ����� ������� (m_myPropLong = 55;)
        DECLARE_PROPERTY((long) m_myPropLong, 55);

    public:
        // ������������� ��������
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

/// ����� ��������� �������������� �������
DECLARE_COM_INTERFACE(ISerializable, "515CB11E-2B96-4685-9555-69B28A89B830")
{
    /// <summary>�������� ��� �������.</summary>
    /// <param name="name">��� �������.</param>
    /// <returns>True ���� ������� ����� (��)������������, false - ����������.</returns>
    virtual bool getObjectName(CString* name = nullptr) = 0;
};

/// ��������� �������������� ��������
DECLARE_COM_INTERFACE_(ISerializableProperty, ISerializable, "122BD471-1669-409A-9902-A0DAC9872C99")
{
    /// <summary>������������ ��������.</summary>
    /// <returns>��������� ������������� ��������.</returns>
    virtual CString serializePropertyValue() = 0;
    /// <summary>�������������� ��������.</summary>
    /// <param name="value">��������� ������������� ���������������� ��������.</param>
    virtual void    deserializePropertyValue(const CString& value) = 0;
};

/// ��������� ��������� ������������� ��������
DECLARE_COM_INTERFACE_(ISerializableCollection, ISerializable, "55F5C6DE-2E50-4516-8699-D733A02EC115")
{
    /// <summary>��������� ���������� �������� � ���������.</summary>
    virtual size_t getObjectsCount() = 0;
    /// <summary>�������� ������ �� ��������� �� �������.</summary>
    /// <param name="index">������ � ���������.</param>
    /// <returns>������������� ������.</returns>
    virtual ISerializablePtr getObject(const size_t index) = 0;

    /// <summary>��������� � ������ ������������ ���������.</summary>
    /// <returns>true ���� ��������� ����� �������������.</returns>
    virtual bool onStartSerializing() = 0;
    /// <summary>��������� �� ��������� ������������ ���������.</summary>
    virtual void onEndSerializing() = 0;

    /// <summary>��������� � ������ ��������������, �������� ������ ����������� ��� ������������ ���� ��������.</summary>
    /// <param name="objNames">����� �������� ������� ���� ��������� ��� ������������.</param>
    /// <returns>true ���� ��������� ����� ���������������.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) = 0;
    /// <summary>��������� � ����� �������������� ���������.</summary>
    virtual void onEndDeserializing() = 0;
};

#pragma endregion SerializationInterfaces

////////////////////////////////////////////////////////////////////////////////
// ��������������� ������� ��� ���������� ������������� ��������
// ������������ ��� ������������ �� SerializableObjectsCollection
#pragma region SerializationMacros

//----------------------------------------------------------------------------//
// ���������� ������������� ������� ����� ������� �������� ������

#define BEGIN_SERIALIZABLE_PROPERTIES(CollectionName) \
virtual CString declareSerializableProperties() override\
{\
    CString collectionName = L#CollectionName;

// ���������� (��)�������������� �������� �������� ����
#define ADD_PROPERTY(Property) \
    addProperty<decltype(Property)>(L#Property, &Property);

// ���������� (��)�������������� ���������� ������� ����� (std::vector|set|map<long> � �.�)
#define ADD_CONTAINER(Container) \
    addSTDContainer<decltype(Container)>(L#Container, &Container);

// ���������� �������� ����������� ISerializableCollection/ISerializableProperty
#define ADD_SERIALIZABLE(Serializable) \
    addSerializable(Serializable.address());

// �������� ����� ����������� ������������� �������� � ������� �����
#define CHAIN_SERIALIZABLE_PROPERTIES(BaseClass) \
    /* ���� �� �������� ��� ��������� - ����� � ��������� ������ */ \
    if (auto baseClassName = BaseClass::declareSerializableProperties();    \
        collectionName.IsEmpty()) \
        collectionName = baseClassName;

#define END_SERIALIZABLE_PROPERTIES() \
    return collectionName; \
}

//----------------------------------------------------------------------------//
// ������� � ����������� ������� � ����������� �� ����� � ������ ������������� ���
// ��������� ������������. ��������� ��������� ���������� ������������� ����.

// ���������� ����� ������������� ���������
#define DECLARE_COLLECTION_NAME(CollectionName)                 \
virtual CString declareSerializableProperties() override        \
{ return CollectionName; }

// ���������� ������������� ���������
#define DECLARE_COLLECTION(Collection) DECLARE_COLLECTION_NAME(L#Collection);

// ���������� �������������� �������� � ����������� ��� � ������� ������
// ������� ������������� DECLARE_PROPERTY((long) m_propName); => long m_propName = long();
// ������� ������������� DECLARE_PROPERTY((long) m_propName, 105); => long m_propName = long(105);
// !!������� ������� ����� ������ � ����� - �����������
// !!������ ������������ ����������� � ���� ����������
#define DECLARE_PROPERTY(Property, ...) \
DECLARE_OBJECT(Property) =              \
REGISTER_SERIALIZABLE_OBJECT(GET_OBJECT(Property), registerProperty, __VA_ARGS__);

// ���������� �������������� ���������� � ����������� ��� � ������� ������
// ������� ������������� DECLARE_CONTAINER((std::vector<long>) m_propName); => std::vector<long> m_propName;
// !!������� ������� ����� ������ � ����� - �����������
// !!������ ������������ ����������� � ����� ����������
#define DECLARE_CONTAINER(Container, ...)   \
DECLARE_OBJECT(Container) =                 \
REGISTER_SERIALIZABLE_OBJECT(GET_OBJECT(Container), registerSTDContainer, __VA_ARGS__)

// ���������� �������������� ������� � ����������� ��� � ������� ������
// ������� �������������:
// DECLARE_SERIALIZABLE((TestSerializing::Ptr) m_serializingClass,
//                      TestSerializing::create()); =>
// TestSerializing::Ptr m_serializingClass = TestSerializing::create();
//
// !!������� ������� ����� ������ � ����� - �����������
// !!���������� ������������ ����������� � ���� ����������
#define DECLARE_SERIALIZABLE(Serializable, ...) \
DECLARE_OBJECT(Serializable) =                  \
registerSerializable<                           \
    extract_value_type_v<                       \
        decltype(GET_OBJECT(Serializable)) > >  \
            ([this](){ return GET_OBJECT(Serializable).address(); }, __VA_ARGS__);

#pragma endregion SerializationMacros

#pragma region SerializableClasses

////////////////////////////////////////////////////////////////////////////////
/// ������� ����� ��� �������������� ��������
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
    /// <summary>�������� ��� �������.</summary>
    /// <param name="propertyName">��� �������.</param>
    /// <returns>True ���� ������� ����� (��)������������, false - ����������.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    { if (name) *name = m_propName; return true; }

// ISerializableProperty
protected:
    /// <summary>������������ ��������.</summary>
    /// <param name="propVersion">������ �������������� ��������.</param>
    /// <returns>��������� ������������� ��������.</returns>
    virtual CString serializePropertyValue() override
    { return value_to_sting<T>(*m_pProperty); }
    /// <summary>�������������� ��������.</summary>
    /// <param name="propVersion">������ �������������� ��������.</param>
    /// <param name="value">��������� ������������� ���������������� ��������.</param>
    virtual void deserializePropertyValue(const CString& value) override
    { *m_pProperty = string_to_value<T>(value); }

protected:
    // ��� ��������
    CString m_propName;
    // ��������� �� ��������
    T* m_pProperty;
};

////////////////////////////////////////////////////////////////////////////////
/// ���������� ����������� ������ ��������, ������ ��������� �� ��������
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializablePropertyImpl, SerializablePropertyBase<T>, T)
{
public:
    SerializablePropertyImpl(T* pProperty, const CString& propName)
        : SerializablePropertyBase<T>(pProperty, propName)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// ���������� ����������� ������ �������� � ������������ �������� �������� ��������
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializablePropertyValue, SerializablePropertyBase<T>, T)
{
public:
    SerializablePropertyValue(const T& value, const CString& propName)
        : SerializablePropertyBase<T>(nullptr, propName)
        , m_value(value)
    { SerializablePropertyBase<T>::m_pProperty = &m_value; }

public:
    // ����������� ��������, ��� ����� (��)��������������� ������� �������
    T m_value;
};

////////////////////////////////////////////////////////////////////////////////
// ������� ����� ��� ���������
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
    /// <summary>�������� ��� �������.</summary>
    /// <param name="name">��� �������.</param>
    /// <returns>True ���� ������� ����� (��)������������, false - ����������.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    { if (name) *name = m_collectionName; return true; }

// ISerializableCollection
protected:
    /// <summary>��������� ���������� �������� � ���������.</summary>
    virtual size_t getObjectsCount() override { return m_pCollectionContainer->size(); }
    /// <summary>�������� ������ �� ��������� �� �������.</summary>
    /// <param name="index">������ � ���������.</param>
    /// <returns>������������� ������.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (index >= getObjectsCount())
        {
            assert(!"����� �� ������� ���������� ��������� � ����������");
            return nullptr;
        }

        auto it = std::next(m_pCollectionContainer->begin(), index);
        // ��� map ���������� ���������� ��� mapped_type
        if constexpr (isMapContainer<T>::value)
        {
            if constexpr (isSerializableType<T::mapped_type>::value)
                return it->second;
        }
        // ��������� ��� ��� ��������� ������������� ���������
        else if constexpr (isSerializableType<T::value_type>::value)
            return *it;

        return nullptr;
    }
    /// <summary>��������� � ������ ������������.</summary>
    /// <returns>true ���� ��������� ����� �������������.</returns>
    virtual bool onStartSerializing() override { return true; }
    /// <summary>��������� �� ��������� ������������ ���������.</summary>
    virtual void onEndSerializing()  override { }

    /// <summary>��������� � ������ ��������������, �������� ������ ����������� ��� ������������ ���� ��������.</summary>
    /// <param name="objNames">����� �������� ������� ���� ��������� ��� ������������.</param>
    /// <returns>true ���� ��������� ����� ���������������.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& /*objNames*/) override { return true; }
    /// <summary>��������� � ����� �������������� ���������.</summary>
    virtual void onEndDeserializing()  override { }

protected:
    // ��������� ����������� ��� �������� ��� �������� �������������(����������� �� ISerializable)
    template <class T>
    struct isSerializableType
    {
        // 1. ������� ��������� ���(���� �������� ���������)
        // 2. ������� ������� ���� ��� ���������
        // 3. ��������� ��� ����������� �� ISerializable
        static constexpr bool value =
            std::is_base_of_v<ISerializable,
            std::remove_pointer_t<
            extract_value_type_v<T>>>;
    };

protected:
    // ��� ���������
    CString m_collectionName;
    // ��������� �� ��������� � ����������
    T* m_pCollectionContainer;
};

////////////////////////////////////////////////////////////////////////////////
/// ���������� ����������� ������ ���������
template <class T>
DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(SerializableCollectionImpl, SerializableCollectionBase<T>, T)
{
public:
    SerializableCollectionImpl(T* pCollectionContainer, const CString& containerName)
        : SerializableCollectionBase<T>(pCollectionContainer, containerName)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// ���������� ����������� ������ ���������, � ������� ����� ������������ stl ����������
/// ����� ��� map/vector/list ����������� �����
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
    /// <summary>�������� ������ �� ��������� �� �������.</summary>
    /// <param name="index">������ � ���������.</param>
    /// <returns>������������� ������.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (index >= BaseClass::getObjectsCount())
        {
            assert(!"����� �� ������� ���������� ��������� � ����������");
            return nullptr;
        }

        auto it = std::next(BaseClass::m_pCollectionContainer->begin(), index);

        if constexpr (isMapContainer<T>::value)
        {
            // ��� map ���������� ���������� ��� mapped_type
            if constexpr (BaseClass::isSerializableType<T::mapped_type>::value)
                // � ����� ������ � ����������� ���� ������ ��� � ��������
                return it->second;
            else
                // ��� ���������� map ������ ���� ���������, � ���� ���� ��� ��� �����
                return SerializablePropertyImpl<T::mapped_type>::make(
                    &it->second, m_kNotSerializableAppendText + value_to_sting<T::key_type>(it->first));
        }
        // ��������� ��� ��� ��������� ������������� ���������
        else if constexpr (BaseClass::isSerializableType<T::value_type>::value)
            // ���� ������ ��� ����������� �� ISerializable - ���������� ���
            return *it;
        else
        {
            if constexpr (std::is_same_v<std::set<T::value_type>, T> ||
                          std::is_same_v<std::multiset<T::value_type>, T>)
                // � std::set �������� � ���� ���������
                return SerializablePropertyValue<T::value_type>::make(
                    *it, m_kNotSerializableAppendText + value_to_sting(*it));
            else
                // ���� ������ �������� ���� - ����������� � SerializablePropertyImpl
                return SerializablePropertyImpl<T::value_type>::make(
                    &*it, m_kNotSerializableAppendText + value_to_sting(index));
        }
    }

    /// <summary>��������� � ������ ��������������, �������� ������ ����������� ��� ������������ ���� ��������.</summary>
    /// <param name="objNames">����� �������� ������� ���� ��������� ��� ������������.</param>
    /// <returns>true ���� ��������� ����� ���������������.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) override
    {
        BaseClass::m_pCollectionContainer->clear();
        // ������� ��������� � ��������������
        if constexpr (std::is_same_v<std::vector<T::value_type>, T> ||
                      std::is_same_v<std::list<T::value_type>, T>)
        {
            // ���� ��� ���������� ���� vector/list �������� �� �� ���������� ��������
            BaseClass::m_pCollectionContainer->resize(objNames.size());

            if constexpr (BaseClass::isSerializableType<T::value_type>::value)
            {
                // ��������� ���� std::vector<ISerializable::Ptr>
                // ������� ������ ������
                for (auto& item : *BaseClass::m_pCollectionContainer)
                    // � �������� ������ ���� ����������� ��� ����������, ����� �����������
                    // ��������� ����� SerializableCollectionBase
                    item = extract_value_type_v<T::value_type>::create();
            }
        }
        // ���� ��� set ����������
        else if constexpr (std::is_same_v<std::set<T::value_type>, T> ||
                           std::is_same_v<std::multiset<T::value_type>, T>)
        {
            // ��������� ���� std::set<ISerializable>
            static_assert(!BaseClass::isSerializableType<T::value_type>::value,
                          "Container std::set<ISerializable> not implementeed, use SerializableCollectionBase");

            // � std::set objName ��� ����, ��������� � ��������� ��������
            for (auto& objName : objNames)
            {
                if (objName.Find(m_kNotSerializableAppendText) == 0)
                {
                    // �������� ���
                    CString setKeyTextVal = objName.Mid(m_kNotSerializableAppendText.GetLength());
                    BaseClass::m_pCollectionContainer->emplace(string_to_value<T::key_type>(setKeyTextVal));
                }
                else
                    assert(!"� ������ ����� � ����� std::set ���������� m_kNotSerializableAppendText");
            }
        }
        // ���� ��� map ����������
        else if constexpr (isMapContainer<T>::value)
        {
            // � std::map objName ��� ����, ��������� � ���� ������ �������� � ������ ������
            for (auto& objName : objNames)
            {
                if (objName.Find(m_kNotSerializableAppendText) == 0)
                {
                    // �������� ���
                    CString mapKeyTextVal = objName.Mid(m_kNotSerializableAppendText.GetLength());

                    if constexpr (BaseClass::isSerializableType<T::mapped_type>::value)
                        // ��������� ���� std::map<ValueType, ISerializable>,
                        // � �������� ������ ���� ����������� ��� ����������, ����� �����������
                        // ��������� ����� SerializableCollectionBase
                        BaseClass::m_pCollectionContainer->try_emplace(
                            string_to_value<T::key_type>(mapKeyTextVal),
                            extract_value_type_v<T::value_type>::create());
                    else
                        BaseClass::m_pCollectionContainer->try_emplace(
                            string_to_value<T::key_type>(mapKeyTextVal),
                            T::mapped_type());
                }
                else
                    assert(!"� ������ ����� � ����� std::map ���������� m_kNotSerializableAppendText");
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
    // �����, ������������ � ������ ���������� ������������� �������� ���������
    // ����������� ��� ���� ���� �� ���� ������� ���� <0>��������</0> ����� ���� ������ ��� ��������
    const CString m_kNotSerializableAppendText = L"ind_";
};

////////////////////////////////////////////////////////////////////////////////
// ����� ������� ������ ����� ������� ������������ ISerializableProperty �
// ������������ ��� ������ �� ����������� � ����
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
    /// <summary>�������� ��� �������.</summary>
    /// <param name="propertyName">��� �������.</param>
    /// <returns>True ���� ������� ����� (��)������������, false - ����������.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    {
        if (ISerializablePtr pSerializableObj = *m_pPropertyAddress)
            return pSerializableObj->getObjectName(name);

        assert(!"��� �������������� �������!");
        return false;
    }

// ISerializableProperty
protected:
    /// <summary>������������ ��������.</summary>
    /// <param name="propVersion">������ �������������� ��������.</param>
    /// <returns>��������� ������������� ��������.</returns>
    virtual CString serializePropertyValue() override
    {
        if (ISerializablePropertyPtr pPropertyObj = *m_pPropertyAddress)
            return pPropertyObj->serializePropertyValue();

        assert(!"��� ���������!");
        return CString();
    }
    /// <summary>�������������� ��������.</summary>
    /// <param name="propVersion">������ �������������� ��������.</param>
    /// <param name="value">��������� ������������� ���������������� ��������.</param>
    virtual void deserializePropertyValue(const CString& value) override
    {
        if (ISerializablePropertyPtr pPropertyObj = *m_pPropertyAddress)
            pPropertyObj->deserializePropertyValue(value);
        else
            assert(!"��� ��������!");
    }

protected:
    // ������ ��������
    T** m_pPropertyAddress;
};

////////////////////////////////////////////////////////////////////////////////
// ����� ������� ������ ����� ������� ������������ ISerializableCollection �
// ������������ ��� ������ �� ����������� � ����
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
    /// <summary>�������� ��� �������.</summary>
    /// <param name="propertyName">��� �������.</param>
    /// <returns>True ���� ������� ����� (��)������������, false - ����������.</returns>
    virtual bool getObjectName(CString* name = nullptr) override
    {
        if (ISerializablePtr pSerializableObj = *m_pCollectionAddress)
            return pSerializableObj->getObjectName(name);

        assert(!"��� �������������� �������!");
        return false;
    }

// ISerializableCollection
protected:
    /// <summary>��������� ���������� �������� � ���������.</summary>
    virtual size_t getObjectsCount() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->getObjectsCount();

        assert(!"��� ���������!");
        return 0;
    }
    /// <summary>�������� ������ �� ��������� �� �������.</summary>
    /// <param name="index">������ � ���������.</param>
    /// <returns>������������� ������.</returns>
    virtual ISerializablePtr getObject(const size_t index) override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->getObject(index);

        assert(!"��� ���������!");
        return nullptr;
    }

    /// <summary>��������� � ������ ������������ ���������.</summary>
    /// <returns>true ���� ��������� ����� �������������.</returns>
    virtual bool onStartSerializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->onStartSerializing();

        assert(!"��� ���������!");
        return false;
    }

    /// <summary>��������� �� ��������� ������������ ���������.</summary>
    virtual void onEndSerializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            pCollection->onEndSerializing();
        else
            assert(!"��� ���������!");
    }

    /// <summary>��������� � ������ ��������������, �������� ������ ����������� ��� ������������ ���� ��������.</summary>
    /// <param name="objNames">����� �������� ������� ���� ��������� ��� ������������.</param>
    /// <returns>true ���� ��������� ����� ���������������.</returns>
    virtual bool onStartDeserializing(const std::list<CString>& objNames) override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            return pCollection->onStartDeserializing(objNames);

        assert(!"��� ���������!");
        return false;
    }

    /// <summary>��������� � ����� �������������� ���������.</summary>
    virtual void onEndDeserializing() override
    {
        if (ISerializableCollectionPtr pCollection = *m_pCollectionAddress)
            pCollection->onEndDeserializing();
        else
            assert(!"��� ���������!");
    }

protected:
    // ������ ��������
    T** m_pCollectionAddress;
};

////////////////////////////////////////////////////////////////////////////////
/// ��������� ������������� ��������, ������ ������������� COMInternal �������
/// ���������� ��������������� declareSerializableProperties, �� �������
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
                   "������ ��� � �������������� �������!");
        }

        return res;
    }

protected:
    /// <summary>���������� �������� � ���������, ������������ � ������� ADD_PROPERTY.</summary>
    /// <param name="propName">��� (��)�������������� ��������.</param>
    /// <param name="pProperty">��������� �� ��������.</param>
    template<class T>
    void addProperty(const CString& propName, T* pProperty)
    {
        m_collectionSerializableProperties.emplace_back(
            SerializablePropertyImpl<T>::make(pProperty, propName));
    }

    /// <summary>���������� �������������� �������� � ��������� � ���������� �������� ����� �������
    /// ������������ � ������� DECLARE_PROPERTY.</summary>
    /// <param name="propName">��� (��)�������������� ��������.</param>
    /// <param name="getPropPointerFunc">��������� �� ��������.</param>
    /// <param name="value">�������� �������� ������� ����� ������������ ��������.</param>
    /// <returns>�������� ������� �������� � value.</returns>
    template<class T>
    T registerProperty(const CString& propName,
                       const std::function<T*()>& getPropPointerFunc,
                       const T& value)
    {
        addProperty<T>(propName, getPropPointerFunc());
        return value;
    }

    /// <summary>���������� �������������� ���������� (�������� std::vector/list/map/set)
    /// ������������ � ������� ADD_CONTAINER.</summary>
    /// <param name="containerName">��� ���������� (��)�������������� ����������.</param>
    /// <param name="p�ontainer">��������� �� ���������.</param>
    template<class T>
    void addSTDContainer(const CString& containerName, T* p�ontainer)
    {
        m_collectionSerializableProperties.emplace_back(
            SerializableSTLContainer<T>::make(p�ontainer, containerName));
    }

    /// <summary>����������� �������������� ���������� (�������� std::vector/list/map/set)
    /// ������������ � ������� DECLARE_CONTAINER.</summary>
    /// <param name="containerName">��� ���������� (��)�������������� ����������.</param>
    /// <param name="getContainerPointerFunc">������� ��������� ��������� �� ���������.</param>
    /// <param name="value">��������� ������� ����� ������������ ��������.</param>
    /// <returns>��������� ������� �������� � value.</returns>
    template<class T>
    T registerSTDContainer(const CString& containerName,
                           const std::function<T*()>& getContainerPointerFunc,
                           const T& value)
    {
        addSTDContainer<T>(containerName, getContainerPointerFunc());
        return value;
    }

    /// <summary>���������� �������������� ������� � ���������
    /// ������������ � ������� ADD_SERIALIZABLE.</summary>
    /// <param name="pSerializableObjectAddress">������ �������������� �������.</param>
    template <class T>
    void addSerializable(T** pSerializableObjectAddress)
    {
        // ����� ����������� ������ � Proxy ������ �.�. �� ������ ����������
        // ������ ����� ��� �� ������������ � ��� ���� ��������� ��� �����
        // ����� ����� ��� ��������/������� ���������� � ����
        if constexpr (std::is_base_of_v<ISerializableProperty, T>)
            m_collectionSerializableProperties.emplace_back(
                SerializablePropertyProxy<T>::make(pSerializableObjectAddress));
        else if constexpr (std::is_base_of_v<ISerializableCollection, T>)
            m_collectionSerializableProperties.emplace_back(
                SerializableCollectionProxy<T>::make(pSerializableObjectAddress));
        else
            static_assert(false, "ISerializableProperty and ISerializableCollection not implemented");
    }

    /// <summary>���������� �������������� ������� � ���������
    /// ������������ � ������� DECLARE_SERIALIZABLE.</summary>
    /// <param name="getSerializableAddressFunc">������� ��������� ������� �������������� �������.</param>
    /// <param name="value">������� ������� ����� ������������ ��������.</param>
    /// <returns>�������� ������� �������� � value.</returns>
    template <class T, class U>
    U registerSerializable(const std::function<T**()>& getSerializableAddressFunc,
                           const U& value)
    {
        addSerializable<T>(getSerializableAddressFunc());
        return value;
    }

protected:
    /// <summary>������� ��� ���������� ������������� �������
    /// �� DECLARE_COLLECTION/DECLARE_COLLECTION_NAME � BEGIN_SERIALIZABLE_PROPERTIES.</summary>
    /// <returns>��� ������������� ���������.</returns>
    virtual CString declareSerializableProperties() = 0;

protected:
    // ������ ������������ �������
    std::list<ISerializablePtr> m_collectionSerializableProperties;
};

#pragma endregion SerializableClasses
