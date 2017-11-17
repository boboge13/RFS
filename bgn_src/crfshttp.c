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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>

#include "type.h"
#include "mm.h"
#include "log.h"
#include "cstring.h"
#include "cqueue.h"

#include "cbc.h"

#include "cmisc.h"

#include "task.h"

#include "csocket.h"

#include "cmpie.h"

#include "cepoll.h"

#include "crfs.h"
#include "chttp.inc"
#include "chttp.h"
#include "crfshttp.h"

#include "cbuffer.h"
#include "cstrkv.h"
#include "chunk.h"

#include "json.h"
#include "cbase64code.h"

#include "findex.inc"

/**
protocol
=========
    (case sensitive)

    1. Read File
    REQUEST:
        GET /get/<cache_key> HTTP/1.1
        Host: <host ipaddr | hostname>
        //Date: <date in second>
        store_offset: <file offset>
        store_size: <read length>
    RESPONSE:
        status: 200(HTTP_OK), 400(HTTP_BAD_REQUEST), 404(HTTP_NOT_FOUND)
        body: <file content>
         
    2. Write File 
    REQUEST:
        POST /set/<cache_key> HTTP/1.1
        Host: <host ipaddr | hostname>
        Content-Length: <file length>
        Expires: <date in second>
        body: <file content>
    RESPONSE:
        status: 200(HTTP_OK), 400(HTTP_BAD_REQUEST), 404(HTTP_NOT_FOUND, HTTP_ERROR)
        body: <null>

    3. Update File
    REQUEST: 
        POST /update/<cache_key> HTTP/1.1
        Host: <host ipaddr | hostname>
        Content-Length: <file length>
        Expires: <date in second>
        body: <file content>
    RESPONSE:
        status: 200(HTTP_OK), 400(HTTP_BAD_REQUEST), 404(HTTP_NOT_FOUND, HTTP_ERROR)
        body: <null>

    4. Delete File
    REQUEST: 
        GET /delete/<cache_key> HTTP/1.1
        Host: <host ipaddr | hostname>
        //Date: <date in second>
    RESPONSE:
        status: 200(HTTP_OK), 400(HTTP_BAD_REQUEST), 404(HTTP_NOT_FOUND, HTTP_ERROR)
        body: <null>     
**/

#if 0
#define CRFSHTTP_PRINT_UINT8(info, buff, len) do{\
    uint32_t __pos;\
    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "%s: ", info);\
    for(__pos = 0; __pos < len; __pos ++)\
    {\
        sys_print(LOGSTDOUT, "%02x,", ((uint8_t *)buff)[ __pos ]);\
    }\
    sys_print(LOGSTDOUT, "\n");\
}while(0)

#define CRFSHTTP_PRINT_CHARS(info, buff, len) do{\
    uint32_t __pos;\
    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "%s: ", info);\
    for(__pos = 0; __pos < len; __pos ++)\
    {\
        sys_print(LOGSTDOUT, "%c", ((uint8_t *)buff)[ __pos ]);\
    }\
    sys_print(LOGSTDOUT, "\n");\
}while(0)
#else
#define CRFSHTTP_PRINT_UINT8(info, buff, len) do{}while(0)
#define CRFSHTTP_PRINT_CHARS(info, buff, len) do{}while(0)
#endif



#if 1
#define CRFSHTTP_ASSERT(condition) do{\
    if(!(condition)) {\
        sys_log(LOGSTDOUT, "error: assert failed at %s:%d\n", __FUNCTION__, __LINE__);\
        exit(EXIT_FAILURE);\
    }\
}while(0)
#endif

#if 0
#define CRFSHTTP_ASSERT(condition) do{}while(0)
#endif
 
#if 1 
//#define CRFSHTTP_TIME_COST_FORMAT " BegTime:%u.%03u EndTime:%u.%03u Elapsed:%u "
#define CRFSHTTP_TIME_COST_FORMAT " %u.%03u %u.%03u %u "
#define CRFSHTTP_TIME_COST_VALUE(chttp_node)  \
    (uint32_t)CTMV_NSEC(CHTTP_NODE_START_TMV(chttp_node)), (uint32_t)CTMV_MSEC(CHTTP_NODE_START_TMV(chttp_node)), \
    (uint32_t)CTMV_NSEC(task_brd_default_get_daytime()), (uint32_t)CTMV_MSEC(task_brd_default_get_daytime()), \
    (uint32_t)((CTMV_NSEC(task_brd_default_get_daytime()) - CTMV_NSEC(CHTTP_NODE_START_TMV(chttp_node))) * 1000 + CTMV_MSEC(task_brd_default_get_daytime()) - CTMV_MSEC(CHTTP_NODE_START_TMV(chttp_node)))                 
#endif

static EC_BOOL g_crfshttp_log_init = EC_FALSE;

EC_BOOL crfshttp_log_start()
{
    TASK_BRD        *task_brd;

    if(EC_TRUE == g_crfshttp_log_init)
    {
        return (EC_TRUE);
    }

    g_crfshttp_log_init = EC_TRUE;
    
    task_brd = task_brd_default_get();

#if 0/*support rotate*/
    if(EC_TRUE == task_brd_check_is_work_tcid(TASK_BRD_TCID(task_brd)))
    {
        CSTRING *log_file_name;
     
        log_file_name = cstring_new(NULL_PTR, LOC_CRFSHTTP_0001);
        cstring_format(log_file_name, "%s/rfs_%s_%ld.log",
                        (char *)TASK_BRD_LOG_PATH_STR(task_brd),
                        c_word_to_ipv4(TASK_BRD_TCID(task_brd)),
                        TASK_BRD_RANK(task_brd));
        if(EC_FALSE == user_log_open(LOGUSER08, (char *)cstring_get_str(log_file_name), "a+"))/*append mode. scenario: after restart*/
        {
            dbg_log(SEC_0015_TASK, 0)(LOGSTDOUT, "error:crfshttp_log_start: user_log_open '%s' -> LOGUSER08 failed\n",
                               (char *)cstring_get_str(log_file_name));
            cstring_free(log_file_name);
            /*task_brd_default_abort();*/
        }
        else
        {
            cstring_free(log_file_name); 
        }
    } 
#endif
    if(EC_TRUE == task_brd_check_is_work_tcid(TASK_BRD_TCID(task_brd)))
    {
        CSTRING *log_file_name;
        LOG     *log;
     
        /*open log and redirect LOGUSER08 to it*/
        log_file_name = cstring_new(NULL_PTR, LOC_CRFSHTTP_0002);
        cstring_format(log_file_name, "%s/rfs_%s_%ld",
                        (char *)TASK_BRD_LOG_PATH_STR(task_brd),
                        c_word_to_ipv4(TASK_BRD_TCID(task_brd)),
                        TASK_BRD_RANK(task_brd));
        log = log_file_open((char *)cstring_get_str(log_file_name), /*"a+"*/"w+",
                            TASK_BRD_TCID(task_brd), TASK_BRD_RANK(task_brd),
                            LOGD_FILE_RECORD_LIMIT_ENABLED, SWITCH_OFF,
                            LOGD_SWITCH_OFF_ENABLE, LOGD_PID_INFO_ENABLE);
        if(NULL_PTR == log)
        {
            dbg_log(SEC_0015_TASK, 0)(LOGSTDOUT, "error:crfshttp_log_start: log_file_open '%s' -> LOGUSER08 failed\n",
                               (char *)cstring_get_str(log_file_name));
            cstring_free(log_file_name);
            /*task_brd_default_abort();*/
        }
        else
        {
            sys_log_redirect_setup(LOGUSER08, log);
         
            dbg_log(SEC_0015_TASK, 0)(LOGSTDOUT, "[DEBUG] crfshttp_log_start: log_file_open '%s' -> LOGUSER08 done\n",
                               (char *)cstring_get_str(log_file_name));
         
            cstring_free(log_file_name);
        }
    }
#if 0
    if(EC_TRUE == task_brd_check_is_work_tcid(TASK_BRD_TCID(task_brd)))
    {
        CSTRING *log_file_name;
        LOG     *log;
     
        /*open log and redirect LOGUSER08 to it*/
        log_file_name = cstring_new(NULL_PTR, LOC_CRFSHTTP_0003);
        cstring_format(log_file_name, "%s/debug_%s_%ld",
                        (char *)TASK_BRD_LOG_PATH_STR(task_brd),
                        c_word_to_ipv4(TASK_BRD_TCID(task_brd)),
                        TASK_BRD_RANK(task_brd));
        log = log_file_open((char *)cstring_get_str(log_file_name), /*"a+"*/"w+",
                            TASK_BRD_TCID(task_brd), TASK_BRD_RANK(task_brd),
                            LOGD_FILE_RECORD_LIMIT_ENABLED, SWITCH_OFF,
                            LOGD_SWITCH_OFF_ENABLE, LOGD_PID_INFO_ENABLE);
        if(NULL_PTR == log)
        {
            dbg_log(SEC_0015_TASK, 0)(LOGSTDOUT, "error:crfshttp_log_start: log_file_open '%s' -> LOGUSER07 failed\n",
                               (char *)cstring_get_str(log_file_name));
            cstring_free(log_file_name);
            /*task_brd_default_abort();*/
        }
        else
        {
            sys_log_redirect_setup(LOGUSER07, log);
            cstring_free(log_file_name);
        }
    } 
#endif
    return (EC_TRUE);
}



/*---------------------------------------- ENTRY: HTTP COMMIT REQUEST FOR HANDLER  ----------------------------------------*/
EC_BOOL crfshttp_commit_request(CHTTP_NODE *chttp_node)
{
    http_parser_t *http_parser;

    http_parser = CHTTP_NODE_PARSER(chttp_node);

    if(HTTP_GET == http_parser->method)
    {
        CROUTINE_NODE  *croutine_node;
     
        croutine_node = croutine_pool_load(TASK_REQ_CTHREAD_POOL(task_brd_default_get()),
                                           (UINT32)crfshttp_commit_http_get, 1, chttp_node);
        if(NULL_PTR == croutine_node)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_request: cthread load for HTTP_GET failed\n");
            return (EC_BUSY);
        }
        CHTTP_NODE_LOG_TIME_WHEN_LOADED(chttp_node);/*record http request was loaded time in coroutine*/
        CHTTP_NODE_CROUTINE_NODE(chttp_node) = croutine_node;
        CROUTINE_NODE_COND_RELEASE(croutine_node, LOC_CRFSHTTP_0004); 
     
        return (EC_TRUE);
    }

    if(HTTP_POST == http_parser->method)
    {
        CROUTINE_NODE  *croutine_node;
     
        croutine_node = croutine_pool_load(TASK_REQ_CTHREAD_POOL(task_brd_default_get()),
                                           (UINT32)crfshttp_commit_http_post, 1, chttp_node);
        if(NULL_PTR == croutine_node)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_request: cthread load for HTTP_POST failed\n");
            return (EC_BUSY);
        }
        CHTTP_NODE_LOG_TIME_WHEN_LOADED(chttp_node);/*record http request was loaded time in coroutine*/
        CHTTP_NODE_CROUTINE_NODE(chttp_node) = croutine_node;
        CROUTINE_NODE_COND_RELEASE(croutine_node, LOC_CRFSHTTP_0005); 

        return (EC_TRUE);
    }

    if(HTTP_HEAD == http_parser->method)
    {
        CROUTINE_NODE  *croutine_node;
     
        croutine_node = croutine_pool_load(TASK_REQ_CTHREAD_POOL(task_brd_default_get()),
                                           (UINT32)crfshttp_commit_http_head, 1, chttp_node);
        if(NULL_PTR == croutine_node)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_request: cthread load for HTTP_HEAD failed\n");
            return (EC_BUSY);
        }
        CHTTP_NODE_LOG_TIME_WHEN_LOADED(chttp_node);/*record http request was loaded time in coroutine*/
        CHTTP_NODE_CROUTINE_NODE(chttp_node) = croutine_node;
        CROUTINE_NODE_COND_RELEASE(croutine_node, LOC_CRFSHTTP_0006); 
     
        return (EC_TRUE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_request: not support http method %d yet\n", http_parser->method);
    return (EC_FALSE);/*note: this chttp_node must be discarded*/
}

EC_BOOL crfshttp_commit_http_head(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;

    CHTTP_NODE_LOG_TIME_WHEN_HANDLE(chttp_node);/*record rfs beg to handle time*/
 
    if(EC_TRUE == crfshttp_is_http_head_getsmf(chttp_node))
    {
        ret = crfshttp_commit_getsmf_head_request(chttp_node);
    }
    else
    {
        CBUFFER *uri_cbuffer;
     
        uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_http_head: invalid uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));

        ret = EC_FALSE;
    }

    return crfshttp_commit_end(chttp_node, ret);
}

EC_BOOL crfshttp_commit_http_post(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;

    CHTTP_NODE_LOG_TIME_WHEN_HANDLE(chttp_node);/*record rfs beg to handle time*/
 
    if(EC_TRUE == crfshttp_is_http_post_setsmf(chttp_node))
    {
        ret = crfshttp_commit_setsmf_post_request(chttp_node);
    }
    /* only write memory cache but NOT rfs */
    else if(EC_TRUE == crfshttp_is_http_post_setsmf_memc(chttp_node))
    {
        ret = crfshttp_commit_setsmf_memc_post_request(chttp_node);
    }
    /* only update memory cache but NOT rfs */
    else if(EC_TRUE == crfshttp_is_http_post_update_memc(chttp_node))
    {
        ret = crfshttp_commit_update_memc_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_mexpire(chttp_node))
    {
        ret = crfshttp_commit_mexpire_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_mdsmf(chttp_node))
    {
        ret = crfshttp_commit_mdsmf_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_mdbgf(chttp_node))
    {
        ret = crfshttp_commit_mdbgf_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_mddir(chttp_node))
    {
        ret = crfshttp_commit_mddir_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_update(chttp_node))
    {
        ret = crfshttp_commit_update_post_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_post_renew(chttp_node))
    {
        ret = crfshttp_commit_renew_post_request(chttp_node);
    }
    else
    {
        CBUFFER *uri_cbuffer;
     
        uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_http_post: invalid uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));

        ret = EC_FALSE;
    }

    return crfshttp_commit_end(chttp_node, ret);
}

EC_BOOL crfshttp_commit_http_get(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_http_get: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(CHTTP_NODE_URI(chttp_node)),
                        CBUFFER_DATA(CHTTP_NODE_URI(chttp_node)),
                        CBUFFER_USED(CHTTP_NODE_URI(chttp_node)));

    CHTTP_NODE_LOG_TIME_WHEN_HANDLE(chttp_node);/*record rfs beg to handle time*/
 
    if(EC_TRUE == crfshttp_is_http_get_getsmf(chttp_node))
    {
        ret = crfshttp_commit_getsmf_get_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_get_check_memc(chttp_node))
    {
        ret = crfshttp_commit_check_memc_get_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_get_getsmf_memc(chttp_node))
    {
        ret = crfshttp_commit_getsmf_memc_get_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_get_dsmf_memc(chttp_node))
    {
        ret = crfshttp_commit_dsmf_memc_get_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_get_ddir_memc(chttp_node))
    {
        ret = crfshttp_commit_ddir_memc_get_request(chttp_node);
    }
    else if(EC_TRUE == crfshttp_is_http_get_getbgf(chttp_node))
    {
        ret = crfshttp_commit_getbgf_get_request(chttp_node);
    }   
    else if (EC_TRUE == crfshttp_is_http_get_dsmf(chttp_node))
    {
        ret = crfshttp_commit_dsmf_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_ddir(chttp_node))
    {
        ret = crfshttp_commit_ddir_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_getrgf(chttp_node))
    {
        ret = crfshttp_commit_getrgf_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_sexpire(chttp_node))
    {
        ret = crfshttp_commit_sexpire_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_lock_req(chttp_node))
    {
        ret = crfshttp_commit_lock_req_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_unlock_req(chttp_node))
    {
        ret = crfshttp_commit_unlock_req_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_unlock_notify_req(chttp_node))
    {
        ret = crfshttp_commit_unlock_notify_req_get_request(chttp_node);
    } 
    else if (EC_TRUE == crfshttp_is_http_get_recycle(chttp_node))
    {
        ret = crfshttp_commit_recycle_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_flush(chttp_node))
    {
        ret = crfshttp_commit_flush_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_retire(chttp_node))
    {
        ret = crfshttp_commit_retire_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_breathe(chttp_node))
    {
        ret = crfshttp_commit_breathe_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_logrotate(chttp_node))
    {
        ret = crfshttp_commit_logrotate_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_actsyscfg(chttp_node))
    {
        ret = crfshttp_commit_actsyscfg_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_qtree(chttp_node))
    {
        ret = crfshttp_commit_qtree_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_file_wait(chttp_node))
    {
        ret = crfshttp_commit_file_wait_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_file_notify(chttp_node))
    {
        ret = crfshttp_commit_file_notify_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_cond_wakeup(chttp_node))
    {
        ret = crfshttp_commit_cond_wakeup_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_renew_header(chttp_node))
    {
        ret = crfshttp_commit_renew_header_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_wait_header(chttp_node))
    {
        ret = crfshttp_commit_wait_header_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_locked_file_retire(chttp_node))
    {
        ret = crfshttp_commit_locked_file_retire_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_rfs_up(chttp_node))
    {
        ret = crfshttp_commit_rfs_up_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_rfs_down(chttp_node))
    {
        ret = crfshttp_commit_rfs_down_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_rfs_add(chttp_node))
    {
        ret = crfshttp_commit_rfs_add_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_rfs_del(chttp_node))
    {
        ret = crfshttp_commit_rfs_del_get_request(chttp_node);
    }
    else if (EC_TRUE == crfshttp_is_http_get_rfs_list(chttp_node))
    {
        ret = crfshttp_commit_rfs_list_get_request(chttp_node);
    }
    else
    {
        CBUFFER *uri_cbuffer;
     
        uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_http_get: invalid uri %.*s\n",
                            CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_NOT_ACCEPTABLE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_commit_http_get: invalid uri %.*s", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_ACCEPTABLE;
        ret = EC_FALSE;
    }

    return crfshttp_commit_end(chttp_node, ret);
}

