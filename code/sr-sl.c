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
#define CCA_COUNT_MAX                    2
#define CCA_COUNT_MAX_TX                 6
#define CCA_CHECK_TIME                   (RTIMER_SECOND/8192)
#define CCA_SLEEP_TIME                   ((RTIMER_SECOND/200) +1)
//=========================================================================//
static volatile uint8_t  radio_is_on_flag                       = 0;
static volatile uint8_t  random_wait_flag			= 0;
//=========================================================================//
static volatile uint16_t 		 slot_counter 		= 0;
static volatile uint16_t                 probe_time   		= 0;
static volatile uint16_t                 period_length 		= 0;
static volatile uint16_t                 anchor_time            = 0;
static volatile uint16_t                 node_slots_offset      = 0;
static volatile uint16_t 		 discovery_time  	= 0;
static volatile uint16_t 		 upper_bound_time       = 0;
static volatile uint16_t		 cycle_upper_bound_time = 0;
static volatile uint16_t		 random_wait_slots      = 0;
//=========================================================================//
static volatile uint8_t                  probe_offset           = 0;
static volatile uint8_t 		 probe_counter          = 0;
static volatile uint8_t		   extra_probe_counter          = 0;
static volatile uint8_t    		 discovery_is_on	= 0;
static volatile uint8_t    		 num_repetitions        = 0;
//=========================================================================//
/*static volatile uint8_t                  extra_energy_budget    = 0;
static volatile uint8_t             extra_energy_budget_counter = 0;*/ 

static volatile uint8_t		         energy_budget_period   = 0;
static volatile uint16_t           	   total_energy_budget  = 0;
//=========================================================================//
static struct pt pt;
static struct rtimer generic_timer;
//=========================================================================//
static volatile uint8_t  			curr_frac_nodes = 0;
static volatile uint8_t 			tx_packet_flag  = 0;
//=========================================================================//
#define DUTY_CYCLES_LEN 5
#define PERIODS_VEC_SIZE 4
static const uint8_t duty_cycles[]={1,1,2,3,5};
static const uint8_t periods_vec[]={200, 167, 133, 100};

#if EXTRA_ENERGY != 0
static const uint8_t extra_energy = EXTRA_ENERGY;
#else // EXTRA_ENERGY
static const uint8_t extra_energy = 5; //in case not defined,.. we use 5%
#endif // EXTRA_ENERGY
static volatile uint8_t 		 extra_energy_K   = extra_energy;
static volatile uint8_t 	 extra_energy_K_counter   = 0;

static volatile uint16_t 		      upper_bound_time = 0;
//=========================================================================//
#define GROUP_MERGE_TIME 10233
static uint8_t i3e154_channels[]  ={11,15,20,25,26};

//=========================================================================//
//=========================================================================//
const char *auto_msg             = "newround";

PROCESS(mainloop_process, "SR-SL Main Loop Process");
PROCESS(output_process,   "Output Process");
//=========================================================================//
//=========================================================================//

//=========================================================================/
static uint8_t energy_usage(){
  return total_energy_budget;
}
//=========================================================================/
static uint8_t energy_per_period(){
  //two probes (ANCHOR + PROBE) + Extra Probes
  return (2+extra_energy_K_counter);
}
//=========================================================================/
/**
 * @brief random_int
 * @param size_num
 * @return
 */
uint16_t random_int(uint16_t size_num){

    uint16_t rand_num = random_rand();

    if(size_num == 0){

        return 0;
    }

    uint16_t val = (65535 / size_num);
    
    if(rand_num < val){
        
        PRINTF("Hereee: %u\n", rand_num);
        return 0;

    }else{

        uint16_t k;

        for(k = 1; k < size_num; k++){

            if (rand_num >= k*val && rand_num <= (k+1)*val){

                return k;
            }
        }
    }
    return 0;
}
//=========================================================================//
static uint16_t divide_round(const uint16_t n, const uint16_t d){
    return ((n+d/2)/d);
}
//=========================================================================//
static void set_protocol_parameters(){
  //set period period length
  period_length = periods_vec[random_rand()%PERIODS_VEC_SIZE];
  
  //set upper time , i.e, total running time..
  upper_bound_time = (period_length*period_length)/4;
  
}

