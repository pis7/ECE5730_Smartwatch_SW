#ifndef WIFI_UDP_H
#define WIFI_UDP_H

#include "common.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include <time.h>

#define NTP_PORT 123
#define NTP_SERVER "pool.ntp.org"
#define NTP_PACKET_SIZE 48
#define NTP_UNIX_OFFSET 2208988800u
#define UDP_PORT 1234
#define BEACON_TARGET "172.20.10.4"
#define BEACON_MSG_LEN_MAX 127

typedef struct {
  struct udp_pcb *pcb;
  ip_addr_t ntp_server_ip;
  uint8_t ntp_request[NTP_PACKET_SIZE];
} NTP_T;

extern bool ntp_time_ready;
extern volatile bool screen_update_needed;
const char *get_latest_udp_message();
int wifi_udp_init(char** ip, int* port, char** mac);
void start_wifi_threads();
void get_current_ntp_time(datetime_t *out_time);

#endif