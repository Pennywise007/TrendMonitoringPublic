/**
* @file   Logger.h
* @author ���� �.
* @short  ���������� �����������.
*/

#pragma once

// ��� ��������� ����������� ���������� �������� ��� ����� #define OUTPUT_LOGGING

#ifdef OUTPUT_LOGGING

#include <afx.h>
#include <afxstr.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <stdio.h>
#include <Windows.h>
#include <WinBase.h>
#include <corecrt.h>

namespace custom_logger
{
    // ���������� ���������� ������� �������� �� ��������� ����������� � ������� ������
    inline std::atomic_bool g_enableLogger = true;
    // ������� ������� �� ������� ���� �����������, ����� ����� �������� ����� ������ ������ �������
    inline std::atomic_int g_loggerLevel = 0;

    class Logging
    {
    public:
        // funcName - ��� ������� ������� ��������
        // logEnter - ���� ��� ����� ���������� � ������ � �������
        Logging(const CString& funcName, bool logEnter)
            : m_funcName(funcName)
        {
            m_currentLoggingLevel = g_loggerLevel++;

            // ���� ��� ������ - ����������� �� ���� ������ ����� ����� <>
            auto start = m_funcName.Find(u'<');
            auto end = m_funcName.Find(u'>');
            if (start != -1 && end != -1 && start < end)
                m_funcName = m_funcName.Mid(start) +
                    m_funcName.Mid(end + 1, m_funcName.GetLength() - end - 1);

            startLogging = std::chrono::system_clock::now();
            if (logEnter)
                OutputLoggerText(m_funcName + L" ����");
        }

        ~Logging()
        {
            CString debugText;  // ����� ��������� � �����
            debugText.Format(L"%s�����; %s",
                             (m_funcName.IsEmpty() ? m_funcName : (m_funcName + L" ")).GetString(),
                             (m_comment.IsEmpty()  ? m_comment : (m_comment + L"; ")).GetString());

            // ���� ���� ���������� ����� ����������
            if (m_bEnableCalcTime)
            {
                auto finish = std::chrono::system_clock::now();
                long long elapsed_seconds =
                    std::chrono::duration_cast<std::chrono::milliseconds>(finish - startLogging).count();

                debugText.Format(L"%s; ����� ���������� %lld ��",
                                 debugText.GetString(),
                                 elapsed_seconds);
            }

            OutputLoggerText(debugText);

            --g_loggerLevel;
        }

        // ���������� ����������� � �������
        template <typename ...Args>
        void addComment(const CString& format, Args&&... args)
        {
            if (!m_comment.IsEmpty())
                m_comment += L";\t";

            m_comment.AppendFormat(format, args...);
        }

        // ��������� �����������
        template <typename ...Args>
        void setText(const CString& format, Args&&... args)
        {
            m_comment.Format(format, args...);
        }

        // ����� ������ �� ������� ������, �� ������ �� ���������� ����� ������
        template <typename ...Args>
        void outputText(const CString& format, Args&&... args)
        {
            CString debugText;  // ����� ��������� � �����
            debugText.Format(format, args...);

            OutputLoggerText(debugText);
        }

        // ������� � ����� ������� ����������
        void outputDebug()
        {
            CString debugText;  // ����� ��������� � �����

            if (!m_funcName.IsEmpty())
                debugText = m_funcName + L"; ";
            debugText += m_comment;
            OutputLoggerText(debugText);
        }

        // ���������� �����������
        void disableLog()
        {
            m_bEnableLog = false;
        }

        // �������� ������� ������� ����������
        void enableCalcTime(bool enable)
        {
            m_bEnableCalcTime = enable;
        }


    private:
        // �������� ������� ��� ������ �������� ����������
        void OutputLoggerText(CString str)
        {
            if (!g_enableLogger || !m_bEnableLog)
                return;

            CString extraText;
            // ��������� ������� �� ������� ������ �������� ��� �����
            for (int curLevel = m_currentLoggingLevel; curLevel != 0; --curLevel)
                extraText += L"|\t";

            // � ��������� ���� ��������� �������
            str.Replace(L"\n", L"\n" + extraText);

            OutputDebugString((extraText + str + L'\n').GetString());
        }

    private:
        CString m_funcName;   // ��� �������
        CString m_comment;    // ����������� � ����
        std::chrono::system_clock::time_point startLogging; // ����� ������ �����������
        bool m_bEnableLog = true;           // ���� ����������� �����������
        bool m_bEnableCalcTime = false;     // ���� ������������� ����������� ������� ����������

        int m_currentLoggingLevel;  // ������� �� ������� ������������ ����� � ����
    };

} // custom_logger

// �������� ������� � ������������� ����� �������
#define OUTPUT_LOG_FUNC std::shared_ptr<custom_logger::Logging> outputLogger = custom_logger::g_enableLogger ? \
                                        std::make_shared<custom_logger::Logging>(CString(__func__), false) : nullptr;

// �������� ������� � ������������� ����� ������� � ������������ ��� � ������� �����
#define OUTPUT_LOG_FUNC_ENTER  std::shared_ptr<custom_logger::Logging> outputLogger = custom_logger::g_enableLogger ? \
                                        std::make_shared<custom_logger::Logging>(CString(__func__), true) : nullptr;

// ���������� ����������� � �������
#define OUTPUT_LOG_ADD_COMMENT(...) if (outputLogger) outputLogger->addComment(__VA_ARGS__);

// ��������� ����������� � �������
#define OUTPUT_LOG_SET_TEXT(...) if (outputLogger) outputLogger->setText(__VA_ARGS__);

// ����� ������ � ��� �� �������� ������ �������
#define OUTPUT_LOG_TEXT(...) if (outputLogger) outputLogger->outputText(__VA_ARGS__);

// ���������� �����������
#define OUTPUT_LOG_DISABLE  if (outputLogger) outputLogger->disableLog();

// ��������� ����������� ������� ����������
#define OUTPUT_LOG_CALCTIME_ENABLE(enable) if (outputLogger) outputLogger->enableCalcTime(enable);

// ����������� ����������� �������� �����������
#define OUTPUT_LOG_DO  if (outputLogger) outputLogger->outputDebug();

// �������� �����������
#define OUTPUT_LOG_ENABLELOGGING(expression)  custom_logger::g_enableLogger = !!(expression);

// ���������� ������� � ������ ������������ ���������
#define OUTPUT_LOG_DEBUGBREAK(expression)  if (!!(expression) && custom_logger::g_enableLogger) ::DebugBreak();

#else   // OUTPUT_LOGGING

#define OUTPUT_LOG_FUNC ;
#define OUTPUT_LOG_FUNC_ENTER ;
#define OUTPUT_LOG_ADD_COMMENT(...) ;
#define OUTPUT_LOG_SET_TEXT(...) ;
#define OUTPUT_LOG_TEXT(...) ;
#define OUTPUT_LOG_DISABLE ;
#define OUTPUT_LOG_CALCTIME_ENABLE(enable) ;
#define OUTPUT_LOG_DO ;
#define OUTPUT_LOG_ENABLELOGGING(expression) ;
#define OUTPUT_LOG_DEBUGBREAK(expression) ;
#endif
