/***********************************************************************
 * Copyright 2020, by the California Institute of Technology.
 * ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged.
 * Any commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * This software may be subject to U.S. export control laws.
 * By accepting this software, the user agrees to comply with
 * all applicable U.S. export laws and regulations. User has the
 * responsibility to obtain export licenses, or other export authority
 * as may be required before exporting such information to foreign
 * countries or providing access to foreign persons.
 *
 * @file        zsc_test_private.h
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Definitions of private macros for zsc
 *
 * A user must provide a demosaic_conf_private.h to define the following macros.
 * This file is copied for unit testing.
 */

#ifndef ZSC_CONF_PRIVATE_H
#define ZSC_CONF_PRIVATE_H

#include <assert.h>
#include <stdio.h>

// private functions are preceded by ZSC_PRIVATE
// this can be defined as 0 when compiling unit tests to allow access
// if your infrastructure does something similar, replace it here
#ifndef ZSC_PRIVATE
#define ZSC_PRIVATE static
#endif

/* ZSC was developed with the philosophy that assertions be used to
   check anomalous conditions. Demosaic functions assert if inputs
   indicate there is a logic error.
   See http://spinroot.com/p10/rule5.html.

   This file defines the DEMOSAIC_ASSERT macros used in demosaic.c as simple
   c asserts, for testing. THe google test suite for the repo uses
   "death tests" to test that they are called appropriately.

   It is the intent that user of the demosaic library will copy an
   appropriate demosaic_conf.h to include/demosaic, such that asserts
   are defined appropriately for the application.

   Possible asserts:
        cstd asserts
        ROS_ASSERT
        BOOST_ASSERT
        (test) ? (void)(0)
               : send_asynchronous_safing_alert_to_other_process(), assert(0):

   Asserts could also be disabled, but this is is discouraged.
 */
#define ZSC_ASSERT(test) assert(test)
#define ZSC_ASSERT_1(test, arg1) assert(test)
#define ZSC_ASSERT_2(test, arg1, arg2) assert(test)
#define ZSC_ASSERT_DBL_1(test, arg1) assert(test)



// if your framework has messaging/logging infrasturcture, replace here
// or define as nothing to disable
// FIXME make better
#define ZSC_WARN(fmt) printf("WARNING "fmt"\n")
#define ZSC_WARN1(fmt, arg1) printf("WARNING "fmt"\n", arg1)
#define ZSC_WARN2(fmt, arg1, arg2) printf("WARNING "fmt"\n", arg1, arg2)
#define ZSC_WARN3(fmt, arg1, arg2, arg3) printf("WARNING "fmt"\n", arg1, arg2, arg3)



#endif /* ZSC_CONF_PRIVATE_H */
