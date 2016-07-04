#include "contiki-conf.h"
#include "dev/leds.h"

#include "lib/list.h"
#include "lib/memb.h"
#include "dev/watchdog.h"
#include "lib/random.h"
/** RDC driver file*/
#include "./sr-sl.h"
/** */
#include "net/netstack.h"
#include "net/rime.h"
#include "net/linkaddr.h"
#include "sys/compower.h"



#include "./cc2420.h"
#include "dev/serial-line.h"
#include "node-id.h"

//#include "./dnde-neigh-G.h"
#include "./sr-nodes.h"

///=========================================================================/
///=========================================================================/
#define CDEBUG 0
#if CDEBUG
#include <stdio.h>
volatile char *cooja_debug_ptr;
#define COOJA_DEBUG_PRINTF(...) \
    do{ char tmp[100]; sprintf(tmp,__VA_ARGS__); cooja_debug_ptr=tmp; } while(0);
#else //COOJA_DEBUG_PRINTF
#define COOJA_DEBUG_PRINTF(...)
#endif //COOJA_DEBUG_PRINTF
///=========================================================================/
///=========================================================================/
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else   //DEBUG
#define PRINTF(...)
#endif //DEBUG

///=========================================================================/
///=========================================================================/
//LIST(srnodes_list);
/*#if CONF_NETWORK_SIZE != 0
MEMB(srnodes_memb, struct sr_nodes, CONF_NETWORK_SIZE);
#else //#if CONF_NETWORK_SIZE != 0
MEMB(srnodes_memb, struct sr_nodes, 2);
#endif //#if CONF_NETWORK_SIZE != 0*/

#define HASH_TABLE_SIZE 13
static struct hash_table_t hash_table[CONF_NETWORK_SIZE];
MEMB(srnodes_memb, struct sr_nodes, CONF_NETWORK_SIZE);
///=========================================================================/
///=========================================================================/
void sr_nodes_init(){
  uint8_t k;

  //list_init(srnodes_list);
  memb_init(&srnodes_memb);

  for(k = 0; k < HASH_TABLE_SIZE; k++){
      LIST_STRUCT_INIT(&hash_table[k], node);
  }

  
  //flush all nodes and initialize list of neighbors
  sr_nodes_flush();
}
///=========================================================================/
///=========================================================================/
uint8_t sr_nodes_get_key(const linkaddr_t *addr){
    uint16_t key = (addr->u8[1]<< 8)+ addr->u8[0];
    return (uint8_t)((key%HASH_TABLE_SIZE));
}
///=========================================================================/
///=========================================================================/
struct sr_nodes* sr_nodes_get(const linkaddr_t *addr){
    uint8_t key;
    struct sr_nodes *lc_ptr = NULL;
    
    key = sr_nodes_get_key (addr);

    lc_ptr = list_head(&hash_table[key].node_list);
    
    for(; lc_ptr != NULL; lc_ptr = list_item_next(lc_ptr)){
        if(linkaddr_cmp(&lc_ptr->addr, addr)){
            return lc_ptr;
        }
    }
    return NULL;
}