//=========================================================================//
void sr_sl_update_nbr(void* nbr_tgt){
  
}
//=========================================================================//
static void reschedule_process(){
  //reset all parameters here and reschedule a new discovery cycle..
  slot_counter           = 0;
  
  curr_frac_nodes        = 0;
  
  discovery_is_on        = 0;
  
  discovery_time         = 0,
    
  total_energy_budget    = 0;
  
  energy_budget_period   = 0;
  
  extra_energy_K_counter = 0;
  
  
  
  
  
  
  
  
  //flush all 
  sr_nodes_flush();
  
  //
  process_post(&mainloop_process, serial_line_event_message, auto_msg);
}
//=========================================================================//
static void sr_sl_send_probe(){
    
    uint8_t pld_len = 0;
    
  
    packetbuf_clear();
    
    sr_packet_t *srpkt = (sr_packet_t*)packetbuf_dataptr();
    
    srpkt->offset      = probe_offset;
    srpkt->period_len  = period_length;
    linkaddr_copy(&srpkt->src_addr, &linkaddr_node_addr);
    
    //pld_len = SR_PACKET_HDR_LEN;
    pld_len = sr_nodes_add_data(&srpkt->data[0], probe_offset);
    
    //account for the packet header..
    pld_len += SR_PACKET_HDR_LEN;
    
    uint8_t i = 0, pkt_on_air = 0, cca_counter = 0;
    
    cca_counter = (uint8_t)random_int(CCA_COUNT_MAX_TX);
    
    watchdog_periodic();
    //do CCA - Clear Channel Assessment... prevents packet collisions
    while(i < cca_counter){
        watchdog_periodic();
	
	to = RTIMER_NOW();
	
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + CCA_CHECK_TIME)){
                    //do nothing.. KUNANGA
        }
        
        if(NETSTACK_RADIO.channel_clear() == 0){
                pkt_on_air = 1;
                break; // maybe return here .. collision is expected
        } 
        //increment control variable... 
        i++;
    } //
  
    if((pkt_on_air == 0) && (radio_read_flag == 0)){
	NETSTACK_RADIO.send((void*)packetbuf_dataptr(),pld_len);
    }  
}