EC_BOOL crfshttp_commit_end(CHTTP_NODE *chttp_node, EC_BOOL result)
{
    EC_BOOL ret;

    ret = result;

    CHTTP_NODE_CROUTINE_NODE(chttp_node) = NULL_PTR; /*clear croutine mounted point*/

    if(EC_DONE == ret)
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(NULL_PTR != csocket_cnode)
        {
            CEPOLL *cepoll;
            int     sockfd;

            cepoll = TASK_BRD_CEPOLL(task_brd_default_get());
            sockfd = CSOCKET_CNODE_SOCKFD(csocket_cnode);
         
            dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_end: sockfd %d done, remove all epoll events\n", sockfd); 
            cepoll_del_all(cepoll, sockfd);

            /*chttp_node resume for next request handling if keep-alive*/
            chttp_node_wait_resume(chttp_node);

            return (EC_DONE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_end: csocket_cnode of chttp_node %p is null\n", chttp_node);

        CHTTP_NODE_KEEPALIVE(chttp_node) = EC_FALSE;
        chttp_node_free(chttp_node);

        return (EC_FALSE);
    }
 
    if(EC_FALSE == ret)
    {
        ret = chttp_commit_error_request(chttp_node);
    }

    if(EC_FALSE == ret)
    {
        CSOCKET_CNODE * csocket_cnode;

        /*umount from defer request queue if necessary*/
        chttp_defer_request_queue_erase(chttp_node);     

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(NULL_PTR != csocket_cnode)
        {
            CEPOLL *cepoll;
            int     sockfd;

            cepoll = TASK_BRD_CEPOLL(task_brd_default_get());
            sockfd = CSOCKET_CNODE_SOCKFD(csocket_cnode);
         
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_commit_end: sockfd %d false, remove all epoll events\n", sockfd); 
            cepoll_del_all(cepoll, sockfd);

            /* unbind */
            CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
            CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;
         
            csocket_cnode_close(csocket_cnode);

            /*free*/
            chttp_node_free(chttp_node);
            return (EC_FALSE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_end: csocket_cnode of chttp_node %p is null\n", chttp_node);

        /*free*/
        chttp_node_free(chttp_node);

        return (EC_FALSE);
    }
 
    /*EC_TRUE, EC_DONE*/
    return (ret); 
}

EC_BOOL crfshttp_commit_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;
    EC_BOOL ret;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    ret = chttp_csocket_cnode_send_rsp(csocket_cnode);
    if(EC_AGAIN != ret)
    {
        return (ret);
    }

    ret = cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                     (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_send_rsp, csocket_cnode);
    if(EC_FALSE == ret)
    {
        return (EC_FALSE);
    }
    return (EC_AGAIN);
}
#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: getsmf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_getsmf_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/getsmf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/getsmf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_getsmf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_getsmf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_getsmf_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_getsmf_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_getsmf_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_getsmf_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_getsmf_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_getsmf_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    char          *expired_body_str;
    char          *store_offset_str;
    char          *store_size_str;

    EC_BOOL        expired_body_needed;
    UINT32         expires_timestamp;
    char           expires_str[64];
    uint32_t       expires_str_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getsmf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getsmf");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_getsmf_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    expired_body_str = chttp_node_get_header(chttp_node, (const char *)"expired-body");
    if(NULL_PTR == expired_body_str || 0 == c_str_to_uint32(expired_body_str))
    {
        expired_body_needed = EC_TRUE;/*even if file expired, return file content*/
    }
    else
    {
        expired_body_needed = EC_FALSE;/*if file expired, NOT return file content*/
    }

    store_offset_str = chttp_node_get_header(chttp_node, (const char *)"store-offset");
    if(NULL_PTR != store_offset_str)
    {
        CSOCKET_CNODE * csocket_cnode;
     
        uint32_t store_offset;
        uint32_t store_size;
     
        UINT32   offset;
        UINT32   max_len;
     
        store_size_str   = chttp_node_get_header(chttp_node, (const char *)"store-size");

        store_offset = c_str_to_uint32(store_offset_str);
        store_size   = c_str_to_uint32(store_size_str);/*note: when store_size_str is null, store_size is zero*/

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        offset        = store_offset;
        max_len       = store_size;

        if(EC_FALSE == crfs_read_e(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, &offset, max_len, content_cbytes, &expires_timestamp, expired_body_needed))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_get_request: crfs read %s with offset %u, size %u failed\n",
                                (char *)cstring_get_str(&path_cstr), store_offset, store_size);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getsmf_get_request: crfs read %s with offset %u, size %u failed", (char *)cstring_get_str(&path_cstr), store_offset, store_size);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            cbytes_clean(content_cbytes);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_get_request: crfs read %s with offset %u, size %u done\n",
                            (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getsmf_get_request: crfs read %s with offset %u, size %u done", (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else/*read whole file content*/
    {
        CSOCKET_CNODE * csocket_cnode;
     
        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_read(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, &expires_timestamp, expired_body_needed))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_get_request: crfs read %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getsmf_get_request: crfs read %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            cbytes_clean(content_cbytes);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_get_request: crfs read %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getsmf_get_request: crfs read %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                         
    }

    expires_str_len = snprintf(expires_str, sizeof(expires_str), "Expires:%ld\r\n", expires_timestamp);
    cbuffer_set(CHTTP_NODE_EXPIRES(chttp_node), (uint8_t *)expires_str, expires_str_len);

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_getsmf_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getsmf");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getsmf");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getsmf_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_get_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_getsmf_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif
#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: lock_req ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_lock_req_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/lock_req/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/lock_req/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_lock_req(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_lock_req: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_lock_req_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_lock_req_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_lock_req_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_lock_req_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_lock_req_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_lock_req_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_lock_req_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_lock_req_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

static UINT32 __crfshttp_convert_expires_str_to_nseconds(const char *expires_str)
{
    char *str;
    char *fields[2];
    UINT32 seg_num;
    UINT32 expires_nsec;

    str = c_str_dup(expires_str);
    if(NULL_PTR == str)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_convert_expires_str_to_nseconds: dup str '%s' failed\n", expires_str);
        return ((UINT32)0);
    }
 
    seg_num = c_str_split(str, ".", fields, 2);
    if(1 == seg_num)
    {
        expires_nsec = c_str_to_word(fields[0]);
        safe_free(str, LOC_CRFSHTTP_0007);
        return (expires_nsec);
    }

    if(2 == seg_num)/*if has dot, we regard client gives absolute time (in seconds)*/
    {
        UINT32 expire_when;
        CTIMET cur_time; /*type: long, unit: second*/
     
        /*note: ignore part after dot (million seconds)*/
        expire_when = c_str_to_word(fields[0]);

        CTIMET_GET(cur_time);

        expires_nsec = (expire_when - cur_time);

        safe_free(str, LOC_CRFSHTTP_0008);
        return (expires_nsec);
    }

    safe_free(str, LOC_CRFSHTTP_0009);
    return ((UINT32)0);
}
EC_BOOL crfshttp_handle_lock_req_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CSTRING        token_cstr;

    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
    uint8_t        auth_token_header[CMD5_DIGEST_LEN * 8];
    uint32_t       auth_token_header_len;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/lock_req");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/lock_req");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    cstring_init(&token_cstr, NULL_PTR);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_lock_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_lock_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_lock_req_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_lock_req_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_lock_req_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_lock_req_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }


    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_lock_req_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        char    *expires_str;
        UINT32   expires_nsec;
        UINT32   locked_flag;

        char    *tcid_str;
        UINT32   tcid;
     
        expires_str  = chttp_node_get_header(chttp_node, (const char *)"Expires");
        expires_nsec = __crfshttp_convert_expires_str_to_nseconds(expires_str);
        locked_flag  = EC_FALSE;

        tcid_str     = chttp_node_get_header(chttp_node, (const char *)"tcid");
        if(NULL_PTR == tcid_str)
        {
            tcid = CMPI_ERROR_TCID;
        }
        else
        {
            tcid = c_ipv4_to_word(tcid_str);
        }

        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_lock_req_get_request: header Expires %s => %ld\n",
                                expires_str, expires_nsec);
    
        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_lock(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, expires_nsec, &token_cstr, &locked_flag))
        {
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "error:crfshttp_handle_lock_req_get_request: crfs lock %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            if(EC_TRUE == locked_flag)/*flag was set*/
            {
                cbytes_set(content_cbytes, (UINT8 *)"locked-already:true\r\n", sizeof("locked-already:true\r\n") - 1);
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_lock_req_get_request: crfs lock %s failed", (char *)cstring_get_str(&path_cstr));
            }
            else /*flag was not set which means some error happen*/
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_lock_req_get_request: crfs lock %s failed", (char *)cstring_get_str(&path_cstr));
            }
                             
            cstring_clean(&path_cstr);
            cstring_clean(&token_cstr);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_lock_req_get_request: crfs lock %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_lock_req_get_request: crfs lock %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
 
    auth_token_header_len = snprintf((char *)auth_token_header, sizeof(auth_token_header),
                                    "auth-token:%.*s\r\n", (uint32_t)CSTRING_LEN(&token_cstr), (char *)CSTRING_STR(&token_cstr));
    cbytes_set(content_cbytes, auth_token_header, auth_token_header_len);
 
    cstring_clean(&path_cstr);
    cstring_clean(&token_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_lock_req_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint8_t       *token_buf;
    uint32_t       token_len;
 
    /*note: content carry on auth-token info but not response body*/
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    token_buf      = CBYTES_BUF(content_cbytes);
    token_len      = (uint32_t)CBYTES_LEN(content_cbytes);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/lock_req");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/lock_req");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_lock_req_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_lock_req_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_lock_req_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_token(chttp_node, token_buf, token_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_lock_req_get_response: make response header token failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_lock_req_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_lock_req_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_lock_req_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: unlock_req ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_unlock_req_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/unlock_req/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/unlock_req/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_unlock_req(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_unlock_req: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_unlock_req_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_unlock_req_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_unlock_req_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_req_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_unlock_req_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_req_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_unlock_req_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_req_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_unlock_req_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
 
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/unlock_req");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/unlock_req");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_unlock_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_req_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_req_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_req_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_unlock_req_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_unlock_req_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        char    *auth_token_header;

        CSTRING  token_cstr;

        auth_token_header = chttp_node_get_header(chttp_node, (const char *)"auth-token");
        if(NULL_PTR == auth_token_header)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT,
                            "error:crfshttp_handle_unlock_req_get_request: crfs unlock %s failed due to header 'auth-token' absence\n",
                            (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_unlock_req_get_request: crfs unlock %s failed due to header 'auth-token' absence", (char *)cstring_get_str(&path_cstr));
                         
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }

        cstring_set_str(&token_cstr, (const UINT8 *)auth_token_header);
     
        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_unlock(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, &token_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_unlock_req_get_request: crfs unlock %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_unlock_req_get_request: crfs unlock %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_unlock_req_get_request: crfs unlock %s done\n",
                            (char *)cstring_get_str(&path_cstr));     
     
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_unlock_req_get_request: crfs unlock %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_unlock_req_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/unlock_req");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/unlock_req");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_unlock_req_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_req_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_req_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }   

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_req_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_unlock_req_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_req_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: unlock_notify_req ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_unlock_notify_req_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/unlock_notify_req/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/unlock_notify_req/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_unlock_notify_req(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_unlock_notify_req: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_unlock_notify_req_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_unlock_notify_req_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_unlock_notify_req_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_notify_req_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_unlock_notify_req_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_notify_req_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_unlock_notify_req_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_notify_req_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_unlock_notify_req_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
 
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/unlock_notify_req");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/unlock_notify_req");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_unlock_notify_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_notify_req_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_notify_req_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_notify_req_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_unlock_notify_req_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_unlock_notify_req_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_unlock_notify_req_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_unlock_notify(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_unlock_notify_req_get_request: crfs unlock_notify %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_unlock_notify_req_get_request: crfs unlock_notify %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_unlock_notify_req_get_request: crfs unlock_notify %s done\n",
                            (char *)cstring_get_str(&path_cstr));     
     
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_unlock_notify_req_get_request: crfs unlock_notify %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_unlock_notify_req_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/unlock_notify_req");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/unlock_notify_req");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_unlock_notify_req_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_notify_req_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_notify_req_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }   

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_unlock_notify_req_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_unlock_notify_req_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_unlock_notify_req_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: recycle ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_recycle_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/recycle") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/recycle")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_recycle(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_recycle: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_recycle_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_recycle_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_recycle_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_recycle_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_recycle_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_recycle_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_recycle_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_recycle_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_recycle_get_request(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    CBUFFER       *uri_cbuffer;
     
    //uint8_t       *cache_key;
    //uint32_t       cache_len;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    //cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/recycle");
    //cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/recycle");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_recycle_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_recycle_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_recycle_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_recycle_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);
 
    if(EC_TRUE == __crfshttp_uri_is_recycle_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        char  *max_num_per_np_str;
        UINT32 max_num_per_np;
        UINT32 complete_num;

        uint8_t  recycle_result[ 32 ];
        uint32_t recycle_result_len;
     
        max_num_per_np_str = chttp_node_get_header(chttp_node, (const char *)"max-num-per-np");
        max_num_per_np = c_str_to_word(max_num_per_np_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_recycle(CSOCKET_CNODE_MODI(csocket_cnode), max_num_per_np, &complete_num))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_recycle_get_request: crfs recycle failed\n");

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_recycle_get_request: crfs recycle failed");

            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_recycle_get_request: crfs recycle done\n");

        /*prepare response header*/
        recycle_result_len = snprintf((char *)recycle_result, sizeof(recycle_result), "recycle-completion:%ld\r\n", complete_num);
        cbytes_set(content_cbytes, recycle_result, recycle_result_len);     

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_recycle_get_request: crfs recycle done");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_recycle_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint8_t       *recycle_result_buf;
    uint32_t       recycle_result_len;

    /*note: content carry on recycle-completion info but not response body*/
    content_cbytes    = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    recycle_result_buf = CBYTES_BUF(content_cbytes);
    recycle_result_len = (uint32_t)CBYTES_LEN(content_cbytes);

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_recycle_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_recycle_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_recycle(chttp_node, recycle_result_buf, recycle_result_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_recycle_get_response: make response header recycle failed\n");
        return (EC_FALSE);
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_recycle_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_recycle_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_recycle_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: flush ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_flush_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/flush") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/flush")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_flush(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_flush: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_flush_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_flush_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_flush_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_flush_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_flush_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_flush_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_flush_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_flush_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_flush_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    //uint8_t       *cache_key;
    //uint32_t       cache_len;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    //cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/flush");
    //cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/flush");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_flush_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_flush_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_flush_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_flush_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    } 

    if(EC_TRUE == __crfshttp_uri_is_flush_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_flush(CSOCKET_CNODE_MODI(csocket_cnode)))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_flush_get_request: crfs flush failed\n");
            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_flush_get_request: crfs flush failed");

            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            return (EC_TRUE);
        }
     
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_flush_get_request: crfs flush done\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_flush_get_request: crfs flush done");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_flush_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_flush_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_flush_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_flush_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_flush_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_flush_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: retire ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_retire_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/retire") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/retire")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_retire(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_retire: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_retire_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_retire_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_retire_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_retire_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_retire_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_retire_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_retire_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_retire_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_retire_get_request(CHTTP_NODE *chttp_node)
{    
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    char          *retire_seconds_str;
    char          *retire_files_str;
    char          *retire_max_step_per_loop_str;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_retire_get_request\n");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_retire_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_retire_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_retire_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_retire_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    retire_seconds_str = chttp_node_get_header(chttp_node, (const char *)"retire-seconds");
    if(NULL_PTR == retire_seconds_str) /*invalid retire request*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_retire_get_request: http header 'retire-seconds' absence\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_retire_get_request: http header 'retire-seconds' absence");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    retire_files_str   = chttp_node_get_header(chttp_node, (const char *)"retire-files");
    if(NULL_PTR == retire_files_str) /*invalid retire request*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_retire_get_request: http header 'retire-files' absence\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_retire_get_request: http header 'retire-files' absence");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    } 

    retire_max_step_per_loop_str   = chttp_node_get_header(chttp_node, (const char *)"retire-max-step-per-loop");
 
    if(NULL_PTR != retire_seconds_str && NULL_PTR != retire_files_str)
    {
        CSOCKET_CNODE * csocket_cnode;
     
        UINT32   retire_seconds;
        UINT32   retire_files;
        UINT32   retire_max_step_per_loop;
        UINT32   complete_num;

        uint8_t  retire_result[ 32 ];
        uint32_t retire_result_len;
     
        retire_seconds = c_str_to_word(retire_seconds_str);
        retire_files   = c_str_to_word(retire_files_str);
     
        retire_max_step_per_loop = c_str_to_word(retire_max_step_per_loop_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
   
        if(EC_FALSE == crfs_retire(CSOCKET_CNODE_MODI(csocket_cnode), retire_seconds, retire_files, retire_max_step_per_loop, &complete_num))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_retire_get_request: crfs retire with nsec %ld, expect retire num %ld failed\n",
                                retire_seconds, retire_files);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_retire_get_request: crfs retire with nsec %ld, expect retire num %ld failed", retire_seconds, retire_files);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
            return (EC_TRUE);
        }

        /*prepare response header*/
        retire_result_len = snprintf((char *)retire_result, sizeof(retire_result), "retire-completion:%ld\r\n", complete_num);
        cbytes_set(content_cbytes, retire_result, retire_result_len);
                        
        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_retire_get_request: crfs retire with nsec %ld, expect retire %ld, complete %ld done\n",
                            retire_seconds, retire_files, complete_num);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_retire_get_request: crfs retire with nsec %ld, expect retire %ld, complete %ld done", retire_seconds, retire_files, complete_num);
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                    
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_retire_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint8_t       *retire_result_buf;
    uint32_t       retire_result_len;

    /*note: content carry on retire-completion info but not response body*/
    content_cbytes    = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    retire_result_buf = CBYTES_BUF(content_cbytes);
    retire_result_len = (uint32_t)CBYTES_LEN(content_cbytes); 

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_retire_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_retire_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_retire(chttp_node, retire_result_buf, retire_result_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_retire_get_response: make response header retire failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_retire_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_retire_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_retire_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: breathe ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_breathe_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/breathe") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/breathe")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_breathe(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_breathe: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_breathe_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_breathe_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_breathe_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_breathe_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_breathe_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_breathe_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_breathe_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_breathe_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_breathe_get_request(CHTTP_NODE *chttp_node)
{    
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_breathe_get_request\n");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_breathe_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_breathe_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_breathe_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_breathe_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(1)
    {
        //CSOCKET_CNODE * csocket_cnode;
     
        //csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        breathing_static_mem();
     
        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_breathe_get_request: memory breathing done\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_breathe_get_request: memory breathing done");
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_breathe_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_breathe_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_breathe_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_breathe_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_breathe_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_breathe_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif



#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: getbgf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_getbgf_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/getbgf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/getbgf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_getbgf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_getbgf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_getbgf_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_getbgf_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_getbgf_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getbgf_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_getbgf_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getbgf_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_getbgf_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getbgf_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

/*get block fd and pos*/
static EC_BOOL __crfshttp_handle_getbgf_get_request_read_block(CSOCKET_CNODE *csocket_cnode)
{    
    CHTTP_NODE *chttp_node;

    uint64_t   offset;
    uint32_t   max_len;

    EC_BOOL    expires_timestamp;
    EC_BOOL    expired_body_needed;

    CSTRING   *store_path;

    uint32_t   block_size;
    int        block_fd;

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode); 

    offset        = CHTTP_NODE_STORE_CUR_OFFSET(chttp_node);
    max_len       = CPGB_CACHE_MAX_BYTE_SIZE;/*adjust later*/

    store_path    = CHTTP_NODE_STORE_PATH(chttp_node);

    /*adjust max_len*/
    if(CHTTP_NODE_STORE_END_OFFSET(chttp_node) < CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) + CPGB_CACHE_MAX_BYTE_SIZE)
    {
        max_len = (UINT32)(CHTTP_NODE_STORE_END_OFFSET(chttp_node) - CHTTP_NODE_STORE_BEG_OFFSET(chttp_node));
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getbgf_get_request_read_block: read offset reach %ld\n", offset);

    expired_body_needed = (EC_BOOL)CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node); /*save expire in extra*/
 
    if(EC_FALSE == crfs_fetch_block_fd_b(CSOCKET_CNODE_MODI(csocket_cnode), store_path,
                                          offset, &expires_timestamp, expired_body_needed,
                                          &block_size, &block_fd))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_read_block: crfs fetch %s from offset %ld, max_len %u failed\n",
                            (char *)cstring_get_str(store_path), offset, max_len);

        return (EC_FALSE);
    }

    CHTTP_NODE_SEND_BLOCK_FD(chttp_node)   = block_fd;
    CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node) = block_size;
    CHTTP_NODE_SEND_BLOCK_POS(chttp_node)  = (UINT32)(offset % CPGB_CACHE_MAX_BYTE_SIZE);

    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getbgf_get_request_send_block(CSOCKET_CNODE * csocket_cnode)
{
    CHTTP_NODE *chttp_node;

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);

    if(CHTTP_NODE_SEND_BLOCK_POS(chttp_node) < CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node))
    { 
        UINT32 pos;

        pos = CHTTP_NODE_SEND_BLOCK_POS(chttp_node);
        if(EC_FALSE == csocket_sendfile(CSOCKET_CNODE_SOCKFD(csocket_cnode),
                                   CHTTP_NODE_SEND_BLOCK_FD(chttp_node),
                                   CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node),
                                   &pos))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_send_block: sockfd %d sendfile %ld bytes failed\n",
                               CSOCKET_CNODE_SOCKFD(csocket_cnode),
                               CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node) - CHTTP_NODE_SEND_BLOCK_POS(chttp_node)
                               );
            return (EC_FALSE);                        
        }

        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getbgf_get_request_send_body: write %ld bytes from %ld to %ld\n",
                           pos - CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node),
                           CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node), pos);

        CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) += (pos - CHTTP_NODE_SEND_BLOCK_POS(chttp_node));
        CHTTP_NODE_SEND_BLOCK_POS(chttp_node)    = pos;
    }

    if(CHTTP_NODE_SEND_BLOCK_POS(chttp_node) < CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node))
    {
        /*wait for next writing*/
        return (EC_TRUE);
    }

    CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node) = 0;
    CHTTP_NODE_SEND_BLOCK_POS(chttp_node)  = 0;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getbgf_get_request_send_block: write offset reach %ld\n",
                       CHTTP_NODE_STORE_CUR_OFFSET(chttp_node));
    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getbgf_get_request_send_block_more(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE    *chttp_node;

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_send_block_more: socket %d -> chttp_node is null\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);                        
    }
 
    /*send data*/
    if(EC_FALSE == __crfshttp_handle_getbgf_get_request_send_block(csocket_cnode))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_send_block_more: sockfd %d send body failed where store [%ld, %ld) and reached %ld\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_CUR_OFFSET(chttp_node)
                           );

        return (EC_FALSE);                        
    }

    if(CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) >= CHTTP_NODE_STORE_END_OFFSET(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getbgf_get_request_send_block_more: sockfd %d send [%ld, %ld) and len %ld done\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_SIZE(chttp_node)
                           );

        if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
        {
            cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT);

            cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT,
                             (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_recv_req, (void *)csocket_cnode);

            chttp_resume_parser(CHTTP_NODE_PARSER(chttp_node)); /*resume parser*/
            chttp_parse(chttp_node); /*try to parse if i-buffer has data more*/     
            return (EC_TRUE);/*wait for next http request*/
        }
        return (EC_DONE);/*return EC_DONE will trigger CEPOLL cleanup*/
    }

    /*read block (fd) for next sending*/
    if(EC_FALSE ==__crfshttp_handle_getbgf_get_request_read_block(csocket_cnode))
    {
        CSTRING *store_path;

        store_path = CHTTP_NODE_STORE_PATH(chttp_node);
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_send_block_more: crfs read %s failed\n",
                            (char *)cstring_get_str(store_path));

        return (EC_FALSE);
    }

    /*wait for next writing*/
    cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                     (CEPOLL_EVENT_HANDLER)CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node), (void *)csocket_cnode);  

    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getbgf_get_request_fetch_path(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
    CSTRING       *store_path; 
    uint8_t       *cache_key;
    uint32_t       cache_len;

    uri_cbuffer   = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getbgf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getbgf");
 
    store_path = cstring_new(NULL_PTR, LOC_CRFSHTTP_0010);
    if(NULL_PTR == store_path)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getbgf_get_request_fetch_path: new cstring for store path %.*s failed\n",
                            cache_len, cache_key);
        return (EC_FALSE);
    }
    cstring_append_chars(store_path, cache_len, cache_key);
    CHTTP_NODE_STORE_PATH(chttp_node) = store_path;

    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getbgf_get_request_fetch_expired_body_need(CHTTP_NODE *chttp_node)
{
    char          *expired_body_str;
 
    expired_body_str = chttp_node_get_header(chttp_node, (const char *)"expired-body");
    if(NULL_PTR == expired_body_str || 0 == c_str_to_uint32(expired_body_str))
    {
        CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node) = BIT_TRUE;/*even if file expired, return file content*/
    }
    else
    {
        CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node) = BIT_FALSE;/*if file expired, NOT return file content*/
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_handle_getbgf_get_request(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE *csocket_cnode;

    CSTRING       *store_path;

    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    char          *data_offset_str;
    char          *data_size_str;

    UINT32         expires_timestamp;
    char           expires_str[64];
    uint32_t       expires_str_len;
    uint32_t       expired_body_needed;
    //int            sock_sendbuf_size;
    uint64_t       store_size_of_file;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    if(EC_FALSE == __crfshttp_handle_getbgf_get_request_fetch_path(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getbgf_get_request: fetch store path failed\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getbgf_get_request: fetch store path failed");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
                          
        //return (EC_FALSE);
        return (EC_TRUE);
    }
    store_path = CHTTP_NODE_STORE_PATH(chttp_node);
 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getbgf_get_request: path %s\n", (char *)cstring_get_str(store_path));
 
    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getbgf_get_request: path %s\n", (char *)cstring_get_str(store_path));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getbgf_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getbgf_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getbgf_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_getbgf_get_request: path %s", (char *)cstring_get_str(store_path));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    /*clean content which will never be used*/
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    /*get expired_body_need flag*/
    __crfshttp_handle_getbgf_get_request_fetch_expired_body_need(chttp_node);
    expired_body_needed = CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node);

    /*get file store_size*/
    if(EC_FALSE == crfs_store_size_b(CSOCKET_CNODE_MODI(csocket_cnode), store_path, &store_size_of_file, &expires_timestamp))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getbgf_get_request: crfs get store size of file %s failed\n",
                            (char *)cstring_get_str(store_path));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getbgf_get_request: crfs get store size of file %s failed", (char *)cstring_get_str(store_path));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
    
        //return (EC_FALSE);
        return (EC_TRUE);
    } 

    /*set default [beg, end)*/
    CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) = 0;
    CHTTP_NODE_STORE_END_OFFSET(chttp_node) = store_size_of_file;
 
    data_offset_str = chttp_node_get_header(chttp_node, (const char *)"store-offset");
    data_size_str   = chttp_node_get_header(chttp_node, (const char *)"store-size");

    if(NULL_PTR != data_offset_str)
    {
        uint64_t   data_offset;
     
        data_offset = c_str_to_uint64(data_offset_str);
        if(data_offset >= store_size_of_file) /*invalid offset*/
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getbgf_get_request: crfs file %s, data_offset %ld >= store_size_of_file %ld\n",
                                (char *)cstring_get_str(store_path), data_offset, store_size_of_file);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getbgf_get_request: crfs file %s, data_offset %ld >= store_size_of_file %ld", (char *)cstring_get_str(store_path), data_offset, store_size_of_file);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
        
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) = data_offset;
    }

    if(NULL_PTR != data_size_str)
    {
        uint64_t  data_len;
     
        data_len  = c_str_to_uint64(data_size_str);/*note: when data_size_str is null, data_len is zero*/

        /*if 0 == data_len, from offset to end of file*/
        /*else if data_len + beg > end, from offset to end of file*/
        /*else, from offset to beg + end*/

        if(0 == data_len) /*ok, from offset to end of file*/
        {
            /*nothing to do*/
        }

        else if (data_len + CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) > CHTTP_NODE_STORE_END_OFFSET(chttp_node))
        {
            /*nothing to do*/
        }

        else
        {
            CHTTP_NODE_STORE_END_OFFSET(chttp_node) = CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) + data_len;
        }
    }

    /*set cur*/
    CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) = CHTTP_NODE_STORE_BEG_OFFSET(chttp_node);
 
    /*set http status code: OK*/
    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    /*set Expires header*/
    expires_str_len = snprintf(expires_str, sizeof(expires_str), "Expires:%ld\r\n", expires_timestamp);
    cbuffer_set(CHTTP_NODE_EXPIRES(chttp_node), (uint8_t *)expires_str, expires_str_len);

    CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node) = (CHTTP_NODE_SEND_MORE_FUNC)__crfshttp_handle_getbgf_get_request_send_block_more;
    CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node)  = (UINT32)expired_body_needed; /*transfer only*/
    CHTTP_NODE_SEND_DATA_BUFF(chttp_node)      = NULL_PTR;
    CHTTP_NODE_SEND_DATA_TOTAL_LEN(chttp_node) = 0;
    CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node)  = 0;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getbgf_get_request: to read file %s in range [%ld, %ld) and len %ld\n",
                       (char *)cstring_get_str(store_path),
                       CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                       CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                       CHTTP_NODE_STORE_SIZE(chttp_node));

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CHTTP_NODE_STORE_SIZE(chttp_node));
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getbgf_get_request: to read file %s in range [%ld, %ld) and len %ld", (char *)cstring_get_str(store_path),CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),CHTTP_NODE_STORE_END_OFFSET(chttp_node),CHTTP_NODE_STORE_SIZE(chttp_node));

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_getbgf_get_response(CHTTP_NODE *chttp_node)
{
    //CSOCKET_CNODE *csocket_cnode;
 
    uint64_t       store_size;

    //csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    store_size    = CHTTP_NODE_STORE_SIZE(chttp_node);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
     
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer   = CHTTP_NODE_URI(chttp_node);
 
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getbgf");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getbgf");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getbgf_get_response: path %.*s\n", cache_len, cache_key);
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getbgf_get_response: store_size %ld\n", store_size);
    } 

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, store_size))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getbgf_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getbgf_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getbgf_get_response: make header end failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_getbgf_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getbgf_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: getrgf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_getrgf_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/getrgf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/getrgf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*regular file*/
EC_BOOL crfshttp_is_http_get_getrgf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_getrgf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_getrgf_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_getrgf_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_getrgf_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getrgf_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_getrgf_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getrgf_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_getrgf_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getrgf_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

