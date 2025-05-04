#ifndef WIFI_UDP_H
#define WIFI_UDP_H

extern bool ntp_time_ready;

int wifi_udp_init();
void start_wifi_threads();

void get_current_ntp_time(datetime_t *out_time);

#endif