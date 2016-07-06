/*
///=========================================================================/
 * ------------------------------------------------------------------------
 * @author: António Gonga <gonga@ee.kth.se>, PhD
 * @date: 01 June 2016
 * @file: sr-sl.h
 * @update: Adding ACC-accelerated asymmetric deterministic discovery.
 * @brief: file contains auxiliary functions and the main algorithm logic
           implementation.
///=========================================================================/ 
 * @info: soon
///=========================================================================*/
#include "contiki-conf.h"
#include "dev/leds.h"
#include "dev/radio.h"
#include "dev/watchdog.h"
#include "lib/random.h"
/** RDC driver file*/
#include "./sr-sl.h"
/** */
#include "net/netstack.h"
#include "net/rime.h"
#include "sys/compower.h"
#include "sys/pt.h"
#include "sys/rtimer.h"


#include "dev/leds.h"
#include "./cc2420.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include "node-id.h"

#include <string.h>

#include "sr-nodes.h"


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
//=========================================================================//
#define CCA_COUNT_MAX                    2
#define CCA_COUNT_MAX_TX                 6
#define CCA_CHECK_TIME                   (RTIMER_SECOND/8192)
#define CCA_SLEEP_TIME                   ((RTIMER_SECOND/200) +1)
//=========================================================================//
static volatile uint8_t  radio_is_on_flag                       = 0;

//=========================================================================//
static volatile uint16_t 		 slot_counter 		= 0;
static volatile uint16_t                 probe_time   		= 0;
static volatile uint16_t                 period_length 		= 0;
static volatile uint16_t                 anchor_time            = 0;
static volatile uint16_t                 node_slots_offset      = 0;
static volatile uint16_t 		 discovery_time  	= 0;
//=========================================================================//
static volatile uint8_t                  probe_offset           = 0;
static volatile uint8_t    		 discovery_is_on	= 0;
static volatile uint8_t    		 num_repetitions        = 0;
//=========================================================================//
static struct pt pt;
static struct rtimer generic_timer;
//=========================================================================//
#define DUTY_CYCLES_LEN 5
#define PERIODS_VEC_SIZE 4
static const uint8_t duty_cycles[]={1,1,2,3,5};
static const uint8_t periods_vec[]={200, 167, 133, 100};
//=========================================================================//
#define GROUP_MERGE_TIME 10233
static uint8_t i3e154_channels[]  ={11,15,20,25,26};



PROCESS(mainloop_process, "SR-SL Main Loop Process");
PROCESS(output_process,   "Output Process");

//=========================================================================//
void sr_sl_update_nbr(void* nbr_tgt){
  
}
//=========================================================================//
static void sr_sl_send_probe(void *ptr){
  
}

//=========================================================================//
static void 
sr_sl_schedule_fixed(struct rtimer *sch_rt, rtimer_clock_t next_time){
   int ret;
   
   if(eprobingmac_is_on){
	if(RTIMER_CLOCK_LT(next_time, RTIMER_NOW() + 1)){
	    next_time = RTIMER_NOW() + 1; //maybe some 2... better be safe
	}
	
        ret = rtimer_set(sch_rt, next_time, 1,
		  (void (*)(struct rtimer *, void *))sr_sl_pcycle, NULL);
        if(ret != RTIMER_OK) {
	    PRINTF("schedule_powercycle: could not set rtimer\n");
        }
   }
}
//=========================================================================//
static char sr_sl_pcycle(struct rtimer *pc_prt, void*  dptr){

      PT_BEGIN(&pc_pt);

      
      PT_END(&pc_pt);
}
















//=========================================================================/
static void packet_input(){
  int packet_len = 0;
  
  //read packet from radio driver 
  radio_read_flag  = 1;
  
  packetbuf_clear();
  packet_len = cc2420_read(packetbuf_dataptr(), PACKETBUF_SIZE);
    
  packetbuf_set_datalen(len);
  
  if(packet_len <= 0){
      radio_read_flag  = 0;
      return;
  }
  
  //disable read flag..
  radio_read_flag  = 0;
  
  sr_packet_t inpkt;
  
  //copy packet to new space..
  memcpy((void*)&inpkt, packetbuf_dataptr(), packet_len);
  
  //test packet..
  
  
  
}
//=========================================================================/
static void send_packet(mac_callback_t sent, void *ptr){
    //do nothing...
}
//=========================================================================/
static void 
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list){
}
//=========================================================================/
static int radio_on(){
  return NETSTACK_RADIO.oon();
}
//=========================================================================/
static int radio_off(){
  return NETSTACK_RADIO.off();
}
//=========================================================================/
static unsigned short channel_check_interval(){
   return 8; 
}
//=========================================================================/
static void init(){
    //set up parameters
    set_protocol_parameters();
    
    //initialize list of neighbors
    sr_nodes_init();
    
    //start ProtoThread function 
    PT_INIT(&pt);
    
    //start processes 
    process_start(&mainloop_process, NULL);
    
    process_start(&output_process, NULL);
}
//=========================================================================/
const struct rdc_driver srsl_driver = {
    "SR-SL RDC v1.0",
    init,
    send_packet,
    send_list,
    packet_input,
    radio_on,
    radio_off,
    channel_check_interval,
};
//=========================================================================/
void init_discovery_protocol(){
   if(1){
      num_repetitions++;
      discovery_is_on = 1;
      
      //turn radio off 
      off();
      
      //start discovery protocol here..
      start_discovery_process();
   }
}
//=========================================================================/
PROCESS_THREAD(output_process, ev, data){
  
}
//=========================================================================/
PROCESS_THREAD(mainloop_process, ev, data){
  
  PROCESS_BEGIN();
  //print boot up information
  PRINTF("NodeID:%u.%u booted...T=%u\n",
	  linkaddr_node_addr.u8[1],
	  linkaddr_node_addr.u8[0],
	  period_length
  );
  
  while(1){
       //pause until a serial line event message is received
       PROCESS_WAIT_UNTIL(ev==serial_line_event_message);
       
       char *command = (char*)data;
       
       if(command != NULL){
	   //parse the string...and execute the appropriate action
	   if(!strcmp(command, "allconv")){
		discovery_is_on = 0;
	   }
	   
	   if(strcmp(command, "newround")){
		  //increment numer of repetitions
		  num_repetitions  = num_repetitions + 1;
		  
		  //set that discovery is on
		  discovery_is_on  = 1;
		  
		  //turn radio off 
		  off();
		  
		  //restart the process 
		  start_discovery_process();
	   }	   
       }           
  }//end of while
  
  PROCESS_END();
}
//=========================================================================/