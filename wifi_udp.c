#include "wifi_udp.h"
#include "pt_cornell_rp2040_v1_3.h"

uint32_t country = CYW43_COUNTRY_USA;
uint32_t auth    = CYW43_AUTH_WPA2_MIXED_PSK;

// Wifi information
// #define WIFI_SSID "georg’s iPhone"
// #define WIFI_PASSWORD "Marioiscool57"

// #define WIFI_SSID "CTT-WIFI"
// #define WIFI_PASSWORD "5FBUMP4E"

#define WIFI_SSID "Parker’s iPhone"
#define WIFI_PASSWORD "Temp123!"

bool ntp_time_ready = false;
static NTP_T ntp;
void update_ntp_time(datetime_t *ntp_time);

// Flag to indicate if the screen needs to be updated
static datetime_t current_ntp_time = {0};

// Protocol control block for UDP receive connection
static struct udp_pcb *udp_rx_pcb;

// Buffer in which to copy received messages
char received_data[BEACON_MSG_LEN_MAX];

// Semaphore for signaling a new received message
struct pt_sem new_message;

// WiFi MAC address storage
char mac_address_str[18];

// Function to get the latest UDP message
const char *get_latest_udp_message() {
  return received_data;
}

// Function to receive UDP messages
static void udpecho_raw_recv(
  void *arg, 
  struct udp_pcb *upcb, 
  struct pbuf *p,
  const ip_addr_t *addr, 
  u16_t port
) {
  LWIP_UNUSED_ARG(arg);
  if (p != NULL) {
    memcpy(received_data, p->payload, BEACON_MSG_LEN_MAX);
    PT_SEM_SAFE_SIGNAL(pt, &new_message);
    memset(p->payload, 0, BEACON_MSG_LEN_MAX + 1);
    pbuf_free(p);
  }
  else
    printf("NULL pbuf in callback\n");
}

// Function to initialize the UDP receive connection
static void udpecho_raw_init() {
  udp_rx_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (udp_rx_pcb != NULL){
    err_t err = udp_bind(udp_rx_pcb, netif_ip_addr4(netif_default), UDP_PORT);
    if (err == ERR_OK)
      udp_recv(udp_rx_pcb, udpecho_raw_recv, NULL);
    else
      printf("Bind error!\n");
  } else 
    printf("UDP PCB allocation error!\n");
}

// Function to get the MAC address of the device
static void get_MAC_address(
  char *mac
) {
  struct netif *netif = netif_default;
  if (netif == NULL) {
    sprintf(mac, "00:00:00:00:00:00");
    return;
  }
  const uint8_t *mac_addr = (const uint8_t *)netif->hwaddr;
  sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac_addr[0], mac_addr[1], mac_addr[2],
          mac_addr[3], mac_addr[4], mac_addr[5]);
}

// Function to send an NTP request
void ntp_send_request() {
  memset(ntp.ntp_request, 0, NTP_PACKET_SIZE);
  ntp.ntp_request[0] = 0b11100011; // LI, Version, Mode
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_PACKET_SIZE, PBUF_RAM);
  if (!p) return;
  memcpy(p->payload, ntp.ntp_request, NTP_PACKET_SIZE);
  udp_sendto(ntp.pcb, p, &ntp.ntp_server_ip, NTP_PORT);
  pbuf_free(p);
}

// DNS callback function
static void dns_callback(
  const char *name, 
  const ip_addr_t *ipaddr, 
  void *arg
) {
  if (ipaddr) {
    ntp.ntp_server_ip = *ipaddr;
    ntp_send_request();
  } else
    printf("DNS resolution failed for %s\n", name);
}

