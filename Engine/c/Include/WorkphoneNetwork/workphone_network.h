#ifndef NET_C89_H
#define NET_C89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*
    C89 RakNet-style replacement layer.

    Intended usage:

        NetContext net;
        NetEvent event;

        net_init();

        net_context_init(&net);
        net_start_server(&net, 45000, 32);

        while (running)
        {
            net_update(&net);

            while (net_poll_event(&net, &event))
            {
                ...
            }
        }

        net_context_shutdown(&net);
        net_shutdown();
*/

#define NET_MAX_PACKET_SIZE 1200
#define NET_MAX_EVENTS 256
#define NET_MAX_PEERS 64
#define NET_PROTOCOL_ID 0x57475031UL /* "WGP1" style id */
#define NET_INVALID_PEER ( -1 )

typedef int NetPeerId;

typedef enum NetResult
{
    NET_RESULT_OK = 0,
    NET_RESULT_ERROR = -1
} NetResult;

typedef enum NetMode
{
    NET_MODE_NONE = 0,
    NET_MODE_SERVER,
    NET_MODE_CLIENT
} NetMode;

typedef enum NetEventType
{
    NET_EVENT_NONE = 0,
    NET_EVENT_CONNECTED,
    NET_EVENT_DISCONNECTED,
    NET_EVENT_PACKET,
    NET_EVENT_ERROR
} NetEventType;

typedef enum NetInternalMessage
{
    NET_MSG_USER = 0,
    NET_MSG_CONNECT_REQUEST = 1,
    NET_MSG_CONNECT_ACCEPT = 2,
    NET_MSG_DISCONNECT = 3,
    NET_MSG_PING = 4,
    NET_MSG_PONG = 5
} NetInternalMessage;

typedef struct NetAddress
{
    unsigned int host;
    unsigned short port;
} NetAddress;

typedef struct NetEvent
{
    NetEventType type;
    NetPeerId peer_id;

    unsigned char data[NET_MAX_PACKET_SIZE];
    unsigned int size;

    char message[256];
} NetEvent;

typedef struct NetPeer
{
    int active;
    NetPeerId id;
    NetAddress address;

    unsigned int last_receive_time_ms;
    unsigned int last_send_time_ms;

    unsigned short next_send_sequence;
    unsigned short last_recv_sequence;
} NetPeer;

typedef struct NetPacketHeader
{
    unsigned int protocol_id;
    unsigned char message_type;
    unsigned short sequence;
    unsigned short ack;
    unsigned int ack_bits;
} NetPacketHeader;

typedef struct NetContext
{
    NetMode mode;

    int socket_handle;
    int running;

    NetPeer peers[NET_MAX_PEERS];
    int max_peers;

    NetPeerId server_peer_id;
    NetAddress server_address;

    NetEvent events[NET_MAX_EVENTS];
    int event_head;
    int event_tail;

    unsigned short next_sequence;
} NetContext;

/* Global socket startup/shutdown. Required on Windows. */
int net_init( void );
void net_shutdown( void );

/* Context lifetime. */
void net_context_init( NetContext *ctx );
void net_context_shutdown( NetContext *ctx );

/* Server/client startup. */
int net_start_server( NetContext *ctx, unsigned short port, int max_peers );
int net_start_client( NetContext *ctx );
int net_connect( NetContext *ctx, const char *host, unsigned short port );

/* Main update. Call once per frame/tick. */
void net_update( NetContext *ctx );

/* Events. */
int net_poll_event( NetContext *ctx, NetEvent *out_event );

/* Sending. */
int net_send( NetContext *ctx, NetPeerId peer_id, const void *data, unsigned int size );
int net_send_to_server( NetContext *ctx, const void *data, unsigned int size );
void net_disconnect_peer( NetContext *ctx, NetPeerId peer_id );
void net_disconnect( NetContext *ctx );

/* Address helpers. */
NetAddress net_make_address( unsigned int host, unsigned short port );
int net_address_equal( const NetAddress *a, const NetAddress *b );
const char *net_address_to_string( NetAddress address, char *buffer, unsigned int buffer_size );

/* Time helper. */
unsigned int net_time_ms( void );

#ifdef __cplusplus
}
#endif

#endif
