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
const struct rdc_driver srsl_driver = {
    "SR-SL RDC v1.0",
    init,
    send_packet,
    send_list,
    input,
    radio_on,
    radio_off,
    channel_check_interval,
};
//=========================================================================/