/******************************************************************************
*
* Copyright (C) Chaoyong Zhou
* Email: bgnvendor@163.com 
* QQ: 2796796 
*
*******************************************************************************/
#ifdef __cplusplus
extern "C"{
#endif/*__cplusplus*/

#ifndef _CONTROL_H
#define _CONTROL_H

/*Super Package Debug Switch*/
#define SUPER_DEBUG_SWITCH SWITCH_ON

/*CDFS Package Debug Switch*/
#define CDFS_DEBUG_SWITCH SWITCH_ON

/*CRFS Package Debug Switch*/
#define CRFS_DEBUG_SWITCH SWITCH_ON

/*CRFSMON Package Debug Switch*/
#define CRFSMON_DEBUG_SWITCH SWITCH_ON

/*CHFSMON Package Debug Switch*/
#define CHFSMON_DEBUG_SWITCH SWITCH_ON

/*CSFSMON Package Debug Switch*/
#define CSFSMON_DEBUG_SWITCH SWITCH_ON

/*CRFSC Package Debug Switch*/
#define CRFSC_DEBUG_SWITCH SWITCH_ON

/*CHFS Package Debug Switch*/
#define CHFS_DEBUG_SWITCH SWITCH_ON

/*CSFS Package Debug Switch*/
#define CSFS_DEBUG_SWITCH SWITCH_ON

/*CBGT Package Debug Switch*/
#define CBGT_DEBUG_SWITCH SWITCH_ON

/*CSESSION Package Debug Switch*/
#define CSESSION_DEBUG_SWITCH SWITCH_ON

/*CTimer Package Debug Switch*/
#define CTIMER_DEBUG_SWITCH SWITCH_ON

/*CVENDOR Package Debug Switch*/
#define CVENDOR_DEBUG_SWITCH SWITCH_ON

/*Encode/Decode Functions Debug Switch*/
#define ENCODE_DEBUG_SWITCH SWITCH_ON

/*Task Functions Debug Switch*/
#define TASK_DEBUG_SWITCH SWITCH_ON

/*TASKC Functions Debug Switch*/
#define TASKC_DEBUG_SWITCH SWITCH_ON

/*Static Memory Control Switch*/
#define STATIC_MEMORY_SWITCH SWITCH_ON

/*Print Static Memory Stats Info Switch*/
#define STATIC_MEM_STATS_INFO_PRINT_SWITCH SWITCH_OFF

/*Static Memory Diagnostication Location Switch*/
#define STATIC_MEM_DIAG_LOC_SWITCH SWITCH_ON

/*Stack Memory Control Switch*/
#define STACK_MEMORY_SWITCH SWITCH_OFF

/*CLIST Memory Control Switch*/
#define CLIST_STATIC_MEM_SWITCH SWITCH_ON

/*CSET Memory Control Switch*/
#define CSET_STATIC_MEM_SWITCH SWITCH_OFF

/*CSTACK Memory Control Switch*/
#define CSTACK_STATIC_MEM_SWITCH SWITCH_OFF

/*CQUEUE Memory Control Switch*/
#define CQUEUE_STATIC_MEM_SWITCH SWITCH_OFF

#if (STATIC_MEMORY_SWITCH == STACK_MEMORY_SWITCH)
#error "fatal error: STATIC_MEMORY_SWITCH equal to STACK_MEMORY_SWITCH"
#endif/* STATIC_MEMORY_SWITCH == STACK_MEMORY_SWITCH */

#define ASM_DISABLE_SWITCH SWITCH_ON

#define TASKS_WR_EVENT_SWITCH SWITCH_OFF

#endif /* _CONTROL_H */

#ifdef __cplusplus
}
#endif/*__cplusplus*/

