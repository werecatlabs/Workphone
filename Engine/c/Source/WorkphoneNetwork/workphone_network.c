#include "workphone_network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined( _WIN32 )
#    define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <windows.h>
#    include <bcrypt.h>

typedef SOCKET NetSocket;
typedef int NetSockLen;

#    define NET_SOCKET_INVALID INVALID_SOCKET
#    define net_close_socket closesocket

#else
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <sys/ioctl.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <errno.h>

typedef int NetSocket;
typedef socklen_t NetSockLen;

#    define NET_SOCKET_INVALID ( -1 )
#    define net_close_socket close
#endif

#ifndef MSG_NOSIGNAL
#    define MSG_NOSIGNAL 0
#endif

static int net_socket_is_valid( NetSocketHandle socket_handle )
{
#if defined( _WIN32 )
    return socket_handle != INVALID_SOCKET;
#else
    return socket_handle >= 0;
#endif
}

static int net_event_count( const NetContext *ctx )
{
    return ( ctx->event_tail + NET_MAX_EVENTS - ctx->event_head ) % NET_MAX_EVENTS;
}

static void net_push_event( NetContext *ctx, const NetEvent *event )
{
    int next_tail;

    if( !ctx || !event )
        return;

    next_tail = ( ctx->event_tail + 1 ) % NET_MAX_EVENTS;

    if( next_tail == ctx->event_head ||
        ( event->type == NET_EVENT_PACKET &&
          net_event_count( ctx ) >= NET_MAX_EVENTS - NET_CONTROL_EVENT_RESERVE - 1 ) )
    {
        ++ctx->events_dropped;
        return;
    }

    ctx->events[ctx->event_tail] = *event;
    ctx->event_tail = next_tail;
}

static void net_push_error( NetContext *ctx, const char *message )
{
    NetEvent event;

    memset( &event, 0, sizeof( event ) );
    event.type = NET_EVENT_ERROR;
    event.peer_id = NET_INVALID_PEER;

    if( message )
    {
        strncpy( event.message, message, sizeof( event.message ) - 1 );
        event.message[sizeof( event.message ) - 1] = '\0';
    }

    net_push_event( ctx, &event );
}

static unsigned short net_host_to_network_u16( unsigned short x )
{
    return htons( x );
}

static unsigned short net_network_to_host_u16( unsigned short x )
{
    return ntohs( x );
}

static unsigned int net_host_to_network_u32( unsigned int x )
{
    return htonl( x );
}

static unsigned int net_network_to_host_u32( unsigned int x )
{
    return ntohl( x );
}

static void net_write_header( unsigned char *buffer, NetInternalMessage message_type,
                              unsigned short sequence, unsigned short ack, unsigned int ack_bits )
{
    unsigned int protocol_net;
    unsigned short sequence_net;
    unsigned short ack_net;
    unsigned int ack_bits_net;

    protocol_net = net_host_to_network_u32( NET_PROTOCOL_ID );
    sequence_net = net_host_to_network_u16( sequence );
    ack_net = net_host_to_network_u16( ack );
    ack_bits_net = net_host_to_network_u32( ack_bits );

    memcpy( buffer + 0, &protocol_net, 4 );
    buffer[4] = (unsigned char)message_type;
    memcpy( buffer + 5, &sequence_net, 2 );
    memcpy( buffer + 7, &ack_net, 2 );
    memcpy( buffer + 9, &ack_bits_net, 4 );
}

static int net_read_header( const unsigned char *buffer, unsigned int size, NetPacketHeader *out_header )
{
    unsigned int protocol_net;
    unsigned short sequence_net;
    unsigned short ack_net;
    unsigned int ack_bits_net;

    if( !buffer || !out_header )
        return NET_RESULT_ERROR;

    if( size < 13 )
        return NET_RESULT_ERROR;

    memcpy( &protocol_net, buffer + 0, 4 );
    memcpy( &sequence_net, buffer + 5, 2 );
    memcpy( &ack_net, buffer + 7, 2 );
    memcpy( &ack_bits_net, buffer + 9, 4 );

    out_header->protocol_id = net_network_to_host_u32( protocol_net );
    out_header->message_type = buffer[4];
    out_header->sequence = net_network_to_host_u16( sequence_net );
    out_header->ack = net_network_to_host_u16( ack_net );
    out_header->ack_bits = net_network_to_host_u32( ack_bits_net );

    if( out_header->protocol_id != NET_PROTOCOL_ID )
        return NET_RESULT_ERROR;

    return NET_RESULT_OK;
}

