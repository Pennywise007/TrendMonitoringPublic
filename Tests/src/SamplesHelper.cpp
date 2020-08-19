// ��������������� ���� ��� ������ � ���������

#include "pch.h"
#include "resource.h"

#include <filesystem>
#include <fstream>


///////////////////////////////////////////////////////////////////////////////
// �������������� ����� ��� ���������� � �������� ��������
struct ResourceLocker
{
    ResourceLocker(HGLOBAL hLoadedResource)
    {
        m_hLoadedResource = hLoadedResource;
        // ����������� �������
        m_pLockedResource = ::LockResource(hLoadedResource);
    }
    // ����������� �������
    ~ResourceLocker() { ::FreeResource(m_hLoadedResource); }

    LPVOID m_pLockedResource;
    HGLOBAL m_hLoadedResource;
};

//****************************************************************************//
// ������� ��������� ����� � ��������� ������
void compareWithResourceFile(const CString& verifiableFile,
                             const UINT resourceId, const CString& resourceName)
{
    std::ifstream file(verifiableFile, std::ifstream::binary | std::ifstream::ate);
    ASSERT_FALSE(file.fail()) << "�������� ��� �������� ����� " + CStringA(verifiableFile);

    // ������ ��������� ����
    HRSRC hResource = ::FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), resourceName);
    ASSERT_TRUE(hResource) << "������ ��� �������� ������� " + CStringA(resourceName);
    HGLOBAL hGlob = LoadResource(GetModuleHandle(NULL), hResource);
    ASSERT_TRUE(hGlob) << "������ ��� �������� ������� " + CStringA(resourceName);

    // ����� ������� � �������� ������ ������
    ResourceLocker resourceLocker(hGlob);
    DWORD dwResourceSize = ::SizeofResource(GetModuleHandle(NULL), hResource);
    ASSERT_TRUE(resourceLocker.m_pLockedResource && dwResourceSize != 0) << "������ ��� �������� ������� " + CStringA(resourceName);

    // ��������� ��� ������� ���������
    ASSERT_EQ(file.tellg(), dwResourceSize) <<
        "������ ����� " + CStringA(verifiableFile) + " � ������� " + CStringA(resourceName) + " ����������";

    // ������������ � ������ � ����� ���������� ����������
    file.seekg(0, std::ifstream::beg);

    // ��������� ���������� ����� � ������
    std::string fileText;
    std::copy(std::istreambuf_iterator<char>(file),
              std::istreambuf_iterator<char>(),
              std::insert_iterator<std::string>(fileText, fileText.begin()));

    // ���������� ���������� ������� � �����
    ASSERT_EQ(fileText, (char*)resourceLocker.m_pLockedResource) <<
        "���������� ����� " + CStringA(verifiableFile) + " � ������� " + CStringA(resourceName) + " ����������";
}