static EC_BOOL __crfshttp_handle_getrgf_get_request_send_block(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE    *chttp_node;
 
    chttp_node    = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);

    if(CHTTP_NODE_SEND_BLOCK_POS(chttp_node) < CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node))
    { 
        UINT32 pos;

        pos = CHTTP_NODE_SEND_BLOCK_POS(chttp_node);
        if(EC_FALSE == csocket_sendfile(CSOCKET_CNODE_SOCKFD(csocket_cnode),
                                   CHTTP_NODE_SEND_BLOCK_FD(chttp_node),
                                   CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node),
                                   &pos))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getrgf_get_request_send_block: sockfd %d sendfile %ld bytes failed\n",
                               CSOCKET_CNODE_SOCKFD(csocket_cnode),
                               CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node) - CHTTP_NODE_SEND_BLOCK_POS(chttp_node)
                               );
            return (EC_FALSE);                        
        }
#if 0
        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getrgf_get_request_send_body: write %ld bytes from %ld to %ld\n",
                           pos - CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node),
                           CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node), pos);
#endif
        CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) += (pos - CHTTP_NODE_SEND_BLOCK_POS(chttp_node));
        CHTTP_NODE_SEND_BLOCK_POS(chttp_node)    = pos;
    }

    if(CHTTP_NODE_SEND_BLOCK_POS(chttp_node) < CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node))
    {
        /*wait for next writing*/

        return (EC_TRUE);
    }

    CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node) = 0;
    CHTTP_NODE_SEND_BLOCK_POS(chttp_node)  = 0;

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getrgf_get_request_send_block: write offset reach %ld\n",
                       CHTTP_NODE_STORE_CUR_OFFSET(chttp_node));
    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getrgf_get_request_send_block_more(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE    *chttp_node;

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getrgf_get_request_send_block_more: socket %d -> chttp_node is null\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);                        
    }
 
    /*send data*/
    if(EC_FALSE == __crfshttp_handle_getrgf_get_request_send_block(csocket_cnode))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getrgf_get_request_send_block_more: sockfd %d send body failed where store [%ld, %ld) and reached %ld\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_CUR_OFFSET(chttp_node)
                           );

        return (EC_FALSE);                        
    }

    if(CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) >= CHTTP_NODE_STORE_END_OFFSET(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] __crfshttp_handle_getrgf_get_request_send_block_more: sockfd %d send [%ld, %ld) and len %ld done\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                           CHTTP_NODE_STORE_SIZE(chttp_node)
                           );

        if(ERR_FD != CHTTP_NODE_SEND_BLOCK_FD(chttp_node))
        {
            c_file_close(CHTTP_NODE_SEND_BLOCK_FD(chttp_node));
            CHTTP_NODE_SEND_BLOCK_FD(chttp_node) = ERR_FD;
        }

        if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
        {
            cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT);

            cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT,
                             (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_recv_req, (void *)csocket_cnode);

            chttp_resume_parser(CHTTP_NODE_PARSER(chttp_node)); /*resume parser*/
            chttp_parse(chttp_node); /*try to parse if i-buffer has data more*/      
            return (EC_TRUE);/*wait for next http request*/
        }
        return (EC_DONE);/*return EC_DONE will trigger CEPOLL cleanup*/   
    }

    /*wait for next sending*/
    cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                     (CEPOLL_EVENT_HANDLER)CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node), (void *)csocket_cnode);
                  
    return (EC_TRUE);
}

