#include <WorkphoneNetwork/workphone_network.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#endif

#define CHECK(condition) do { if(!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; } } while(0)
static NetContext server, clients[3], probe;
static void pause_ms(void)
{
#if defined(_WIN32)
    Sleep(1);
#else
    struct timespec delay = {0, 1000000}; nanosleep(&delay, NULL);
#endif
}
static void pump(void)
{
    int i;
    net_update(&server);
    for(i=0;i<3;++i) net_update(&clients[i]);
    pause_ms();
}
static void drain(NetContext *ctx)
{
    NetEvent event;
    while(net_poll_event(ctx,&event)) {}
}
static int raw_send(NetContext *from, unsigned short port, const void *bytes, size_t size)
{
    struct sockaddr_in target;
    memset(&target,0,sizeof(target)); target.sin_family=AF_INET;
    target.sin_addr.s_addr=htonl(0x7f000001); target.sin_port=htons(port);
    return sendto(from->socket_handle,(const char*)bytes,(int)size,0,(struct sockaddr*)&target,sizeof(target));
}
int main(void)
{
    int i, j, connected, count;
    unsigned int protocol, before, start;
    unsigned short port;
    NetEvent event;
    unsigned char forged[25], oversized[NET_MAX_PACKET_SIZE+NET_HEADER_SIZE+32];
    char guard[4] = {'a','b','c','d'};
    CHECK(net_init()==NET_RESULT_OK);
    net_context_init(&server); net_context_init(&probe);
    for(i=0;i<3;++i) net_context_init(&clients[i]);
    CHECK(net_start_server(&server,0,3)==NET_RESULT_OK);
    port=net_get_bound_port(&server); CHECK(port!=0);
    CHECK(net_start_server(&server,0,3)==NET_RESULT_ERROR);
    CHECK(net_start_server(&probe,port,3)==NET_RESULT_ERROR);
    CHECK(net_start_client(&probe)==NET_RESULT_OK);
    CHECK(sizeof(NetSocketHandle)>=sizeof(server.socket_handle));
    net_address_to_string(net_make_address(0x7f000001,65535),guard+1,2);
    CHECK(guard[0]=='a' && guard[2]=='\0' && guard[3]=='d');

    /* Unsolicited and wrong-endpoint accepts cannot create a connection. */
    memset(forged,0,sizeof(forged)); protocol=htonl(NET_PROTOCOL_ID);
    memcpy(forged,&protocol,4); forged[4]=NET_MSG_CONNECT_ACCEPT; forged[24]=99;
    CHECK(raw_send(&server,net_get_bound_port(&probe),forged,sizeof(forged))==(int)sizeof(forged));
    pause_ms(); net_update(&probe);
    CHECK(probe.server_peer_id==NET_INVALID_PEER && probe.local_player_id==NET_INVALID_PEER);

    for(i=0;i<3;++i)
    {
        CHECK(net_start_client(&clients[i])==NET_RESULT_OK);
        CHECK(net_connect(&clients[i],"127.0.0.1",port)==NET_RESULT_OK);
    }
    memcpy(forged+NET_HEADER_SIZE,clients[0].attempt_nonce,8);
    CHECK(raw_send(&probe,net_get_bound_port(&clients[0]),forged,sizeof(forged))==(int)sizeof(forged));
    pause_ms(); net_update(&clients[0]); CHECK(clients[0].connecting);
    start=net_time_ms();
    do
    {
        pump(); connected=0;
        for(i=0;i<3;++i) connected += clients[i].server_peer_id!=NET_INVALID_PEER;
    } while(connected!=3 && (unsigned int)(net_time_ms()-start)<3000);
    CHECK(connected==3);
    for(i=0;i<3;++i)
    {
        CHECK(clients[i].local_player_id>0);
        for(j=i+1;j<3;++j) CHECK(clients[i].local_player_id!=clients[j].local_player_id);
        drain(&clients[i]);
    }
    count=0; while(net_poll_event(&server,&event)) if(event.type==NET_EVENT_CONNECTED) ++count;
    CHECK(count==3);
    CHECK(net_send(&clients[0],clients[0].server_peer_id,NULL,1)==NET_RESULT_ERROR);
    CHECK(net_send(&clients[0],clients[0].server_peer_id,"x",NET_MAX_PACKET_SIZE+1)==NET_RESULT_TOO_LARGE);
    CHECK(net_send_reliable(&clients[0],clients[0].server_peer_id,"x",1)==NET_RESULT_UNSUPPORTED);
    CHECK(net_send_to_server(&clients[0],"hello",5)==NET_RESULT_OK);
    for(i=0;i<10;++i) pump();
    count=0; while(net_poll_event(&server,&event)) if(event.type==NET_EVENT_PACKET)
    {
        CHECK(event.peer_id==clients[0].local_player_id);
        CHECK(event.size==5 && memcmp(event.data,"hello",5)==0); ++count;
    }
    CHECK(count==1);

    /* Oversized datagrams must be rejected, never accepted as a prefix. */
    memset(oversized,0,sizeof(oversized)); memcpy(oversized,&protocol,4); oversized[4]=NET_MSG_USER;
    CHECK(raw_send(&clients[0],port,oversized,sizeof(oversized))==(int)sizeof(oversized));
    for(i=0;i<5;++i) pump();
    CHECK(server.malformed_datagrams>0);
    while(net_poll_event(&server,&event)) CHECK(event.type!=NET_EVENT_PACKET);

    server.receive_budget=2; before=server.datagrams_received;
    for(i=0;i<8;++i) CHECK(net_send_to_server(&clients[0],"x",1)==NET_RESULT_OK);
    pause_ms(); net_update(&server); CHECK(server.datagrams_received-before<=2);
    server.receive_budget=NET_MAX_DATAGRAMS_PER_UPDATE;
    for(i=0;i<10;++i) pump(); drain(&server);

    /* Receive saturation is bounded; control space remains reserved. */
    for(i=0;i<400;++i)
    {
        CHECK(net_send_to_server(&clients[0],"x",1)==NET_RESULT_OK);
        if(i%16==0) net_update(&server);
    }
    net_update(&server);
    CHECK((server.event_tail+NET_MAX_EVENTS-server.event_head)%NET_MAX_EVENTS <
          NET_MAX_EVENTS-NET_CONTROL_EVENT_RESERVE);
    drain(&server);
    for(i=0;i<10;++i) { pump(); drain(&server); }

    net_disconnect(&clients[1]); CHECK(clients[1].server_peer_id==NET_INVALID_PEER);
    CHECK(clients[1].local_player_id==NET_INVALID_PEER);
    count=0; while(net_poll_event(&clients[1],&event)) if(event.type==NET_EVENT_DISCONNECTED) ++count;
    CHECK(count==1);
    for(i=0;i<10;++i) pump(); drain(&server);
    j=clients[0].local_player_id;
    net_disconnect(&clients[0]); drain(&clients[0]);
    for(i=0;i<10;++i) pump(); drain(&server);
    CHECK(net_connect(&clients[0],"127.0.0.1",port)==NET_RESULT_OK);
    for(i=0;i<20;++i) pump(); CHECK(clients[0].local_player_id>j);

    /* Retry/timeout scheduling is deterministic, including uint32 wrap. */
    CHECK(net_connect(&probe,"127.0.0.1",net_get_bound_port(&clients[1]))==NET_RESULT_OK);
    probe.connect_started_ms=0xfffffff0u; probe.last_connect_send_ms=0xfffffff0u;
    net_update_at(&probe,0x200u); CHECK(probe.connecting && probe.last_connect_send_ms==0x200u);
    net_update_at(&probe,0xfffffff0u+probe.connect_timeout_ms);
    CHECK(!probe.connecting); count=0;
    while(net_poll_event(&probe,&event)) if(event.type==NET_EVENT_ERROR) ++count;
    CHECK(count==1);
    net_update_at(&probe,0xfffffff0u+probe.connect_timeout_ms+1u);
    CHECK(!net_poll_event(&probe,&event));

    for(i=0;i<3;++i) { net_context_shutdown(&clients[i]); net_context_shutdown(&clients[i]); }
    net_context_shutdown(&probe); net_context_shutdown(&server);
    CHECK(net_get_bound_port(&server)==0 && server.local_player_id==NET_INVALID_PEER);
    CHECK(net_start_server(&server,port,3)==NET_RESULT_OK);
    net_context_shutdown(&server); net_shutdown();
    puts("PASS: native sockets, distinct identities, handshake binding, budgets, bounds, timeout and teardown");
    return 0;
}
