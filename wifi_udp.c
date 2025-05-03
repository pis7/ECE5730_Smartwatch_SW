// wifi_udp.c

// Standard libraries
#include <string.h>

// LWIP libraries
#include "lwip/pbuf.h"
#include "lwip/udp.h"

// Pico SDK hardware support libraries
#include "hardware/sync.h"

// Protothreads
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "pt_cornell_rp2040_v1_3.h"

// Our header for making WiFi connection
#include "connect.h"
#include "wifi_udp.h"

// WiFi credentials
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"

// Destination port and IP address
#define UDP_PORT 1234
#define BEACON_TARGET "172.20.10.2"

// Maximum length of our message
#define BEACON_MSG_LEN_MAX 127

// =======================
// Global Variables

// Protocol control block for UDP receive connection
static struct udp_pcb *udp_rx_pcb;

// Buffer in which to copy received messages
char received_data[BEACON_MSG_LEN_MAX];

// Semaphore for signaling a new received message
struct pt_sem new_message;

// WiFi MAC address storage
char mac_address_str[18];

// =======================
// Internal Helper Functions

static void udpecho_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);

  if (p != NULL)
  {
    memcpy(received_data, p->payload, BEACON_MSG_LEN_MAX);
    PT_SEM_SAFE_SIGNAL(pt, &new_message);
    memset(p->payload, 0, BEACON_MSG_LEN_MAX + 1);
    pbuf_free(p);
  }
  else
  {
    printf("NULL pbuf in callback\n");
  }
}

static void udpecho_raw_init(void)
{
  udp_rx_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);

  if (udp_rx_pcb != NULL)
  {
    err_t err = udp_bind(udp_rx_pcb, netif_ip_addr4(netif_default), UDP_PORT);
    if (err == ERR_OK)
    {
      udp_recv(udp_rx_pcb, udpecho_raw_recv, NULL);
    }
    else
    {
      printf("Bind error!\n");
    }
  }
  else
  {
    printf("UDP PCB allocation error!\n");
  }
}

static void get_MAC_address(char *mac)
{
  struct netif *netif = netif_default;
  if (netif == NULL)
  {
    sprintf(mac, "00:00:00:00:00:00");
    return;
  }

  const uint8_t *mac_addr = (const uint8_t *)netif->hwaddr;
  sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac_addr[0], mac_addr[1], mac_addr[2],
          mac_addr[3], mac_addr[4], mac_addr[5]);
}

// =======================
// Protothreads for Send and Receive

static PT_THREAD(protothread_receive(struct pt *pt))
{
  PT_BEGIN(pt);

  while (1)
  {
    PT_SEM_SAFE_WAIT(pt, &new_message);

    // Print received message
    printf("Received: %s\n", received_data);

    // You can parse received command here if needed
  }

  PT_END(pt);
}

static PT_THREAD(protothread_send(struct pt *pt))
{
  PT_BEGIN(pt);

  static struct udp_pcb *pcb;
  pcb = udp_new();

  static ip_addr_t addr;
  ipaddr_aton(BEACON_TARGET, &addr);

  while (1)
  {
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
    {
      printf("Failed to send UDP packet! error=%d\n", er);
    }
  }

  PT_END(pt);
}

// =======================
// Public Functions (called by main app)

int wifi_udp_init()
{
  if (connectWifi(country, WIFI_SSID, WIFI_PASSWORD, auth))
  {
    printf("Failed WiFi connection.\n");
    return 1;
  }

  get_MAC_address(mac_address_str);
  printf("Pico W MAC Address: %s\n", mac_address_str);

  PT_SEM_INIT(&new_message, 0);

  udpecho_raw_init();
  return 0;
}

void start_wifi_threads()
{
  pt_add_thread(protothread_send);
  pt_add_thread(protothread_receive);
}