static EC_BOOL __crfshttp_handle_getrgf_get_request_fetch_path(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
    CSTRING       *store_path; 
    uint8_t       *cache_key;
    uint32_t       cache_len;

    uri_cbuffer   = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getrgf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getrgf");
 
    store_path = cstring_new(NULL_PTR, LOC_CRFSHTTP_0011);
    if(NULL_PTR == store_path)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:__crfshttp_handle_getrgf_get_request_fetch_path: new cstring for store path %.*s failed\n",
                            cache_len, cache_key);
        return (EC_FALSE);
    }
    cstring_append_chars(store_path, cache_len, cache_key);
    CHTTP_NODE_STORE_PATH(chttp_node) = store_path;

    return (EC_TRUE);
}

EC_BOOL crfshttp_handle_getrgf_get_request(CHTTP_NODE *chttp_node)
{
    //CSOCKET_CNODE *csocket_cnode;
 
    CSTRING       *store_path;

    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    char          *data_offset_str;
    char          *data_size_str;

    uint64_t       store_size_of_file;

    int            fd;

    //csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    if(EC_FALSE == __crfshttp_handle_getrgf_get_request_fetch_path(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getrgf_get_request: fetch store path failed\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getrgf_get_request: fetch store path failed");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
                          
        //return (EC_FALSE);
        return (EC_TRUE);
    }
    store_path = CHTTP_NODE_STORE_PATH(chttp_node);
 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getrgf_get_request: path %s\n", (char *)cstring_get_str(store_path));
 
    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getrgf_get_request: path %s\n", (char *)cstring_get_str(store_path));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getrgf_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getrgf_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getrgf_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_getrgf_get_request: path %s", (char *)cstring_get_str(store_path));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    /*clean content which will never be used*/
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    /*open file*/
    fd = c_file_open((char *)cstring_get_str(store_path), O_RDONLY, 0666);
    if(ERR_FD == fd)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getrgf_get_request: open file %s failed\n",
                            (char *)cstring_get_str(store_path));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getrgf_get_request: open file %s failed", (char *)cstring_get_str(store_path));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
    
        //return (EC_FALSE);
        return (EC_TRUE);
    }

    /*get file size*/
    if(EC_FALSE == c_file_size_b(fd, &store_size_of_file))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getrgf_get_request: get size of file %s failed\n",
                            (char *)cstring_get_str(store_path));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getrgf_get_request: get size of file %s failed", (char *)cstring_get_str(store_path));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
        c_file_close(fd);
        //return (EC_FALSE);
        return (EC_TRUE);
    }

    CHTTP_NODE_SEND_BLOCK_FD(chttp_node)   = fd;

    /*set default [beg, end)*/
    CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) = 0;
    CHTTP_NODE_STORE_END_OFFSET(chttp_node) = store_size_of_file;

    data_offset_str = chttp_node_get_header(chttp_node, (const char *)"store-offset");
    data_size_str   = chttp_node_get_header(chttp_node, (const char *)"store-size");

    if(NULL_PTR != data_offset_str)
    {
        uint64_t   data_offset;
     
        data_offset = c_str_to_uint64(data_offset_str);
        if(data_offset >= store_size_of_file) /*invalid offset*/
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getrgf_get_request: crfs file %s, data_offset %ld >= store_size_of_file %ld\n",
                                (char *)cstring_get_str(store_path), data_offset, store_size_of_file);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getrgf_get_request: crfs file %s, data_offset %ld >= store_size_of_file %ld", (char *)cstring_get_str(store_path), data_offset, store_size_of_file);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
        
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) = data_offset;
    }

    if(NULL_PTR != data_size_str)
    {
        uint64_t  data_len;
     
        data_len  = c_str_to_uint64(data_size_str);/*note: when data_size_str is null, data_len is zero*/

        /*if 0 == data_len, from offset to end of file*/
        /*else if data_len + beg > end, from offset to end of file*/
        /*else, from offset to beg + end*/

        if(0 == data_len) /*ok, from offset to end of file*/
        {
            /*nothing to do*/
        }

        else if (data_len + CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) > CHTTP_NODE_STORE_END_OFFSET(chttp_node))
        {
            /*nothing to do*/
        }

        else
        {
            CHTTP_NODE_STORE_END_OFFSET(chttp_node) = CHTTP_NODE_STORE_BEG_OFFSET(chttp_node) + data_len;
        }
    }

    /*set cur*/
    CHTTP_NODE_STORE_CUR_OFFSET(chttp_node) = CHTTP_NODE_STORE_BEG_OFFSET(chttp_node);
 
    /*set http status code: OK*/
    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node) = (CHTTP_NODE_SEND_MORE_FUNC)__crfshttp_handle_getrgf_get_request_send_block_more;
    CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node)  = (UINT32)0; /*unused*/
    CHTTP_NODE_SEND_DATA_BUFF(chttp_node)      = NULL_PTR;
    CHTTP_NODE_SEND_DATA_TOTAL_LEN(chttp_node) = 0;
    CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node)  = 0;

    CHTTP_NODE_SEND_BLOCK_POS(chttp_node)      = (UINT32)CHTTP_NODE_STORE_BEG_OFFSET(chttp_node);
    CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node)     = (UINT32)CHTTP_NODE_STORE_SIZE(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getrgf_get_request: to read file %s in range [%ld, %ld) and len %ld\n",
                       (char *)cstring_get_str(store_path),
                       CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),
                       CHTTP_NODE_STORE_END_OFFSET(chttp_node),
                       CHTTP_NODE_STORE_SIZE(chttp_node));

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CHTTP_NODE_STORE_SIZE(chttp_node));
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getrgf_get_request: to read file %s in range [%ld, %ld) and len %ld", (char *)cstring_get_str(store_path),CHTTP_NODE_STORE_BEG_OFFSET(chttp_node),CHTTP_NODE_STORE_END_OFFSET(chttp_node),CHTTP_NODE_STORE_SIZE(chttp_node));
    return (EC_TRUE);
}

EC_BOOL crfshttp_make_getrgf_get_response(CHTTP_NODE *chttp_node)
{
    //CSOCKET_CNODE *csocket_cnode;
 
    uint64_t       store_size;

    //csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    store_size    = CHTTP_NODE_STORE_SIZE(chttp_node);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer   = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getrgf");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getrgf");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getrgf_get_response: path %.*s\n", cache_len, cache_key);
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getrgf_get_response: store_size %ld\n", store_size);
    } 

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, store_size))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getrgf_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getrgf_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getrgf_get_response: make header end failed\n");
        return (EC_FALSE);
    }  
    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_getrgf_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getrgf_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: setsmf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_setsmf_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/setsmf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/setsmf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_post_setsmf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_setsmf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_setsmf_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_setsmf_post_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_setsmf_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_setsmf_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_setsmf_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_setsmf_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}


EC_BOOL crfshttp_handle_setsmf_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);/*note: maybe request does not carry on Content-Lenght header*/
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: path %.*s, invalid content length %"PRId64"\n",
                                                 CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmf"),
                                                 CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmf"),
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_setsmf_post_request: path %.*s, invalid content length %"PRId64, (uint32_t)(CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmf")),(char *)(CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmf")),content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmf");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_post_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: path %*s, invalid body length %"PRId64"\n",
                                                 (char *)cstring_get_str(&path_cstr),
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_setsmf_post_request: path %s, invalid body length %"PRId64, (char *)cstring_get_str(&path_cstr),body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_setsmf_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_setsmf_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = cbytes_new(0);
    if(NULL_PTR == content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_post_request: new cbytes without buff failed");
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        cbytes_free(content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_setsmf_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        char    *expired_str;
        uint32_t expired_nsec;

        expired_str  = chttp_node_get_header(chttp_node, (const char *)"Expires");
        expired_nsec = c_str_to_uint32(expired_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_write(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_write_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done\n",
                            (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done", (char *)cstring_get_str(&path_cstr));
#endif
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cstring_clean(&path_cstr);
    cbytes_free(content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_setsmf_post_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_post_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_setsmf_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*-------------------------------------- HTTP METHOD: POST, FILE OPERATOR: setsmfmemc ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_setsmf_memc_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/setsmfmemc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/setsmfmemc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_post_setsmf_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_setsmf_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_setsmf_memc_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_setsmf_memc_post_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_setsmf_memc_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_memc_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_setsmf_memc_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_memc_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_setsmf_memc_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_setsmf_memc_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_memc_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}


EC_BOOL crfshttp_handle_setsmf_memc_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);/*note: maybe request does not carry on Content-Lenght header*/
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: path %.*s, invalid content length %"PRId64"\n",
                                                 CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmfmemc"),
                                                 CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmfmemc"),
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_setsmf_memc_post_request: path %.*s, invalid content length %"PRId64, (uint32_t)(CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmfmemc")),(char *)(CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmfmemc")),content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/setsmfmemc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/setsmfmemc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_memc_post_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: path %*s, invalid body length %"PRId64"\n",
                                                 (char *)cstring_get_str(&path_cstr),
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_setsmf_memc_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_setsmf_memc_post_request: path %s, invalid body length %"PRId64, (char *)cstring_get_str(&path_cstr),body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_setsmf_memc_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_setsmf_memc_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = cbytes_new(0);
    if(NULL_PTR == content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_memc_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_memc_post_request: new cbytes without buff failed");
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_memc_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_memc_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        cbytes_free(content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_setsmf_memc_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
/* because write memory cache only, expired_nsec is not needed */
/*
        char    *expired_str;
        uint32_t expired_nsec;

        expired_str  = chttp_node_header_get(chttp_node, (const char *)"Expires");
        expired_nsec = c_str_to_uint32(expired_str);
*/   

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_write_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_memc_post_request: crfs write memcache %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_memc_post_request: crfs write memcache %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_memc_post_request: crfs write memcache %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_setsmf_memc_post_request: crfs write memcache %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_write_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_setsmf_post_request: crfs write %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done\n",
                            (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_setsmf_post_request: crfs write %s done", (char *)cstring_get_str(&path_cstr));
#endif
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_setsmf_memc_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cstring_clean(&path_cstr);
    cbytes_free(content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_setsmf_memc_post_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_memc_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_memc_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_setsmf_memc_post_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_setsmf_memc_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_setsmf_memc_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/* check whether file is in memory cache */
/*--------------------------------------- HTTP METHOD: GET, FILE OPERATOR: checkmemc ---------------------------------------*/
static EC_BOOL __crfshttp_uri_is_check_memc_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/checkmemc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/checkmemc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_check_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_check_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_check_memc_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_check_memc_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_check_memc_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_check_memc_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_check_memc_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_check_memc_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_check_memc_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_check_memc_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_check_memc_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    //CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/checkmemc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/checkmemc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_check_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_check_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_check_memc_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_check_memc_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_check_memc_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_check_memc_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    //content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    //cbytes_clean(content_cbytes);
 
    CSOCKET_CNODE * csocket_cnode;     

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    //if(EC_FALSE == crfs_check_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes))

    /* if file is NOT in memcache, the response is NOT_FOUND, else the response is OK */
    if(EC_FALSE == crfs_check_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_check_memc_get_request: crfs check memcache %s failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_check_memc_get_request: crfs check memcache %s failed", (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
     
        cstring_clean(&path_cstr);
        //cbytes_clean(content_cbytes);
        //return (EC_FALSE);
        return (EC_TRUE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_check_memc_get_request: crfs check memcache %s done\n",
                        (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    //CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_check_memc_get_request: crfs check memcache %s done", (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                         
 
    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_check_memc_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/checkmemc");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/checkmemc");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_check_memc_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_check_memc_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_check_memc_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_check_memc_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_check_memc_get_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_check_memc_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_check_memc_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*------------------------------------- HTTP METHOD: GET, FILE OPERATOR: getsmfmemc ---------------------------------------*/
static EC_BOOL __crfshttp_uri_is_getsmf_memc_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/getsmfmemc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/getsmfmemc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_getsmf_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_getsmf_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_getsmf_memc_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_getsmf_memc_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_getsmf_memc_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_memc_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_getsmf_memc_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_memc_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_getsmf_memc_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_memc_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_getsmf_memc_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;
/*
    char          *expired_body_str;
    char          *store_offset_str;
    char          *store_size_str;

    EC_BOOL        expired_body_needed;
    UINT32         expires_timestamp;
    char           expires_str[64];
    uint32_t       expires_str_len;
*/
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getsmfmemc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getsmfmemc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_memc_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_memc_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_getsmf_memc_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_getsmf_memc_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);
#if 0
    expired_body_str = chttp_node_header_get(chttp_node, (const char *)"expired-body");
    if(NULL_PTR == expired_body_str || 0 == c_str_to_uint32(expired_body_str))
    {
        expired_body_needed = EC_TRUE;/*even if file expired, return file content*/
    }
    else
    {
        expired_body_needed = EC_FALSE;/*if file expired, NOT return file content*/
    }
#endif
#if 0
    store_offset_str = chttp_node_header_get(chttp_node, (const char *)"store-offset");
    if(NULL_PTR != store_offset_str)
    {
        CSOCKET_CNODE * csocket_cnode;
     
        uint32_t store_offset;
        uint32_t store_size;
     
        UINT32   offset;
        UINT32   max_len;
     
        store_size_str   = chttp_node_header_get(chttp_node, (const char *)"store-size");

        store_offset = c_str_to_uint32(store_offset_str);
        store_size   = c_str_to_uint32(store_size_str);/*note: when store_size_str is null, store_size is zero*/

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        offset        = store_offset;
        max_len       = store_size;

        if(EC_FALSE == crfs_read_e(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, &offset, max_len, content_cbytes, &expires_timestamp, expired_body_needed))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_memc_get_request: crfs read %s with offset %u, size %u failed\n",
                                (char *)cstring_get_str(&path_cstr), store_offset, store_size);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getsmf_memc_get_request: crfs read %s with offset %u, size %u failed", (char *)cstring_get_str(&path_cstr), store_offset, store_size);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            cbytes_clean(content_cbytes);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_memc_get_request: crfs read %s with offset %u, size %u done\n",
                            (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getsmf_memc_get_request: crfs read %s with offset %u, size %u done", (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
#endif
 
    /*read whole file content*/
 
    CSOCKET_CNODE * csocket_cnode;   

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(EC_FALSE == crfs_read_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_memc_get_request: crfs read %s from memcache failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getsmf_memc_get_request: crfs read %s from memcache failed", (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
     
        cstring_clean(&path_cstr);
        cbytes_clean(content_cbytes);
        //return (EC_FALSE);
        return (EC_TRUE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_memc_get_request: crfs read %s from memcache done\n",
                        (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getsmf_memc_get_request: crfs read %s from memcache done", (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                         


//    expires_str_len = snprintf(expires_str, sizeof(expires_str), "Expires:%ld\r\n", expires_timestamp);
//    cbuffer_set(CHTTP_NODE_EXPIRES(chttp_node), (uint8_t *)expires_str, expires_str_len);

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_getsmf_memc_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getsmfmemc");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getsmfmemc");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_getsmf_memc_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_memc_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_memc_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_memc_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_memc_get_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_getsmf_memc_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_memc_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*-------------------------------------- HTTP METHOD: POST, FILE OPERATOR: updatememc --------------------------------------*/
static EC_BOOL __crfshttp_uri_is_update_memc_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/updatememc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/updatememc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_post_update_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_update_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_update_memc_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_update_memc_post_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_update_memc_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_memc_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_update_memc_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_memc_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_update_memc_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_update_memc_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_memc_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_update_memc_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(!((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: path %.*s, invalid content length %"PRId64"\n",
                                                 CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/updatememc"),
                                                 CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/updatememc"),
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_update_memc_post_request: path %.*s, invalid content length %"PRId64, (uint32_t)(CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/update_memc")),(char *)(CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/update_memc")),content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    } 

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/updatememc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/updatememc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_update_memc_post_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: path %*s, invalid body length %"PRId64"\n",
                                                 (char *)cstring_get_str(&path_cstr),
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_memc_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_update_memc_post_request: path %s, invalid body length %"PRId64, (char *)cstring_get_str(&path_cstr),body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_update_memc_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_update_memc_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = cbytes_new(0);
    if(NULL_PTR == content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_memc_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_memc_post_request: new cbytes with len zero failed");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_memc_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_memc_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        cbytes_free(content_cbytes);
        return (EC_TRUE);
    } 
 
    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node); 
 
    if(EC_TRUE == __crfshttp_uri_is_update_memc_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        //char    *expired_str;
        //uint32_t expired_nsec;

        //expired_str  = chttp_node_header_get(chttp_node, (const char *)"Expires");
        //expired_nsec = c_str_to_uint32(expired_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_update_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_memc_post_request: crfs update_memc %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_memc_post_request: crfs update_memc %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_update_memc_post_request: crfs update_memc %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_update_memc_post_request: crfs update_memc %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_update_memc_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_memc_post_request: crfs update_memc %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_memc_post_request: crfs update_memc %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
#endif
    }
    
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_memc_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cstring_clean(&path_cstr);
    cbytes_free(content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_update_memc_post_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_memc_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_memc_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_memc_post_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_update_memc_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_memc_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*-------------------------------------- HTTP METHOD: GET, FILE OPERATOR: dsmfmemc --------------------------------------*/
static EC_BOOL __crfshttp_uri_is_dsmf_memc_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/dsmfmemc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/dsmfmemc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete small/regular file*/
EC_BOOL crfshttp_is_http_get_dsmf_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_dsmf_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_dsmf_memc_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_dsmf_memc_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_dsmf_memc_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_memc_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_dsmf_memc_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_memc_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_dsmf_memc_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_memc_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_dsmf_memc_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/dsmfmemc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/dsmfmemc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_dsmf_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_memc_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_memc_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_memc_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_dsmf_memc_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    } 

    if(EC_TRUE == __crfshttp_uri_is_dsmf_memc_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_delete_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_REG))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_memc_get_request: crfs delete file %s from memcache failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_dsmf_memc_get_request: crfs delete file %s from memcache failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_dsmf_memc_get_request: crfs delete file %s from memcache done\n",
                            (char *)cstring_get_str(&path_cstr)); 

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_dsmf_memc_get_request: crfs delete file %s from memcache done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_delete_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_REG, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_memc_get_request: crfs delete file %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_dsmf_memc_get_request: crfs delete file %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
#endif
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_memc_get_request: should never reach here!\n");     
        task_brd_default_abort();
    }

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_dsmf_memc_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_memc_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_memc_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }    

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_memc_get_response: make header end failed\n");
        return (EC_FALSE);
    } 

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_dsmf_memc_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_memc_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*-------------------------------------- HTTP METHOD: GET, FILE OPERATOR: ddirmemc --------------------------------------*/
static EC_BOOL __crfshttp_uri_is_ddir_memc_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/ddirmemc/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/ddirmemc/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete dir*/
EC_BOOL crfshttp_is_http_get_ddir_memc(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_ddir_memc: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_ddir_memc_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_ddir_memc_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_ddir_memc_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_memc_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_ddir_memc_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_memc_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_ddir_memc_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_memc_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_ddir_memc_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/ddirmemc");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/ddirmemc");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_ddir_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_memc_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_memc_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_memc_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_memc_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_ddir_memc_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(EC_TRUE == __crfshttp_uri_is_ddir_memc_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_delete_memc(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_DIR))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_memc_get_request: crfs delete dir %s from memcache failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_ddir_memc_get_request: crfs delete dir %s from memcache failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_ddir_memc_get_request: crfs delete dir %s from memcache done\n",
                            (char *)cstring_get_str(&path_cstr));  

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_ddir_memc_get_request: crfs delete dir %s from memcache done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_delete_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_DIR, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_memc_get_request: crfs delete dir %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_ddir_memc_get_request: crfs delete dir %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
#endif
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_memc_get_request: should never reach here!\n");     
        task_brd_default_abort();
    } 

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_ddir_memc_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_memc_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_memc_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_memc_get_response: make header end failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_ddir_memc_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_memc_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    } 

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*---------------------------------------- HTTP METHOD: PUT, FILE OPERATOR: getsmf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_getsmf_head_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/getsmf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/getsmf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_head_getsmf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_head_getsmf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_getsmf_head_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}
EC_BOOL crfshttp_commit_getsmf_head_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_getsmf_head_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_head_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_getsmf_head_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_head_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_getsmf_head_request: make 'SET' response done\n");

    ret = crfshttp_commit_getsmf_head_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_head_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_getsmf_head_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    CBYTES        *content_cbytes;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
 
    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/getsmf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/getsmf");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_head_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);
 
    if(EC_TRUE == __crfshttp_uri_is_getsmf_head_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        uint64_t        file_size;

        uint8_t        file_size_header[32];
        uint32_t       file_size_header_len;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_size(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, &file_size))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_head_request: crfs get size of %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "HEAD", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_getsmf_head_request: crfs get size of %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_getsmf_head_request: crfs get size of %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %"PRId64, "HEAD", CHTTP_OK, file_size);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_getsmf_head_request: crfs get size of %s done", (char *)cstring_get_str(&path_cstr));

        file_size_header_len = snprintf((char *)file_size_header, sizeof(file_size_header),
                                        "file-size:%"PRId64"\r\n", file_size);
        cbytes_set(content_cbytes, file_size_header, file_size_header_len);
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_getsmf_head_request: should never reach here!\n");     
        task_brd_default_abort();
    }

    cstring_clean(&path_cstr);
 
    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_getsmf_head_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint8_t       *file_size_buf;
    uint32_t       file_size_len;
 
    /*note: content carry on file-size info but not response body*/
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    file_size_buf  = CBYTES_BUF(content_cbytes);
    file_size_len  = (uint32_t)CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_head_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_head_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_data(chttp_node, file_size_buf, file_size_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_head_response: make response header file-size failed\n");
        return (EC_FALSE);
    } 
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_getsmf_head_response: make header end failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_getsmf_head_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_getsmf_head_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: update ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_update_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/update/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/update/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_post_update(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_update: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_update_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_update_post_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_update_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_update_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_update_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_update_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_update_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(!((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: path %.*s, invalid content length %"PRId64"\n",
                                                 CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/update"),
                                                 CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/update"),
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_update_post_request: path %.*s, invalid content length %"PRId64, (uint32_t)(CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/update")),(char *)(CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/update")),content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    } 

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/update");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/update");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_update_post_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node); ;
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: path %*s, invalid body length %"PRId64"\n",
                                                 (char *)cstring_get_str(&path_cstr),
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_update_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_update_post_request: path %s, invalid body length %"PRId64, (char *)cstring_get_str(&path_cstr),body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_update_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_update_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = cbytes_new(0);
    if(NULL_PTR == content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_post_request: new cbytes with len zero failed");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        cbytes_free(content_cbytes);
        return (EC_TRUE);
    } 
 
    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node); 
 
    if(EC_TRUE == __crfshttp_uri_is_update_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        char    *expired_str;
        uint32_t expired_nsec;

        expired_str  = chttp_node_get_header(chttp_node, (const char *)"Expires");
        expired_nsec = c_str_to_uint32(expired_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_update(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_post_request: crfs update %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_post_request: crfs update %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_update_post_request: crfs update %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "POST", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_update_post_request: crfs update %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_update_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, content_cbytes, expired_nsec, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_post_request: crfs update %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_update_post_request: crfs update %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
#endif
    }
    
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_update_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cstring_clean(&path_cstr);
    cbytes_free(content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_update_post_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_update_post_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_update_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_update_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: renew ----------------------------------------*/
/*renew expires setting*/
static EC_BOOL __crfshttp_uri_is_renew_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/renew/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/renew/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_post_renew(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_renew: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_renew_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_renew_post_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_renew_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_renew_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_renew_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_renew_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_renew_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: path %.*s, invalid content length %"PRId64"\n",
                                                 CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/renew"),
                                                 CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/renew"),
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_renew_post_request: path %.*s, invalid content length %"PRId64, (uint32_t)(CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/renew")),(char *)(CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/renew")),content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/renew");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/renew");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_post_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: path %*s, invalid body length %"PRId64"\n",
                                                 (char *)cstring_get_str(&path_cstr),
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_post_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);
     
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_renew_post_request: path %s, invalid body length %"PRId64, (char *)cstring_get_str(&path_cstr),body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_renew_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_renew_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = cbytes_new(0);
    if(NULL_PTR == content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_post_request: new cbytes without buff failed");
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);
                         
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        cbytes_free(content_cbytes);
        return (EC_TRUE);
    }

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node); 
 
    if(EC_TRUE == __crfshttp_uri_is_renew_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        char    *expired_str;
        uint32_t expired_nsec;

        expired_str  = chttp_node_get_header(chttp_node, (const char *)"Expires");
        expired_nsec = c_str_to_uint32(expired_str);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_renew(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, expired_nsec))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_post_request: crfs renew %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_post_request: crfs renew %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_post_request: crfs renew %s done\n",
                            (char *)cstring_get_str(&path_cstr));   

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_renew_post_request: crfs renew %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_renew_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, expired_nsec, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_post_request: crfs renew %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_post_request: crfs renew %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            cbytes_free(content_cbytes);
            return (EC_TRUE);
        }
