#pragma once

#include <memory>
#include "local_server_connection_win.h"

#pragma pack(1)
typedef struct packet_header
{
    uint32_t packet_type;
    uint32_t payload_size;
} packet_header;
#pragma pack()


class demo_connection : public local_server_connection_win
{
public:
    demo_connection(void* socket);
    virtual ~demo_connection();

protected:
    virtual void on_read_bytes(const uint8_t* data, int size) override;
    virtual void on_disconnected() override;

private:
    std::vector<uint8_t> m_data;

};

using demo_connection_ptr = std::shared_ptr<demo_connection>;
