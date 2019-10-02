#include "stdafx.h"
#include <iostream>
#include "demo_connection.h"
#include "demo_server.h"

demo_connection::demo_connection(void* socket, demo_server* server) :
    local_server_connection_win(socket),
    m_server(server)
{
}

demo_connection::~demo_connection(void)
{
    close();
}

void demo_connection::on_read_bytes(const uint8_t* data, int size)
{
    m_data.insert(m_data.end(), data, data + size);
    while (true)
    {
        if (m_data.size() >= sizeof(packet_header))
        {
            // try to extract next command
            packet_header* in_header = (packet_header*)m_data.data();
            if (m_data.size() >= (in_header->payload_size + sizeof(packet_header)))
            {
                // extract the next command and process it
                // processCommand(header);
                std::string s((char*)m_data.data() + sizeof(packet_header), in_header->payload_size);
                std::cout << s.c_str();

                {
                    // notify the client
                    char buf[256];
                    char* payload = buf + sizeof(packet_header);
                    sprintf_s(payload, 128, "Server OK\n");
                    packet_header* out_header = (packet_header*)buf;
                    out_header->packet_type = 0;
                    out_header->payload_size = (uint32_t)strlen(payload);
                    write(buf, sizeof(packet_header) + out_header->payload_size);

                    auto connections = m_server->connections();
                    for (auto c : connections)
                    {
                        if (c.get() != this)
                        {
                            sprintf_s(payload, 128, "%s", s.c_str());
                            out_header->packet_type = 0;
                            out_header->payload_size = (uint32_t)strlen(payload);
                            c->write(buf, sizeof(packet_header) + out_header->payload_size);
                        }
                    }
                }

                // eat up the bytes consumed
                m_data.erase(m_data.begin(), m_data.begin() + in_header->payload_size + sizeof(packet_header));
            }
            else
            {
                // not enough bytes for a complete packet - maybe next time...
                break;
            }
        }
        else
        {
            // not enough bytes for even a header...
            break;
        }
    }
}

void demo_connection::on_disconnected()
{

}