#endif
    } 
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cstring_clean(&path_cstr);
    cbytes_free(content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_renew_post_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_post_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_renew_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: dsmf ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_dsmf_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/dsmf/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/dsmf/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete small/regular file*/
EC_BOOL crfshttp_is_http_get_dsmf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_dsmf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_dsmf_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_dsmf_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_dsmf_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_dsmf_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_dsmf_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_dsmf_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/dsmf");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/dsmf");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_dsmf_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_dsmf_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_dsmf_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    } 

    if(EC_TRUE == __crfshttp_uri_is_dsmf_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_delete(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_REG))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_get_request: crfs delete file %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_dsmf_get_request: crfs delete file %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_dsmf_get_request: crfs delete file %s done\n",
                            (char *)cstring_get_str(&path_cstr)); 

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_dsmf_get_request: crfs delete file %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_delete_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_REG, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_get_request: crfs delete file %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_dsmf_get_request: crfs delete file %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
#endif
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_dsmf_get_request: should never reach here!\n");     
        task_brd_default_abort();
    }

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_dsmf_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }    

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_dsmf_get_response: make header end failed\n");
        return (EC_FALSE);
    } 

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_dsmf_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_dsmf_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: ddir ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_ddir_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/ddir/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/ddir/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete dir*/
EC_BOOL crfshttp_is_http_get_ddir(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_ddir: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_ddir_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_ddir_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_ddir_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_ddir_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_ddir_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_ddir_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/ddir");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/ddir");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_ddir_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_ddir_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_ddir_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(EC_TRUE == __crfshttp_uri_is_ddir_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 1
        if(EC_FALSE == crfs_delete(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_DIR))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_get_request: crfs delete dir %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_ddir_get_request: crfs delete dir %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_ddir_get_request: crfs delete dir %s done\n",
                            (char *)cstring_get_str(&path_cstr));  

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_ddir_get_request: crfs delete dir %s done", (char *)cstring_get_str(&path_cstr));
#endif
#if 0
        if(EC_FALSE == crfs_delete_r(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_DIR, CRFS_MAX_REPLICA_NUM))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_get_request: crfs delete dir %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_ddir_get_request: crfs delete dir %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
#endif
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_ddir_get_request: should never reach here!\n");     
        task_brd_default_abort();
    } 

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_ddir_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_ddir_get_response: make header end failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_ddir_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_ddir_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    } 

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: sexpire ----------------------------------------*/
/*expire single file*/
static EC_BOOL __crfshttp_uri_is_sexpire_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/sexpire/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/sexpire/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*expire single file*/
EC_BOOL crfshttp_is_http_get_sexpire(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_sexpire: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_sexpire_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_sexpire_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_sexpire_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_sexpire_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_sexpire_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_sexpire_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_sexpire_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_sexpire_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_sexpire_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/sexpire");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/sexpire");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_sexpire_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_sexpire_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_sexpire_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_sexpire_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_sexpire_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_sexpire_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    if(EC_TRUE == __crfshttp_uri_is_sexpire_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_expire(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_sexpire_get_request: crfs exipre file %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_sexpire_get_request: crfs exipre file %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_sexpire_get_request: crfs exipre file %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_sexpire_get_request: crfs exipre file %s done", (char *)cstring_get_str(&path_cstr));
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_sexpire_get_request: should never reach here!\n");     
        task_brd_default_abort();
    } 

    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_sexpire_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_sexpire_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_sexpire_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_sexpire_get_response: make header end failed\n");
        return (EC_FALSE);
    } 

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_sexpire_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_sexpire_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: mexpire ----------------------------------------*/
/*expire multiple files*/
static EC_BOOL __crfshttp_uri_is_mexpire_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/mexpire") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/mexpire")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*expire multiple files*/
EC_BOOL crfshttp_is_http_post_mexpire(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_mexpire: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_mexpire_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_mexpire_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER *uri_cbuffer;
    EC_BOOL ret;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mexpire_post_request: uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));


    if(EC_FALSE == crfshttp_handle_mexpire_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mexpire_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_mexpire_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mexpire_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mexpire_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_mexpire_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mexpire_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_mexpire_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    CBYTES        *req_content_cbytes;
    CBYTES        *rsp_content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: invalid content length %"PRId64"\n",
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mexpire_post_request: invalid content length %"PRId64, content_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }   
 
    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: invalid body length %"PRId64"\n",
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mexpire_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mexpire_post_request: invalid body length %"PRId64, body_len);
                                              
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_mexpire_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_mexpire_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        return (EC_TRUE);
    }

    if(0 == body_len)/*request carry on empty body, nothing to do*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "info:crfshttp_handle_mexpire_post_request: request body is empty\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "info:crfshttp_handle_mexpire_post_request: request body is empty");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
        return (EC_TRUE); 
    }  

    req_content_cbytes = cbytes_new(0);
    if(NULL_PTR == req_content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mexpire_post_request: new cbytes with len zero failed");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, req_content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mexpire_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cbytes_free(req_content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_mexpire_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        json_object   * files_obj;
        json_object   * rsp_body_obj;
        const char    * rsp_body_str;
        size_t          idx;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        rsp_content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
        cbytes_clean(rsp_content_cbytes);

        files_obj = json_tokener_parse((const char *)CBYTES_BUF(req_content_cbytes));
        if(NULL_PTR == files_obj)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: bad request %.*s\n",
                                    (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mexpire_post_request: bad request %.*s", (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
                                                              
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
     
            cbytes_free(req_content_cbytes);
            /*no response body*/
            return (EC_TRUE);
        }

        rsp_body_obj = json_object_new_array();

        for(idx = 0; idx < json_object_array_length(files_obj); idx ++)
        {
            json_object *file_obj;
         
            CSTRING  path_cstr;
            char    *path;
         
            file_obj = json_object_array_get_idx(files_obj, idx);
            if(NULL_PTR == file_obj)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: invalid file at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            path = (char *)json_object_to_json_string_ext(file_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
            if(NULL_PTR == path)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: path is null at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            if('"' == (*path))
            {
                path ++;
            }         

            if('/' == (*path))
            {
                cstring_init(&path_cstr, NULL_PTR);
            }
            else
            {
                cstring_init(&path_cstr, (const UINT8 *)"/");
            }
         
            cstring_append_str(&path_cstr, (const UINT8 *)path);
            cstring_trim(&path_cstr, (UINT8)'"');
         
            if(EC_FALSE == crfs_file_expire(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr))
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: crfs expire %s failed\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_NOT_FOUND);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mexpire_post_request: crfs expire %s failed", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("404"));
            }
            else
            {
                dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_mexpire_post_request: crfs expire %s done\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_mexpire_post_request: crfs expire %s done", (char *)cstring_get_str(&path_cstr));
             
                json_object_array_add(rsp_body_obj, json_object_new_string("200"));
            }
            cstring_clean(&path_cstr);         
        }

        rsp_body_str = json_object_to_json_string_ext(rsp_body_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
        cbytes_set(rsp_content_cbytes, (const UINT8 *)rsp_body_str, strlen(rsp_body_str) + 1);
     
        /*free json obj*/
        json_object_put(files_obj);
        json_object_put(rsp_body_obj);
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mexpire_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cbytes_free(req_content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_mexpire_post_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mexpire_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mexpire_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mexpire_post_response: make header end failed\n");
        return (EC_FALSE);
    }

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mexpire_post_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_mexpire_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mexpire_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: mdsmf ----------------------------------------*/
/*delete multiple files*/
static EC_BOOL __crfshttp_uri_is_mdsmf_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/mdsmf") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/mdsmf")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete multiple files*/
EC_BOOL crfshttp_is_http_post_mdsmf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_mdsmf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_mdsmf_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_mdsmf_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER *uri_cbuffer;
    EC_BOOL ret;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mdsmf_post_request: uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));

    if(EC_FALSE == crfshttp_handle_mdsmf_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdsmf_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_mdsmf_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdsmf_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mdsmf_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_mdsmf_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdsmf_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_mdsmf_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    CBYTES        *req_content_cbytes;
    CBYTES        *rsp_content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: invalid content length %"PRId64"\n",
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mdsmf_post_request: invalid content length %"PRId64, content_len);
             
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: invalid body length %"PRId64"\n",
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdsmf_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mdsmf_post_request: invalid body length %"PRId64, body_len);
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_mdsmf_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_mdsmf_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        return (EC_TRUE);
    }

    if(0 == body_len)/*request carry on empty body, nothing to do*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "info:crfshttp_handle_mdsmf_post_request: request body is empty\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "info:crfshttp_handle_mdsmf_post_request: request body is empty");
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
        return (EC_TRUE); 
    }
 
    req_content_cbytes = cbytes_new(0);
    if(NULL_PTR == req_content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdsmf_post_request: new cbytes with len zero failed");
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, req_content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdsmf_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cbytes_free(req_content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_mdsmf_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        json_object   * files_obj;
        json_object   * rsp_body_obj;
        const char    * rsp_body_str;
        size_t          idx;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        rsp_content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
        cbytes_clean(rsp_content_cbytes);

        files_obj = json_tokener_parse((const char *)CBYTES_BUF(req_content_cbytes));
        if(NULL_PTR == files_obj)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: bad request %.*s\n",
                                    CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdsmf_post_request: bad request %.*s", (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
                                                               
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
     
            cbytes_free(req_content_cbytes);
            /*no response body*/
            return (EC_TRUE);
        }

        rsp_body_obj = json_object_new_array();

        for(idx = 0; idx < json_object_array_length(files_obj); idx ++)
        {
            json_object *file_obj;
         
            CSTRING  path_cstr;
            char    *path;
         
            file_obj = json_object_array_get_idx(files_obj, idx);
            if(NULL_PTR == file_obj)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: invalid file at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            path = (char *)json_object_to_json_string_ext(file_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
            if(NULL_PTR == path)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: path is null at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            if('"' == (*path))
            {
                path ++;
            }
         
            if('/' == (*path))
            {
                cstring_init(&path_cstr, NULL_PTR);
            }
            else
            {
                cstring_init(&path_cstr, (const UINT8 *)"/");
            }
            cstring_append_str(&path_cstr, (const UINT8 *)path);
            cstring_trim(&path_cstr, (UINT8)'"');

            if(EC_FALSE == crfs_delete(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_REG))
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: crfs delete %s failed\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_NOT_FOUND);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdsmf_post_request: crfs delete %s failed", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("404"));
            }
            else
            {
                dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_mdsmf_post_request: crfs delete %s done\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_mdsmf_post_request: crfs delete %s done", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("200"));
            }
            cstring_clean(&path_cstr);         
        }

        rsp_body_str = json_object_to_json_string_ext(rsp_body_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
        cbytes_set(rsp_content_cbytes, (const UINT8 *)rsp_body_str, strlen(rsp_body_str) + 1);
     
        /*free json obj*/
        json_object_put(files_obj);
        json_object_put(rsp_body_obj);
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdsmf_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cbytes_free(req_content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_mdsmf_post_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdsmf_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdsmf_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }    

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdsmf_post_response: make header end failed\n");
        return (EC_FALSE);
    }

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdsmf_post_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_mdsmf_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdsmf_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: mdbgf ----------------------------------------*/
/*delete multiple files*/
static EC_BOOL __crfshttp_uri_is_mdbgf_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/mdbgf") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/mdbgf")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete multiple files*/
EC_BOOL crfshttp_is_http_post_mdbgf(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_mdbgf: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_mdbgf_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_mdbgf_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER *uri_cbuffer;
    EC_BOOL ret;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mdbgf_post_request: uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));


    if(EC_FALSE == crfshttp_handle_mdbgf_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdbgf_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_mdbgf_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdbgf_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mdbgf_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_mdbgf_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdbgf_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_mdbgf_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;

    CBYTES        *req_content_cbytes;
    CBYTES        *rsp_content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: invalid content length %"PRId64"\n",
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mdbgf_post_request: invalid content length %"PRId64, content_len);
                 
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: invalid body length %"PRId64"\n",
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mdbgf_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mdbgf_post_request: invalid body length %"PRId64, body_len);
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_mdbgf_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_mdbgf_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        return (EC_TRUE);
    }

    if(0 == body_len)/*request carry on empty body, nothing to do*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "info:crfshttp_handle_mdbgf_post_request: request body is empty\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "info:crfshttp_handle_mdbgf_post_request: request body is empty");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
        return (EC_TRUE); 
    } 

    req_content_cbytes = cbytes_new(0);
    if(NULL_PTR == req_content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdbgf_post_request: new cbytes with len zero failed");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, req_content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdbgf_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                                            
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cbytes_free(req_content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_mdbgf_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        json_object   * files_obj;
        json_object   * rsp_body_obj;
        const char    * rsp_body_str;
        size_t          idx;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        rsp_content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
        cbytes_clean(rsp_content_cbytes);

        files_obj = json_tokener_parse((const char *)CBYTES_BUF(req_content_cbytes));
        if(NULL_PTR == files_obj)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: bad request %.*s\n",
                                    (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdbgf_post_request: bad request %.*s", (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
                     
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
     
            cbytes_free(req_content_cbytes);
            /*no response body*/
            return (EC_TRUE);
        }

        rsp_body_obj = json_object_new_array();

        for(idx = 0; idx < json_object_array_length(files_obj); idx ++)
        {
            json_object *file_obj;
         
            CSTRING  path_cstr;
            char    *path;
         
            file_obj = json_object_array_get_idx(files_obj, idx);
            if(NULL_PTR == file_obj)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: invalid file at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            path = (char *)json_object_to_json_string_ext(file_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
            if(NULL_PTR == path)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: path is null at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            if('"' == (*path))
            {
                path ++;
            }

            if('/' == (*path))
            {
                cstring_init(&path_cstr, NULL_PTR);
            }
            else
            {
                cstring_init(&path_cstr, (const UINT8 *)"/");
            }
            cstring_append_str(&path_cstr, (const UINT8 *)path);
            cstring_trim(&path_cstr, (UINT8)'"');

            if(EC_FALSE == crfs_delete(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_BIG))
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: crfs delete %s failed\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_NOT_FOUND);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mdbgf_post_request: crfs delete %s failed", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("404"));
            }
            else
            {
                dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_mdbgf_post_request: crfs delete %s done\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_mdbgf_post_request: crfs delete %s done", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("200"));
            }
            cstring_clean(&path_cstr);         
        }

        rsp_body_str = json_object_to_json_string_ext(rsp_body_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
        cbytes_set(rsp_content_cbytes, (const UINT8 *)rsp_body_str, (UINT32)(strlen(rsp_body_str) + 1));
     
        /*free json obj*/
        json_object_put(files_obj);
        json_object_put(rsp_body_obj);
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mdbgf_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cbytes_free(req_content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_mdbgf_post_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdbgf_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdbgf_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdbgf_post_response: make header end failed\n");
        return (EC_FALSE);
    }

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdbgf_post_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_mdbgf_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mdbgf_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: POST, FILE OPERATOR: mddir ----------------------------------------*/
/*delete multiple files*/
static EC_BOOL __crfshttp_uri_is_mddir_post_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/mddir") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/mddir")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

