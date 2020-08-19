// �������� COM ��������, �� ������� ����� � �����������

#include "pch.h"

#include <COM.h>

DECLARE_COM_INTERFACE(ITest, "A740AEFD-B160-447C-8771-43E29D8310DD")
{
    virtual int getVal() = 0;
};

DECLARE_COM_INTERFACE(ITestSecond, "1F42CF13-9B8A-47DE-B75F-43CB0D215E7A")
{
};

DECLARE_COM_INTERFACE_(ITestWithBase, ITest, "16400C3E-C5DA-4C69-BA84-6F68F871ACF2")
{
};

// ��������� � ��������� ITest, ��������� �������� � COM_MAP
DECLARE_COM_BASE_CLASS(TestBaseClass)
    , public ITest
{
public:
    TestBaseClass() = default;
    TestBaseClass(int a) { val = a; }

    BEGIN_COM_MAP(TestBaseClass)
        COM_INTERFACE_ENTRY(ITest)
    END_COM_MAP()

    static const int kValue = -10;

// ITest
    virtual int getVal() override { return val; }

    int val = kValue;
};
// ��������� ������ ITest, ��������� �������� � COM_MAP
DECLARE_COM_INTERNAL_CLASS(TestClass)
    , public ITest
{
public:
    TestClass() = default;
    TestClass(int a) { val = a; }

    BEGIN_COM_MAP(TestClass)
        COM_INTERFACE_ENTRY(ITest)
    END_COM_MAP()

    static const int kValue = 101;

// ITest
    int getVal() override { return val; }

    int val = kValue;
};
// ��������� ITestSecond � TestBaseClass, �������� COM_MAP �������� ������
DECLARE_COM_INTERNAL_CLASS_(TestClassSecond, TestBaseClass)
    , public ITestSecond
{
public:
    TestClassSecond() { val = kValue; }
    TestClassSecond(int a) : TestBaseClass(a) { val = a; }

    BEGIN_COM_MAP(TestClassSecond)
        COM_INTERFACE_ENTRY(ITestSecond)
        COM_INTERFACE_ENTRY_CHAIN(TestBaseClass)
    END_COM_MAP()

    static const int kValue = 201;

// ITest
    int getVal() override { return val; }
};
// ��������� TestBaseClass, �� �� �������� ��� COM_MAP
DECLARE_COM_INTERNAL_CLASS_(TestClassSecondNoChain, TestBaseClass)
    , public ITestSecond
{
public:
    TestClassSecondNoChain() = default;

    BEGIN_COM_MAP(TestClassSecondNoChain)
        COM_INTERFACE_ENTRY(ITestSecond)
    END_COM_MAP()

    static const int kValue = 70;

// ITest
    int getVal() override { return kValue; }
};
// ��������� ITestWithBase, �� ITest �� �������� � COM_MAP
DECLARE_COM_INTERNAL_CLASS(TestClassInterfaceWithBase)
    , public ITestWithBase
{
public:
    TestClassInterfaceWithBase() = default;

    BEGIN_COM_MAP(TestClassInterfaceWithBase)
        COM_INTERFACE_ENTRY(ITestWithBase)
    END_COM_MAP()

    static const int kValue = 7;

// ITest
    int getVal() override { return kValue; }
};


