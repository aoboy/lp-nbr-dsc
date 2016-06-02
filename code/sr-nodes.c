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
LIST(srnodes_list);
#if CONF_NETWORK_SIZE != 0
MEMB(srnodes_memb, struct sr_nodes, CONF_NETWORK_SIZE);
#else //#if CONF_NETWORK_SIZE != 0
MEMB(srnodes_memb, struct sr_nodes, 2);
#endif //#if CONF_NETWORK_SIZE != 0

///=========================================================================/
///=========================================================================/
void sr_nodes_init(){
  list_init(srnodes_list);
  memb_init(&srnodes_memb);
  
  //flush all nodes and initialize list of neighbors
  sr_nodes_flush();
}
///=========================================================================/
///=========================================================================/
struct sr_nodes* sr_nodes_get(const linkaddr_t *addr){
    struct sr_nodes *lc_ptr = NULL;
    
    lc_ptr = list_head(srnodes_list);
    
    for(; lc_ptr != NULL; lc_ptr = list_item_next(lc_ptr)){
	if(linkaddr_cmp(&lc_ptr->addr, addr)){
	    return lc_ptr;
	}
    }
    return NULL;
}

///=========================================================================/
///=========================================================================/
static void sr_node_addself(){
  struct sr_nodes *nself = NULL;
  
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
	  list_add(nself);
      }
  }//nself == NULL
}
///=========================================================================/
///=========================================================================/
void sr_nodes_flush(){
   struct sr_nodes *nfls = NULL;
   
   while(1){
       nfls = list_pop(srnodes_list);
       if(nfls != NULL){
	  list_remove(srnodes_list, nfls);
	  memb_remove(&srnodes_memb, nfls);
       }else{
	  sr_node_addself();
	  //exit loop
	  break;
       }     
   }
}
///=========================================================================/
///=========================================================================/
static void sr_nodes_add_nbr(linkaddr_t *src_addr, 
                             uint8_t     offset,
			     uint8_t     period_len,
			     uint8_t     hopc
){
  
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
      
      list_add(novo_nbr);
      
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
  uint8_t spat_sim = 0, k = 0, offset_h1 = 0, offset_h2 = 0;
  
  if((probe_offset - pkt->offset) 

}
///=========================================================================/
///=========================================================================/
uint8_t sr_nodes_add_data(uint8_t *data_ptr, uint8_t offset){
  uint8_t off_ptr = 0;
  
  struct sr_nodes *lst_ptr = NULL;
  
  lst_ptr = list_head(srnodes_list);
  while(lst_ptr != NULL){
     if((lst_ptr->hopc < MAX_HOP_LIMIT) &&
        (!linkaddr_cmp(&lst_ptr->addr, &linkaddr_node_addr))
        (!linkaddr_cmp(&lst_ptr->addr, &linkaddr_null))){
       
        sr_data_t *d2s = (sr_data_t*)(&data_ptr[off_ptr]);
     
        d2s->hopcount  = lst_ptr->hopc + 1;
        d2s->offset    = lst_ptr->offset;
	d2s->period_len= lst_ptr->period_len;
	
        linkaddr_copy(&d2s->addr, &lst_ptr->addr);
        
        off_ptr = off_ptr + SR_DATA_LEN;
     } //end of IF
     lst_ptr = list_item_next(lst_ptr);
  }//end for
  
  if(off_ptr){
      return off_ptr;
  }
  
  return 0;
}
///=========================================================================/
///=========================================================================/