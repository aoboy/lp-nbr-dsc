



#ifndef SR_NODES_H
#define SR_NODES_H

#include "contiki.h"
#include "lib/random.h"
#include "lib/list.h"
#include "net/linkaddr.h"
#include "sys/ctimer.h"


//=========================================================================//
#define ONE_KILO 1024
#define ONE_MSEC (RTIMER_SECOND/ONE_KILO)
#define HALF_MSEC (ONE_MSEC/2)
//=========================================================================//
#define TS_LEN   (20*ONE_MSEC)

#define TS_LEN_50 (TS_LEN/2)
#define TS_LEN_25 (TS_LEN/4)
#define TS_LEN_75 ((3*TS_LEN)/4)
#define TS_LEN_20 (TS_LEN/5)
#define TS_LEN_80 (4*TS_LEN_20)
#define TS_LEN_10 (TS_LEN/10)
#define TS_LEn_90 (10*TS_LEN_10)


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
#error "network size unspecified"
#endif //CONF_NETWORK_SIZE != 0
//=========================================================================//
typedef struct{
  uint8_t offset;
  uint8_t period_len;
  linkaddr_t src_addr;
  uint8_t data[NETWORK_PAYLOAD_SIZE];
}sr_packet_t;

#define SR_PACKET_LEN sizeof(sr_packet_t)
#define SR_PACKET_HDR_LEN (SR_PACKET_LEN - NETWORK_PAYLOAD_SIZE)
//=========================================================================//
struct hash_table_t{
    LIST_STRUCT(node);
};
//=========================================================================//
struct sr_nodes{
   struct sr_nodes *next;
   linkaddr_t addr;
   uint8_t hopc;
   uint8_t period_len;
   uint8_t offset;
   uint8_t offset_0;
   uint8_t tmp_div;
   uint8_t spat_sim;
   uint16_t slot_gain;
   uint16_t j_factor;
   uint16_t t_anchor;
   struct ctimer nbr_timer; //update nbr offset
};
//=========================================================================//
struct sr_gains{
  linkaddr_t addr;
  uint8_t offset;
  uint16_t gains;
  struct sr_gains *next;
};
//=========================================================================//
enum{
  SR_SL_ANCHOR    = 0x11,
  SR_SL_PROBE     = 0x12
};
//=========================================================================//

#endif SR_NODES_H