///=========================================================================/
///=========================================================================/
static void sr_nodes_addself(){
    uint8_t key;
    struct sr_nodes *nself = NULL;

    key = sr_nodes_get_key (&linkaddr_node_addr);

    nself = sr_nodes_get(&linkaddr_node_addr);

    if(nself == NULL){
        nself = memb_alloc(&srnodes_memb);
        if(nself != NULL){
            //copy own link address
            linkaddr_copy(&nself->addr, &linkaddr_node_addr);

            nself->hopc        = 0;
            nself->offset      = 0;

            nself->slot_gain   = 0;
            nself->spat_sim    = 0;
            nself->tmp_div     = 0;

            nself->period_len  = sr_sl_get_period();

            nself->next        = NULL;

            //add to list
            list_add(&hash_table[key].node_list, nself);
        }
    }//nself == NULL
}
///=========================================================================/
///=========================================================================/
void sr_nodes_flush(){
    uint8_t key;
    struct sr_nodes *nfls = NULL;

    for( k = 0; k < HASH_TABLE_SIZE; key++){

        while(1){
            nfls = list_pop(&hash_table[key].node_list);
            if(nfls != NULL){
                list_remove(&hash_table[key].node_list, nfls);
                memb_remove(&srnodes_memb, nfls);
            }else{
                //exit loop
                break;
            }
        }//end of while
    }
    //add self
    sr_node_addself();
}
///=========================================================================/
///=========================================================================/
static void sr_nodes_add_nbr(linkaddr_t *src_addr, 
                             uint8_t     offset,
			     uint8_t     period_len,
			     uint8_t     hopc
){
  
  uint8_t key;
  struct sr_nodes *novo_nbr = NULL;
  
  novo_nbr = memb_alloc(&srnodes_memb);
  
  if(novo_nbr != NULL){
      novo_nbr->hopc       = hopc;
      novo_nbr->offset     = offset;
      novo_nbr->period_len = period_len;
      
      novo_nbr->tmp_div    = 0;
      novo_nbr->spat_sim   = 0;
      novo_nbr->slot_gain  = 0;
      //copy address
      linkaddr_copy(&novo_nbr->addr, src_addr);
      
      novo_nbr->next = NULL;
      
      //get hash KEY
      key = sr_nodes_get_key (src_addr);

      list_add(&hash_table[key].node_list, novo_nbr);
      
      //set up timer here..
  }
}
///=========================================================================/
///=========================================================================/
static void sr_node_nbr_update(struct sr_nodes *nd_ptr,
                             uint8_t     offset,
			     uint8_t     hopc){

  if(nd_ptr != NULL){
      nd_ptr->offset     = offset;
      nd_ptr->hopc       = hopc;

      if(hopc == 1){
	 //trigger update timer here..
      }
  }
}			       
///=========================================================================/
///=========================================================================/
void 
sr_nodes_add_nbrs(sr_packet_t *pkt, uint8_t pld_len, uint8_t probe_offset){
    uint8_t spat_sim = 1, k = 0, offset_h1 = 0, offset_h2 = 0;

    //compute offset H1 here...
    if((probe_offset - pkt->offset)){

    }
    
    
    struct sr_nodes node = sr_nodes_get(&pkt->src_addr);
    
    if(node != NULL){
        //update neighbor here
        sr_node_nbr_update(node, offset_h1, 1);

        //update spatial similarity
        spat_sim++;
    }else{
        //add a new 1 hop neighbor here..
        sr_nodes_add_nbr(&pkt->src_addr, offset_h1, pkt->period_len, 1);
    }
    
    //now go over the packet payload and add every node if does not exist yet
    for(k = 0; k < pld_len; k++){
        uint8_t dpos = k*SR_DATA_LEN;
        sr_data_t *sr_item = (sr_data_t*)(&pkt->data[dpos]);

        if((!linkaddr_cmp(&sr_item->addr, &linkaddr_null)) &&
                (!linkaddr_cmp(&sr_item->addr, &linkaddr_node_addr))){

            struct sr_nodes *sr_node = sr_nodes_get(&sr_item->addr);

            if(sr_node != NULL){
                //increment spatial similarity..
                spat_sim++;
            }else{
                /*we have an unknown 2hop^* neighbor, add it
                1. compute node H2 offset,
                2. add neighbor*/


                //2.here we add the neighbor..
                sr_nodes_add_nbr(&sr_item->addr, offset_h2, sr_item->period_len, 2);
            }
        }
    }

}
///=========================================================================/
///=========================================================================/
uint8_t sr_nodes_add_data(uint8_t *data_ptr, uint8_t offset){
  uint8_t off_ptr = 0, key;
  
  struct sr_nodes *lst_ptr = NULL;
  
  for(key = 0; key < HASH_TABLE_SIZE; key++){
      uint8_t exit_loop = 0;
      lst_ptr = list_head(&hash_table[key].node_list);

      while(lst_ptr != NULL){
          if((lst_ptr->hopc < MAX_HOP_LIMIT)
                  && (!linkaddr_cmp(&lst_ptr->addr, &linkaddr_node_addr))
                  &&(!linkaddr_cmp(&lst_ptr->addr, &linkaddr_null))){

              //fill in the payload..
              sr_data_t *d2s  = (sr_data_t*)(&data_ptr[off_ptr]);

              d2s->hopcount   = lst_ptr->hopc + 1;
              d2s->offset     = lst_ptr->offset;
              d2s->period_len = lst_ptr->period_len;

              linkaddr_copy(&d2s->addr, &lst_ptr->addr);

              off_ptr = off_ptr + SR_DATA_LEN;

              if(off_ptr >= NETWORK_PAYLOAD_SIZE){
                  exit_loop = 1;
                  break;
              }
          } //end of IF

          //get next element in the list
          lst_ptr = list_item_next(lst_ptr);
      }//end while..

      if(exit_loop){
          break;
      }
  }

  if(off_ptr > NETWORK_PAYLOAD_SIZE){
      off_ptr = NETWORK_PAYLOAD_SIZE;
  }
  
  return off_ptr;
}
///=========================================================================/
///=========================================================================/
