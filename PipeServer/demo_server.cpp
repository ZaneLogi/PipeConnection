#include "stdafx.h"
#include "demo_server.h"

demo_server::demo_server(void) : local_server_win(false)
{
    m_pipeName = "DEMO_SERVER";
}

demo_server::~demo_server(void)
{
}

void demo_server::on_new_connection(void* socket)
{
    // add the connection to our list
    demo_connection_ptr connection = demo_connection_ptr(new demo_connection(socket, this));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.push_back(connection);
}

void demo_server::on_stopping(void)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_connections.empty())
    {
        demo_connection_ptr connection = m_connections.back();
        m_connections.pop_back();

        m_mutex.unlock();
        connection->close();
        m_mutex.lock();
    }
    local_server_win::on_stopping();
}