// Function to receive NTP packets
void ntp_recv(
  void *arg, 
  struct udp_pcb *pcb, 
  struct pbuf *p,
  const ip_addr_t *addr, 
  u16_t port
) {
  if (p && p->len >= NTP_PACKET_SIZE) {
    uint8_t *payload = (uint8_t *)p->payload;
    uint32_t seconds_since_1900 =
        (payload[40] << 24) | (payload[41] << 16) |
        (payload[42] << 8) | payload[43];
    time_t unix_time = seconds_since_1900 - NTP_UNIX_OFFSET;
    int timezone_offset_hours = -4;
    unix_time += timezone_offset_hours * 3600;
    struct tm *utc = gmtime(&unix_time);
    datetime_t dt = {
        .year = utc->tm_year + 1900,
        .month = utc->tm_mon + 1,
        .day = utc->tm_mday,
        .hour = utc->tm_hour,
        .min = utc->tm_min,
        .sec = utc->tm_sec,
    };
    update_ntp_time(&dt);
    printf("NTP time synced: %04d-%02d-%02d %02d:%02d:%02d\n",
           dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
  }
  pbuf_free(p);
}

// Initialize NTP
void ntp_init() {
  ntp.pcb = udp_new();
  if (!ntp.pcb) {
    printf("Failed to create PCB\n");
    return;
  }
  udp_recv(ntp.pcb, ntp_recv, NULL);
  err_t res = dns_gethostbyname(NTP_SERVER, &ntp.ntp_server_ip, dns_callback, NULL);
  if (res == ERR_OK)
    ntp_send_request();
  else if (res == ERR_INPROGRESS)
    printf("DNS lookup happening");
  else
    printf("DNS lookup failed");
}

// Update NTP time
void update_ntp_time(
  datetime_t *ntp_time
) {
  if (ntp_time) {
    current_ntp_time = *ntp_time;
    ntp_time_ready = true;
  }
}

// Get current NTP time
void get_current_ntp_time(
  datetime_t *out_time
) {
  if (out_time)
    *out_time = current_ntp_time;
}

// UDP receive thread
static PT_THREAD(
  protothread_receive(struct pt *pt)
) {
  PT_BEGIN(pt);
  while (1) {
    PT_SEM_SAFE_WAIT(pt, &new_message);
    screen_update_needed = true;
    printf("Received: %s\n", received_data);
  }
  PT_END(pt);
}

// UDP send thread
static PT_THREAD(
  protothread_send(struct pt *pt)
) {
  PT_BEGIN(pt);
  static struct udp_pcb *pcb;
  static ip_addr_t addr;

  pcb = udp_new();
  ipaddr_aton(BEACON_TARGET, &addr);

  while (1) {
    // Prompt the user
    sprintf(pt_serial_out_buffer, "> ");
    serial_write;

    // Perform a non-blocking serial read for a string
    serial_read;

    // Allocate a PBUF
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, BEACON_MSG_LEN_MAX + 1, PBUF_RAM);
    char *req = (char *)p->payload;
    memset(req, 0, BEACON_MSG_LEN_MAX + 1);

    // Fill the pbuf payload
    snprintf(req, BEACON_MSG_LEN_MAX, "%s: %s\n",
             ip4addr_ntoa(netif_ip_addr4(netif_default)), pt_serial_in_buffer);

    // Send the UDP packet
    err_t er = udp_sendto(pcb, p, &addr, UDP_PORT);
    pbuf_free(p);
    if (er != ERR_OK)
      printf("Failed to send UDP packet! error=%d\n", er);
  }
  PT_END(pt);
}

// Function for connecting to a WiFi access point
int wifi_connect(
  uint32_t country, 
  const char *ssid, 
  const char *pass, 
  uint32_t auth, 
  char** ip
) {

  // Initialize the hardware
  if (cyw43_arch_init_with_country(country)) {
    printf("Failed to initialize hardware.\n");
    return 1;
  }

  // Put the device into station mode
  cyw43_arch_enable_sta_mode();

  // Print a status message
  printf("Attempting connection . . . \n");

  // Connect to the network (timeout after 5 seconds)
  if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, auth, 10000))
    return 2;
  
  // Report the IP, netmask, and gateway that we've been assigned
  printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default)));
  printf("Mask: %s\n", ip4addr_ntoa(netif_ip_netmask4(netif_default)));
  printf("Gateway: %s\n", ip4addr_ntoa(netif_ip_gw4(netif_default)));
  *ip = ip4addr_ntoa(netif_ip_addr4(netif_default));

  return 0;
}

// Initialize WiFi and UDP
int wifi_udp_init(
  char** ip, 
  int* port, 
  char** mac
) {
  *port = UDP_PORT;

  // Connect to WiFi
  if (wifi_connect(country, WIFI_SSID, WIFI_PASSWORD, auth, ip)) {
    printf("Failed WiFi connection.\n");
    return 1;
  }

  // Get MAC address
  get_MAC_address(mac_address_str);
  printf("Pico W MAC Address: %s\n", mac_address_str);
  *mac = mac_address_str;

  // Initialize NTP
  PT_SEM_INIT(&new_message, 0);
  udpecho_raw_init();
  ntp_init();
  return 0;
}

// Start the WiFi threads
void start_wifi_threads() {
  pt_add_thread(protothread_send);
  pt_add_thread(protothread_receive);
}
