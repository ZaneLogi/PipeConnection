// PipeClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

static int PACKET_MAX_SIZE = 1024;

#pragma pack(1)
typedef struct packet_header
{
    uint32_t packet_type;
    uint32_t payload_size;
} packet_header;
#pragma pack()

enum eErrorType
{
    SUCCESS,
    ERROR_WRONG_PARAMETER,
    ERROR_NULL_PARAMETER,
    ERROR_WRONG_FILEPATH,
    ERROR_SDK_NOT_INITIALIZED,
    ERROR_SDK_ALREADY_INITIALIZED,
    ERROR_CONNECTION_BROKEN,
    ERROR_CREATING_THREAD,
    ERROR_MEMORY_COPY
};

static LONG g_lastError = SUCCESS;
static LONG g_initialized = 0;

class client_connection
{
public:
    client_connection()
    {
        m_pipe = INVALID_HANDLE_VALUE;
        ZeroMemory(m_bufReceive, sizeof(m_bufReceive));
        ZeroMemory(&m_overlapped, sizeof(m_overlapped));
    }

    virtual ~client_connection(void)
    {
        if (INVALID_HANDLE_VALUE != m_pipe)
        {
            CancelIo(m_pipe);
            if (m_pipe != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_pipe);
                m_pipe = INVALID_HANDLE_VALUE;
            }
        }
    }

    bool connectToPipe()
    {
        char szPipeName[64];
        sprintf_s(szPipeName, "\\\\.\\pipe\\%s-%08x", "DEMO_SERVER", WTSGetActiveConsoleSessionId());
        std::cout << szPipeName << std::endl;

        for (;;)
        {
            m_pipe = CreateFileA(szPipeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

            if (m_pipe != INVALID_HANDLE_VALUE)
            {
                std::cout << "pipe is created successfully\n";
                return true;
            }
            else
            {
                if (GetLastError() != ERROR_PIPE_BUSY)
                {
                    std::cout << "pipe is error\n";
                    return false;
                }
                // All pipe instances are busy, so wait for 2 seconds.
                if (!WaitNamedPipeA(szPipeName, 2000))
                {
                    std::cout << "pipe is busy, wait\n";
                    return false;
                }
            }
        }
    }

    bool startReading()
    {
        ZeroMemory(&m_overlapped, sizeof(m_overlapped));
        m_overlapped.hEvent = (HANDLE)this;

        if (!ReadFileEx(m_pipe, m_bufReceive, PACKET_MAX_SIZE, &m_overlapped, _OnReadCompleted))
        {
            int ret = GetLastError();
            if (GetLastError() != ERROR_IO_PENDING)
            {
                CancelIo(m_pipe);
                CloseHandle(m_pipe);
                m_pipe = INVALID_HANDLE_VALUE;
            }
            InterlockedExchange(&g_lastError, ERROR_CONNECTION_BROKEN);
            return false;
        }
        InterlockedExchange(&g_lastError, SUCCESS);
        return true;
    }

    bool write(void *data, size_t size)
    {
        DWORD dwBytesWritten = 0;
        if (!WriteFile(m_pipe, data, (DWORD)size, &dwBytesWritten, NULL))
        {
            InterlockedExchange(&g_lastError, ERROR_CONNECTION_BROKEN);
            InterlockedExchange(&g_initialized, 0);
            return false;
        }
        FlushFileBuffers(m_pipe);
        InterlockedExchange(&g_lastError, SUCCESS);
        return true;
    }

    void shutdownAndCleanUp()
    {
        CancelIo(m_pipe);
    }

protected:
    void OnReadCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
    {
        if (ERROR_SUCCESS != dwErrorCode)
        {
            return;
        }

        if (dwNumberOfBytesTransfered != 0 && GetOverlappedResult(m_pipe, lpOverlapped, &dwNumberOfBytesTransfered, TRUE))
        {
            unsigned int offset = 0;
            while (offset < dwNumberOfBytesTransfered)
            {
                packet_header *hdr = (packet_header *)(m_bufReceive + offset);

                // do something
                std::string s((char*)(hdr+1), hdr->payload_size);
                std::cout << s.c_str();

                offset += hdr->payload_size + sizeof(packet_header);
            }

        }
        //Start next read
        if (!ReadFileEx(m_pipe, m_bufReceive, PACKET_MAX_SIZE, &m_overlapped, _OnReadCompleted))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                CancelIo(m_pipe);
                CloseHandle(m_pipe);
                m_pipe = INVALID_HANDLE_VALUE;
                InterlockedExchange(&g_lastError, ERROR_CONNECTION_BROKEN);
                InterlockedExchange(&g_initialized, 0);
            }
            return;
        }
    }

    static VOID CALLBACK _OnReadCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
    {
        client_connection *pConnection = (client_connection*)lpOverlapped->hEvent;
        if (NULL != pConnection)
        {
            pConnection->OnReadCompleted(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
        }
    }

private:
    static const int RECEIVE_BUFSIZE = 1024;

    HANDLE m_pipe;
    BYTE m_bufReceive[RECEIVE_BUFSIZE];
    OVERLAPPED m_overlapped;
};

int main()
{
    client_connection client;
    if (client.connectToPipe() && client.startReading())
    {
        while (true)
        {
            char msg[256];
            std::cout << "IN:";
            std::cin >> msg;
            if (std::string(msg) == "quit")
                break;

            char buf[256];
            char* payload = buf + sizeof(packet_header);
            sprintf_s(payload, 128, "message '%s'\n", msg);
            packet_header* header = (packet_header*)buf;
            header->packet_type = 0;
            header->payload_size = (uint32_t)strlen(payload);
            if (client.write(buf, sizeof(packet_header) + header->payload_size))
            {
                std::cout << "sending " << payload << "\n";
                SleepEx(100, TRUE);
            }
            else
            {
                std::cout << "connection broken\n";
                break;
            }
        }

        client.shutdownAndCleanUp();
    }
    return 0;
}