/*delete multiple files*/
EC_BOOL crfshttp_is_http_post_mddir(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_post_mddir: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_mddir_post_op(uri_cbuffer))
    {
        return (EC_TRUE);
    } 

    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_mddir_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER *uri_cbuffer;
    EC_BOOL ret;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node); 
    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mddir_post_request: uri %.*s\n", CBUFFER_USED(uri_cbuffer), CBUFFER_DATA(uri_cbuffer));


    if(EC_FALSE == crfshttp_handle_mddir_post_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mddir_post_request: handle 'SET' request failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_mddir_post_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mddir_post_request: make 'SET' response failed\n");
        return (EC_FALSE);
    }

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_commit_mddir_post_request: make 'SET' response done\n");

    ret = crfshttp_commit_mddir_post_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mddir_post_request: commit 'SET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_mddir_post_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    CBYTES        *req_content_cbytes;
    CBYTES        *rsp_content_cbytes;

    uint64_t       body_len;
    uint64_t       content_len;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);
    content_len  = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > content_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > content_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: invalid content length %"PRId64"\n",
                                                 content_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mddir_post_request: invalid content length %"PRId64, content_len);
                                                     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    body_len = chttp_node_recv_len(chttp_node);
    /*CRFSHTTP_ASSERT((uint64_t)0x100000000 > body_len);*//*not consider this scenario yet*/
    if(! ((uint64_t)0x100000000 > body_len))
    {
        CHUNK_MGR *req_body_chunks;
        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: invalid body length %"PRId64"\n",
                                                 body_len);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_mddir_post_request: chunk mgr %p str\n", req_body_chunks);
        chunk_mgr_print_str(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_mddir_post_request: invalid body length %"PRId64, body_len);
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }
 
    if(content_len > body_len)
    {
        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "warn:crfshttp_handle_mddir_post_request: content_len %"PRId64" > body_len %"PRId64"\n", content_len, body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_PARTIAL_CONTENT);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "warn:crfshttp_handle_mddir_post_request: content_len %"PRId64" > body_len %"PRId64, content_len, body_len);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_PARTIAL_CONTENT;
     
        return (EC_TRUE);
    }

    if(0 == body_len)/*request carry on empty body, nothing to do*/
    {
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "info:crfshttp_handle_mddir_post_request: request body is empty\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "info:crfshttp_handle_mddir_post_request: request body is empty");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
        return (EC_TRUE); 
    }
 
    req_content_cbytes = cbytes_new(0);
    if(NULL_PTR == req_content_cbytes)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: new cbytes without buff failed\n");
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INSUFFICIENT_STORAGE);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mddir_post_request: new cbytes with len zero failed");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INSUFFICIENT_STORAGE;
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_node_recv_export_to_cbytes(chttp_node, req_content_cbytes, (UINT32)body_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: export body with len %ld to cbytes failed\n",
                            (UINT32)body_len);
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mddir_post_request: export body with len %ld to cbytes failed", (UINT32)body_len);
                                           
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cbytes_free(req_content_cbytes);
        return (EC_TRUE);
    } 

    /*clean body chunks*/
    chttp_node_recv_clean(chttp_node);
 
    if(EC_TRUE == __crfshttp_uri_is_mddir_post_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        json_object   * files_obj;
        json_object   * rsp_body_obj;
        const char    * rsp_body_str;
        size_t          idx;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        rsp_content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
        cbytes_clean(rsp_content_cbytes);

        files_obj = json_tokener_parse((const char *)CBYTES_BUF(req_content_cbytes));
        if(NULL_PTR == files_obj)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: bad request %.*s\n",
                                    CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "POST", CHTTP_BAD_REQUEST);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mddir_post_request: bad request %.*s", (uint32_t)CBYTES_LEN(req_content_cbytes), (char *)CBYTES_BUF(req_content_cbytes));
                                                                 
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
     
            cbytes_free(req_content_cbytes);
            /*no response body*/
            return (EC_TRUE);
        }

        rsp_body_obj = json_object_new_array();

        for(idx = 0; idx < json_object_array_length(files_obj); idx ++)
        {
            json_object *file_obj;
         
            CSTRING  path_cstr;
            char    *path;
         
            file_obj = json_object_array_get_idx(files_obj, idx);
            if(NULL_PTR == file_obj)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: invalid file at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            path = (char *)json_object_to_json_string_ext(file_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
            if(NULL_PTR == path)
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: path is null at %ld\n", idx);

                json_object_array_add(rsp_body_obj, json_object_new_string("404"));       
                continue;
            }

            if('"' == (*path))
            {
                path ++;
            }
         
            if('/' == (*path))
            {
                cstring_init(&path_cstr, NULL_PTR);
            }
            else
            {
                cstring_init(&path_cstr, (const UINT8 *)"/");
            }
            cstring_append_str(&path_cstr, (const UINT8 *)path);
            cstring_trim(&path_cstr, (UINT8)'"');

            if(EC_FALSE == crfs_delete(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, CRFSNP_ITEM_FILE_IS_DIR))
            {
                dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: crfs delete %s failed\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "POST", CHTTP_NOT_FOUND);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_mddir_post_request: crfs delete %s failed", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("404"));
            }
            else
            {
                dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_mddir_post_request: crfs delete %s done\n",
                                    (char *)cstring_get_str(&path_cstr));

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "POST", CHTTP_OK);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_mddir_post_request: crfs delete %s done", (char *)cstring_get_str(&path_cstr));
                                 
                json_object_array_add(rsp_body_obj, json_object_new_string("200"));
            }
            cstring_clean(&path_cstr);
        }

        rsp_body_str = json_object_to_json_string_ext(rsp_body_obj, JSON_C_TO_STRING_NOSLASHESCAPE);
        cbytes_set(rsp_content_cbytes, (const UINT8 *)rsp_body_str, (UINT32)(strlen(rsp_body_str) + 1));
     
        /*free json obj*/
        json_object_put(files_obj);
        json_object_put(rsp_body_obj);
    }
    else
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_mddir_post_request: should never reach here!\n");     
        task_brd_default_abort();
    }
 
    cbytes_free(req_content_cbytes);

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_mddir_post_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mddir_post_response: make response header failed\n");
     
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mddir_post_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mddir_post_response: make header end failed\n");
        return (EC_FALSE);
    }

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mddir_post_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_mddir_post_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_mddir_post_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: logrotate ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_logrotate_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/logrotate") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/logrotate")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_logrotate(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_logrotate: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_logrotate_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_logrotate_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_logrotate_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_logrotate_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_logrotate_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_logrotate_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_logrotate_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_logrotate_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_logrotate_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    //uint8_t       *cache_key;
    //uint32_t       cache_len;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    //cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/logrotate");
    //cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/logrotate");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_logrotate_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_logrotate_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_logrotate_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_logrotate_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    if(EC_TRUE == __crfshttp_uri_is_logrotate_get_op(uri_cbuffer))
    {
        //CSOCKET_CNODE * csocket_cnode;
        UINT32 super_md_id;

        char  *log_index_str;
        UINT32 log_index;

        super_md_id = 0;
        //csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

        log_index_str = chttp_node_get_header(chttp_node, (const char *)"log-index");
        if(NULL_PTR != log_index_str)
        {
            log_index = c_str_to_word(log_index_str);
        }
        else
        {
            log_index = DEFAULT_USRER08_LOG_INDEX; /*default LOGUSER08*/
        }
     
        if(EC_FALSE == super_rotate_log(super_md_id, log_index))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_logrotate_get_request: log rotate %ld failed\n", log_index);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_logrotate_get_request: log rotate %ld failed", log_index);

            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_logrotate_get_request: log rotate %ld done\n", log_index);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_logrotate_get_request: log rotate %ld done", log_index);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_logrotate_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_logrotate_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_logrotate_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_logrotate_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_logrotate_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_logrotate_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: actsyscfg ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_actsyscfg_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/actsyscfg") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/actsyscfg")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_actsyscfg(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_actsyscfg: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_actsyscfg_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_actsyscfg_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_actsyscfg_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_actsyscfg_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_actsyscfg_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_actsyscfg_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_actsyscfg_get_response(chttp_node);
 
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_actsyscfg_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_actsyscfg_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    //uint8_t       *cache_key;
    //uint32_t       cache_len;

    UINT32         req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    //cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/actsyscfg");
    //cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/actsyscfg");

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_actsyscfg_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_actsyscfg_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_actsyscfg_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_actsyscfg_get_request: bad request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    if(EC_TRUE == __crfshttp_uri_is_actsyscfg_get_op(uri_cbuffer))
    {
        UINT32 super_md_id;

        super_md_id = 0;

        super_activate_sys_cfg(super_md_id);

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_actsyscfg_get_request done\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_actsyscfg_get_request done");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_actsyscfg_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_actsyscfg_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_actsyscfg_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_actsyscfg_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_actsyscfg_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_actsyscfg_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: qtree ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_qtree_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/qtree") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/qtree")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_qtree(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_qtree: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_qtree_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_qtree_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_qtree_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_qtree_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_qtree_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_qtree_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_qtree_get_response(chttp_node);
 
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_qtree_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_qtree_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER     *uri_cbuffer;

    uint8_t     *cache_key;
    uint32_t     cache_len;

    CSTRING      path;

    UINT32       req_body_chunk_num;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/qtree");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/qtree");

    cstring_init(&path, NULL_PTR);
    cstring_append_chars(&path, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_qtree_get_request: path %s\n", (char *)cstring_get_str(&path));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_qtree_get_request: path %s\n", (char *)cstring_get_str(&path));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_qtree_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_qtree_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_qtree_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_qtree_get_request: bad request: path %s", (char *)cstring_get_str(&path));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
     
        cstring_clean(&path);
     
        return (EC_TRUE);
    }

    if(EC_TRUE == __crfshttp_uri_is_qtree_get_op(uri_cbuffer))
    {
        UINT32       super_md_id;

        CBYTES      *rsp_content_cbytes;

        CVECTOR      *path_cstr_vec;

        json_object *rsp_body_obj;
        const char  *rsp_body_str;

        super_md_id = 0;

        path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_CRFSHTTP_0012);
     
        if(EC_FALSE == crfs_qlist_tree(super_md_id, &path, path_cstr_vec))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_qtree_get_request failed\n");

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_qtree_get_request failed");

            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            cstring_clean(&path);
         
            cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_CRFSHTTP_0013);
            cvector_free(path_cstr_vec, LOC_CRFSHTTP_0014);

            return (EC_TRUE);
        }

        /* qtree success, get path from path_cstr_vec */
     
        rsp_content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
        cbytes_clean(rsp_content_cbytes);
     
        rsp_body_obj = json_object_new_array();

        UINT32 pos;
        CSTRING *path_cstr;
     
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
         
            if(NULL_PTR == path_cstr)
            {
                continue;
            }
         
            json_object_array_add(rsp_body_obj, json_object_new_string((const char *)cstring_get_str(path_cstr)));

            cvector_set(path_cstr_vec, pos, NULL_PTR);
            cstring_free(path_cstr);
        }

        rsp_body_str = json_object_to_json_string_ext(rsp_body_obj, JSON_C_TO_STRING_NOSLASHESCAPE);

        cbytes_set(rsp_content_cbytes, (const UINT8 *)rsp_body_str, strlen(rsp_body_str) + 1);

        dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_qtree_get_request done\n");

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_qtree_get_request done");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        cstring_clean(&path);

        cvector_free(path_cstr_vec, LOC_CRFSHTTP_0015);

        /* free json obj */
        json_object_put(rsp_body_obj);
     
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_qtree_get_response(CHTTP_NODE *chttp_node)
{

    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_qtree_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_qtree_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }  
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_qtree_get_response: make header end failed\n");
        return (EC_FALSE);
    }

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_mdsmf_post_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_qtree_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_qtree_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: file_wait ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_file_wait_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/file_wait/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/file_wait/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_file_wait(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_file_wait: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_file_wait_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_file_wait_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_file_wait_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_wait_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_file_wait_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_wait_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_file_wait_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_wait_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_file_wait_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    char          *tcid_str;
    UINT32         tcid;

    char          *wait_data_str; 
    char          *store_offset_str;
    char          *store_size_str;


    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/file_wait");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/file_wait");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_wait_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_wait_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_wait_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_wait_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_wait_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_file_wait_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    tcid_str = chttp_node_get_header(chttp_node, (const char *)"tcid");
    if(NULL_PTR == tcid_str)
    {
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_file_wait_get_request: path %s, tcid absence", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    tcid = c_ipv4_to_word(tcid_str);

    wait_data_str = chttp_node_get_header(chttp_node, (const char *)"wait-data");
    if(NULL_PTR != wait_data_str && EC_FALSE == c_str_to_bool(wait_data_str))
    {
        CSOCKET_CNODE  *csocket_cnode;
        EC_BOOL         data_ready;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        data_ready    = EC_OBSCURE;/*means wait file only without reading data util file is ready and notification is sent*/
     
        if(EC_FALSE == crfs_file_wait(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, NULL_PTR, &data_ready))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_file_wait_get_request: crfs wait %s only failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_file_wait_get_request: crfs wait %s only failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
         
            cstring_clean(&path_cstr);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s only done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, (UINT32)0);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s only done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"data-ready", c_bool_str(data_ready));

        cstring_clean(&path_cstr);

        return (EC_TRUE);
    }

    store_offset_str = chttp_node_get_header(chttp_node, (const char *)"store-offset");
    if(NULL_PTR != store_offset_str)
    {
        CSOCKET_CNODE * csocket_cnode;
     
        uint32_t store_offset;
        uint32_t store_size;
     
        UINT32   offset;
        UINT32   max_len;

        EC_BOOL  data_ready;
     
        store_size_str   = chttp_node_get_header(chttp_node, (const char *)"store-size");

        store_offset = c_str_to_uint32(store_offset_str);
        store_size   = c_str_to_uint32(store_size_str);/*note: when store_size_str is null, store_size is zero*/

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        offset        = store_offset;
        max_len       = store_size;
        data_ready    = EC_FALSE;
     
        if(EC_FALSE == crfs_file_wait_e(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, &offset, max_len, content_cbytes, &data_ready))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_file_wait_get_request: crfs wait %s with offset %u, size %u failed\n",
                                (char *)cstring_get_str(&path_cstr), store_offset, store_size);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_file_wait_get_request: crfs wait %s with offset %u, size %u failed", (char *)cstring_get_str(&path_cstr), store_offset, store_size);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            cbytes_clean(content_cbytes);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s with offset %u, size %u done\n",
                            (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s with offset %u, size %u done", (char *)cstring_get_str(&path_cstr), store_offset, store_size);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"data-ready", c_bool_str(data_ready));

        cstring_clean(&path_cstr);
    }
    else/*wait whole file content*/
    {
        CSOCKET_CNODE  *csocket_cnode;
        EC_BOOL         data_ready;
     
        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        data_ready    = EC_FALSE;
     
        if(EC_FALSE == crfs_file_wait(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, content_cbytes, &data_ready))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_file_wait_get_request: crfs wait %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_file_wait_get_request: crfs wait %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);
            cbytes_clean(content_cbytes);
            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_file_wait_get_request: crfs wait %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"data-ready", c_bool_str(data_ready));

        cstring_clean(&path_cstr);
    }
 
    return (EC_TRUE);
}

