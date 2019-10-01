#pragma once

#include <list>
#include "local_server_win.h"
#include "demo_connection.h"

class demo_server : public local_server_win
{
public:
    demo_server();
    virtual ~demo_server();

    std::list<demo_connection_ptr> connections(void) { return m_connections; }

protected:
    virtual void on_new_connection(void* socket) override;
    virtual void on_stopping(void) override;


private:
    std::mutex m_mutex;
    std::list<demo_connection_ptr> m_connections;
};
