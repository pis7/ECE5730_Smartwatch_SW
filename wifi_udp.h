#ifndef WIFI_UDP_H
#define WIFI_UDP_H

extern bool ntp_time_ready;
extern volatile bool screen_update_needed;

const char *get_latest_udp_message();

int wifi_udp_init(char** ip, int* port, char** mac);
void start_wifi_threads();

void get_current_ntp_time(datetime_t *out_time);

#endif