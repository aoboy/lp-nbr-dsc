/**
///=========================================================================/
 * ------------------------------------------------------------------------
 * @author: António Gonga <gonga@ee.kth.se>, PhD
 * @date: 01 June 2016
 * @file: sr-sl.h
 * @update: Adding ACC-accelerated asymmetric deterministic discovery.
 * @brief: file contains RDC driver declaration
///=========================================================================/ 
 * @info: none
///=========================================================================*/


#ifndef SR_SL_H
#define SR_SL_H

#include "net/mac/rdc.h"
#include "dev/radio.h"


///SR_SL driver
extern const struct rdc_driver srsl_driver;


#endif //SR_SL_H