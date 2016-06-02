



#include "contiki.h"
#include "lib/random.h"
#include "net/linkaddr.h"


#define MAX_HOP_LIMIT 2
//=========================================================================//
typedef struct{
  linkaddr_t addr;
  uint8_t period_len;
  uint8_t offset;
  uint8_t hopcount;
}__attribute__ ((__packed__)) sr_data_t;

#define SR_DATA_LEN sizeof(sr_data_t)
//=========================================================================//
#if CONF_NETWORK_SIZE != 0
static uint8_t num_neighbors = CONF_NETWORK_SIZE;

#if CONF_NETWORK_SIZE < 25
#define NETWORK_PAYLOAD_SIZE CONF_NETWORK_SIZE*SR_DATA_LEN
#else //CONF_NETWORK_SIZE<20
#define NETWORK_PAYLOAD_SIZE 24*SR_DATA_LEN
#endif //CONF_NETWORK_SIZE

#else //CONF_NETWORK_SIZE != 0
error "network size unspecified"
#endif //CONF_NETWORK_SIZE != 0
//=========================================================================//
typedef struct{
  uint8_t offset;
  uint8_t period_len;
  linkaddr_t src_addr;
  uint8_t data[NETWORK_PAYLOAD_SIZE];
}sr_packet_t;

#define SR_PACKET_LEN sizeof(sr_packet_t)
#define SR_SL_PKT_HDR_LEN 4
//=========================================================================//
struct sr_nodes{
   struct sr_nodes *next;
   linkaddr_t addr;
   uint8_t hopc;
   uint8_t period_len;
   uint8_t offset;
   uint8_t tmp_div;
   uint8_t spat_sim;
   uint16_t slot_gain;
   uint16_t j_factor;
   uint16_t t_anchor;
   struct ctimer nbr_timer; //update nbr offset
};
//=========================================================================//
enum{
  SR_SL_ANCHOR    = 0x11,
  SR_SL_PROBE     = 0x12
};
//=========================================================================//