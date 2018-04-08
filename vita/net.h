#ifndef __NET_H_
#define __NET_H_

int net_init();
void net_finish();

void net_kcp_listen(uint16_t port);
void net_kcp_process();

#endif