// �������� ����������� �������� COM �������
TEST(TestCOM, TestClassCreation)
{
    // ����� ��������� �� ITest
    TestClass::Ptr pTestClass;
    // ����� ������� ����������� �� TestBaseClass
    TestClassSecond::Ptr pTestSecondClass;
    // ����� ������� ����������� �� TestBaseClass �� �� �������� ��� COM_INTERFACE_ENTRY_CHAIN
    TestClassSecondNoChain::Ptr pTestClassSecondNoChain;
    // ����� ������� ����������� �� ���������� � �������������
    TestClassInterfaceWithBase::Ptr pTestClassInterfaceWithBase;

    pTestClass = TestClass::create();
    EXPECT_FALSE(!pTestClass || pTestClass->getVal() != TestClass::kValue) << "�� ������� ������� ����� ����� ::create()";
    pTestClass = TestClass::make(10);
    EXPECT_FALSE(!pTestClass || pTestClass->getVal() != 10) << "�� ������� ������� ����� ����� ::make()";

    // ������� ����� ������� ����������� �� TestBaseClass
    pTestSecondClass = TestClassSecond::create();
    EXPECT_FALSE(!pTestSecondClass || pTestSecondClass->getVal() != TestClassSecond::kValue) << "�� ������� ������� ����� � ���������";

    // ������� ����� ������� ����������� �� TestBaseClass
    pTestSecondClass = TestClassSecond::make(110);
    EXPECT_FALSE(!pTestSecondClass || pTestSecondClass->getVal() != 110) << "�� ������� ������� ����� � ���������";

    // ������� ����� ������� ����������� �� TestBaseClass
    pTestClassSecondNoChain = TestClassSecondNoChain::create();
    EXPECT_FALSE(!pTestClassSecondNoChain ||
                 pTestClassSecondNoChain->getVal() != TestClassSecondNoChain::kValue) << "�� ������� ������� ����� � ��������� ��� COM_INTERFACE_ENTRY_CHAIN";

    // ������� ����� ������� ����������� �� ���������� � �������������
    pTestClassInterfaceWithBase = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(!pTestClassInterfaceWithBase ||
                 pTestClassInterfaceWithBase->getVal() != TestClassInterfaceWithBase::kValue) << "�� ������� ������� ����� �� ���������� � �������������";
}

// �������� ����������� �������� ������ � (��)����������� �����������
TEST(TestCOM, TestClassToInterfaceQuery)
{
    // ����������
    ITestPtr pITest;
    ITestSecondPtr pITestSecond;
    ITestWithBasePtr pITestWithBase;

    pITest = TestClass::create();
    EXPECT_FALSE(!pITest || pITest->getVal() != TestClass::kValue) << "�� ������� �������� ����� � ������������ ����������";
    pITest = TestClassSecond::create();
    EXPECT_FALSE(!pITest || pITest->getVal() != TestClassSecond::kValue) << "�� ������� �������� ����� � ������������ ����������";
    pITest = TestClassSecondNoChain::create();
    EXPECT_FALSE(pITest) << "������� �������� ����� � ���������� �������� ��� � COM_MAP";
    pITest = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(pITest) << "������� �������� ����� � ���������� �������� ��� � COM_MAP";

    pITestSecond = TestClass::create();
    EXPECT_FALSE(pITestSecond) << "������� �������� ����� � �� ������������ ����������";
    pITestSecond = TestClassSecond::create();
    EXPECT_TRUE(pITestSecond) << "�� ������� �������� ����� � ������������ ����������";
    pITestSecond = TestClassSecondNoChain::create();
    EXPECT_TRUE(pITestSecond) << "�� ������� �������� ����� � ������������ ����������";
    pITestSecond = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(pITestSecond) << "������� �������� ����� � �� ������������ ����������";

    pITestWithBase = TestClass::create();
    EXPECT_FALSE(pITestWithBase) << "������� �������� ����� � �� ������������ ����������";
    pITestWithBase = TestClassInterfaceWithBase::create();
    EXPECT_TRUE(pITestWithBase) << "�� ������� �������� ����� � ���������� � �������������";
}