//=========================================================================// 
static void 
sr_sl_schedule_fixed(struct rtimer *sch_rt, rtimer_clock_t next_time){
   int ret;
   
   if(eprobingmac_is_on){
	if(RTIMER_CLOCK_LT(next_time, RTIMER_NOW() + RTIMER_GUARD_TIME)){
	    next_time = RTIMER_NOW() + RTIMER_GUARD_TIME; 
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

      PT_BEGIN(&pt); 
      while(1){ 
           if(random_wait_flag){
	     while(slot_counter < random_wait_slots){
		  //acquire time... 
		  rtimer_clock_t tnow_wait = RTIMER_NOW();
		  
		  //increment counter.. 
		  slot_counter++;
		  
		  sr_sl_schedule_fixed(pc_prt, tnow_wait + TS_LEN);
		  
		  PT_YIELD(&pt);
	     }
	     
	     //random period is over.. 
	     random_wait_flag   = 0;
	    COOJA_DEBUG_PRINTF("Random period end..%u\n", random_wait_slots); 
	  }
	  
	  for(slot_counter = 0; slot_counter < cycle_upper_bound_time; slot_counter++){
	       //get start and end times...
	       rtimer_clock_t ts_init = RTIMER_NOW();
	       rtimer_clock_t ts_end  = ts_init + TS_LEN;
	       
	       //which slot are we in inside a period
	       probe_offset = (slot_counter % period_length);
	       
	       if((slot_counter%period_length) == 0){
		 
		    //set transmission flag
		    tx_packet_flag = 1;
		    
		    //increment position of the probe..	
		    probe_counter = (probe_counter + 2)%(period_length/2 + 2);
		    
		    //for cases of 0 and 1
		    if(probe_counter < 2){
			//reset probe counter..
			probe_counter = 2;
		    }
		    
		    //reset extra energy per period
		    //extra_energy_budget_counter = 0;
		    
		    //probe time...
		    probe_time = slot_counter + probe_counter;
	      }else{
	      
		  //here is a probe node.. 
		  if(slot_counter == probe_time){
		      //set transmission flag
		      tx_packet_flag = 1;		    
		    
		    
		  }else{		    
		      //not an anchor not a probe.. check if there is a node
		      //slot.. we can possibly probe.
		      if((extra_energy_K_counter < extra_energy_K) && 
			  is_there_anchor(extra_energy_K, probe_offset)){
			  
			  //set transmission flag
			  tx_packet_flag = 1;	
		      
		          //count how much extra power was spent..
		          extra_energy_K_counter++;
		      } //....		    
		  }//END ELSE
	      } //END ELSE
	      
	      //transmit probe here.. 
	      if(tx_packet_flag){
		  //reset tx probe 
		  tx_packet_flag  = 0;
		  
		  //total energy 
		  total_energy_budget++;
		  
		  //period energy.. 
		  energy_budget_period = 2 + extra_energy_K_counter++;
		  
		  //check radio.. 
		  if(radio_is_on_flag == 0){
		    on();
		  }
		  
		  //transmit first beacon here.. 
		  sr_sl_send_probe();
		  
		  //wait some time here.. 
		  sr_sl_schedule_fixed(pc_prt, ts_init+TS_LEN_75);
		  PT_YIELD(&pt);
		  
		  //transmit second beacon here..
		  sr_sl_send_probe();
		  
		  //print output and reset energy here..
		  if((slot_counter != 0) && (probe_offset == 0)){
		
		  }		  
		  
	      }else{
		   //turn the radio off
		   if(radio_is_on_flag){
		      off();
		   }
		   
		   //sort slots here..
		   
		   
		   //print output and reset energy here..
		   if((slot_counter != 0) && (probe_offset == 0)){
		
		   }		   
		   
		   //schedule at the end of this time-slot..
		   /*sr_sl_schedule_fixed(pc_prt, ts_end);
		   PT_YIELD(&pt);*/
	      } //END ELSE..
	      
	      //schedule at the end of this time-slot..
	      sr_sl_schedule_fixed(pc_prt, ts_end);
	      PT_YIELD(&pt);	      
	    
	  }//end of FOR LOOP....
	  
	  COOJA_DEBUG_PRINTF("END oF DISCOVERY CYCLE:%u\n",num_repetitions);	
	  
	  //reset discovery variable-- 
	  discovery_is_on = 0;
	  
	  //
	  reschedule_process();
	  
          //cycle is over, YIELD waiting for another call..
          PT_YIELD(&pt);	  
	
      } //WHILE(1) termination.....

      PT_END(&pt);
}
//=========================================================================//
//=========================================================================//
static void start_discovery_process(){
    
  
     upper_bound_time = (period_length*period_length)/4;
     
     random_wait_slots = 1 + (random_rand()%upper_bound_time);
     
     
     random_wait_flag    = 1;
     
     cycle_upper_bound_time = 10000 - random_wait_slots;
     
     //split nodes into groups
     if(1){
         
        //do nothing yet      
     }
     
     rtimer_clock_t start_time = RTIMER_NOW() + RTIMER_GUARD_TIME;
     
     int ret = rtimer_set(&generic_timer, start_time, 1, 
		(void (*)(struct rtimer*, void*))sr_sl_pcycle, NULL);
     
     if(ret){
	COOJA_DEBUG_PRINTF("RTIMER schedule faild! %s\n", "start process");
     }         
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
  
  sr_packet_t *inpkt = (sr_packet_t*)packetbuf_dataptr();
  
  //copy packet to new space..
  //memcpy((void*)&inpkt, packetbuf_dataptr(), packet_len);
  
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
  
  PROCESS_BEGIN();

  while(1){
    
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    
    char *cmd_string = (char*) data;
    
    if(cmd_string == NULL){
      
      
    }
    
    
  }

  PROCESS_END();  
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