EC_BOOL crfshttp_make_file_wait_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);

    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/file_wait");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/file_wait");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_file_wait_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_kvs(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make header kvs failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR);

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_file_wait_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_wait_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: file_notify ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_file_notify_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/file_notify/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/file_notify/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_file_notify(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_file_notify: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_file_notify_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_file_notify_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_file_notify_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_notify_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_file_notify_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_notify_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_file_notify_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_notify_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_file_notify_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;


    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/file_notify");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/file_notify");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_notify_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_notify_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_notify_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_notify_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_file_notify_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_file_notify_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_file_notify_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;     

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_file_notify(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_file_notify_get_request: crfs notify %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_NOT_FOUND);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_file_notify_get_request: crfs notify %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_FOUND;
         
            cstring_clean(&path_cstr);

            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_file_notify_get_request: crfs notify %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_file_notify_get_request: crfs notify %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                         
    }
 
    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_file_notify_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/file_notify");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/file_notify");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_file_notify_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_notify_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_notify_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_notify_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_file_notify_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_file_notify_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: cond_wakeup ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_cond_wakeup_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/cond_wakeup/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/cond_wakeup/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_cond_wakeup(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_cond_wakeup: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_cond_wakeup_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_cond_wakeup_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_cond_wakeup_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_cond_wakeup_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_cond_wakeup_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_cond_wakeup_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_cond_wakeup_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_cond_wakeup_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_cond_wakeup_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;


    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/cond_wakeup");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/cond_wakeup");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_cond_wakeup_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_cond_wakeup_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_cond_wakeup_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_cond_wakeup_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_cond_wakeup_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_cond_wakeup_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_cond_wakeup_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;
        UINT32 tag;

        tag = MD_CRFS;
     
        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == super_cond_wakeup(/*CSOCKET_CNODE_MODI(csocket_cnode)*/0, tag, &path_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_cond_wakeup_get_request: cond wakeup %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_cond_wakeup_get_request: cond wakeup %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
         
            cstring_clean(&path_cstr);

            //return (EC_FALSE);
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_cond_wakeup_get_request: cond wakeup %s done\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u %ld", "GET", CHTTP_OK, CBYTES_LEN(content_cbytes));
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_cond_wakeup_get_request: cond wakeup %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;                         
    }
 
    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_cond_wakeup_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/cond_wakeup");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/cond_wakeup");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_cond_wakeup_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_cond_wakeup_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_cond_wakeup_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_cond_wakeup_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_cond_wakeup_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_cond_wakeup_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: renew_header ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_renew_header_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/renew_header/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/renew_header/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_renew_header(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_renew_header: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_renew_header_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_renew_header_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_renew_header_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_header_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_renew_header_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_header_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_renew_header_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_header_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_renew_header_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    CSOCKET_CNODE *csocket_cnode;
    char          *renew_num;

    CSTRKV_MGR    *cstrkv_mgr;
    uint32_t       num;
    uint32_t       idx;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/renew_header");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/renew_header");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_header_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_header_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_header_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_header_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_renew_header_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_renew_header_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

    renew_num  = chttp_node_get_header(chttp_node, (const char *)"renew-num");
    if(NULL_PTR == renew_num)
    {
        char    *renew_key;
        char    *renew_val;

        CSTRING  renew_key_cstr;
        CSTRING  renew_val_cstr;

        renew_key  = chttp_node_get_header(chttp_node, (const char *)"renew-key");
        if(NULL_PTR == renew_key)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to 'renew-key' absence\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to 'renew-key' absence", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        renew_val  = chttp_node_get_header(chttp_node, (const char *)"renew-val");
        if(NULL_PTR == renew_val)
        {
#if 1
            dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s would remove header ['%s'] due to 'renew-val' absence\n",
                                (char *)cstring_get_str(&path_cstr), renew_key);
#endif
#if 0     
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to 'renew-val' absence\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to 'renew-val' absence", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
#endif         
        }

        cstring_set_str(&renew_key_cstr, (UINT8 *)renew_key);
        cstring_set_str(&renew_val_cstr, (UINT8 *)renew_val);
     
        if(EC_FALSE == crfs_renew_http_header(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, &renew_key_cstr, &renew_val_cstr))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s done\n",
                            (char *)cstring_get_str(&path_cstr));   

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    cstrkv_mgr = cstrkv_mgr_new();
    if(NULL_PTR == cstrkv_mgr)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to new cstrkv_mgr failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to new cstrkv_mgr failed",
                (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    num = c_str_to_uint32(renew_num);
    for(idx = 0; idx < num; idx ++)
    {
        char     renew_key_tag[ 16 ];
        char     renew_val_tag[ 16 ];
     
        char    *renew_key;
        char    *renew_val;
     
        snprintf(renew_key_tag, sizeof(renew_key_tag)/sizeof(renew_key_tag[ 0 ]), "renew-key-%u", idx + 1);
        snprintf(renew_val_tag, sizeof(renew_val_tag)/sizeof(renew_val_tag[ 0 ]), "renew-val-%u", idx + 1);

        renew_key  = chttp_node_get_header(chttp_node, (const char *)renew_key_tag);
        if(NULL_PTR == renew_key)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), (char *)renew_key_tag);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to '%s' absence",
                    (char *)cstring_get_str(&path_cstr), (char *)renew_key_tag);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        renew_val  = chttp_node_get_header(chttp_node, (const char *)renew_val_tag);
        if(NULL_PTR == renew_val)
        {
#if 1
            dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s would remove header ['%s'] due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), renew_key, (char *)renew_val_tag);
#endif
#if 0     
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), (char *)renew_val_tag);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to '%s' absence",
                    (char *)cstring_get_str(&path_cstr), (char *)renew_val_tag);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
#endif         
        }

        if(EC_FALSE == cstrkv_mgr_add_kv_str(cstrkv_mgr, renew_key, renew_val))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to add '%s:%s' failed\n",
                                (char *)cstring_get_str(&path_cstr), renew_key, renew_val);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed due to add '%s:%s' failed",
                    (char *)cstring_get_str(&path_cstr), renew_key, renew_val);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
    }

    if(EC_FALSE == crfs_renew_http_headers(CSOCKET_CNODE_MODI(csocket_cnode), &path_cstr, cstrkv_mgr))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_renew_header_get_request: crfs renew %s failed", (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

        cstrkv_mgr_free(cstrkv_mgr);
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s done\n",
                        (char *)cstring_get_str(&path_cstr));   

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_renew_header_get_request: crfs renew %s done", (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    cstrkv_mgr_free(cstrkv_mgr);
    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_renew_header_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/renew_header");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/renew_header");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_renew_header_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_header_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_header_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_renew_header_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_renew_header_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_renew_header_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: wait_header ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_wait_header_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/wait_header/") < uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/wait_header/")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_wait_header(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_wait_header: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_wait_header_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_wait_header_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_wait_header_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_wait_header_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_wait_header_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_wait_header_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_wait_header_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_wait_header_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_wait_header_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    uint8_t       *cache_key;
    uint32_t       cache_len;

    CSTRING        path_cstr;
    CBYTES        *content_cbytes;

    UINT32         req_body_chunk_num;

    CSOCKET_CNODE *csocket_cnode;
    char          *wait_num;

    char          *tcid_str;
    UINT32         tcid;

    CSTRKV_MGR    *cstrkv_mgr;
    uint32_t       num;
    uint32_t       idx;
 
    EC_BOOL        header_ready;

    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/wait_header");
    cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/wait_header");
 
    cstring_init(&path_cstr, NULL_PTR);
    cstring_append_chars(&path_cstr, cache_len, cache_key);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_wait_header_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
     
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_wait_header_get_request: path %s\n", (char *)cstring_get_str(&path_cstr));
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_wait_header_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_wait_header_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_wait_header_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_wait_header_get_request: path %s", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);

    tcid_str = chttp_node_get_header(chttp_node, (const char *)"tcid");
    if(NULL_PTR == tcid_str)
    {
        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error: crfshttp_handle_wait_header_get_request: path %s, tcid absence", (char *)cstring_get_str(&path_cstr));
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    tcid = c_ipv4_to_word(tcid_str);

    wait_num  = chttp_node_get_header(chttp_node, (const char *)"wait-num");
    if(NULL_PTR == wait_num)
    {
        char    *wait_key;
        char    *wait_val;

        CSTRING  wait_key_cstr;
        CSTRING  wait_val_cstr;

        wait_key  = chttp_node_get_header(chttp_node, (const char *)"wait-key");
        if(NULL_PTR == wait_key)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to 'wait-key' absence\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to 'wait-key' absence", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        wait_val  = chttp_node_get_header(chttp_node, (const char *)"wait-val");
        if(NULL_PTR == wait_val)
        {
#if 1
            dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s would remove header ['%s'] due to 'wait-val' absence\n",
                                (char *)cstring_get_str(&path_cstr), wait_key);
#endif
#if 0     
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to 'wait-val' absence\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to 'wait-val' absence", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
#endif         
        }

        cstring_set_str(&wait_key_cstr, (UINT8 *)wait_key);
        cstring_set_str(&wait_val_cstr, (UINT8 *)wait_val);
     
        if(EC_FALSE == crfs_wait_http_header(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, &wait_key_cstr, &wait_val_cstr, &header_ready))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed\n",
                                (char *)cstring_get_str(&path_cstr));

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed", (char *)cstring_get_str(&path_cstr));
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;
         
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s done\n",
                            (char *)cstring_get_str(&path_cstr));   

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s done", (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"header-ready", c_bool_str(header_ready));
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }

    cstrkv_mgr = cstrkv_mgr_new();
    if(NULL_PTR == cstrkv_mgr)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to new cstrkv_mgr failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to new cstrkv_mgr failed",
                (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;
     
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    num = c_str_to_uint32(wait_num);
    for(idx = 0; idx < num; idx ++)
    {
        char     wait_key_tag[ 16 ];
        char     wait_val_tag[ 16 ];
     
        char    *wait_key;
        char    *wait_val;
     
        snprintf(wait_key_tag, sizeof(wait_key_tag)/sizeof(wait_key_tag[ 0 ]), "wait-key-%u", idx + 1);
        snprintf(wait_val_tag, sizeof(wait_val_tag)/sizeof(wait_val_tag[ 0 ]), "wait-val-%u", idx + 1);

        wait_key  = chttp_node_get_header(chttp_node, (const char *)wait_key_tag);
        if(NULL_PTR == wait_key)
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), (char *)wait_key_tag);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to '%s' absence",
                    (char *)cstring_get_str(&path_cstr), (char *)wait_key_tag);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
     
        wait_val  = chttp_node_get_header(chttp_node, (const char *)wait_val_tag);
        if(NULL_PTR == wait_val)
        {
#if 1
            dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s would remove header ['%s'] due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), wait_key, (char *)wait_val_tag);
#endif
#if 0     
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to '%s' absence\n",
                                (char *)cstring_get_str(&path_cstr), (char *)wait_val_tag);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to '%s' absence",
                    (char *)cstring_get_str(&path_cstr), (char *)wait_val_tag);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