// �������� ���������� � ������������� �����������
TEST(TestCOM, TestInterfaceQuerying)
{
    // ����������
    ITestPtr pITest;

    CComPtr<ITest> pCComITest;
    CComQIPtr<ITest> pCComQIITest;

    {
        pITest = TestClass::create();

        // ��������� = � COM �������� �� ������ ����������
        pCComITest = pITest;
        EXPECT_TRUE(pCComITest) << "�� ������� ���������� ��������� � CComPtr";
        pCComQIITest = pITest;
        EXPECT_TRUE(pCComITest) << "�� ������� ���������� ��������� � CComQIPtr";
        pITest = pCComITest;
        EXPECT_TRUE(pITest) << "�� ������� ���������� CComPtr � ����������";
        pITest = pCComQIITest;
        EXPECT_TRUE(pITest) << "�� ������� ���������� CComQIPtr � ����������";
    }

    {
        pITest = TestClass::create();

        // ������������
        CComPtr<ITest> pCComITest(pITest);
        EXPECT_TRUE(pCComITest) << "�� ������� �������� ��������� � ����������� CComPtr";
        CComQIPtr<ITest> pCComQIITest(pITest);
        EXPECT_TRUE(pCComQIITest) << "�� ������� �������� ��������� � ����������� CComQIPtr";
        ITestPtr pITest1(pCComITest);
        EXPECT_TRUE(pITest1) << "�� ������� �������� CComPtr � ����������� ����������";
        ITestPtr pITest2(pCComQIITest);
        EXPECT_TRUE(pITest2) << "�� ������� �������� CComQIPtr � ����������� ����������";
    }

    {
        // ��������� = � COM �������� �� ������ �����������

        ITestSecondPtr pITestSecond = TestClassSecond::create();

        pITest = pITestSecond;
        EXPECT_TRUE(pITest) << "�� ������� ���������� ITestSecondPtr � ITestPtr";
        pCComITest = pITestSecond;
        EXPECT_TRUE(pCComITest) << "�� ������� ���������� ITestSecondPtr � CComPtr<ITest>";
        pCComQIITest = pITestSecond;
        EXPECT_TRUE(pCComQIITest) << "�� ������� ���������� ITestSecondPtr � CComQIPtr<ITest>";
        pITestSecond = pCComITest;
        EXPECT_TRUE(pITestSecond) << "�� ������� ���������� CComPtr<ITest> � ITestSecondPtr";
        pITestSecond = pCComQIITest;
        EXPECT_TRUE(pITestSecond) << "�� ������� ���������� CComQIPtr<ITest> � ITestSecondPtr";
    }

    {
        // ������������ � COM �������� �� ������ �����������

        ITestSecondPtr pITestSecond = TestClassSecond::create();

        ITestPtr pITest(pITestSecond);
        EXPECT_TRUE(pITest) << "�� ������� �������� ITestSecondPtr � ����������� ITestPtr";
        CComPtr<ITest> pCComITest(pITestSecond);
        EXPECT_TRUE(pCComITest) << "�� ������� �������� ITestSecondPtr � ����������� CComPtr<ITest>";
        CComQIPtr<ITest> pCComQIITest(pITestSecond);
        EXPECT_TRUE(pCComQIITest) << "�� ������� �������� ITestSecondPtr � ����������� CComQIPtr<ITest>";
        ITestSecondPtr pITestSecond1(pCComITest);
        EXPECT_TRUE(pITestSecond1) << "�� ������� �������� CComPtr<ITest> � ����������� ITestSecondPtr";
        ITestSecondPtr pITestSecond2(pCComQIITest);
        EXPECT_TRUE(pITestSecond2) << "�� ������� �������� CComQIPtr<ITest> � ����������� ITestSecondPtr";
    }

    {
        // ���������� �����, ������ �������� �� ������

        CComPtr<ITestSecond> pCComITestSecond = TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "�� ������� �������� CComPtr �� ComInternal ����� �����������";
        // � CComPtr �������� � ���������� = � ������ �����������, ���������� ����������
        pCComITestSecond = (decltype(pCComITestSecond))TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "�� ������� �������� CComPtr �� ComInternal ����� =";

        // � CComQIPtr ��� ������������ � COM �������� � ������ �����������, ���������� ����������
        CComQIPtr<ITestSecond> pCComQIITestSecond =
            (decltype(pCComQIITestSecond))TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "�� ������� �������� CComQIPtr �� ComInternal ����� �����������";
        pCComQIITestSecond = TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "�� ������� �������� CComQIPtr �� ComInternal ����� =";
    }

    {
        // �������� ������� ����� �������

        ITestSecondPtr pITestSecond;
        EXPECT_FALSE(pITestSecond) << "����� ������������ ���������� - ������ �� ������";

        {
            ITestPtr pITestInternal = TestClassSecond::create();
            pITestSecond = pITestInternal;
        }

        EXPECT_TRUE(pITestSecond) << "����� ������ �� ������� ��������� ���������� - ������ �����������";
    }
}