static int net_set_nonblocking( NetSocketHandle socket_handle )
{
#if defined( _WIN32 )
    u_long nonblocking;
    nonblocking = 1;

    if( ioctlsocket( (SOCKET)socket_handle, FIONBIO, &nonblocking ) != 0 )
        return NET_RESULT_ERROR;

    return NET_RESULT_OK;
#else
    int flags;

    flags = fcntl( socket_handle, F_GETFL, 0 );
    if( flags == -1 )
        return NET_RESULT_ERROR;

    if( fcntl( socket_handle, F_SETFL, flags | O_NONBLOCK ) == -1 )
        return NET_RESULT_ERROR;

    return NET_RESULT_OK;
#endif
}

static NetSocketHandle net_create_udp_socket( unsigned short port )
{
    NetSocket s;
    struct sockaddr_in addr;
    int yes;

    s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if( s == NET_SOCKET_INVALID )
        return NET_SOCKET_INVALID;

    yes = 1;
#if defined( _WIN32 )
    if( setsockopt( s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&yes, sizeof( yes ) ) != 0 )
    {
        net_close_socket( s );
        return NET_SOCKET_INVALID;
    }
#endif

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    addr.sin_port = htons( port );

    if( bind( s, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 )
    {
        net_close_socket( s );
        return NET_SOCKET_INVALID;
    }

    if( net_set_nonblocking( s ) != NET_RESULT_OK )
    {
        net_close_socket( s );
        return NET_SOCKET_INVALID;
    }

    return s;
}

static NetAddress net_address_from_sockaddr( const struct sockaddr_in *addr )
{
    NetAddress out;

    out.host = ntohl( addr->sin_addr.s_addr );
    out.port = ntohs( addr->sin_port );

    return out;
}

static void net_sockaddr_from_address( const NetAddress *address, struct sockaddr_in *out_addr )
{
    memset( out_addr, 0, sizeof( *out_addr ) );

    out_addr->sin_family = AF_INET;
    out_addr->sin_addr.s_addr = htonl( address->host );
    out_addr->sin_port = htons( address->port );
}

static int net_resolve_address( const char *host, unsigned short port, NetAddress *out_address )
{
    struct addrinfo hints;
    struct addrinfo *entry;
    const struct sockaddr_in *addr;

    if( !host || !*host || !out_address || port == 0 )
        return NET_RESULT_ERROR;

    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    entry = NULL;
    if( getaddrinfo( host, NULL, &hints, &entry ) != 0 || !entry )
        return NET_RESULT_ERROR;
    addr = (const struct sockaddr_in *)entry->ai_addr;
    out_address->host = ntohl( addr->sin_addr.s_addr );
    out_address->port = port;
    freeaddrinfo( entry );

    return NET_RESULT_OK;
}

static int net_send_raw( NetContext *ctx, const NetAddress *address, NetInternalMessage message_type,
                         const void *data, unsigned int size )
{
    unsigned char packet[NET_MAX_PACKET_SIZE + 13];
    struct sockaddr_in to_addr;
    int sent;
    unsigned int total_size;

    if( !ctx || !address || ( size != 0 && !data ) )
        return NET_RESULT_ERROR;

    if( !net_socket_is_valid( ctx->socket_handle ) )
        return NET_RESULT_ERROR;

    if( size > NET_MAX_PACKET_SIZE )
        return NET_RESULT_TOO_LARGE;

    net_write_header( packet, message_type, ctx->next_sequence++, 0, 0 );

    if( data && size > 0 )
        memcpy( packet + 13, data, size );

    total_size = size + 13;

    net_sockaddr_from_address( address, &to_addr );

    sent = sendto( ctx->socket_handle, (const char *)packet, (int)total_size, MSG_NOSIGNAL,
                   (struct sockaddr *)&to_addr, sizeof( to_addr ) );

    if( sent < 0 )
    {
#if defined( _WIN32 )
        if( WSAGetLastError() == WSAEWOULDBLOCK )
#else
        if( errno == EAGAIN || errno == EWOULDBLOCK )
#endif
            return NET_RESULT_BACKPRESSURE;
        return NET_RESULT_ERROR;
    }

    if( (unsigned int)sent != total_size )
        return NET_RESULT_ERROR;

    return NET_RESULT_OK;
}

static NetPeer *net_find_peer_by_id( NetContext *ctx, NetPeerId peer_id )
{
    int i;

    if( !ctx )
        return NULL;

    for( i = 0; i < ctx->max_peers; ++i )
    {
        if( ctx->peers[i].active && ctx->peers[i].id == peer_id )
            return &ctx->peers[i];
    }

    return NULL;
}

static NetPeer *net_find_peer_by_address( NetContext *ctx, const NetAddress *address )
{
    int i;

    if( !ctx || !address )
        return NULL;

    for( i = 0; i < ctx->max_peers; ++i )
    {
        if( ctx->peers[i].active && net_address_equal( &ctx->peers[i].address, address ) )
            return &ctx->peers[i];
    }

    return NULL;
}

static NetPeer *net_add_peer( NetContext *ctx, const NetAddress *address )
{
    int i;

    if( !ctx || !address )
        return NULL;

    for( i = 0; i < ctx->max_peers; ++i )
    {
        if( !ctx->peers[i].active )
        {
            memset( &ctx->peers[i], 0, sizeof( NetPeer ) );

            if( ctx->next_peer_id <= 0 || ctx->next_peer_id > 65535 )
                return NULL;
            ctx->peers[i].active = 1;
            ctx->peers[i].id = ctx->next_peer_id++;
            ctx->peers[i].address = *address;
            ctx->peers[i].last_receive_time_ms = net_time_ms();
            ctx->peers[i].next_send_sequence = 1;

            return &ctx->peers[i];
        }
    }

    return NULL;
}

static void net_remove_peer( NetContext *ctx, NetPeerId peer_id )
{
    NetPeer *peer;

    peer = net_find_peer_by_id( ctx, peer_id );
    if( peer )
        memset( peer, 0, sizeof( NetPeer ) );
}

static void net_handle_connect_request( NetContext *ctx, const NetAddress *from,
                                        const unsigned char *payload, unsigned int size,
                                        unsigned int now_ms )
{
    NetPeer *peer;
    NetEvent event;
    unsigned char reply[12];
    unsigned int assigned_id;

    if( !ctx || !from )
        return;

    if( ctx->mode != NET_MODE_SERVER || size != 8 )
        return;

    peer = net_find_peer_by_address( ctx, from );

    /* Repeated attempts from an occupied endpoint must not take its identity. */
    if( peer && memcmp( peer->attempt_nonce, payload, 8 ) != 0 )
        return;

    if( !peer )
    {
        peer = net_add_peer( ctx, from );

        if( !peer )
        {
            return;
        }

        memset( &event, 0, sizeof( event ) );
        event.type = NET_EVENT_CONNECTED;
        event.peer_id = peer->id;
        net_push_event( ctx, &event );
        memcpy( peer->attempt_nonce, payload, 8 );
    }

    peer->last_receive_time_ms = now_ms;
    memcpy( reply, payload, 8 );
    assigned_id = htonl( (unsigned int)peer->id );
    memcpy( reply + 8, &assigned_id, 4 );
    net_send_raw( ctx, from, NET_MSG_CONNECT_ACCEPT, reply, sizeof( reply ) );
}

static void net_handle_connect_accept( NetContext *ctx, const NetAddress *from,
                                       const unsigned char *payload, unsigned int size,
                                       unsigned int now_ms )
{
    NetPeer *peer;
    NetEvent event;
    unsigned int assigned_id;

    if( !ctx || !from )
        return;

    if( ctx->mode != NET_MODE_CLIENT || !ctx->connecting || size != 12 ||
        !net_address_equal( from, &ctx->server_address ) ||
        memcmp( payload, ctx->attempt_nonce, 8 ) != 0 )
        return;

    memcpy( &assigned_id, payload + 8, 4 );
    assigned_id = ntohl( assigned_id );
    if( assigned_id == 0 || assigned_id > 65535 )
        return;

    peer = net_find_peer_by_address( ctx, from );

    if( !peer )
    {
        peer = net_add_peer( ctx, from );

        if( !peer )
        {
            net_push_error( ctx, "Client peer table full" );
            return;
        }

        ctx->server_peer_id = peer->id;
        ctx->local_player_id = (NetPeerId)assigned_id;
        ctx->connecting = 0;
        peer->last_receive_time_ms = now_ms;

        memset( &event, 0, sizeof( event ) );
        event.type = NET_EVENT_CONNECTED;
        event.peer_id = peer->id;
        net_push_event( ctx, &event );
    }
}

static void net_handle_disconnect( NetContext *ctx, const NetAddress *from )
{
    NetPeer *peer;
    NetEvent event;

    if( !ctx || !from )
        return;

    peer = net_find_peer_by_address( ctx, from );

    if( !peer )
        return;

    memset( &event, 0, sizeof( event ) );
    event.type = NET_EVENT_DISCONNECTED;
    event.peer_id = peer->id;
    net_push_event( ctx, &event );

    net_remove_peer( ctx, peer->id );
    if( ctx->mode == NET_MODE_CLIENT )
    {
        ctx->server_peer_id = NET_INVALID_PEER;
        ctx->local_player_id = NET_INVALID_PEER;
        ctx->connecting = 0;
    }
}

static void net_handle_user_packet( NetContext *ctx, const NetAddress *from,
                                    const unsigned char *payload, unsigned int payload_size,
                                    unsigned int now_ms )
{
    NetPeer *peer;
    NetEvent event;

    if( !ctx || !from )
        return;

    peer = net_find_peer_by_address( ctx, from );

    if( !peer )
    {
        /*
            For server mode you may choose to ignore unknown peers.
            For convenience, this accepts them only after connect request,
            so unknown user packets are ignored.
        */
        return;
    }

    peer->last_receive_time_ms = now_ms;

    memset( &event, 0, sizeof( event ) );
    event.type = NET_EVENT_PACKET;
    event.peer_id = peer->id;

    if( payload && payload_size > 0 )
    {
        if( payload_size > NET_MAX_PACKET_SIZE )
            payload_size = NET_MAX_PACKET_SIZE;

        memcpy( event.data, payload, payload_size );
        event.size = payload_size;
    }

    net_push_event( ctx, &event );
}

int net_init( void )
{
#if defined( _WIN32 )
    WSADATA data;

    if( WSAStartup( MAKEWORD( 2, 2 ), &data ) != 0 )
        return NET_RESULT_ERROR;
#endif

    return NET_RESULT_OK;
}

void net_shutdown( void )
{
#if defined( _WIN32 )
    WSACleanup();
#endif
}

void net_context_init( NetContext *ctx )
{
    if( !ctx )
        return;

    memset( ctx, 0, sizeof( NetContext ) );

    ctx->mode = NET_MODE_NONE;
    ctx->socket_handle = NET_SOCKET_INVALID;
    ctx->running = 0;
    ctx->max_peers = NET_MAX_PEERS;
    ctx->server_peer_id = NET_INVALID_PEER;
    ctx->event_head = 0;
    ctx->event_tail = 0;
    ctx->next_sequence = 1;
    ctx->next_peer_id = 1;
    ctx->local_player_id = NET_INVALID_PEER;
    ctx->connect_timeout_ms = 10000;
    ctx->peer_timeout_ms = 15000;
    ctx->receive_budget = NET_MAX_DATAGRAMS_PER_UPDATE;
}

void net_context_shutdown( NetContext *ctx )
{
    if( !ctx )
        return;

    if( net_socket_is_valid( ctx->socket_handle ) )
    {
        net_close_socket( ctx->socket_handle );
        ctx->socket_handle = NET_SOCKET_INVALID;
    }

    ctx->running = 0;
    ctx->mode = NET_MODE_NONE;
    ctx->connecting = 0;
    ctx->server_peer_id = NET_INVALID_PEER;
    ctx->local_player_id = NET_INVALID_PEER;
    memset( ctx->peers, 0, sizeof( ctx->peers ) );
    ctx->event_head = ctx->event_tail = 0;
}

int net_start_server( NetContext *ctx, unsigned short port, int max_peers )
{
    if( !ctx || ctx->running )
        return NET_RESULT_ERROR;

    if( max_peers <= 0 )
        max_peers = NET_MAX_PEERS;

    if( max_peers > NET_MAX_PEERS )
        max_peers = NET_MAX_PEERS;

    ctx->socket_handle = net_create_udp_socket( port );

    if( !net_socket_is_valid( ctx->socket_handle ) )
    {
        net_push_error( ctx, "Failed to create server socket" );
        return NET_RESULT_ERROR;
    }

    ctx->mode = NET_MODE_SERVER;
    ctx->running = 1;
    ctx->max_peers = max_peers;
    ctx->local_player_id = 0;

    return NET_RESULT_OK;
}

int net_start_client( NetContext *ctx )
{
    if( !ctx || ctx->running )
        return NET_RESULT_ERROR;

    /*
        Bind to port 0 so OS assigns an ephemeral port.
    */
    ctx->socket_handle = net_create_udp_socket( 0 );

    if( !net_socket_is_valid( ctx->socket_handle ) )
    {
        net_push_error( ctx, "Failed to create client socket" );
        return NET_RESULT_ERROR;
    }

    ctx->mode = NET_MODE_CLIENT;
    ctx->running = 1;
    ctx->max_peers = 1;
    ctx->server_peer_id = NET_INVALID_PEER;

    return NET_RESULT_OK;
}

int net_connect( NetContext *ctx, const char *host, unsigned short port )
{
    if( !ctx )
        return NET_RESULT_ERROR;

    if( ctx->mode != NET_MODE_CLIENT || !ctx->running || ctx->connecting ||
        ctx->server_peer_id != NET_INVALID_PEER )
        return NET_RESULT_ERROR;

    if( net_resolve_address( host, port, &ctx->server_address ) != NET_RESULT_OK )
    {
        net_push_error( ctx, "Failed to resolve server address" );
        return NET_RESULT_ERROR;
    }

#if defined( _WIN32 )
    if( BCryptGenRandom( NULL, ctx->attempt_nonce, sizeof( ctx->attempt_nonce ),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG ) != 0 )
        return NET_RESULT_ERROR;
#else
    {
        FILE *random_file;
        size_t count;
        random_file = fopen( "/dev/urandom", "rb" );
        if( !random_file )
            return NET_RESULT_ERROR;
        count = fread( ctx->attempt_nonce, 1, sizeof( ctx->attempt_nonce ), random_file );
        fclose( random_file );
        if( count != sizeof( ctx->attempt_nonce ) )
            return NET_RESULT_ERROR;
    }
#endif
    ctx->connect_started_ms = ctx->last_connect_send_ms = net_time_ms();
    ctx->connecting = 1;
    if( net_send_raw( ctx, &ctx->server_address, NET_MSG_CONNECT_REQUEST,
                      ctx->attempt_nonce, sizeof( ctx->attempt_nonce ) ) != NET_RESULT_OK )
    {
        net_push_error( ctx, "Failed to send connection request" );
        ctx->connecting = 0;
        return NET_RESULT_ERROR;
    }

    return NET_RESULT_OK;
}

void net_update( NetContext *ctx )
{
    net_update_at( ctx, net_time_ms() );
}

void net_update_at( NetContext *ctx, unsigned int now_ms )
{
    unsigned char buffer[NET_MAX_PACKET_SIZE + NET_HEADER_SIZE + 1];
    struct sockaddr_in from_addr;
    NetAddress from;
    NetSockLen from_len;
    int received;
    NetPacketHeader header;
    unsigned int payload_size;
    unsigned char *payload;
    unsigned int processed;
    unsigned int budget;
    int i;
    NetPeer *peer;

    if( !ctx || !ctx->running )
        return;

    budget = ctx->receive_budget;
    if( budget == 0 || budget > NET_MAX_DATAGRAMS_PER_UPDATE )
        budget = NET_MAX_DATAGRAMS_PER_UPDATE;
    for( processed = 0; processed < budget &&
         net_event_count( ctx ) < NET_MAX_EVENTS - NET_CONTROL_EVENT_RESERVE - 1; ++processed )
    {
        memset( &from_addr, 0, sizeof( from_addr ) );
        from_len = (NetSockLen)sizeof( from_addr );

        received = recvfrom( ctx->socket_handle, (char *)buffer, sizeof( buffer ), 0,
                             (struct sockaddr *)&from_addr, &from_len );

        if( received < 0 )
        {
#if defined( _WIN32 )
            int error_code;
            error_code = WSAGetLastError();
            if( error_code == WSAEMSGSIZE )
            {
                ++ctx->malformed_datagrams;
                continue;
            }
            if( error_code != WSAEWOULDBLOCK && error_code != WSAECONNRESET )
#else
            if( errno == EINTR )
                continue;
            if( errno != EAGAIN && errno != EWOULDBLOCK )
#endif
                net_push_error( ctx, "UDP receive failed" );
            break;
        }

        ++ctx->datagrams_received;
        if( received > NET_MAX_PACKET_SIZE + NET_HEADER_SIZE )
        {
            ++ctx->malformed_datagrams;
            continue;
        }

        from = net_address_from_sockaddr( &from_addr );

        if( net_read_header( buffer, (unsigned int)received, &header ) != NET_RESULT_OK )
        {
            ++ctx->malformed_datagrams;
            continue;
        }

        payload = buffer + 13;
        payload_size = (unsigned int)received - 13;

        switch( (NetInternalMessage)header.message_type )
        {
        case NET_MSG_CONNECT_REQUEST:
            net_handle_connect_request( ctx, &from, payload, payload_size, now_ms );
            break;

        case NET_MSG_CONNECT_ACCEPT:
            net_handle_connect_accept( ctx, &from, payload, payload_size, now_ms );
            break;

        case NET_MSG_DISCONNECT:
            if( payload_size == 0 )
                net_handle_disconnect( ctx, &from );
            break;

        case NET_MSG_USER:
            net_handle_user_packet( ctx, &from, payload, payload_size, now_ms );
            break;

        case NET_MSG_PING:
            peer = net_find_peer_by_address( ctx, &from );
            if( peer && payload_size == 0 )
            {
                peer->last_receive_time_ms = now_ms;
                net_send_raw( ctx, &from, NET_MSG_PONG, NULL, 0 );
            }
            break;

        case NET_MSG_PONG:
            peer = net_find_peer_by_address( ctx, &from );
            if( peer && payload_size == 0 )
                peer->last_receive_time_ms = now_ms;
            break;

        default:
            break;
        }
    }

    if( ctx->connecting )
    {
        if( (unsigned int)( now_ms - ctx->connect_started_ms ) >= ctx->connect_timeout_ms )
        {
            ctx->connecting = 0;
            net_push_error( ctx, "Connection timed out" );
        }
        else if( (unsigned int)( now_ms - ctx->last_connect_send_ms ) >= 500 )
        {
            net_send_raw( ctx, &ctx->server_address, NET_MSG_CONNECT_REQUEST,
                          ctx->attempt_nonce, sizeof( ctx->attempt_nonce ) );
            ctx->last_connect_send_ms = now_ms;
        }
    }
    for( i = 0; i < ctx->max_peers; ++i )
    {
        peer = &ctx->peers[i];
        if( !peer->active )
            continue;
        if( (unsigned int)( now_ms - peer->last_receive_time_ms ) >= ctx->peer_timeout_ms )
        {
            net_handle_disconnect( ctx, &peer->address );
        }
        else if( (unsigned int)( now_ms - peer->last_send_time_ms ) >= 1000 )
        {
            net_send_raw( ctx, &peer->address, NET_MSG_PING, NULL, 0 );
            peer->last_send_time_ms = now_ms;
        }
    }
}

int net_poll_event( NetContext *ctx, NetEvent *out_event )
{
    if( !ctx || !out_event )
        return 0;

    if( ctx->event_head == ctx->event_tail )
        return 0;

    *out_event = ctx->events[ctx->event_head];
    ctx->event_head = ( ctx->event_head + 1 ) % NET_MAX_EVENTS;

    return 1;
}

int net_send( NetContext *ctx, NetPeerId peer_id, const void *data, unsigned int size )
{
    NetPeer *peer;

    if( !ctx || !ctx->running || ( size != 0 && !data ) )
        return NET_RESULT_ERROR;

    if( size > NET_MAX_PACKET_SIZE )
        return NET_RESULT_TOO_LARGE;

    peer = net_find_peer_by_id( ctx, peer_id );

    if( !peer )
        return NET_RESULT_NOT_READY;

    return net_send_raw( ctx, &peer->address, NET_MSG_USER, data, size );
}

int net_send_to_server( NetContext *ctx, const void *data, unsigned int size )
{
    if( !ctx )
        return NET_RESULT_ERROR;

    if( ctx->mode != NET_MODE_CLIENT )
        return NET_RESULT_ERROR;

    if( ctx->server_peer_id == NET_INVALID_PEER )
        return NET_RESULT_ERROR;

    return net_send( ctx, ctx->server_peer_id, data, size );
}

void net_disconnect_peer( NetContext *ctx, NetPeerId peer_id )
{
    NetPeer *peer;

    if( !ctx )
        return;

    peer = net_find_peer_by_id( ctx, peer_id );

    if( !peer )
        return;

    net_send_raw( ctx, &peer->address, NET_MSG_DISCONNECT, NULL, 0 );
    net_handle_disconnect( ctx, &peer->address );
}

void net_disconnect( NetContext *ctx )
{
    if( !ctx )
        return;

    if( ctx->mode == NET_MODE_CLIENT && ctx->connecting )
    {
        ctx->connecting = 0;
        net_push_error( ctx, "Connection cancelled" );
    }
    if( ctx->mode == NET_MODE_CLIENT && ctx->server_peer_id != NET_INVALID_PEER )
    {
        net_disconnect_peer( ctx, ctx->server_peer_id );
        ctx->server_peer_id = NET_INVALID_PEER;
    }
}

NetAddress net_make_address( unsigned int host, unsigned short port )
{
    NetAddress address;

    address.host = host;
    address.port = port;

    return address;
}

int net_address_equal( const NetAddress *a, const NetAddress *b )
{
    if( !a || !b )
        return 0;

    return a->host == b->host && a->port == b->port;
}

const char *net_address_to_string( NetAddress address, char *buffer, unsigned int buffer_size )
{
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;

    if( !buffer || buffer_size == 0 )
        return "";

    a = ( address.host >> 24 ) & 0xff;
    b = ( address.host >> 16 ) & 0xff;
    c = ( address.host >> 8 ) & 0xff;
    d = address.host & 0xff;

    if( snprintf( buffer, buffer_size, "%u.%u.%u.%u:%u", a, b, c, d, address.port ) < 0 )
        buffer[0] = '\0';
    buffer[buffer_size - 1] = '\0';

    return buffer;
}

unsigned int net_time_ms( void )
{
#if defined( _WIN32 )
    return (unsigned int)GetTickCount64();
#else
    struct timespec now;
    if( clock_gettime( CLOCK_MONOTONIC, &now ) != 0 )
        return 0;
    return (unsigned int)( (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000 );
#endif
}

int net_send_reliable( NetContext *ctx, NetPeerId peer_id, const void *data, unsigned int size )
{
    (void)ctx;
    (void)peer_id;
    (void)data;
    (void)size;
    return NET_RESULT_UNSUPPORTED;
}

unsigned short net_get_bound_port( const NetContext *ctx )
{
    struct sockaddr_in address;
    NetSockLen size;
    if( !ctx || !net_socket_is_valid( ctx->socket_handle ) )
        return 0;
    memset( &address, 0, sizeof( address ) );
    size = (NetSockLen)sizeof( address );
    if( getsockname( ctx->socket_handle, (struct sockaddr *)&address, &size ) != 0 )
        return 0;
    return ntohs( address.sin_port );
}
