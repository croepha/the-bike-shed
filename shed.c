

#include <bits/types/struct_iovec.h>
#define LOG(fmt, ...) printf("%s:%s:%d:" fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define DEBUG(fmt, ...) LOG("DEBUG:" fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG("ERROR:" fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) LOG("FATAL:" fmt, ##__VA_ARGS__)

typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef   signed char   s8;
typedef   signed short s16;
typedef   signed int   s32;
typedef   signed long  s64;

#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <assert.h>
#include <netdb.h>



const u16 PEER_PORT = 5423; // TODO: What port number should we use?

u64 time_ms_fast() {
    struct timespec tp;
    int r = clock_gettime(CLOCK_REALTIME_COARSE, &tp);
    assert(r!=-1);
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

u8 buffer[1<<20];

enum {
    EVENT_TYPE_PEER,
};

int main() {
    DEBUG("Started");

    int ep_fd = epoll_create1(EPOLL_CLOEXEC);
    assert(ep_fd != -1);

    // TODO: Restore time from last saved time

    u64 next_timer_ms = time_ms_fast() + 1000; // Set a timer to go off 1 second from now

    // Setup the peer-to-peer socket...
    int peer_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert(peer_fd >= 0);
    struct sockaddr_in bind_addr = { .sin_family=AF_INET, .sin_port=htons(PEER_PORT), {.s_addr=INADDR_ANY}};
    int bind_ret = bind(peer_fd, (struct sockaddr*)&bind_addr, sizeof bind_addr);
    assert(bind_ret == 0);
    struct epoll_event ep_ctle = { .events=EPOLLIN, {.u64=EVENT_TYPE_PEER } };
    int r1 = epoll_ctl(ep_fd, EPOLL_CTL_ADD, peer_fd, &ep_ctle);
    assert(r1 == 0);

    for(;;) {
        struct epoll_event ep_event = {};

        u64 ep_now = time_ms_fast();
        s32 ep_timeout = -1;
        if (next_timer_ms) {
            if (next_timer_ms <= ep_now) {
                ep_timeout = 0;
            } else {
                ep_timeout = next_timer_ms - ep_now;
            }
        }
        int ep_r = epoll_wait(ep_fd, &ep_event, 1, ep_timeout);
        assert(ep_r == 0 || ep_r == 1);

        if (ep_r) {
            assert(ep_event.data.u64 == EVENT_TYPE_PEER);
            struct sockaddr_in peer_address;
            struct iovec _iov= {.iov_base=buffer, .iov_len=sizeof buffer};
            struct msghdr _mh = {
                .msg_name = &peer_address, .msg_namelen=sizeof peer_address,
                .msg_iov = &_iov, .msg_iovlen=1,
            };
            ssize_t buffer_len = recvmsg(peer_fd, &_mh, 0);
            assert(buffer_len>=0);
            char  peer_address_str[30];
            getnameinfo((struct sockaddr*)&peer_address, sizeof peer_address,
                peer_address_str, sizeof peer_address_str, 0,0,0 );
            peer_address_str[sizeof peer_address-1] = 0;
            DEBUG("Got a UDP packet of size %zd from %s: %08lx", buffer_len, peer_address_str, *(u64*)buffer);

        } else {// Timeout
            DEBUG("Timer fired...");
            next_timer_ms = time_ms_fast() + 1000; // Set a timer to go off 1 second from now
        }



    }

}