#endif         
        }

        if(EC_FALSE == cstrkv_mgr_add_kv_str(cstrkv_mgr, wait_key, wait_val))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to add '%s:%s' failed\n",
                                (char *)cstring_get_str(&path_cstr), wait_key, wait_val);

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_INTERNAL_SERVER_ERROR);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed due to add '%s:%s' failed",
                    (char *)cstring_get_str(&path_cstr), wait_key, wait_val);
                             
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_INTERNAL_SERVER_ERROR;

            cstrkv_mgr_free(cstrkv_mgr);
            cstring_clean(&path_cstr);
            return (EC_TRUE);
        }
    }

    if(EC_FALSE == crfs_wait_http_headers(CSOCKET_CNODE_MODI(csocket_cnode), tcid, &path_cstr, cstrkv_mgr, &header_ready))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed\n",
                            (char *)cstring_get_str(&path_cstr));

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_wait_header_get_request: crfs wait %s failed", (char *)cstring_get_str(&path_cstr));
                         
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

        cstrkv_mgr_free(cstrkv_mgr);
        cstring_clean(&path_cstr);
        return (EC_TRUE);
    }
 
    dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s done\n",
                        (char *)cstring_get_str(&path_cstr));   

    CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
    CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
    CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_wait_header_get_request: crfs wait %s done", (char *)cstring_get_str(&path_cstr));

    CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

    cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"header-ready", c_bool_str(header_ready));

    cstrkv_mgr_free(cstrkv_mgr);
    cstring_clean(&path_cstr);

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_wait_header_get_response(CHTTP_NODE *chttp_node)
{
    if(do_log(SEC_0049_CRFSHTTP, 9))
    {
        CBUFFER       *uri_cbuffer;
        uint8_t       *cache_key;
        uint32_t       cache_len;

        uri_cbuffer    = CHTTP_NODE_URI(chttp_node);
        cache_key = CBUFFER_DATA(uri_cbuffer) + CONST_STR_LEN("/wait_header");
        cache_len = CBUFFER_USED(uri_cbuffer) - CONST_STR_LEN("/wait_header");
     
        sys_log(LOGSTDOUT, "[DEBUG] crfshttp_make_wait_header_get_response: path %.*s\n", cache_len, cache_key);
    }

    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_wait_header_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_wait_header_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_make_response_header_kvs(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_file_wait_get_response: make header kvs failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_wait_header_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_wait_header_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_wait_header_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: locked_file_retire ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_locked_file_retire_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/locked_file_retire") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/locked_file_retire")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_locked_file_retire(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_locked_file_retire: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_locked_file_retire_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_locked_file_retire_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_locked_file_retire_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_locked_file_retire_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_locked_file_retire_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_locked_file_retire_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_locked_file_retire_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_locked_file_retire_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_locked_file_retire_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_locked_file_retire_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_locked_file_retire_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_locked_file_retire_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_locked_file_retire_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_locked_file_retire_get_op(uri_cbuffer))
    {
        CSOCKET_CNODE * csocket_cnode;

        char    *retire_max_num_str;
        UINT32   retire_max_num;
        UINT32   retire_num;
     
        retire_max_num_str = chttp_node_get_header(chttp_node, (const char *)"retire-max-num");
        retire_max_num     = c_str_to_word(retire_max_num_str);
        retire_num         = 0;

        dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_locked_file_retire_get_request: header retire-max-num %s => %ld\n",
                                retire_max_num_str, retire_max_num);

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(EC_FALSE == crfs_locked_file_retire(CSOCKET_CNODE_MODI(csocket_cnode), retire_max_num, &retire_num))
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_locked_file_retire_get_request failed");
  
            return (EC_TRUE);
        }

        dbg_log(SEC_0049_CRFSHTTP, 5)(LOGSTDOUT, "[DEBUG] crfshttp_handle_locked_file_retire_get_request: complete %ld\n", retire_num);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_locked_file_retire_get_request: complete %ld", retire_num);

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        /*prepare response header*/
        cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (char *)"retire-completion", c_word_to_str(retire_num));
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_locked_file_retire_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_locked_file_retire_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_locked_file_retire_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_kvs(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_locked_file_retire_get_response: make header kvs failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_locked_file_retire_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_locked_file_retire_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_locked_file_retire_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: rfs_up ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_rfs_up_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/rfs_up") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/rfs_up")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_rfs_up(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_rfs_up: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_rfs_up_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_rfs_up_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_rfs_up_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_up_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_rfs_up_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_up_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_rfs_up_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_up_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_rfs_up_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_up_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_up_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_up_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_up_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_rfs_up_get_op(uri_cbuffer))
    {
        UINT32       crfsmon_id;
        char        *v;
        CRFS_NODE    crfs_node;

        crfsmon_id = task_brd_default_get_crfsmon_id();
        if(CMPI_ERROR_MODI == crfsmon_id)
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_IMPLEMENTED;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_NOT_IMPLEMENTED);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_up_get_request: no crfsmon start");

            return (EC_TRUE);
        }

        crfs_node_init(&crfs_node);

        CRFS_NODE_MODI(&crfs_node)   = 0; /*default*/
        CRFS_NODE_STATE(&crfs_node) = CRFS_NODE_IS_UP;/*useless*/
     
        v = chttp_node_get_header(chttp_node, (const char *)"rfs-tcid");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_up_get_request: invalid rfs-tcid '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_TCID(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_up_get_request: header rfs-tcid %s => 0x%lx\n",
                                v, CRFS_NODE_TCID(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-ip");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_up_get_request: invalid rfs-ip '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_IPADDR(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_up_get_request: header rfs-ip %s => 0x%lx\n",
                                v, CRFS_NODE_IPADDR(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-port");
        if(NULL_PTR != v)
        {
            CRFS_NODE_PORT(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_up_get_request: header rfs-port %s => %ld\n",
                                v, CRFS_NODE_PORT(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-modi");
        if(NULL_PTR != v)
        {
            CRFS_NODE_MODI(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_up_get_request: header rfs-modi %s => %ld\n",
                                v, CRFS_NODE_MODI(&crfs_node));
        }

        if(EC_FALSE == crfsmon_crfs_node_set_up(crfsmon_id, &crfs_node))
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_up_get_request: set up rfs %s failed", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

            crfs_node_clean(&crfs_node);
            return (EC_TRUE);
        }

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_up_get_request: set up rfs %s done", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        crfs_node_clean(&crfs_node);
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_rfs_up_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_up_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_up_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_up_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_rfs_up_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_up_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: rfs_down ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_rfs_down_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/rfs_down") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/rfs_down")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_rfs_down(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_rfs_down: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_rfs_down_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_rfs_down_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_rfs_down_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_down_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_rfs_down_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_down_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_rfs_down_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_down_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_rfs_down_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_down_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_down_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_down_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_down_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_rfs_down_get_op(uri_cbuffer))
    {
        UINT32       crfsmon_id;
        char        *v;
        CRFS_NODE    crfs_node;

        crfsmon_id = task_brd_default_get_crfsmon_id();
        if(CMPI_ERROR_MODI == crfsmon_id)
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_IMPLEMENTED;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_NOT_IMPLEMENTED);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_down_get_request: no crfsmon start");

            return (EC_TRUE);
        }
     
        crfs_node_init(&crfs_node);

        CRFS_NODE_MODI(&crfs_node)   = 0; /*default*/
        CRFS_NODE_STATE(&crfs_node) = CRFS_NODE_IS_DOWN;/*useless*/
     
        v = chttp_node_get_header(chttp_node, (const char *)"rfs-tcid");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_down_get_request: invalid rfs-tcid '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_TCID(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_down_get_request: header rfs-tcid %s => 0x%lx\n",
                                v, CRFS_NODE_TCID(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-ip");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_down_get_request: invalid rfs-ip '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_IPADDR(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_down_get_request: header rfs-ip %s => 0x%lx\n",
                                v, CRFS_NODE_IPADDR(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-port");
        if(NULL_PTR != v)
        {
            CRFS_NODE_PORT(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_down_get_request: header rfs-port %s => %ld\n",
                                v, CRFS_NODE_PORT(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-modi");
        if(NULL_PTR != v)
        {
            CRFS_NODE_MODI(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_down_get_request: header rfs-modi %s => %ld\n",
                                v, CRFS_NODE_MODI(&crfs_node));
        }

        if(EC_FALSE == crfsmon_crfs_node_set_down(crfsmon_id, &crfs_node))
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_down_get_request: set down rfs %s failed", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

            crfs_node_clean(&crfs_node);
            return (EC_TRUE);
        }

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_down_get_request: set down rfs %s done", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        crfs_node_clean(&crfs_node);
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_rfs_down_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_down_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_down_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_down_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_rfs_down_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_down_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: rfs_add ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_rfs_add_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/rfs_add") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/rfs_add")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_rfs_add(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_rfs_add: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_rfs_add_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_rfs_add_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_rfs_add_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_add_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_rfs_add_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_add_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_rfs_add_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_add_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_rfs_add_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_add_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_add_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_add_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_add_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_rfs_add_get_op(uri_cbuffer))
    {
        UINT32       crfsmon_id;
        char        *v;
        CRFS_NODE    crfs_node;

        crfsmon_id = task_brd_default_get_crfsmon_id();
        if(CMPI_ERROR_MODI == crfsmon_id)
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_IMPLEMENTED;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_NOT_IMPLEMENTED);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_add_get_request: no crfsmon start");

            return (EC_TRUE);
        }

        crfs_node_init(&crfs_node);

        CRFS_NODE_MODI(&crfs_node)   = 0; /*default*/
        CRFS_NODE_STATE(&crfs_node) = CRFS_NODE_IS_UP;/*useless*/
     
        v = chttp_node_get_header(chttp_node, (const char *)"rfs-tcid");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_add_get_request: invalid rfs-tcid '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_TCID(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_add_get_request: header rfs-tcid %s => 0x%lx\n",
                                v, CRFS_NODE_TCID(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-ip");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_add_get_request: invalid rfs-ip '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_IPADDR(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_add_get_request: header rfs-ip %s => 0x%lx\n",
                                v, CRFS_NODE_IPADDR(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-port");
        if(NULL_PTR != v)
        {
            CRFS_NODE_PORT(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_add_get_request: header rfs-port %s => %ld\n",
                                v, CRFS_NODE_PORT(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-modi");
        if(NULL_PTR != v)
        {
            CRFS_NODE_MODI(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_add_get_request: header rfs-modi %s => %ld\n",
                                v, CRFS_NODE_MODI(&crfs_node));
        }
    
        if(EC_FALSE == crfsmon_crfs_node_add(crfsmon_id, &crfs_node))
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_add_get_request: add rfs %s failed", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

            crfs_node_clean(&crfs_node);
            return (EC_TRUE);
        }

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_add_get_request: add rfs %s done", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        crfs_node_clean(&crfs_node);
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_rfs_add_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_add_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_add_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_add_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_rfs_add_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_add_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: rfs_del ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_rfs_del_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/rfs_del") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/rfs_del")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_rfs_del(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_rfs_del: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_rfs_del_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_rfs_del_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_rfs_del_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_del_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_rfs_del_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_del_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_rfs_del_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_del_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_rfs_del_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_del_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_del_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_del_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_del_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_rfs_del_get_op(uri_cbuffer))
    {
        UINT32       crfsmon_id;
        char        *v;
        CRFS_NODE    crfs_node;

        crfsmon_id = task_brd_default_get_crfsmon_id();
        if(CMPI_ERROR_MODI == crfsmon_id)
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_IMPLEMENTED;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_NOT_IMPLEMENTED);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_del_get_request: no crfsmon start");

            return (EC_TRUE);
        }
     
        crfs_node_init(&crfs_node);

        CRFS_NODE_MODI(&crfs_node)   = 0; /*default*/
     
        v = chttp_node_get_header(chttp_node, (const char *)"rfs-tcid");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_del_get_request: invalid rfs-tcid '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_TCID(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_del_get_request: header rfs-tcid %s => 0x%lx\n",
                                v, CRFS_NODE_TCID(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-ip");
        if(NULL_PTR != v)
        {
            if(EC_FALSE == c_ipv4_is_ok(v))
            {
                CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;

                CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
                CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_BAD_REQUEST);
                CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_del_get_request: invalid rfs-ip '%s'", v);

                crfs_node_clean(&crfs_node);
                return (EC_TRUE);         
            }
         
            CRFS_NODE_IPADDR(&crfs_node) = c_ipv4_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_del_get_request: header rfs-ip %s => 0x%lx\n",
                                v, CRFS_NODE_IPADDR(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-port");
        if(NULL_PTR != v)
        {
            CRFS_NODE_PORT(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_del_get_request: header rfs-port %s => %ld\n",
                                v, CRFS_NODE_PORT(&crfs_node));
        }

        v = chttp_node_get_header(chttp_node, (const char *)"rfs-modi");
        if(NULL_PTR != v)
        {
            CRFS_NODE_MODI(&crfs_node) = c_str_to_word(v);
            dbg_log(SEC_0049_CRFSHTTP, 1)(LOGSTDOUT, "[DEBUG] crfshttp_handle_rfs_del_get_request: header rfs-modi %s => %ld\n",
                                v, CRFS_NODE_MODI(&crfs_node));
        }

        if(EC_FALSE == crfsmon_crfs_node_del(crfsmon_id, &crfs_node))
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_FORBIDDEN;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_FORBIDDEN);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_del_get_request: del rfs %s failed", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

            crfs_node_clean(&crfs_node);
            return (EC_TRUE);
        }

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_del_get_request: del rfs %s done", c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)));

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;

        crfs_node_clean(&crfs_node);
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_rfs_del_get_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_del_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_del_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_del_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_rfs_del_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_del_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif

#if 1
/*---------------------------------------- HTTP METHOD: GET, FILE OPERATOR: rfs_list ----------------------------------------*/
static EC_BOOL __crfshttp_uri_is_rfs_list_get_op(const CBUFFER *uri_cbuffer)
{
    const uint8_t *uri_str;
    uint32_t       uri_len;
 
    uri_str      = CBUFFER_DATA(uri_cbuffer);
    uri_len      = CBUFFER_USED(uri_cbuffer);

    if(CONST_STR_LEN("/rfs_list") <= uri_len
    && EC_TRUE == c_memcmp(uri_str, CONST_UINT8_STR_AND_LEN("/rfs_list")))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL crfshttp_is_http_get_rfs_list(const CHTTP_NODE *chttp_node)
{
    const CBUFFER *uri_cbuffer;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0049_CRFSHTTP, 9)(LOGSTDOUT, "[DEBUG] crfshttp_is_http_get_rfs_list: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    if(EC_TRUE == __crfshttp_uri_is_rfs_list_get_op(uri_cbuffer))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

EC_BOOL crfshttp_commit_rfs_list_get_request(CHTTP_NODE *chttp_node)
{
    EC_BOOL ret;
 
    if(EC_FALSE == crfshttp_handle_rfs_list_get_request(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_list_get_request: handle 'GET' request failed\n");     
        return (EC_FALSE);
    }
 
    if(EC_FALSE == crfshttp_make_rfs_list_get_response(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_list_get_request: make 'GET' response failed\n");
        return (EC_FALSE);
    }

    ret = crfshttp_commit_rfs_list_get_response(chttp_node);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_list_get_request: commit 'GET' response failed\n");
        return (EC_FALSE);
    }
 
    return (ret);
}

EC_BOOL crfshttp_handle_rfs_list_get_request(CHTTP_NODE *chttp_node)
{
    CBUFFER       *uri_cbuffer;
     
    UINT32         req_body_chunk_num;
 
    CBYTES        *content_cbytes;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    req_body_chunk_num = chttp_node_recv_chunks_num(chttp_node);
    /*CRFSHTTP_ASSERT(0 == req_body_chunk_num);*/
    if(!(0 == req_body_chunk_num))
    {
        CHUNK_MGR *req_body_chunks;

        req_body_chunks = chttp_node_recv_chunks(chttp_node);
                                             
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_list_get_request: chunk num %ld\n", req_body_chunk_num);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_list_get_request: chunk mgr %p info\n", req_body_chunks);
        chunk_mgr_print_info(LOGSTDOUT, req_body_chunks);

        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error: crfshttp_handle_rfs_list_get_request: chunk mgr %p chars\n", req_body_chunks);
        chunk_mgr_print_chars(LOGSTDOUT, req_body_chunks);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFS_ERR %s %u --", "GET", CHTTP_BAD_REQUEST);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_list_get_request");
     
        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_BAD_REQUEST;
        return (EC_TRUE);
    }

    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    cbytes_clean(content_cbytes);

    if(EC_TRUE == __crfshttp_uri_is_rfs_list_get_op(uri_cbuffer))
    {
        UINT32       crfsmon_id;
        CSTRING      crfs_list_cstr;

        crfsmon_id = task_brd_default_get_crfsmon_id();
        if(CMPI_ERROR_MODI == crfsmon_id)
        {
            CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_NOT_IMPLEMENTED;

            CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
            CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_FAIL %s %u --", "GET", CHTTP_NOT_IMPLEMENTED);
            CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "error:crfshttp_handle_rfs_list_get_request: no crfsmon start");

            return (EC_TRUE);
        }

        cstring_init(&crfs_list_cstr, NULL_PTR);

        crfsmon_crfs_node_list(crfsmon_id, &crfs_list_cstr);

        cbytes_mount(content_cbytes, CSTRING_LEN(&crfs_list_cstr), CSTRING_STR(&crfs_list_cstr));
        cstring_unset(&crfs_list_cstr);

        CHTTP_NODE_LOG_TIME_WHEN_DONE(chttp_node);
        CHTTP_NODE_LOG_STAT_WHEN_DONE(chttp_node, "RFSMON_SUCC %s %u --", "GET", CHTTP_OK);
        CHTTP_NODE_LOG_INFO_WHEN_DONE(chttp_node, "[DEBUG] crfshttp_handle_rfs_list_get_request: list rfs done");

        CHTTP_NODE_RSP_STATUS(chttp_node) = CHTTP_OK;
    }

    return (EC_TRUE);
}

EC_BOOL crfshttp_make_rfs_list_get_response(CHTTP_NODE *chttp_node)
{
    CBYTES        *content_cbytes;
    uint64_t       content_len;
 
    content_cbytes = CHTTP_NODE_CONTENT_CBYTES(chttp_node);
    content_len    = CBYTES_LEN(content_cbytes);
 
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, content_len))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_list_get_response: make response header failed\n");
        return (EC_FALSE);
    }

    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        if(EC_FALSE == chttp_make_response_header_keepalive(chttp_node))
        {
            dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_list_get_response: make response header keepalive failed\n");
            return (EC_FALSE);
        }
    } 

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_list_get_response: make header end failed\n");
        return (EC_FALSE);
    }  

    /*no data copying but data transfering*/
    if(EC_FALSE == chttp_make_response_body_ext(chttp_node,
                                              CBYTES_BUF(content_cbytes),
                                              (uint32_t)CBYTES_LEN(content_cbytes)))
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_make_rfs_list_get_response: make body with len %d failed\n",
                           (uint32_t)CBYTES_LEN(content_cbytes));
        return (EC_FALSE);
    }
    cbytes_umount(content_cbytes, NULL_PTR, NULL_PTR); 

    return (EC_TRUE);
}

EC_BOOL crfshttp_commit_rfs_list_get_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0049_CRFSHTTP, 0)(LOGSTDOUT, "error:crfshttp_commit_rfs_list_get_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }

    return crfshttp_commit_response(chttp_node);
}
#endif


#ifdef __cplusplus
}
#endif/*__cplusplus*/

