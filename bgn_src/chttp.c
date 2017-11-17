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
#include "tasks.h"
#include "csocket.h"

#include "cmpie.h"

#include "cepoll.h"

#include "crfs.h"
#include "crfsmon.h"

#include "cbuffer.h"
#include "cstrkv.h"
#include "chunk.h"

#include "json.h"
#include "cbase64code.h"

#include "chttp.inc"
#include "chttp.h"
#include "cdns.h"
#include "coroutine.h"
#include "csrv.h"
#include "cconnp.h"
#include "super.h"

#include "findex.inc"


static const CHTTP_KV g_chttp_status_kvs[] = {
    { 100, "Continue" },
    { 101, "Switching Protocols" },
    { 102, "Processing" }, /* WebDAV */
    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 203, "Non-Authoritative Information" },
    { 204, "No Content" },
    { 205, "Reset Content" },
    { 206, "Partial Content" },
    { 207, "Multi-status" }, /* WebDAV */
    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },
    { 306, "(Unused)" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 407, "Proxy Authentication Required" },
    { 408, "Request Timeout" },
    { 409, "Conflict" },
    { 410, "Gone" },
    { 411, "Length Required" },
    { 412, "Precondition Failed" },
    { 413, "Request Entity Too Large" },
    { 414, "Request-URI Too Long" },
    { 415, "Unsupported Media Type" },
    { 416, "Requested Range Not Satisfiable" },
    { 417, "Expectation Failed" },
    { 422, "Unprocessable Entity" }, /* WebDAV */
    { 423, "Locked" }, /* WebDAV */
    { 424, "Failed Dependency" }, /* WebDAV */
    { 426, "Upgrade Required" }, /* TLS */
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Not Available" },
    { 504, "Gateway Timeout" },
    { 505, "HTTP Version Not Supported" },
    { 507, "Insufficient Storage" }, /* WebDAV */

    { -1, NULL }
};

static const uint32_t g_chttp_status_kvs_num = sizeof(g_chttp_status_kvs)/sizeof(g_chttp_status_kvs[0]);

static CQUEUE g_chttp_defer_request_queue;
static EC_BOOL g_chttp_defer_request_queue_init_flag = EC_FALSE;

static CLIST  *g_chttp_rest_list  = NULL_PTR;

static UINT32  g_chttp_store_seqno = 10000;

#define CHTTP_STORE_SEQ_NO_GEN(__chttp_store)     do{ CHTTP_STORE_SEQ_NO(__chttp_store) = ++ g_chttp_store_seqno;}while(0)
#define CHTTP_STORE_SEQ_NO_GET(__chttp_store)     (CHTTP_STORE_SEQ_NO(__chttp_store))

#if 1
#define CHTTP_ASSERT(condition) do{\
    if(!(condition)) {\
        sys_log(LOGSTDOUT, "error:assert failed at %s:%d\n", __FUNCTION__, __LINE__);\
        exit(EXIT_FAILURE);\
    }\
}while(0)
#endif

const char *chttp_status_str_get(const uint32_t http_status)
{
    uint32_t idx;

    for(idx = 0; idx < g_chttp_status_kvs_num; idx ++)
    {
        const CHTTP_KV *chttp_kv;
        chttp_kv = &(g_chttp_status_kvs[ idx ]);
        if(http_status == CHTTP_KV_KEY(chttp_kv))
        {
            return (CHTTP_KV_VAL(chttp_kv));
        }
    }
    return ((const char *)"unknown");
}
#if 0
static EC_BOOL __chttp_need_unix_domain_ipc(const UINT32 ipaddr, const UINT32 port)
{
    TASK_BRD *task_brd;
    TASKS_CFG *tasks_cfg;

    if(SWITCH_OFF == CSOCKET_UNIX_DOMAIN_SWITCH)
    {
        return (EC_FALSE);
    }

    if(c_ipv4_to_word((const char *)"127.0.0.1") != ipaddr)
    {
        return (EC_FALSE);
    }
 
    task_brd  = task_brd_default_get();
    tasks_cfg = sys_cfg_search_tasks_cfg_by_ip(TASK_BRD_SYS_CFG(task_brd), ipaddr, port);
    if(NULL_PTR == tasks_cfg)
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}
#endif
/*private interface, not for http parser*/
static EC_BOOL __chttp_on_recv_complete(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE *csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_recv_complete: chttp_node %p -> csocket_cnode is null\n", chttp_node);
        return (EC_FALSE);/*error*/
    } 
 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT,
            "[DEBUG] __chttp_on_recv_complete: sockfd %d, body parsed %ld\n",
            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_BODY_PARSED_LEN(chttp_node));

    CHTTP_NODE_LOG_TIME_WHEN_RCVD(chttp_node);/*record the received or parsed time*/

    if(CHTTP_TYPE_HANDLE_REQ == CHTTP_NODE_TYPE(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_recv_complete: socket %d, [type: HANDLE REQ]\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));
 
        /*note: http request is ready now. stop read from socket to prevent recving during handling request*/
        cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT);
 
        if(BIT_TRUE == CHTTP_NODE_HEADER_COMPLETE(chttp_node))
        {
            chttp_pause_parser(CHTTP_NODE_PARSER(chttp_node)); /*pause parser*/
         
            /*commit*/
            chttp_defer_request_queue_push(chttp_node);

            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_recv_complete: socket %d, [type: HANDLE REQ] commit request\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));

            CHTTP_NODE_RECV_COMPLETE(chttp_node) = BIT_TRUE;
            return (EC_TRUE);
        }
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_recv_complete: socket %d, [type: HANDLE REQ] header not completed\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_recv_complete: socket %d, [type: HANDLE RSP]\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));

        if(CSOCKET_CNODE_WORK_STATUS_ERR == CSOCKET_CNODE_WORK_STATUS(csocket_cnode))
        {
            /*socket does not come from connection pool, cleanup epoll*/
            cepoll_del_all(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode));
        }
        else
        {
            cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT);
        }
 
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0001);
        }

        if(BIT_TRUE == CHTTP_NODE_HEADER_COMPLETE(chttp_node))
        {
            CHTTP_NODE_RECV_COMPLETE(chttp_node) = BIT_TRUE;
        }
        return (EC_TRUE);
    }

    if(CHTTP_TYPE_HANDLE_CHECK == CHTTP_NODE_TYPE(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_recv_complete: socket %d, [type: HANDLE CHECK]\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));
                     
        if(CSOCKET_CNODE_WORK_STATUS_ERR == CSOCKET_CNODE_WORK_STATUS(csocket_cnode))
        {
            /*socket does not come from connection pool, cleanup epoll*/
            cepoll_del_all(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode));
        }
        else
        {
            cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT);
        }
     
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0002);
        }

        CHTTP_NODE_RECV_COMPLETE(chttp_node) = BIT_TRUE;
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_recv_complete: socket %d, [type: HANDLE: unknown 0x%lx]\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_TYPE(chttp_node));
                 
    cepoll_del_all(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode));
 
    return (EC_FALSE);
}

/*private interface, not for http parser*/
static EC_BOOL __chttp_on_send_complete(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE *csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_send_complete: chttp_node %p -> csocket_cnode is null\n", chttp_node);
        return (EC_FALSE);/*error*/
    } 
 
    /*note: http request is ready now. stop read from socket to prevent recving during handling request*/
    cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT);

    if(CHTTP_TYPE_HANDLE_CHECK == CHTTP_NODE_TYPE(chttp_node))
    {
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0003);
        }
    }

    CHTTP_NODE_SEND_COMPLETE(chttp_node) = BIT_TRUE;

    return (EC_TRUE);
}

/*---------------------------------------- HTTP PASER INTERFACE ----------------------------------------*/
static int __chttp_on_message_begin(http_parser_t* http_parser)
{
    CHTTP_NODE *chttp_node;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_message_begin: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_message_begin: chttp_node %p, ***MESSAGE BEGIN***\n",
                    chttp_node);
    return (0);
}

/**
*
* refer http parser case s_headers_almost_done
*
* if return 0, succ
* if return 1, SKIP BODY
* otherwise, error
*
**/
static int __chttp_on_headers_complete(http_parser_t* http_parser, const char* last, size_t length)
{
    CHTTP_NODE    *chttp_node;

    chttp_node = (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_headers_complete: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    chttp_parse_host(chttp_node);
    chttp_parse_uri(chttp_node);
    chttp_parse_content_length(chttp_node);
#if 1
    if(EC_FALSE == chttp_parse_connection_keepalive(chttp_node))
    {
        /*should never reach here due to csocket_cnode was checked before*/
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_headers_complete: chttp_node %p parse connection keepalive failed\n",
                        chttp_node);
        return (-1);/*error*/
    }
#else
    CHTTP_NODE_KEEPALIVE(chttp_node) = BIT_FALSE; /*force to disable keepalive*/
#endif 

    CHTTP_NODE_HEADER_COMPLETE(chttp_node) = BIT_TRUE;  /*header is ready*/
    CHTTP_NODE_HEADER_PARSED_LEN(chttp_node) += length; /*the last part of header*/

    if(do_log(SEC_0149_CHTTP, 9))
    {
        CSOCKET_CNODE *csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        sys_log(LOGSTDOUT, "[DEBUG] __chttp_on_headers_complete: socket %d, ***HEADERS COMPLETE***\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));
        chttp_node_print_header(LOGSTDOUT, chttp_node);
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node) && NULL_PTR != CHTTP_NODE_STORE(chttp_node))
    {
        chttp_node_adjust_seg_id(chttp_node);
        chttp_node_check_cacheable(chttp_node);

        chttp_node_filter_on_header_complete(chttp_node);

        /*if need to store recved data to storage and the starting seg is 0, i.e., header*/
        chttp_node_store_on_headers_complete(chttp_node);
    }

    return (0);/*succ*/
}

static int __chttp_on_message_complete(http_parser_t* http_parser)
{
    CHTTP_NODE    *chttp_node;
    CHTTP_STORE   *chttp_store;
    CSOCKET_CNODE *csocket_cnode;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_message_complete: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_message_complete: http_parser %p -> chttp_node %p -> csocket_cnode is null\n", http_parser, chttp_node);
        return (-1);/*error*/
    } 

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT,
            "[DEBUG] __chttp_on_message_complete: sockfd %d, http state %s, header pased %u, body parsed %"PRId64", errno = %d, name = %s, description = %s\n",
            CSOCKET_CNODE_SOCKFD(csocket_cnode), http_state_str(http_parser->state),
            CHTTP_NODE_HEADER_PARSED_LEN(chttp_node),CHTTP_NODE_BODY_PARSED_LEN(chttp_node),
            HTTP_PARSER_ERRNO(http_parser), http_errno_name(HTTP_PARSER_ERRNO(http_parser)), http_errno_description(HTTP_PARSER_ERRNO(http_parser)));

    chttp_store = CHTTP_NODE_STORE(chttp_node);

    if(do_log(SEC_0149_CHTTP, 9))
    {
        if(NULL_PTR != chttp_store)
        {
            CSOCKET_CNODE *csocket_cnode;

            csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node); 
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_message_complete: socket %d, seg_id %u, cache_ctrl: 0x%lx\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode),
                            CHTTP_STORE_SEG_ID(chttp_store), CHTTP_STORE_CACHE_CTRL(chttp_store));
        }
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node) && NULL_PTR != chttp_store)
    {
        /*after body data was appended to recv_chunks, store recv_chunks to storage*/
        /*if need to store recved data to storage and the starting seg is 0, i.e., header*/
        chttp_node_store_on_message_complete(chttp_node);

        if(EC_TRUE == chttp_node_is_chunked(chttp_node))
        {
            chttp_node_renew_content_length(chttp_node,  chttp_store, CHTTP_NODE_BODY_PARSED_LEN(chttp_node));
        }
    }

    __chttp_on_recv_complete(chttp_node);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_message_complete: socket %d, ***MESSAGE COMPLETE***\n", CSOCKET_CNODE_SOCKFD(csocket_cnode));
    return (0);
}

static int __chttp_on_url(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_NODE    *chttp_node;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_url: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    cbuffer_set(CHTTP_NODE_URL(chttp_node), (uint8_t *)at, length);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_url: chttp_node %p, url: %.*s\n",
                    chttp_node, (int)length, at);

    return (0);
}

/*only for http response*/
static int __chttp_on_status(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_NODE    *chttp_node;
    CSOCKET_CNODE *csocket_cnode;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_status: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_status: http_parser %p -> chttp_node %p -> csocket_cnode is null\n", http_parser, chttp_node);
        return (-1);/*error*/
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node))
    {
        CHTTP_NODE_STATUS_CODE(chttp_node) = http_parser->status_code;

        if(do_log(SEC_0149_CHTTP, 9))
        {
            UINT32 status_code;

            status_code = CHTTP_NODE_STATUS_CODE(chttp_node);
            sys_log(LOGSTDOUT, "[DEBUG] __chttp_on_status: socket %d, status: %u %.*s ==> %ld\n",
                                CSOCKET_CNODE_SOCKFD(csocket_cnode),
                                http_parser->status_code, (int)length, at,
                                status_code);     
        }
    }

    return (0);
}

static int __chttp_on_header_field(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_NODE    *chttp_node;
    CSTRKV *cstrkv;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_header_field: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    cstrkv = cstrkv_new(NULL_PTR, NULL_PTR);
    if(NULL_PTR == cstrkv)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_header_field: new cstrkv failed where header field: %.*s\n",
                           (int)length, at);
        return (-1);
    }

    cstrkv_set_key_bytes(cstrkv, (const uint8_t *)at, (uint32_t)length);
    cstrkv_mgr_add_kv(CHTTP_NODE_HEADER_IN_KVS(chttp_node), cstrkv);

    //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_header_field: chttp_node %p, Header field: '%.*s'\n", chttp_node, (int)length, at);
    return (0);
}

static int __chttp_on_header_value(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_NODE    *chttp_node;
    CSTRKV *cstrkv;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_header_value: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }
 
    cstrkv = cstrkv_mgr_last_kv(CHTTP_NODE_HEADER_IN_KVS(chttp_node));
    if(NULL_PTR == cstrkv)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_header_value: no cstrkv existing where value field: %.*s\n",
                           (int)length, at);
        return (-1);
    }

    cstrkv_set_val_bytes(cstrkv, (const uint8_t *)at, (uint32_t)length);
    //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_header_value: chttp_node %p, Header value: '%.*s'\n", chttp_node, (int)length, at);
#if 0
    if(do_log(SEC_0149_CHTTP, 9))
    {
        cstrkv_print(LOGSTDOUT, cstrkv);
    }
#endif
 
    return (0);
}

static int __chttp_on_body(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_NODE    *chttp_node;
    CHUNK_MGR     *recv_chunks;

    chttp_node= (CHTTP_NODE *)http_parser->data;
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_body: http_parser %p -> chttp_node is null\n", http_parser);
        return (-1);/*error*/
    }

    recv_chunks = CHTTP_NODE_RECV_BUF(chttp_node);

    if(EC_FALSE == chunk_mgr_append_data_min(recv_chunks, (uint8_t *)at, length, CHTTP_IN_BUF_SIZE))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_on_body: append %d bytes failed\n", length);
        return (-1);
    }
    CHTTP_NODE_BODY_PARSED_LEN(chttp_node) += length;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_on_body: chttp_node %p, len %d => body parsed %"PRId64"\n",
                    chttp_node, length, CHTTP_NODE_BODY_PARSED_LEN(chttp_node));

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node) && NULL_PTR != CHTTP_NODE_STORE(chttp_node))
    {
        /*after body data was appended to recv_chunks, store recv_chunks to storage*/
        chttp_node_store_on_body(chttp_node);
    }

    return (0);
}

/*---------------------------------------- INTERFACE WITH HTTP PASER  ----------------------------------------*/
static void __chttp_parser_init(http_parser_t   *http_parser, const UINT32 type)
{
    if(NULL_PTR != http_parser)
    {
        if(CHTTP_TYPE_HANDLE_REQ == type)
        {
            http_parser_init(http_parser, HTTP_REQUEST);
            return;
        }

        if(CHTTP_TYPE_HANDLE_RSP == type)
        {
            http_parser_init(http_parser, HTTP_RESPONSE);
            return;
        }

        if(CHTTP_TYPE_HANDLE_BOTH == type)
        {
            http_parser_init(http_parser, HTTP_BOTH);
            return;
        }
    }
    return;
}

static void __chttp_parser_setting_init(http_parser_settings_t   *http_parser_setting)
{
    if(NULL_PTR != http_parser_setting)
    {
        http_parser_setting->on_message_begin    = __chttp_on_message_begin;
        http_parser_setting->on_url              = __chttp_on_url;
        http_parser_setting->on_status           = __chttp_on_status;
        http_parser_setting->on_header_field     = __chttp_on_header_field;
        http_parser_setting->on_header_value     = __chttp_on_header_value;
        http_parser_setting->on_headers_complete = __chttp_on_headers_complete;
        http_parser_setting->on_body             = __chttp_on_body;
        http_parser_setting->on_message_complete = __chttp_on_message_complete;
    }

    return;
}

static void __chttp_parser_clean(http_parser_t   *http_parser)
{
    if(NULL_PTR != http_parser)
    {
        /** PRIVATE **/
        http_parser->type           = 0x03;   /*2 bits, invalid type, enum http_parser_type*/
        http_parser->flags          = 0x3F;   /*6 bits, invalid flag, enum flags*/
        http_parser->state          = s_undef;/*8 bits, invalid state, enum state*/
        http_parser->header_state   = 0xFF;   /*8 bits, invalid header_state, enum header_states*/
        http_parser->index          = 0xFF;   /*8 bits, invalid index*/
        http_parser->nread          = 0;
        http_parser->content_length = 0;

        /** READ-ONLY **/
        http_parser->http_major     = 0;
        http_parser->http_minor     = 0;
        http_parser->status_code    = 0;       /*16 bits, invalid status code, 1xx, 2xx,3xx,4xx,5xx, responses only*/
        http_parser->method         = 0xFF;    /*8 bits, invalid method, HTTP_METHOD_MAP, requests only*/
        http_parser->http_errno     = 0x7F;    /*7 bits, invalid errno, HTTP_ERRNO_MAP*/
        http_parser->upgrade        = 0;       /*1 bit*/

        /** PUBLIC **/
        http_parser->data           = NULL_PTR;
    }
    return;
}

static void __chttp_parser_setting_clean(http_parser_settings_t   *http_parser_setting)
{
    if(NULL_PTR != http_parser_setting)
    {
        http_parser_setting->on_message_begin    = NULL_PTR;
        http_parser_setting->on_url              = NULL_PTR;
        http_parser_setting->on_status           = NULL_PTR;
        http_parser_setting->on_header_field     = NULL_PTR;
        http_parser_setting->on_header_value     = NULL_PTR;
        http_parser_setting->on_headers_complete = NULL_PTR;
        http_parser_setting->on_body             = NULL_PTR;
        http_parser_setting->on_message_complete = NULL_PTR;
    }

    return;
}

/*---------------------------------------- INTERFACE WITH HTTP STORE  ----------------------------------------*/
CHTTP_STORE *chttp_store_new()
{
    CHTTP_STORE *chttp_store;

    alloc_static_mem(MM_CHTTP_STORE, &chttp_store, LOC_CHTTP_0004);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_new: new chttp_store failed\n");
        return (NULL_PTR);
    }

    if(EC_FALSE == chttp_store_init(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_new: init chttp_store failed\n");
        free_static_mem(MM_CHTTP_STORE, chttp_store, LOC_CHTTP_0005);
        return (NULL_PTR);
    }

    return (chttp_store);
}
EC_BOOL chttp_store_init(CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store)
    {
        CHTTP_STORE_SEG_ID(chttp_store)       = CHTTP_SEG_ERR_ID;
        CHTTP_STORE_SEG_SIZE(chttp_store)     = CHTTP_SEG_ERR_SIZE;
        CHTTP_STORE_SEG_S_OFFSET(chttp_store) = CHTTP_SEG_ERR_OFFSET;
        CHTTP_STORE_SEG_E_OFFSET(chttp_store) = CHTTP_SEG_ERR_OFFSET;
     
        cstring_init(CHTTP_STORE_BASEDIR(chttp_store), NULL_PTR);

        cstring_init(CHTTP_STORE_BILLING_FLAGS(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_BILLING_DOMAIN(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store), NULL_PTR);

        CHTTP_STORE_CACHE_CTRL(chttp_store)      = CHTTP_STORE_CACHE_ERR;
        CHTTP_STORE_MERGE_FLAG(chttp_store)      = EC_FALSE;
        CHTTP_STORE_LOCKED_FLAG(chttp_store)     = EC_FALSE;
        CHTTP_STORE_EXPIRED_FLAG(chttp_store)    = EC_FALSE;
        CHTTP_STORE_CHUNK_FLAG(chttp_store)      = EC_FALSE;
        cstring_init(CHTTP_STORE_AUTH_TOKEN(chttp_store), NULL_PTR);

        CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store) = EC_TRUE;
        cstring_init(CHTTP_STORE_ETAG(chttp_store), NULL_PTR);
        CHTTP_STORE_LAST_MODIFIED(chttp_store)  = CHTTP_STORE_ERR_LAST_MODIFIED;
        CHTTP_STORE_CONTENT_LENGTH(chttp_store) = 0;
        CHTTP_STORE_USE_GZIP_FLAG(chttp_store)  = EC_OBSCURE;

        CHTTP_STORE_CACHE_ALLOW(chttp_store)      = EC_FALSE;
        CHTTP_STORE_CACHE_SWITCH(chttp_store)     = SWITCH_ON;
        cstring_init(CHTTP_STORE_CACHE_HTTP_CODES(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store), NULL_PTR);
        cstring_init(CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store), NULL_PTR);

        CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store) = EC_FALSE;
        CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store) = 0;

        CHTTP_STORE_REDIRECT_CTRL(chttp_store)      = EC_TRUE;
        CHTTP_STORE_REDIRECT_MAX_TIMES(chttp_store) = 0;
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_store_clean(CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store)
    {
        CHTTP_STORE_SEG_ID(chttp_store)       = CHTTP_SEG_ERR_ID;
        CHTTP_STORE_SEG_SIZE(chttp_store)     = CHTTP_SEG_ERR_SIZE;
        CHTTP_STORE_SEG_S_OFFSET(chttp_store) = CHTTP_SEG_ERR_OFFSET;
        CHTTP_STORE_SEG_E_OFFSET(chttp_store) = CHTTP_SEG_ERR_OFFSET;
     
        cstring_clean(CHTTP_STORE_BASEDIR(chttp_store));

        cstring_clean(CHTTP_STORE_BILLING_FLAGS(chttp_store));
        cstring_clean(CHTTP_STORE_BILLING_DOMAIN(chttp_store));
        cstring_clean(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store));

        CHTTP_STORE_CACHE_CTRL(chttp_store)      = CHTTP_STORE_CACHE_ERR;
        CHTTP_STORE_MERGE_FLAG(chttp_store)      = EC_FALSE;
        CHTTP_STORE_LOCKED_FLAG(chttp_store)     = EC_FALSE;
        CHTTP_STORE_EXPIRED_FLAG(chttp_store)    = EC_FALSE;
        CHTTP_STORE_CHUNK_FLAG(chttp_store)      = EC_FALSE;
        cstring_clean(CHTTP_STORE_AUTH_TOKEN(chttp_store));

        CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store) = EC_TRUE;
        cstring_clean(CHTTP_STORE_ETAG(chttp_store));
        CHTTP_STORE_LAST_MODIFIED(chttp_store)  = CHTTP_STORE_ERR_LAST_MODIFIED;
        CHTTP_STORE_CONTENT_LENGTH(chttp_store) = 0;
        CHTTP_STORE_USE_GZIP_FLAG(chttp_store)  = EC_OBSCURE;

        CHTTP_STORE_CACHE_ALLOW(chttp_store)      = EC_FALSE;
        CHTTP_STORE_CACHE_SWITCH(chttp_store)     = SWITCH_ON;
        cstring_clean(CHTTP_STORE_CACHE_HTTP_CODES(chttp_store));
        cstring_clean(CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store));
        cstring_clean(CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store));
        cstring_clean(CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store));
        cstring_clean(CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store));

        CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store) = EC_FALSE;
        CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store) = 0;

        CHTTP_STORE_REDIRECT_CTRL(chttp_store)      = EC_TRUE;
        CHTTP_STORE_REDIRECT_MAX_TIMES(chttp_store) = 0;
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_store_free(CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store)
    {
        chttp_store_clean(chttp_store);
        free_static_mem(MM_CHTTP_STORE, chttp_store, LOC_CHTTP_0006);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_store_clone(const CHTTP_STORE *chttp_store_src, CHTTP_STORE *chttp_store_des)
{
    if(NULL_PTR != chttp_store_des)
    {
        CHTTP_STORE_SEG_ID(chttp_store_des)       = CHTTP_STORE_SEG_ID(chttp_store_src);
        CHTTP_STORE_SEG_SIZE(chttp_store_des)     = CHTTP_STORE_SEG_SIZE(chttp_store_src);
        CHTTP_STORE_SEG_S_OFFSET(chttp_store_des) = CHTTP_STORE_SEG_S_OFFSET(chttp_store_src);
        CHTTP_STORE_SEG_E_OFFSET(chttp_store_des) = CHTTP_STORE_SEG_E_OFFSET(chttp_store_src);
     
        cstring_clone(CHTTP_STORE_BASEDIR(chttp_store_src), CHTTP_STORE_BASEDIR(chttp_store_des));
     
        cstring_clone(CHTTP_STORE_BILLING_FLAGS(chttp_store_src), CHTTP_STORE_BILLING_FLAGS(chttp_store_des));
        cstring_clone(CHTTP_STORE_BILLING_DOMAIN(chttp_store_src), CHTTP_STORE_BILLING_DOMAIN(chttp_store_des));
        cstring_clone(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store_src), CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store_des));

        CHTTP_STORE_CACHE_CTRL(chttp_store_des)   = CHTTP_STORE_CACHE_CTRL(chttp_store_src);
        CHTTP_STORE_MERGE_FLAG(chttp_store_des)   = CHTTP_STORE_MERGE_FLAG(chttp_store_src);
        CHTTP_STORE_LOCKED_FLAG(chttp_store_des)  = CHTTP_STORE_LOCKED_FLAG(chttp_store_src);
        CHTTP_STORE_EXPIRED_FLAG(chttp_store_des) = CHTTP_STORE_EXPIRED_FLAG(chttp_store_src);
        CHTTP_STORE_CHUNK_FLAG(chttp_store_des)   = CHTTP_STORE_CHUNK_FLAG(chttp_store_src);
        cstring_clone(CHTTP_STORE_AUTH_TOKEN(chttp_store_src), CHTTP_STORE_AUTH_TOKEN(chttp_store_des));

        CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store_des) = CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store_src);
        cstring_clone(CHTTP_STORE_ETAG(chttp_store_src), CHTTP_STORE_ETAG(chttp_store_des));
        CHTTP_STORE_LAST_MODIFIED(chttp_store_des)  = CHTTP_STORE_LAST_MODIFIED(chttp_store_src);
        CHTTP_STORE_CONTENT_LENGTH(chttp_store_des) = CHTTP_STORE_CONTENT_LENGTH(chttp_store_src);
        CHTTP_STORE_USE_GZIP_FLAG(chttp_store_des)  = CHTTP_STORE_USE_GZIP_FLAG(chttp_store_src);

        CHTTP_STORE_CACHE_ALLOW(chttp_store_des)  = CHTTP_STORE_CACHE_ALLOW(chttp_store_src);
        CHTTP_STORE_CACHE_SWITCH(chttp_store_des) = CHTTP_STORE_CACHE_SWITCH(chttp_store_src);
        cstring_clone(CHTTP_STORE_CACHE_HTTP_CODES(chttp_store_src), CHTTP_STORE_CACHE_HTTP_CODES(chttp_store_des));
        cstring_clone(CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store_src), CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store_des));
        cstring_clone(CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store_src), CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store_des));
        cstring_clone(CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store_src), CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store_des));
        cstring_clone(CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store_src), CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store_des));

        CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store_des) = CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store_src);
        CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store_des) = CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store_src);

        CHTTP_STORE_REDIRECT_CTRL(chttp_store_des)      =  CHTTP_STORE_REDIRECT_CTRL(chttp_store_src);
        CHTTP_STORE_REDIRECT_MAX_TIMES(chttp_store_des) = CHTTP_STORE_REDIRECT_MAX_TIMES(chttp_store_src);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_store_check(const CHTTP_STORE *chttp_store)
{
    if(CHTTP_SEG_ERR_ID == CHTTP_STORE_SEG_ID(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_check: invalid seg_id\n");
        return (EC_FALSE);
    }

    if(CHTTP_SEG_ERR_ID == CHTTP_STORE_SEG_SIZE(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_check: invalid seg_size\n");
        return (EC_FALSE);
    }

    if(EC_TRUE == cstring_is_empty(CHTTP_STORE_BASEDIR(chttp_store)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_check: basedir is null\n");
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_store_srv_get(const CHTTP_STORE *chttp_store, const CSTRING *path, UINT32 *tcid, UINT32 *srv_ipaddr, UINT32 *srv_port)
{
    UINT32      crfsmon_md_id;
    CRFS_NODE   crfs_node;
    UINT32      hash;

    hash = c_crc32_short(CSTRING_STR(path), (size_t)CSTRING_LEN(path));

    crfsmon_md_id = task_brd_default_get_crfsmon_id();

    crfs_node_init(&crfs_node);
    if(EC_FALSE == crfsmon_crfs_node_get_by_hash(crfsmon_md_id, hash, &crfs_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_srv_get: get crfs_node with crfsmon_md_id %ld and hash %ld failed\n",
                    crfsmon_md_id, hash);
                 
        crfs_node_clean(&crfs_node);
        return (EC_FALSE);
    }

    if(EC_FALSE == crfs_node_is_up(&crfs_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_srv_get: crfs_node (tcid %s, srv %s:%ld, modi %ld, state %s) is not up\n",
                    c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node)),
                    c_word_to_ipv4(CRFS_NODE_IPADDR(&crfs_node)), CRFS_NODE_PORT(&crfs_node),
                    CRFS_NODE_MODI(&crfs_node),
                    crfs_node_state(&crfs_node));
 
        crfs_node_clean(&crfs_node);
        return (EC_FALSE);
    }

    if(NULL_PTR != tcid)
    {
        (*tcid) = CRFS_NODE_TCID(&crfs_node);
    }

    if(NULL_PTR != srv_ipaddr)
    {
        (*srv_ipaddr) = CRFS_NODE_IPADDR(&crfs_node);
    }

    if(NULL_PTR != srv_port)
    {
        (*srv_port) = CRFS_NODE_PORT(&crfs_node);
    }

    crfs_node_clean(&crfs_node);

    return (EC_TRUE);
}

EC_BOOL chttp_store_path_get(const CHTTP_STORE *chttp_store, CSTRING *path)
{
    cstring_append_cstr(path, CHTTP_STORE_BASEDIR(chttp_store));
    cstring_rtrim(path, (uint8_t)'/'); /*discard the redundant seprator for safe reason*/
    cstring_append_char(path, (uint8_t)'/');
    cstring_append_str(path, (uint8_t *)c_word_to_str(CHTTP_STORE_SEG_ID(chttp_store)));
    return (EC_TRUE);
}

void chttp_store_print(LOG *log, const CHTTP_STORE *chttp_store)
{
    sys_log(LOGSTDOUT, "chttp_store_print:seg_id                 : %u\n", CHTTP_STORE_SEG_ID(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:seg_size               : %u\n", CHTTP_STORE_SEG_SIZE(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:seg_s_offset           : %u\n", CHTTP_STORE_SEG_S_OFFSET(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:seg_e_offset           : %u\n", CHTTP_STORE_SEG_E_OFFSET(chttp_store));
 
    sys_log(LOGSTDOUT, "chttp_store_print:basedir                : %.*s\n", CHTTP_STORE_BASEDIR_LEN(chttp_store), CHTTP_STORE_BASEDIR_STR(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:billing flags          : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_BILLING_FLAGS(chttp_store)), CSTRING_STR(CHTTP_STORE_BILLING_FLAGS(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:billing domain         : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_BILLING_DOMAIN(chttp_store)), CSTRING_STR(CHTTP_STORE_BILLING_DOMAIN(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:billing client type    : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store)), CSTRING_STR(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store)));

    sys_log(LOGSTDOUT, "chttp_store_print:cache_ctrl             : 0x%lx\n", CHTTP_STORE_CACHE_CTRL(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:merge_flag             : %s\n", c_bool_str(CHTTP_STORE_MERGE_FLAG(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:locked_flag            : %s\n", c_bool_str(CHTTP_STORE_LOCKED_FLAG(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:expired_flag           : %s\n", c_bool_str(CHTTP_STORE_EXPIRED_FLAG(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:chunk_flag             : %s\n", c_bool_str(CHTTP_STORE_CHUNK_FLAG(chttp_store)));

    sys_log(LOGSTDOUT, "chttp_store_print:auth_token             : %.*s\n", CHTTP_STORE_AUTH_TOKEN_LEN(chttp_store), CHTTP_STORE_AUTH_TOKEN_STR(chttp_store));

    sys_log(LOGSTDOUT, "chttp_store_print:last_modified_switch   : %s\n"       , c_bool_str(CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:etag                   : %.*s\n"     , CHTTP_STORE_ETAG_LEN(chttp_store), CHTTP_STORE_ETAG_STR(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:last_modified          : %"PRId64"\n", CHTTP_STORE_LAST_MODIFIED(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:content_length         : %"PRId64"\n", CHTTP_STORE_CONTENT_LENGTH(chttp_store));
    sys_log(LOGSTDOUT, "chttp_store_print:use_gzip_flag          : %ld\n"      , CHTTP_STORE_USE_GZIP_FLAG(chttp_store));

    sys_log(LOGSTDOUT, "chttp_store_print:cache_allow            : %s\n", c_bool_str(CHTTP_STORE_CACHE_ALLOW(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:cache_flag             : %s\n", c_switch_to_str(CHTTP_STORE_CACHE_SWITCH(chttp_store)));

    sys_log(LOGSTDOUT, "chttp_store_print:cache_http_codes       : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_CACHE_HTTP_CODES(chttp_store)), CSTRING_STR(CHTTP_STORE_CACHE_HTTP_CODES(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:cache_rsp_headers      : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store)), CSTRING_STR(CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:ncache_rsp_headers     : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store)), CSTRING_STR(CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:cache_if_http_codes    : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store)), CSTRING_STR(CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:ncache_if_http_codes   : %.*s\n", (uint32_t)CSTRING_LEN(CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store)), CSTRING_STR(CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store)));

    sys_log(LOGSTDOUT, "chttp_store_print:override_expires_flag  : %s\n", c_bool_str(CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:override_expires_nsec  : %u\n", CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store));

    sys_log(LOGSTDOUT, "chttp_store_print:redirect_ctrl          : %s\n", c_bool_str(CHTTP_STORE_REDIRECT_CTRL(chttp_store)));
    sys_log(LOGSTDOUT, "chttp_store_print:redirect_max_times     : %u\n", (uint32_t)CHTTP_STORE_REDIRECT_MAX_TIMES(chttp_store));

 
    return;
}

/*ref: cache_http_code*/
EC_BOOL chttp_store_has_status_code(CHTTP_STORE *chttp_store, const uint32_t status_code)
{
    CSTRING  *cache_http_codes_cstr;
    char     *cache_http_codes_str;
 
    char     *cache_http_codes[ 32 ];
    UINT32    num;
    UINT32    pos;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: enter\n");

    if(CHTTP_PARTIAL_CONTENT == status_code)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: status_code is %u => true\n", CHTTP_PARTIAL_CONTENT);
        return (EC_TRUE);
    }
 
    cache_http_codes_cstr = CHTTP_STORE_CACHE_HTTP_CODES(chttp_store);
    if(EC_TRUE == cstring_is_empty(cache_http_codes_cstr))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: no http_cache_code => false\n");
        return (EC_FALSE);
    }

    cache_http_codes_str = c_str_dup((char *)CSTRING_STR(cache_http_codes_cstr));
    if(NULL_PTR == cache_http_codes_str)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_status_code: dup str '%.*s' failed\n",
                    (uint32_t)CSTRING_LEN(cache_http_codes_cstr), (char *)CSTRING_STR(cache_http_codes_cstr));
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: check %u in '%s'\n", status_code, cache_http_codes_str);

    num = sizeof(cache_http_codes)/sizeof(cache_http_codes[0]);
    num = c_str_split(cache_http_codes_str, (const char *)" ", (char **)cache_http_codes, num);
    for(pos = 0; pos < num; pos ++)
    {
        char *cache_http_code;

        cache_http_code = cache_http_codes[ pos ];
        if(c_str_to_uint32(cache_http_code) == status_code)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: %u in '%s' => true\n",
                                status_code, (char *)CSTRING_STR(cache_http_codes_cstr));
            c_str_free(cache_http_codes_str);
            return (EC_TRUE);
        }
    }

    c_str_free(cache_http_codes_str);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: leave => false\n");
    return (EC_FALSE);
}

/*ref: cache_if_http_code*/
EC_BOOL chttp_store_has_cache_status_code(CHTTP_STORE *chttp_store, const uint32_t status_code, uint32_t *expires)
{
    CSTRING  *cache_if_http_codes_cstr;
    char     *cache_if_http_codes_str;
 
    char     *cache_if_http_codes[ 32 ];
    UINT32    num;
    UINT32    pos;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: enter\n");
#if 0
    if(CHTTP_PARTIAL_CONTENT == status_code)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: status_code is %u => true\n", CHTTP_PARTIAL_CONTENT);
        return (EC_TRUE);
    }
#endif 
    cache_if_http_codes_cstr = CHTTP_STORE_CACHE_IF_HTTP_CODES(chttp_store);
    if(EC_TRUE == cstring_is_empty(cache_if_http_codes_cstr))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: no http_cache_code => false\n");
        return (EC_FALSE);
    }

    cache_if_http_codes_str = c_str_dup((char *)CSTRING_STR(cache_if_http_codes_cstr));
    if(NULL_PTR == cache_if_http_codes_str)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_cache_status_code: dup str '%.*s' failed\n",
                    (uint32_t)CSTRING_LEN(cache_if_http_codes_cstr), (char *)CSTRING_STR(cache_if_http_codes_cstr));
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: check %u in '%s'\n", status_code, cache_if_http_codes_str);

    num = sizeof(cache_if_http_codes)/sizeof(cache_if_http_codes[0]);
    num = c_str_split(cache_if_http_codes_str, (const char *)";", (char **)cache_if_http_codes, num);
    for(pos = 0; pos < num; pos ++)
    {
        char   *cache_if_http_code;
        char   *kv[ 2 ];
        UINT32  n;

        cache_if_http_code = cache_if_http_codes[ pos ];
        n = c_str_split(cache_if_http_code, (const char *)"=", (char **)kv, 2);
        if(0 == n || 2 < n)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_cache_status_code: invalid '%.*s'\n",
                    (uint32_t)CSTRING_LEN(cache_if_http_codes_cstr), (char *)CSTRING_STR(cache_if_http_codes_cstr));
            c_str_free(cache_if_http_codes_str);
            return (EC_FALSE);
        }
     
        if(c_str_to_uint32(kv[ 0 ]) == status_code)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: %u in '%s' => true\n",
                                status_code, (char *)CSTRING_STR(cache_if_http_codes_cstr));

            if(2 == n && NULL_PTR != expires)
            {
                (*expires) = c_str_to_uint32(kv[ 1 ]);
            }
         
            c_str_free(cache_if_http_codes_str);
            return (EC_TRUE);
        }
    }

    c_str_free(cache_if_http_codes_str);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_status_code: leave => false\n");
    return (EC_FALSE);
}

/*ref: cache_if_http_code*/
EC_BOOL chttp_store_has_not_cache_status_code(CHTTP_STORE *chttp_store, const uint32_t status_code)
{
    CSTRING  *not_cache_if_http_codes_cstr;
    char     *not_cache_if_http_codes_str;
 
    char     *not_cache_if_http_codes[ 32 ];
    UINT32    num;
    UINT32    pos;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: enter\n");
#if 0
    if(CHTTP_PARTIAL_CONTENT == status_code)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: status_code is %u => true\n", CHTTP_PARTIAL_CONTENT);
        return (EC_TRUE);
    }
#endif 
    not_cache_if_http_codes_cstr = CHTTP_STORE_NCACHE_IF_HTTP_CODES(chttp_store);
    if(EC_TRUE == cstring_is_empty(not_cache_if_http_codes_cstr))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: no http_cache_code => false\n");
        return (EC_FALSE);
    }

    not_cache_if_http_codes_str = c_str_dup((char *)CSTRING_STR(not_cache_if_http_codes_cstr));
    if(NULL_PTR == not_cache_if_http_codes_str)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_not_cache_status_code: dup str '%.*s' failed\n",
                    (uint32_t)CSTRING_LEN(not_cache_if_http_codes_cstr), (char *)CSTRING_STR(not_cache_if_http_codes_cstr));
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: check %u in '%s'\n", status_code, not_cache_if_http_codes_str);

    num = sizeof(not_cache_if_http_codes)/sizeof(not_cache_if_http_codes[0]);
    num = c_str_split(not_cache_if_http_codes_str, (const char *)"; ", (char **)not_cache_if_http_codes, num);/*add space seperator. Jun 14,2017*/
    for(pos = 0; pos < num; pos ++)
    {
        char   *not_cache_if_http_code;

        not_cache_if_http_code = not_cache_if_http_codes[ pos ];
     
        if(c_str_to_uint32(not_cache_if_http_code) == status_code)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: %u in '%s' => true\n",
                                status_code, (char *)CSTRING_STR(not_cache_if_http_codes_cstr));
         
            c_str_free(not_cache_if_http_codes_str);
            return (EC_TRUE);
        }
    }

    c_str_free(not_cache_if_http_codes_str);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_status_code: leave => false\n");
    return (EC_FALSE);
}

/*ref: cache_if_reply_header*/
EC_BOOL chttp_store_has_cache_rsp_headers(CHTTP_STORE *chttp_store, const CSTRKV_MGR *rsp_headers)
{
    CSTRING   *cache_rsp_headers_cstr;
    char      *cache_rsp_headers_str;
    char      *cache_rsp_headers[ 32 ];
    UINT32     cache_rsp_headers_num;
    UINT32     cache_rsp_headers_pos;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: enter\n");
    cache_rsp_headers_cstr = CHTTP_STORE_CACHE_RSP_HEADERS(chttp_store);
    if(EC_TRUE == cstring_is_empty(cache_rsp_headers_cstr))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: no cache_if_reply_header => true\n");
        return (EC_TRUE);/*if not given*/
    }
 
    cache_rsp_headers_str = c_str_dup((char *)CSTRING_STR(cache_rsp_headers_cstr));
    if(NULL_PTR == cache_rsp_headers_str)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_cache_rsp_headers: dup str '%.*s' failed\n",
                    (uint32_t)CSTRING_LEN(cache_rsp_headers_cstr), (char *)CSTRING_STR(cache_rsp_headers_cstr));
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_status_code: check cache_if_reply_header '%s'\n", cache_rsp_headers_str);

    cache_rsp_headers_num = sizeof(cache_rsp_headers)/sizeof(cache_rsp_headers[0]);
    cache_rsp_headers_num = c_str_split(cache_rsp_headers_str, (const char *)";", (char **)cache_rsp_headers, cache_rsp_headers_num);
    for(cache_rsp_headers_pos = 0; cache_rsp_headers_pos < cache_rsp_headers_num; cache_rsp_headers_pos ++)
    {
        char   *cache_rsp_header;
        char   *kv[ 2 ];
        UINT32  num;

        kv[0] = NULL_PTR;
        kv[1] = NULL_PTR;
        cache_rsp_header = cache_rsp_headers[ cache_rsp_headers_pos ];
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: cache_rsp_header '%s' ==> \n",
                            cache_rsp_header);

        num = c_str_split(cache_rsp_header, (const char *)"=", (char * *)kv, 2);
        if(0 == num)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_cache_rsp_headers: invalid '%.*s' => false\n",
                        (uint32_t)CSTRING_LEN(cache_rsp_headers_cstr), (char *)CSTRING_STR(cache_rsp_headers_cstr));

            c_str_free(cache_rsp_headers_str);
            return (EC_FALSE);                     
        }

        if(NULL_PTR != kv[0])
        {
            /*for safe reason, trim space of the key*/
            c_str_trim(kv[0], ' ');     
        }

        if(NULL_PTR != kv[1])
        {
            /*trim space*/
            c_str_trim(kv[1], ' ');
            if('\'' != c_str_first_char(kv[1]) || '\'' != c_str_last_char(kv[1]))
            {
                /*remove all spaces in the string*/
                c_str_del(kv[1], ' ');
            }
            else
            {
                /*trim (')*/
                c_str_del(kv[1], '\'');
            }
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: cache_rsp_header '%s' ==> '%s':'%s'\n",
                            cache_rsp_header, kv[0], kv[1]);
     
        if(EC_TRUE == cstrkv_mgr_exist_kv_ignore_case(rsp_headers, kv[0], kv[1]))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: '%s':'%s' [match] => true\n",
                                kv[0], kv[1]);
            c_str_free(cache_rsp_headers_str);
            return (EC_TRUE);
        }
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: '%s':'%s' [mismatch]\n",
                    kv[0], kv[1]);
    }

    c_str_free(cache_rsp_headers_str);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_cache_rsp_headers: leave => false\n");
    return (EC_FALSE);
}

/*ref: no_cache_if_reply_header*/
EC_BOOL chttp_store_has_not_cache_rsp_headers(CHTTP_STORE *chttp_store, const CSTRKV_MGR *rsp_headers)
{
    CSTRING   *not_cache_rsp_headers_cstr;
    char      *not_cache_rsp_headers_str;
    char      *not_cache_rsp_headers[ 32 ];
    UINT32     not_cache_rsp_headers_num;
    UINT32     not_cache_rsp_headers_pos;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: enter\n");
 
    not_cache_rsp_headers_cstr = CHTTP_STORE_NCACHE_RSP_HEADERS(chttp_store);
    if(EC_TRUE == cstring_is_empty(not_cache_rsp_headers_cstr))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: no no_cache_if_reply_header => false\n");
        return (EC_FALSE);
    }
 
    not_cache_rsp_headers_str = c_str_dup((char *)CSTRING_STR(not_cache_rsp_headers_cstr));
    if(NULL_PTR == not_cache_rsp_headers_str)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_not_cache_rsp_headers: dup str '%.*s' failed\n",
                    (uint32_t)CSTRING_LEN(not_cache_rsp_headers_cstr), (char *)CSTRING_STR(not_cache_rsp_headers_cstr));
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: check cache_if_reply_header '%s'\n", not_cache_rsp_headers_str);

    not_cache_rsp_headers_num = sizeof(not_cache_rsp_headers)/sizeof(not_cache_rsp_headers[0]);
    not_cache_rsp_headers_num = c_str_split(not_cache_rsp_headers_str, (const char *)";", (char **)not_cache_rsp_headers, not_cache_rsp_headers_num);
    for(not_cache_rsp_headers_pos = 0; not_cache_rsp_headers_pos < not_cache_rsp_headers_num; not_cache_rsp_headers_pos ++)
    {
        char   *not_cache_rsp_header;
        char   *kv[ 2 ];
        UINT32  num;

        kv[0] = NULL_PTR;
        kv[1] = NULL_PTR;
        not_cache_rsp_header = not_cache_rsp_headers[ not_cache_rsp_headers_pos ];

        num = c_str_split(not_cache_rsp_header, (const char *)"=", (char * *)kv, 2);
        if(0 == num)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_store_has_not_cache_rsp_headers: invalid '%.*s'\n",
                        (uint32_t)CSTRING_LEN(not_cache_rsp_headers_cstr), (char *)CSTRING_STR(not_cache_rsp_headers_cstr));

            c_str_free(not_cache_rsp_headers_str);
            return (EC_FALSE);                     
        }

        if(NULL_PTR != kv[0])
        {
            /*for safe reason, trim space of the key*/
            c_str_trim(kv[0], ' ');     
        }

        if(NULL_PTR != kv[1])
        {
            /*trim space*/
            c_str_trim(kv[1], ' ');
            if('\'' != c_str_first_char(kv[1]) || '\'' != c_str_last_char(kv[1]))
            {
                /*remove all spaces in the string*/
                c_str_del(kv[1], ' ');
            }
            else
            {
                /*trim (')*/
                c_str_del(kv[1], '\'');
            }
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: not_cache_rsp_header '%s' ==> '%s':'%s'\n",
                            not_cache_rsp_header, kv[0], kv[1]);
     
        if(EC_TRUE == cstrkv_mgr_exist_kv_ignore_case(rsp_headers, kv[0], kv[1]))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: found '%s':'%s' [match] => true\n",
                                kv[0], kv[1]);
            c_str_free(not_cache_rsp_headers_str);
            return (EC_TRUE);
        }
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: not found '%s':'%s' [mismatch]\n",
                    kv[0], kv[1]);
    }

    c_str_free(not_cache_rsp_headers_str);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_store_has_not_cache_rsp_headers: leave => false\n");
    return (EC_FALSE);
}

/*---------------------------------------- INTERFACE WITH HTTP STAT  ----------------------------------------*/
CHTTP_STAT *chttp_stat_new()
{
    CHTTP_STAT *chttp_stat;

    alloc_static_mem(MM_CHTTP_STAT, &chttp_stat, LOC_CHTTP_0007);
    if(NULL_PTR == chttp_stat)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_stat_new: new chttp_stat failed\n");
        return (NULL_PTR);
    }

    if(EC_FALSE == chttp_stat_init(chttp_stat))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_stat_new: init chttp_stat failed\n");
        free_static_mem(MM_CHTTP_STAT, chttp_stat, LOC_CHTTP_0008);
        return (NULL_PTR);
    }

    return (chttp_stat);
}
EC_BOOL chttp_stat_init(CHTTP_STAT *chttp_stat)
{
    if(NULL_PTR != chttp_stat)
    {
        BSET(chttp_stat, 0x00, sizeof(CHTTP_STAT));
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_stat_clean(CHTTP_STAT *chttp_stat)
{
    if(NULL_PTR != chttp_stat)
    {
        BSET(chttp_stat, 0x00, sizeof(CHTTP_STAT));
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_stat_free(CHTTP_STAT *chttp_stat)
{
    if(NULL_PTR != chttp_stat)
    {
        chttp_stat_clean(chttp_stat);
        free_static_mem(MM_CHTTP_STAT, chttp_stat, LOC_CHTTP_0009);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_stat_clone(const CHTTP_STAT *chttp_stat_src, CHTTP_STAT *chttp_stat_des)
{
    if(NULL_PTR != chttp_stat_des)
    {
        BCOPY(chttp_stat_src, chttp_stat_des, sizeof(CHTTP_STAT));
    }

    /*ignore status_str and desc_str*/
    return (EC_TRUE);
}

void chttp_stat_print(LOG *log, const CHTTP_STAT *chttp_stat)
{
    sys_log(LOGSTDOUT, "chttp_stat_print:send len   : %u\n", CHTTP_STAT_S_SEND_LEN(chttp_stat));
    sys_log(LOGSTDOUT, "chttp_stat_print:recv len   : %u\n", CHTTP_STAT_S_RECV_LEN(chttp_stat));
 
    sys_log(LOGSTDOUT, "chttp_stat_print:start time : %u.%u\n", CHTTP_STAT_BASIC_S_NSEC(chttp_stat), CHTTP_STAT_BASIC_S_MSEC(chttp_stat));
    sys_log(LOGSTDOUT, "chttp_stat_print:recvd time : %u.%u\n", CHTTP_STAT_BASIC_R_NSEC(chttp_stat), CHTTP_STAT_BASIC_R_MSEC(chttp_stat));
    sys_log(LOGSTDOUT, "chttp_stat_print:done  time : %u.%u\n", CHTTP_STAT_BASIC_D_NSEC(chttp_stat), CHTTP_STAT_BASIC_D_MSEC(chttp_stat));
    sys_log(LOGSTDOUT, "chttp_stat_print:end   time : %u.%u\n", CHTTP_STAT_BASIC_E_NSEC(chttp_stat), CHTTP_STAT_BASIC_E_MSEC(chttp_stat));
 
    sys_log(LOGSTDOUT, "chttp_stat_print:status str : %s\n", CHTTP_STAT_STAT_STR(chttp_stat));
    sys_log(LOGSTDOUT, "chttp_stat_print:info   str : %s\n", CHTTP_STAT_DESC_STR(chttp_stat));
 
    return;
}

/*---------------------------------------- INTERFACE WITH HTTP NODE  ----------------------------------------*/
CHTTP_NODE *chttp_node_new(const UINT32 type)
{
    CHTTP_NODE *chttp_node;

    alloc_static_mem(MM_CHTTP_NODE, &chttp_node, LOC_CHTTP_0010);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_new: new chttp_node failed\n");
        return (NULL_PTR);
    }

    if(EC_FALSE == chttp_node_init(chttp_node, type))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_new: init chttp_node failed\n");
        free_static_mem(MM_CHTTP_NODE, chttp_node, LOC_CHTTP_0011);
        return (NULL_PTR);
    }

    return (chttp_node);
}

EC_BOOL chttp_node_init(CHTTP_NODE *chttp_node, const UINT32 type)
{
    if(NULL_PTR != chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_init: chttp_node: %p\n", chttp_node);
        CHTTP_NODE_CSRV(chttp_node)          = NULL_PTR;
     
        CHTTP_NODE_CROUTINE_NODE(chttp_node) = NULL_PTR;

        CHTTP_NODE_CROUTINE_COND(chttp_node) = NULL_PTR;

        CHTTP_NODE_TYPE(chttp_node) = type;

        CHTTP_NODE_STATUS_CODE(chttp_node) = CHTTP_STATUS_NONE;
     
        __chttp_parser_init(CHTTP_NODE_PARSER(chttp_node), type);
        __chttp_parser_setting_init(CHTTP_NODE_SETTING(chttp_node));

        CHTTP_NODE_CSOCKET_CNODE(chttp_node)     = NULL_PTR;
        CHTTP_NODE_CQUEUE_DATA(chttp_node)       = NULL_PTR;

        cbuffer_init(CHTTP_NODE_URL(chttp_node) , 0);
        cbuffer_init(CHTTP_NODE_HOST(chttp_node), 0);
        cbuffer_init(CHTTP_NODE_URI(chttp_node) , 0);
        cbuffer_init(CHTTP_NODE_EXPIRES(chttp_node) , 0);

        cstrkv_mgr_init(CHTTP_NODE_HEADER_IN_KVS(chttp_node));
        cstrkv_mgr_init(CHTTP_NODE_HEADER_OUT_KVS(chttp_node));

        cstrkv_mgr_init(CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node));
        CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node) = EC_FALSE;
        CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node)  = EC_FALSE;

        cbytes_init(CHTTP_NODE_CONTENT_CBYTES(chttp_node));

        CTMV_INIT(CHTTP_NODE_START_TMV(chttp_node));

        CHTTP_NODE_CONTENT_LENGTH(chttp_node)    = 0;
        CHTTP_NODE_BODY_PARSED_LEN(chttp_node)   = 0;
        CHTTP_NODE_BODY_STORED_LEN(chttp_node)   = 0;
        CHTTP_NODE_HEADER_PARSED_LEN(chttp_node) = 0;
        CHTTP_NODE_RSP_STATUS(chttp_node)        = CHTTP_STATUS_NONE;

        CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node) = BIT_TRUE;
        CHTTP_NODE_KEEPALIVE(chttp_node)         = BIT_FALSE;
        CHTTP_NODE_HEADER_COMPLETE(chttp_node)   = BIT_FALSE;
        CHTTP_NODE_RECV_COMPLETE(chttp_node)     = BIT_FALSE;
        CHTTP_NODE_SEND_COMPLETE(chttp_node)     = BIT_FALSE;
        CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_FALSE;

        cbuffer_init(CHTTP_NODE_IN_BUF(chttp_node), CHTTP_IN_BUF_SIZE);
        chunk_mgr_init(CHTTP_NODE_SEND_BUF(chttp_node));
        chunk_mgr_init(CHTTP_NODE_RECV_BUF(chttp_node));

        CHTTP_NODE_STORE_PATH(chttp_node)          = NULL_PTR;

        CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node) = NULL_PTR;
        CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node)  = 0;
        CHTTP_NODE_SEND_DATA_BUFF(chttp_node)      = NULL_PTR;
        CHTTP_NODE_SEND_DATA_TOTAL_LEN(chttp_node) = 0;
        CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node)  = 0;

        CHTTP_NODE_SEND_BLOCK_FD(chttp_node)       = ERR_FD;
        CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node)     = 0;
        CHTTP_NODE_SEND_BLOCK_POS(chttp_node)      = 0;

        CHTTP_NODE_STORE_BEG_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_END_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_CUR_OFFSET(chttp_node)    = 0;

        CHTTP_NODE_STORE(chttp_node) = NULL_PTR;
     
        chttp_stat_init(CHTTP_NODE_STAT(chttp_node));
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_clean(CHTTP_NODE *chttp_node)
{
    if(NULL_PTR != chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_clean: chttp_node %p\n", chttp_node);

        CHTTP_NODE_CSRV(chttp_node) = NULL_PTR;

        if(NULL_PTR != CHTTP_NODE_CROUTINE_NODE(chttp_node))
        {
            croutine_pool_unload(TASK_REQ_CTHREAD_POOL(task_brd_default_get()), CHTTP_NODE_CROUTINE_NODE(chttp_node));
            CHTTP_NODE_CROUTINE_NODE(chttp_node) = NULL_PTR;
        }

        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node))
        {
            croutine_cond_free(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0012);
            CHTTP_NODE_CROUTINE_COND(chttp_node) = NULL_PTR;
        }

        CHTTP_NODE_TYPE(chttp_node)        = CHTTP_TYPE_HANDLE_ERR;
        CHTTP_NODE_STATUS_CODE(chttp_node) = CHTTP_STATUS_NONE;
 
        __chttp_parser_clean(CHTTP_NODE_PARSER(chttp_node));
        __chttp_parser_setting_clean(CHTTP_NODE_SETTING(chttp_node));

        cbuffer_clean(CHTTP_NODE_IN_BUF(chttp_node));
        chunk_mgr_clean(CHTTP_NODE_SEND_BUF(chttp_node));
        chunk_mgr_clean(CHTTP_NODE_RECV_BUF(chttp_node));
     
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)     = NULL_PTR; /*not handle the mounted csocket_cnode*/
        CHTTP_NODE_CQUEUE_DATA(chttp_node)       = NULL_PTR; /*already umount chttp_node from defer list*/
     
        cbuffer_clean(CHTTP_NODE_URL(chttp_node));
        cbuffer_clean(CHTTP_NODE_HOST(chttp_node));
        cbuffer_clean(CHTTP_NODE_URI(chttp_node));
        cbuffer_clean(CHTTP_NODE_EXPIRES(chttp_node));

        cstrkv_mgr_clean(CHTTP_NODE_HEADER_IN_KVS(chttp_node));
        cstrkv_mgr_clean(CHTTP_NODE_HEADER_OUT_KVS(chttp_node));

        cstrkv_mgr_clean(CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node));
        CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node) = EC_FALSE;
        CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node)  = EC_FALSE;

        cbytes_clean(CHTTP_NODE_CONTENT_CBYTES(chttp_node));

        CTMV_CLEAN(CHTTP_NODE_START_TMV(chttp_node));

        CHTTP_NODE_CONTENT_LENGTH(chttp_node)    = 0;
        CHTTP_NODE_BODY_PARSED_LEN(chttp_node)   = 0;
        CHTTP_NODE_BODY_STORED_LEN(chttp_node)   = 0;
        CHTTP_NODE_HEADER_PARSED_LEN(chttp_node) = 0;
        CHTTP_NODE_RSP_STATUS(chttp_node)        = CHTTP_STATUS_NONE;

        CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node) = BIT_TRUE;
        CHTTP_NODE_KEEPALIVE(chttp_node)         = BIT_FALSE;
        CHTTP_NODE_HEADER_COMPLETE(chttp_node)   = BIT_FALSE;
        CHTTP_NODE_RECV_COMPLETE(chttp_node)     = BIT_FALSE;
        CHTTP_NODE_SEND_COMPLETE(chttp_node)     = BIT_FALSE;
        CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_FALSE;

        if(NULL_PTR != CHTTP_NODE_STORE_PATH(chttp_node))
        {
            cstring_free(CHTTP_NODE_STORE_PATH(chttp_node));
            CHTTP_NODE_STORE_PATH(chttp_node) = NULL_PTR;
        }
 
        CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node) = NULL_PTR;
        CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node)  = 0;
        if(NULL_PTR != CHTTP_NODE_SEND_DATA_BUFF(chttp_node))
        {
            safe_free(CHTTP_NODE_SEND_DATA_BUFF(chttp_node), LOC_CHTTP_0013);
            CHTTP_NODE_SEND_DATA_BUFF(chttp_node) = NULL_PTR;
        }
        CHTTP_NODE_SEND_DATA_TOTAL_LEN(chttp_node) = 0;
        CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node)  = 0;

        CHTTP_NODE_SEND_BLOCK_FD(chttp_node)       = ERR_FD;
        CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node)     = 0;
        CHTTP_NODE_SEND_BLOCK_POS(chttp_node)      = 0;
     
        CHTTP_NODE_STORE_BEG_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_END_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_CUR_OFFSET(chttp_node)    = 0;

        if(NULL_PTR != CHTTP_NODE_STORE(chttp_node))
        {
            chttp_store_free(CHTTP_NODE_STORE(chttp_node));
            CHTTP_NODE_STORE(chttp_node) = NULL_PTR;
        }
        chttp_stat_clean(CHTTP_NODE_STAT(chttp_node));
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_free(CHTTP_NODE *chttp_node)
{
    if(NULL_PTR != chttp_node)
    {
        chttp_node_clean(chttp_node);
        free_static_mem(MM_CHTTP_NODE, chttp_node, LOC_CHTTP_0014);
    }

    return (EC_TRUE);
}

/*note: chttp_node_clear is ONLY for memory recycle asap before it comes to life-cycle end*/
EC_BOOL chttp_node_clear(CHTTP_NODE *chttp_node)
{
    if(NULL_PTR != chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_clear: try to clear chttp_node %p unused data\n", chttp_node);

        //cbuffer_clean(CHTTP_NODE_IN_BUF(chttp_node));
        chunk_mgr_clean(CHTTP_NODE_SEND_BUF(chttp_node));
        chunk_mgr_clean(CHTTP_NODE_RECV_BUF(chttp_node));
     
        cbuffer_clean(CHTTP_NODE_URL(chttp_node));
        cbuffer_clean(CHTTP_NODE_HOST(chttp_node));
        cbuffer_clean(CHTTP_NODE_URI(chttp_node));
        cbuffer_clean(CHTTP_NODE_EXPIRES(chttp_node));

        cstrkv_mgr_clean(CHTTP_NODE_HEADER_IN_KVS(chttp_node));
        cstrkv_mgr_clean(CHTTP_NODE_HEADER_OUT_KVS(chttp_node));
     
        cstrkv_mgr_clean(CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node));
        CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node) = EC_FALSE;
        CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node)  = EC_FALSE;

        cbytes_clean(CHTTP_NODE_CONTENT_CBYTES(chttp_node));

        CTMV_CLEAN(CHTTP_NODE_START_TMV(chttp_node));

        CHTTP_NODE_CONTENT_LENGTH(chttp_node)    = 0;
        CHTTP_NODE_BODY_PARSED_LEN(chttp_node)   = 0;
        CHTTP_NODE_BODY_STORED_LEN(chttp_node)   = 0;
        CHTTP_NODE_HEADER_PARSED_LEN(chttp_node) = 0;
        //CHTTP_NODE_RSP_STATUS(chttp_node)        = CHTTP_STATUS_NONE;

        //CHTTP_NODE_EXPIRED_BODY_NEED(chttp_node) = BIT_TRUE;
        //CHTTP_NODE_KEEPALIVE(chttp_node)         = BIT_FALSE;
        CHTTP_NODE_HEADER_COMPLETE(chttp_node)   = BIT_FALSE;
        CHTTP_NODE_RECV_COMPLETE(chttp_node)     = BIT_FALSE;
        CHTTP_NODE_SEND_COMPLETE(chttp_node)     = BIT_FALSE;
        //CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_FALSE;

        if(NULL_PTR != CHTTP_NODE_STORE_PATH(chttp_node))
        {
            cstring_free(CHTTP_NODE_STORE_PATH(chttp_node));
            CHTTP_NODE_STORE_PATH(chttp_node) = NULL_PTR;
        }
 
        CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node) = NULL_PTR;
        CHTTP_NODE_SEND_DATA_MORE_AUX(chttp_node)  = 0;
        if(NULL_PTR != CHTTP_NODE_SEND_DATA_BUFF(chttp_node))
        {
            safe_free(CHTTP_NODE_SEND_DATA_BUFF(chttp_node), LOC_CHTTP_0015);
            CHTTP_NODE_SEND_DATA_BUFF(chttp_node) = NULL_PTR;
        }
        CHTTP_NODE_SEND_DATA_TOTAL_LEN(chttp_node) = 0;
        CHTTP_NODE_SEND_DATA_SENT_LEN(chttp_node)  = 0;

        CHTTP_NODE_SEND_BLOCK_FD(chttp_node)       = ERR_FD;
        CHTTP_NODE_SEND_BLOCK_SIZE(chttp_node)     = 0;
        CHTTP_NODE_SEND_BLOCK_POS(chttp_node)      = 0;
     
        CHTTP_NODE_STORE_BEG_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_END_OFFSET(chttp_node)    = 0;
        CHTTP_NODE_STORE_CUR_OFFSET(chttp_node)    = 0;

        if(NULL_PTR != CHTTP_NODE_STORE(chttp_node))
        {
            chttp_store_free(CHTTP_NODE_STORE(chttp_node));
            CHTTP_NODE_STORE(chttp_node) = NULL_PTR;
        }
        chttp_stat_clean(CHTTP_NODE_STAT(chttp_node));
    }

    return (EC_TRUE);
}

/*on server side: wait to resume*/
EC_BOOL chttp_node_wait_resume(CHTTP_NODE *chttp_node)
{
    if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
    {
        CSOCKET_CNODE *csocket_cnode;
        CSRV          *csrv;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_wait_resume: chttp_node %p => csocket_cnode is null\n",
                       chttp_node);
            return (EC_FALSE);                    
        }

        CSOCKET_CNODE_WORK_WAIT_RESUME(csocket_cnode) = EC_TRUE;

        csrv = (CSRV *)CHTTP_NODE_CSRV(chttp_node);

        cepoll_set_event(task_brd_default_get_cepoll(),
                          CSOCKET_CNODE_SOCKFD(csocket_cnode),
                          CEPOLL_RD_EVENT,
                          (CEPOLL_EVENT_HANDLER)CSRV_RD_EVENT_HANDLER(csrv),
                          (void *)csocket_cnode);

        cepoll_set_complete(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_COMPLETE_HANDLER(csrv),
                           (void *)csocket_cnode);
                        
        cepoll_set_shutdown(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_CLOSE_HANDLER(csrv),
                           (void *)csocket_cnode);

        cepoll_set_timeout(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CSRV_TIMEOUT_NSEC(csrv),
                           (CEPOLL_EVENT_HANDLER)CSRV_TIMEOUT_HANDLER(csrv),
                           (void *)csocket_cnode);

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_wait_resume: sockfd %d >>> resume parser\n",
                   CSOCKET_CNODE_SOCKFD(csocket_cnode));
                
        chttp_resume_parser(CHTTP_NODE_PARSER(chttp_node)); /*resume parser*/

        chttp_node_clear(chttp_node);

        /*reset start time after resume parser*/
        if(SWITCH_ON == HIGH_PRECISION_TIME_SWITCH)
        {
            task_brd_update_time_default();
        }
        CTMV_CLONE(task_brd_default_get_daytime(), CHTTP_NODE_START_TMV(chttp_node));

        if(EC_TRUE == chttp_node_has_data_in(chttp_node))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_wait_resume: sockfd %d has more data in => parse now\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));     

            CSOCKET_CNODE_WORK_WAIT_RESUME(csocket_cnode) = EC_FALSE;
            CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/
     
            chttp_parse(chttp_node); /*try to parse if i-buffer has data more*/ 
        }
        return (EC_TRUE);/*wait for next http request*/
    }

    /*else*/
    chttp_node_clear(chttp_node);

    return (EC_TRUE);
}

void chttp_node_print(LOG *log, const CHTTP_NODE *chttp_node)
{
    sys_log(log, "chttp_node_print:chttp_node: %p\n", chttp_node);
 
    sys_log(log, "chttp_node_print:url : \n");
    cbuffer_print_str(log, CHTTP_NODE_URL(chttp_node));
 
    sys_log(log, "chttp_node_print:host : \n");
    cbuffer_print_str(log, CHTTP_NODE_HOST(chttp_node));

    sys_log(log, "chttp_node_print:uri : \n");
    cbuffer_print_str(log, CHTTP_NODE_URI(chttp_node));

    sys_log(log, "chttp_node_print:header_in kvs: \n");
    cstrkv_mgr_print(log, CHTTP_NODE_HEADER_IN_KVS(chttp_node));

    sys_log(log, "chttp_node_print:header_out kvs: \n");
    cstrkv_mgr_print(log, CHTTP_NODE_HEADER_OUT_KVS(chttp_node));

    sys_log(log, "chttp_node_print:header_modified kvs: \n");
    cstrkv_mgr_print(log, CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node));
 
    sys_log(log, "chttp_node_print:header content length: %"PRId64"\n", CHTTP_NODE_CONTENT_LENGTH(chttp_node));

    //sys_log(LOGSTDOUT, "chttp_node_print:req body chunks: total length %"PRId64"\n", chttp_node_recv_len(chttp_node));
 
    //chunk_mgr_print_str(LOGSTDOUT, chttp_node_recv_chunks(chttp_node));
    //chunk_mgr_print_info(LOGSTDOUT, chttp_node_recv_chunks(chttp_node));

    return;
}

EC_BOOL chttp_node_is_chunked(const CHTTP_NODE *chttp_node)
{
    char *transfer_encoding;
    transfer_encoding = cstrkv_mgr_get_val_str_ignore_case(CHTTP_NODE_HEADER_IN_KVS(chttp_node), (const char *)"Transfer-Encoding");
    if(NULL_PTR == transfer_encoding)
    {
        return (EC_FALSE);
    }

    if(0 != STRCASECMP(transfer_encoding, (const char *)"chunked"))
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_is_norange(const CHTTP_NODE *chttp_node)
{
    char *content_range;
    content_range = cstrkv_mgr_get_val_str_ignore_case(CHTTP_NODE_HEADER_IN_KVS(chttp_node), (const char *)"Content-Range");
    if(NULL_PTR == content_range)
    {
        return (EC_TRUE);
    }
    return (EC_FALSE); 
}

EC_BOOL chttp_node_recv(CHTTP_NODE *chttp_node, CSOCKET_CNODE *csocket_cnode)
{
    CBUFFER *http_in_buffer;
    UINT32   pos;

    http_in_buffer = CHTTP_NODE_IN_BUF(chttp_node);
 
    pos = CBUFFER_USED(http_in_buffer);
    if(EC_FALSE == csocket_cnode_recv(csocket_cnode,
                                CBUFFER_DATA(http_in_buffer),
                                CBUFFER_SIZE(http_in_buffer),
                                &pos))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_recv: read on sockfd %d failed where size %d and used %d\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode),
                            CBUFFER_SIZE(http_in_buffer),
                            CBUFFER_USED(http_in_buffer));

        return (EC_FALSE);                         
    }

    if(CBUFFER_USED(http_in_buffer) == pos)
    {
        __chttp_on_recv_complete(chttp_node);
     
        if(EC_TRUE == csocket_cnode_is_connected(csocket_cnode))
        {
            return (EC_DONE);/*no more data to recv*/
        }
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                            "warn:chttp_node_recv: read nothing on sockfd %d (%s) where buffer size %d and used %d\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode),  csocket_cnode_tcpi_stat_desc(csocket_cnode),
                            CBUFFER_SIZE(http_in_buffer),
                            CBUFFER_USED(http_in_buffer));
     
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT,
                        "[DEBU] chttp_node_recv: read %u bytes on sockfd %d (%s) where buffer size %d and used %d\n",
                        (((uint32_t)pos) - CBUFFER_USED(http_in_buffer)),
                        CSOCKET_CNODE_SOCKFD(csocket_cnode),  csocket_cnode_tcpi_stat_desc(csocket_cnode),
                        CBUFFER_SIZE(http_in_buffer),
                        CBUFFER_USED(http_in_buffer));    

    /*statistics*/
    CHTTP_NODE_S_RECV_LEN_INC(chttp_node, (((uint32_t)pos) - CBUFFER_USED(http_in_buffer)));
    CBUFFER_USED(http_in_buffer) = (uint32_t)pos;
    return (EC_TRUE);
}

EC_BOOL chttp_node_send(CHTTP_NODE *chttp_node, CSOCKET_CNODE *csocket_cnode)
{
    CHUNK_MGR                *send_chunks;
    EC_BOOL                   data_sent_flag;

    send_chunks = CHTTP_NODE_SEND_BUF(chttp_node);

    data_sent_flag = EC_FALSE; /*if any data is sent out, set it to EC_TRUE*/
    while(EC_FALSE == chunk_mgr_is_empty(send_chunks))
    {
        CHUNK *chunk;
        UINT32 pos;
     
        chunk = chunk_mgr_first_chunk(send_chunks);
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_send: sockfd %d chunk %p offset %d, buffer used %d\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode),
                            chunk, CHUNK_OFFSET(chunk), CHUNK_USED(chunk));
        if(CHUNK_OFFSET(chunk) >= CHUNK_USED(chunk))
        {
            /*send completely*/
            chunk_mgr_pop_first_chunk(send_chunks);
            chunk_free(chunk);
            continue;
        }

        pos = CHUNK_OFFSET(chunk);
        if(EC_FALSE == csocket_cnode_send(csocket_cnode, CHUNK_DATA(chunk), CHUNK_USED(chunk), &pos))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_send: sockfd %d send %ld bytes failed\n",
                               CSOCKET_CNODE_SOCKFD(csocket_cnode),
                               CHUNK_USED(chunk) - CHUNK_OFFSET(chunk)
                               );
         
            return (EC_FALSE);                        
        }

        if(CHUNK_OFFSET(chunk) == (uint32_t)pos)
        {
            if(EC_FALSE == csocket_cnode_is_connected(csocket_cnode)) /*Jun 17, 2017*/
            {
                return (EC_FALSE);
            }
     
            if(EC_FALSE == data_sent_flag)/*Exception!*/
            {
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                                    "error:chttp_node_send: send nothing on sockfd %d failed whence chunk offset %d and used %d\n",
                                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                                    CHUNK_OFFSET(chunk),
                                    CHUNK_USED(chunk));

                return (EC_FALSE);
            }
        }
        else
        {
            data_sent_flag = EC_TRUE;
        }

        /*statistics*/
        CHTTP_NODE_S_SEND_LEN_INC(chttp_node, (((uint32_t)pos) - CHUNK_OFFSET(chunk)));

        CHUNK_OFFSET(chunk) = (uint32_t)pos;
        if(CHUNK_OFFSET(chunk) < CHUNK_USED(chunk))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_send: sockfd %d continous chunk %p, offset %u size %u\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode), chunk, CHUNK_OFFSET(chunk), CHUNK_USED(chunk)); 
     
            return (EC_TRUE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_send: sockfd %d pop chunk %p and clean it, size %u\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode), chunk, CHUNK_USED(chunk)); 
     
        /*chunk is sent completely*/
        chunk_mgr_pop_first_chunk(send_chunks);
        chunk_free(chunk);
    } 

    return (EC_TRUE);
}

EC_BOOL chttp_node_need_send(CHTTP_NODE *chttp_node)
{
    CHUNK_MGR                *send_chunks;

    send_chunks = CHTTP_NODE_SEND_BUF(chttp_node);

    if(EC_TRUE == chunk_mgr_is_empty(send_chunks))
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_need_parse(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == cbuffer_is_empty(CHTTP_NODE_IN_BUF(chttp_node)))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL chttp_node_has_data_in(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == cbuffer_is_empty(CHTTP_NODE_IN_BUF(chttp_node)))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL chttp_node_recv_export_to_cbytes(CHTTP_NODE *chttp_node, CBYTES *cbytes, const UINT32 body_len)
{
    uint8_t       *data;
    uint32_t       size;

    if(EC_TRUE == chunk_mgr_umount_data(CHTTP_NODE_RECV_BUF(chttp_node), &data, &size)) /*no data copying but data transfering*/
    {
        ASSERT(body_len == size);
        cbytes_mount(cbytes, size, data);
        return (EC_TRUE);
    }

    if(EC_FALSE == cbytes_expand_to(cbytes, body_len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_recv_export_to_cbytes: expand cbytes to %ld failed\n",
                        body_len);
        return (EC_FALSE);
    }

    return chunk_mgr_export(CHTTP_NODE_RECV_BUF(chttp_node), CBYTES_BUF(cbytes), CBYTES_LEN(cbytes), &CBYTES_LEN(cbytes));/*try again*/
}

EC_BOOL chttp_node_recv_clean(CHTTP_NODE *chttp_node)
{
    return chunk_mgr_clean(CHTTP_NODE_RECV_BUF(chttp_node));
}

/*for debug only*/
CHUNK_MGR *chttp_node_recv_chunks(const CHTTP_NODE *chttp_node)
{
    return (CHUNK_MGR *)CHTTP_NODE_RECV_BUF(chttp_node);
}

/*for debug only*/
UINT32 chttp_node_recv_chunks_num(const CHTTP_NODE *chttp_node)
{
    return chunk_mgr_count_chunks(CHTTP_NODE_RECV_BUF(chttp_node));
}


/*for debug only*/
UINT32 chttp_node_recv_len(const CHTTP_NODE *chttp_node)
{
    return (UINT32)chunk_mgr_total_length(CHTTP_NODE_RECV_BUF(chttp_node));
}

/*for debug only*/
UINT32 chttp_node_send_len(const CHTTP_NODE *chttp_node)
{
    return (UINT32)chunk_mgr_send_length(CHTTP_NODE_SEND_BUF(chttp_node));
}

EC_BOOL chttp_node_add_header(CHTTP_NODE *chttp_node, const char *k, const char *v)
{
    return cstrkv_mgr_add_kv_str(CHTTP_NODE_HEADER_IN_KVS(chttp_node), k, v);
}

char *chttp_node_get_header(CHTTP_NODE *chttp_node, const char *k)
{
    return cstrkv_mgr_get_val_str_ignore_case(CHTTP_NODE_HEADER_IN_KVS(chttp_node), k);
}

EC_BOOL chttp_node_del_header(CHTTP_NODE *chttp_node, const char *k)
{
    return cstrkv_mgr_del_key_str(CHTTP_NODE_HEADER_IN_KVS(chttp_node), k);
}

EC_BOOL chttp_node_renew_header(CHTTP_NODE *chttp_node, const char *k, const char *v)
{
    if(NULL_PTR == k)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_renew_header: k is null => no header renewed\n");
        return (EC_FALSE);
    }
 
    chttp_node_del_header(chttp_node, k);

    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_renew_header: v is null => header ['%s'] was deleted only\n");
        return (EC_FALSE);
    }
 
    chttp_node_add_header(chttp_node, k, v);

    return (EC_TRUE);
}

EC_BOOL chttp_node_fetch_header(CHTTP_NODE *chttp_node, const char *k, CSTRKV_MGR *cstrkv_mgr)
{
    char   * v;
 
    v = chttp_node_get_header(chttp_node, k);
    if(NULL_PTR != v)
    {
        cstrkv_mgr_add_kv_str(cstrkv_mgr, k, v);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_fetch_headers(CHTTP_NODE *chttp_node, const char *keys, CSTRKV_MGR *cstrkv_mgr)
{
    char    *s;
    char    *k[ 16 ];
 
    UINT32   num;
    UINT32   idx;

    s = c_str_dup(keys);
    if(NULL_PTR == s)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_fetch_headers: dup '%s' failed\n", keys);
        return (EC_FALSE);
    }

    num = c_str_split(s, ":;", k, sizeof(k)/sizeof(k[0]));
    for(idx = 0; idx < num; idx ++)
    {
        char   * v;
     
        v = chttp_node_get_header(chttp_node, k[ idx ]);
        if(NULL_PTR != v)
        {
            cstrkv_mgr_add_kv_str(cstrkv_mgr, k[ idx ], v);
        } 
    }

    safe_free(s, LOC_CHTTP_0016);

    return (EC_TRUE);
}

EC_BOOL chttp_node_has_header_key(CHTTP_NODE *chttp_node, const char *k)
{
    char *val;

    val = chttp_node_get_header(chttp_node, k);
    if(NULL_PTR == val)
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_has_header(CHTTP_NODE *chttp_node, const char *k, const char *v)
{
    char *val;

    val = chttp_node_get_header(chttp_node, k);
    if(NULL_PTR == val)
    {
        return (EC_FALSE);
    }

    if(NULL_PTR == v || 0 == STRCASECMP(val, v))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

void chttp_node_print_header(LOG *log, const CHTTP_NODE *chttp_node)
{
    sys_log(LOGSTDOUT, "chttp_node_print_header:header_in kvs: \n");
    cstrkv_mgr_print(LOGSTDOUT, CHTTP_NODE_HEADER_IN_KVS(chttp_node));

    sys_log(LOGSTDOUT, "chttp_node_print_header:header_out kvs: \n");
    cstrkv_mgr_print(LOGSTDOUT, CHTTP_NODE_HEADER_OUT_KVS(chttp_node));

    sys_log(LOGSTDOUT, "chttp_node_print_header:header_modified kvs: \n");
    cstrkv_mgr_print(LOGSTDOUT, CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node));

    return;
}

EC_BOOL chttp_node_check_http_cache_control(CHTTP_NODE *chttp_node)
{
    char     *v;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_http_cache_control: enter\n");
    /*http specification*/
    v = chttp_node_get_header(chttp_node, (const char *)"Cache-Control");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "no-cache,no-store,private"))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_http_cache_control: '%s' => false\n", v);
            return (EC_FALSE);
        }
        if(EC_TRUE == c_str_is_in(v, ",", "max-age=0"))/*case hpcc-52*/
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_http_cache_control: '%s' => true\n", v);
            return (EC_TRUE);
        }
    }
 
    v = chttp_node_get_header(chttp_node, (const char *)"Pragma");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "no-cache"))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_http_cache_control: '%s' => false\n", v);
            return (EC_FALSE);
        }
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_http_cache_control: leave\n");
    return (EC_TRUE);
}

EC_BOOL chttp_node_check_private_cache_control(CHTTP_NODE *chttp_node)
{
    char     *v;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_private_cache_control: enter\n");
    v = chttp_node_get_header(chttp_node, (const char *)"Expires");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "-1"))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_private_cache_control: '%s' => false\n", v);
            return (EC_FALSE);
        }
    }
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_private_cache_control: leave\n");
    return (EC_TRUE);
}

void chttp_node_check_cacheable(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE *chttp_store;
    char        *v;
 
    uint32_t     status_code;
    uint32_t     expires;
    EC_BOOL      ret_of_status_code;

    chttp_store = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: chttp_store is null\n");
        return;
    }

    /*force not to cache*/
    if(CHTTP_STORE_CACHE_NONE == CHTTP_STORE_CACHE_CTRL(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: is CHTTP_STORE_CACHE_NONE => leave\n");
        return;
    }

    if(EC_FALSE == chttp_node_check_http_cache_control(chttp_node))
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [1.1] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    status_code = (uint32_t)CHTTP_NODE_STATUS_CODE(chttp_node);
 
    expires     = 0; /*init*/
    if(EC_TRUE == chttp_store_has_cache_status_code(chttp_store, status_code, &expires))
    {
        /*cache*/
        if(0 < expires)
        {
            char *expires_str;

            expires_str = c_http_time(task_brd_default_get_time() + (time_t)expires);
            chttp_node_renew_header(chttp_node, (const char *)"Expires", expires_str);
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [1.2] renew header: 'Expires':'%s'\n", expires_str);
        }
     
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_BOTH;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [1.2] set CHTTP_STORE_CACHE_BOTH\n");
        return;
    }

    if(EC_TRUE == chttp_store_has_not_cache_status_code(chttp_store, status_code))
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [1.3] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    ret_of_status_code = chttp_store_has_status_code(chttp_store, status_code);
    if(EC_FALSE == ret_of_status_code)
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [2] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    if(SWITCH_ON == CHTTP_STORE_CACHE_SWITCH(chttp_store)
    && EC_FALSE == chttp_store_has_cache_rsp_headers(chttp_store, CHTTP_NODE_HEADER_IN_KVS(chttp_node)))
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [3] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    if(SWITCH_ON == CHTTP_STORE_CACHE_SWITCH(chttp_store)
    && EC_TRUE  == chttp_store_has_not_cache_rsp_headers(chttp_store, CHTTP_NODE_HEADER_IN_KVS(chttp_node)))
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [4] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    if(EC_FALSE == chttp_node_check_private_cache_control(chttp_node))
    {
        /*not cache*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [5] set CHTTP_STORE_CACHE_NONE\n");
        return;
    }

    /*now determine cache what*/
#if 0 
    if(CHTTP_MOVED_PERMANENTLY == status_code
    || CHTTP_MOVED_TEMPORARILY == status_code
    )
    {
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_WHOLE;
        return;
    }
#endif
    if(CHTTP_NOT_MODIFIED == status_code)
    {
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_HEADER;
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [6] set CHTTP_STORE_CACHE_HEADER\n");
        return;
    } 
#if 0
    if(CHTTP_BAD_REQUEST <= status_code)
    {
        CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_WHOLE;
        return;
    }
#endif
#if 0
    /*http specification*/
    v = chttp_node_get_header(chttp_node, (const char *)"Cache-Control");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "no-cache,no-store,private,max-age=0"))
        {
            CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_HEADER;
            return;
        }

        if(EC_TRUE == c_str_is_in(v, ",", "public"))
        {
            CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_BOTH;
            return;
        }
    }
 
    v = chttp_node_get_header(chttp_node, (const char *)"Pragma");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "no-cache"))
        {
            CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_HEADER;
            return;
        }
    }

    v = chttp_node_get_header(chttp_node, (const char *)"Expires");
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "-1"))
        {
            CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_NONE;
            return;
        }
    } 
#endif
    /*private specification*/
    v = chttp_node_get_header(chttp_node, (const char *)"Content-Length");/*http request: ["Range"] = "bytes=0-1"*/
    if(NULL_PTR != v)
    {
        if(EC_TRUE == c_str_is_in(v, ",", "2"))
        {
            CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_WHOLE;
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [7] set CHTTP_STORE_CACHE_WHOLE\n");
            return;
        }
    }
 
    CHTTP_STORE_CACHE_CTRL(chttp_store) = CHTTP_STORE_CACHE_BOTH;
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_cacheable: [8] set CHTTP_STORE_CACHE_BOTH\n");
    return;
}

EC_BOOL chttp_node_adjust_seg_id(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE *chttp_store;

    chttp_store = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_adjust_seg_id: chttp_store is null\n");
        return (EC_TRUE);
    }

    if(EC_TRUE == chttp_node_is_chunked(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_adjust_seg_id: chunked, adjust seg_id %u => 0\n",
                    CHTTP_STORE_SEG_ID(chttp_store));

        CHTTP_STORE_SEG_ID(chttp_store) = 0;
        return (EC_TRUE);
    }

    if(EC_TRUE == chttp_node_is_norange(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_adjust_seg_id: norange, content-length: %s, adjust seg_id %u => 0\n",
                    chttp_node_get_header(chttp_node, (const char *)"Content-Range"),
                    CHTTP_STORE_SEG_ID(chttp_store));

        CHTTP_STORE_SEG_ID(chttp_store) = 0;
        return (EC_TRUE);
    } 
 
    return (EC_TRUE);
}

uint64_t chttp_node_fetch_file_size(CHTTP_NODE *chttp_node)
{
    char        *v;
    char        *p;
 
    v = chttp_node_get_header(chttp_node, (const char *)"Content-Range");
    if(NULL_PTR == v)
    {
        v = chttp_node_get_header(chttp_node, (const char *)"Content-Length");
        if(NULL_PTR == v)
        {
            return ((uint64_t)0);
        }
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_fetch_file_size: Content-Length: '%s'\n", v);
        return c_str_to_uint64(v);
    }

    /*format should be xx-xx/xxx*/
    p = v + strlen(v) - 1;
    while('/' != (*p) && p > v)
    {
        p --;
    }

    if(p <= v)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_fetch_file_size: invalid Content-Range: '%s'\n", v);
        return((uint64_t)0);
    }
 
    p ++;
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_fetch_file_size: Content-Range: '%s'\n", p);
    return c_str_to_uint64(p);
}

EC_BOOL chttp_node_check_use_gzip(CHTTP_NODE *chttp_node)
{
    char        *v;
 
    v = chttp_node_get_header(chttp_node, (const char *)"Content-Encoding");
    if(NULL_PTR == v)
    {
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check_use_gzip: Content-Encoding: '%s'\n", v);
    if(0 == STRCASECMP(v, "gzip"))
    {
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

/*---------------------------------------- HTTP HEADER PASER  ----------------------------------------*/

EC_BOOL chttp_parse_host(CHTTP_NODE *chttp_node)
{
    CSTRING       *host_cstr;
    CBUFFER       *url;
    uint8_t       *data;
    uint8_t       *host_str;
    uint32_t       offset;
    uint32_t       host_len;
 
    host_cstr = cstrkv_mgr_get_val_cstr(CHTTP_NODE_HEADER_IN_KVS(chttp_node), "Host");
    if(NULL_PTR != host_cstr)
    {
        cbuffer_set(CHTTP_NODE_HOST(chttp_node), cstring_get_str(host_cstr), (uint32_t)cstring_get_len(host_cstr));
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_host: not found 'Host' in http header\n");

    url  = CHTTP_NODE_URL(chttp_node);
    data = CBUFFER_DATA(url); 
 
    for(offset = CONST_STR_LEN("http://"); offset < CBUFFER_USED(url); offset ++)
    {
        if('/' == data [ offset ])
        {
            break;
        }
    }
 
    host_str = CBUFFER_DATA(url) + CONST_STR_LEN("http://");
    host_len = offset - CONST_STR_LEN("http://");

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_host: fetch domain %.*s as 'Host' in http header\n", host_len, host_str);
 
    cbuffer_set(CHTTP_NODE_HOST(chttp_node), host_str, host_len); 

    return (EC_TRUE);
}

EC_BOOL chttp_parse_content_length(CHTTP_NODE *chttp_node)
{
    CSTRING       *content_length_cstr;
 
    content_length_cstr = cstrkv_mgr_get_val_cstr(CHTTP_NODE_HEADER_IN_KVS(chttp_node), "Content-Length");
    if(NULL_PTR == content_length_cstr)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_content_length: not found 'Content-Length' in http header\n");
        CHTTP_NODE_CONTENT_LENGTH(chttp_node) = 0;
        return (EC_TRUE);
    }
    CHTTP_NODE_CONTENT_LENGTH(chttp_node) = c_chars_to_uint64((char *)cstring_get_str(content_length_cstr),
                                                                    (uint32_t)cstring_get_len(content_length_cstr));
    return (EC_TRUE);
}

EC_BOOL chttp_parse_connection_keepalive(CHTTP_NODE *chttp_node)
{
    CSTRING       *connection_keepalive_cstr;
     
    connection_keepalive_cstr = cstrkv_mgr_get_val_cstr(CHTTP_NODE_HEADER_IN_KVS(chttp_node), "Connection");
    if(NULL_PTR == connection_keepalive_cstr)
    {
        connection_keepalive_cstr = cstrkv_mgr_get_val_cstr(CHTTP_NODE_HEADER_IN_KVS(chttp_node), "Proxy-Connection");
    }
 
    if(NULL_PTR == connection_keepalive_cstr)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_connection_keepalive: no header 'Connection'\n"); 
        return (EC_TRUE);
    }
                     
    if(EC_TRUE == cstring_is_str_ignore_case(connection_keepalive_cstr, (const UINT8 *)"close"))
    {
        CSOCKET_CNODE *csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_parse_connection_keepalive: chttp_node %p -> csocket_cnode is null\n",
                            chttp_node);
            return (EC_FALSE);
        }
     
        CHTTP_NODE_KEEPALIVE(chttp_node) = BIT_FALSE; /*force to disable keepalive*/
     
        if(EC_TRUE == csocket_disable_keepalive(CSOCKET_CNODE_SOCKFD(csocket_cnode)))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_connection_keepalive: disable sockfd %d keepalive done\n", CSOCKET_CNODE_SOCKFD(csocket_cnode));
            return (EC_TRUE);     
        }
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                    "error:chttp_parse_connection_keepalive: disable chttp_node %p -> csocket_cnode %p -> sockfd %d keepalive failed, ignore that\n",
                    chttp_node, csocket_cnode, CSOCKET_CNODE_SOCKFD(csocket_cnode));
                 
        return (EC_TRUE);
    } 

    if(EC_TRUE == cstring_is_str_ignore_case(connection_keepalive_cstr, (const UINT8 *)"keepalive")
    || EC_TRUE == cstring_is_str_ignore_case(connection_keepalive_cstr, (const UINT8 *)"keep-alive"))
    {
        CSOCKET_CNODE *csocket_cnode;

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_parse_connection_keepalive: chttp_node %p -> csocket_cnode is null\n",
                            chttp_node);
            return (EC_FALSE);
        }

        /*force to enable keepalive due to csocket_enable_keepalive not adapative to unix domain socket*/
        CHTTP_NODE_KEEPALIVE(chttp_node) = BIT_TRUE;

        if(BIT_FALSE == CSOCKET_CNODE_UNIX(csocket_cnode))
        {
            if(EC_TRUE == csocket_enable_keepalive(CSOCKET_CNODE_SOCKFD(csocket_cnode)))
            {
                dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_connection_keepalive: enable sockfd %d keepalive done\n", CSOCKET_CNODE_SOCKFD(csocket_cnode));
                return (EC_TRUE);     
            }
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                        "error:chttp_parse_connection_keepalive: enable chttp_node %p -> csocket_cnode %p -> sockfd %d keepalive failed, ignore that\n",
                        chttp_node, csocket_cnode, CSOCKET_CNODE_SOCKFD(csocket_cnode));
            /*note: ingore that enable keepalive failure*/
        }
    }

    return (EC_TRUE);
}

EC_BOOL chttp_parse_uri(CHTTP_NODE *chttp_node)
{
    CBUFFER       *url_cbuffer;
    CBUFFER       *host_cbuffer;
    CBUFFER       *uri_cbuffer;

    uint8_t       *uri_str; 
    uint32_t       uri_len;
    uint32_t       skip_len;
 
    url_cbuffer  = CHTTP_NODE_URL(chttp_node);
    host_cbuffer = CHTTP_NODE_HOST(chttp_node);
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    if(EC_FALSE == cbuffer_cmp_bytes(url_cbuffer, 0, CONST_UINT8_STR_AND_LEN("http://")))
    {
        cbuffer_clone(url_cbuffer, uri_cbuffer);
        return (EC_TRUE);
    }

    skip_len = sizeof("http://") - 1 + CBUFFER_USED(host_cbuffer);
    uri_str  = CBUFFER_DATA(url_cbuffer) + skip_len;
    uri_len  = CBUFFER_USED(url_cbuffer) - skip_len;

    if(EC_FALSE == cbuffer_set(uri_cbuffer, uri_str, uri_len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_parse_uri: set uri %.*s failed\n", uri_len, uri_str);
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_parse_post(CHTTP_NODE *chttp_node, const uint32_t parsed_len)
{
    http_parser_t            *http_parser;
    CBUFFER                  *http_in_buffer;
    CSOCKET_CNODE            *csocket_cnode;

    http_parser     = CHTTP_NODE_PARSER(chttp_node);
    http_in_buffer  = CHTTP_NODE_IN_BUF(chttp_node);
    
    csocket_cnode   = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
#if 0
    if(s_dead == http_parser->state)
    {
        CHTTP_NODE_KEEPALIVE(chttp_node) = BIT_FALSE;

          dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: sockfd %d, http state %s => disable keepalive\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), http_state_str(http_parser->state));
               
    }
#endif
    if(HPE_OK == HTTP_PARSER_ERRNO(http_parser))
    {
        if(0 < parsed_len)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_OK] sockfd %d, header parsed %u,  body parsed %"PRId64", in buf %u => shift out %u\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_HEADER_PARSED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node), CBUFFER_USED(http_in_buffer), parsed_len);
     
            cbuffer_left_shift_out(http_in_buffer, NULL_PTR, parsed_len); 
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_OK] sockfd %d, http state %s, parsed_len %u\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), http_state_str(http_parser->state), parsed_len);
             
        return (EC_TRUE);    
    }

    if(HPE_PAUSED == HTTP_PARSER_ERRNO(http_parser))
    {
        if(0 < parsed_len)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_PAUSED] sockfd %d, header parsed %u,  body parsed %"PRId64", in buf %u => shift out %u\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_HEADER_PARSED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node), CBUFFER_USED(http_in_buffer), parsed_len);
     
            cbuffer_left_shift_out(http_in_buffer, NULL_PTR, parsed_len); 
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_PAUSED] sockfd %d, http state %s, parsed_len %u\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), http_state_str(http_parser->state), parsed_len);
             
        return (EC_TRUE);    
    }

    if(HPE_CLOSED_CONNECTION == HTTP_PARSER_ERRNO(http_parser))
    {
        if(0 < parsed_len)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_CLOSED_CONNECTION] sockfd %d, header parsed %u,  body parsed %"PRId64", in buf %u => shift out %u\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_HEADER_PARSED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node), CBUFFER_USED(http_in_buffer), parsed_len);
     
            cbuffer_left_shift_out(http_in_buffer, NULL_PTR, parsed_len); 
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_parse_post: [HPE_CLOSED_CONNECTION] sockfd %d, http state %s, parsed_len %u\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), http_state_str(http_parser->state), parsed_len);
             
        return (EC_TRUE);    
    }    

    if(HPE_INVALID_URL == HTTP_PARSER_ERRNO(http_parser))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                        "error:chttp_parse_post: [HPE_INVALID_URL] http parser encounter error on sockfd %d where errno = %d, name = %s, description = %s\n[%.*s]\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode),
                        HTTP_PARSER_ERRNO(http_parser),
                        http_errno_name(HTTP_PARSER_ERRNO(http_parser)),
                        http_errno_description(HTTP_PARSER_ERRNO(http_parser)),
                        DMIN(CBUFFER_USED(http_in_buffer), 300), (char *)CBUFFER_DATA(http_in_buffer)
                        );
        return (EC_FALSE);                         
    }
 
    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                        "error:chttp_parse_post: http parser encounter error on sockfd %d where errno = %d, name = %s, description = %s\n[%.*s]\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode),
                        HTTP_PARSER_ERRNO(http_parser),
                        http_errno_name(HTTP_PARSER_ERRNO(http_parser)),
                        http_errno_description(HTTP_PARSER_ERRNO(http_parser)),
                        DMIN(CBUFFER_USED(http_in_buffer), 300), (char *)CBUFFER_DATA(http_in_buffer)
                        );
    return (EC_FALSE);
}

EC_BOOL chttp_parse(CHTTP_NODE *chttp_node)
{
    http_parser_t            *http_parser;
    http_parser_settings_t   *http_parser_setting;
    CBUFFER                  *http_in_buffer;

    uint32_t parsed_len;
   
    http_parser         = CHTTP_NODE_PARSER(chttp_node);
    http_parser_setting = CHTTP_NODE_SETTING(chttp_node);
    http_in_buffer      = CHTTP_NODE_IN_BUF(chttp_node);

    ASSERT(0 < CBUFFER_USED(http_in_buffer)); /*Nov 15, 2017*/
 
    parsed_len = http_parser_execute(http_parser, http_parser_setting, (char *)CBUFFER_DATA(http_in_buffer) , CBUFFER_USED(http_in_buffer));
    return chttp_parse_post(chttp_node, parsed_len);
}

EC_BOOL chttp_pause_parser(http_parser_t* http_parser)
{
    if(NULL_PTR != http_parser)
    {
        http_parser_pause(http_parser, CHTTP_PASER_PAUSED);
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL chttp_resume_parser(http_parser_t* http_parser)
{
    if(NULL_PTR != http_parser)
    {
        http_parser_pause(http_parser, CHTTP_PASER_RESUME);
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

/*---------------------------------------- MAKE RESPONSE ----------------------------------------*/
EC_BOOL chttp_make_response_header_protocol(CHTTP_NODE *chttp_node, const uint16_t major, const uint16_t minor, const uint32_t status)
{
    uint8_t  header_protocol[64];
    uint32_t len;
 
    len = snprintf(((char *)header_protocol), sizeof(header_protocol), "HTTP/%d.%d %d %s\r\n",
                   major, minor, status, chttp_status_str_get(status));
                
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), header_protocol, len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_protocol: append '%.*s' to chunks failed\n",
                           len, (char *)header_protocol);
        return (EC_FALSE);                         
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_keepalive(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN("Connection:keep-alive\r\n")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_keepalive: append 'Connection:keep-alive' to chunks failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_date(CHTTP_NODE *chttp_node)
{
    uint8_t  header_date[64];
    uint32_t len;
    ctime_t  time_in_sec;

    time_in_sec = task_brd_default_get_time();

    /*e.g., Date:Thu, 01 May 2014 12:12:16 GMT*/
    len = strftime(((char *)header_date), sizeof(header_date), "Date:%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&time_in_sec)); 
    //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_make_response_header_date: [%.*s] len = %u\n", len - 1, (char *)header_date, len - 1);
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), header_date, len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_date: append '%.*s' to chunks failed\n", len, header_date);
        return (EC_FALSE);
    }  
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_expires(CHTTP_NODE *chttp_node)
{
    CBUFFER *expires;
 
    expires = CHTTP_NODE_EXPIRES(chttp_node);
     
    if(0 < CBUFFER_USED(expires))
    {
        //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_make_response_header_expires: [%.*s] len = %u\n", CBUFFER_USED(expires), (char *)CBUFFER_DATA(expires), CBUFFER_USED(expires));
        if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CBUFFER_DATA(expires), CBUFFER_USED(expires)))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_expires: append '%.*s' to chunks failed\n",
                               CBUFFER_USED(expires), CBUFFER_DATA(expires));
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_token(CHTTP_NODE *chttp_node, const uint8_t *token, const uint32_t len)
{
    if(0 < len)
    {
        if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), token, len))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_token: append '%.*s' to chunks failed\n",
                               len, token);
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_data(CHTTP_NODE *chttp_node, const uint8_t *data, const uint32_t len)
{
    if(0 < len)
    {
        if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), data, len))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_data: append '%.*s' to chunks failed\n",
                               len, data);
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_retire(CHTTP_NODE *chttp_node, const uint8_t *retire_result, const uint32_t len)
{
    if(0 < len)
    {
        if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), retire_result, len))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_retire: append '%.*s' to chunks failed\n",
                               len, retire_result);
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_recycle(CHTTP_NODE *chttp_node, const uint8_t *recycle_result, const uint32_t len)
{
    if(0 < len)
    {
        if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), recycle_result, len))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_recycle: append '%.*s' to chunks failed\n",
                               len, recycle_result);
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_elapsed(CHTTP_NODE *chttp_node)
{
    uint8_t  header_date[128];
    uint32_t len;
    CTMV    *s_tmv;
    CTMV    *e_tmv;
 
    uint32_t elapsed_msec;

    if(SWITCH_ON == HIGH_PRECISION_TIME_SWITCH)
    {
        task_brd_update_time_default();
    }
    s_tmv = CHTTP_NODE_START_TMV(chttp_node);
    e_tmv = task_brd_default_get_daytime();

    CHTTP_ASSERT(CTMV_NSEC(e_tmv) >= CTMV_NSEC(s_tmv));
    elapsed_msec = (CTMV_NSEC(e_tmv) - CTMV_NSEC(s_tmv)) * 1000 + CTMV_MSEC(e_tmv) - CTMV_MSEC(s_tmv);

    len = 0;
    //len += snprintf(((char *)header_date) + len, sizeof(header_date) - len, "BegTime:%u.%03u\r\n", (uint32_t)CTMV_NSEC(s_tmv), (uint32_t)CTMV_MSEC(s_tmv));
    //len += snprintf(((char *)header_date) + len, sizeof(header_date) - len, "EndTime:%u.%03u\r\n", (uint32_t)CTMV_NSEC(e_tmv), (uint32_t)CTMV_MSEC(e_tmv));
    len += snprintf(((char *)header_date) + len, sizeof(header_date) - len, "Elapsed:%u micro seconds\r\n", elapsed_msec);

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), header_date, len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_elapsed: append '%.*s' to chunks failed\n", len, header_date);
        return (EC_FALSE);
    }  
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_content_type(CHTTP_NODE *chttp_node, const uint8_t *data, const uint32_t size)
{
    if(NULL_PTR == data || 0 == size)
    {
        return (EC_TRUE);
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN("Content-Type:")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_content_type: append 'Content-Type:' to chunks failed\n");
        return (EC_FALSE);
    } 

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), data, size))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_content_type: append %d bytes to chunks failed\n", size);
        return (EC_FALSE);
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN("\r\n")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_content_type: append EOL to chunks failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_content_length(CHTTP_NODE *chttp_node, const uint64_t size)
{
    uint8_t  content_length[64];
    uint32_t len;

    len = snprintf(((char *)content_length), sizeof(content_length), "Content-Length:%"PRId64"\r\n", size);

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), content_length, len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_content_length: append '%.*s' to chunks failed\n",
                           len - 2, (char *)content_length);
        return (EC_FALSE);                         
    }
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_make_response_header_content_length: append '%.*s' to chunks done\n",
                       len - 2, (char *)content_length); 
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_kv(CHTTP_NODE *chttp_node, const CSTRKV *cstrkv)
{
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CSTRKV_KEY_STR(cstrkv), (uint32_t)CSTRKV_KEY_LEN(cstrkv)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_kv: append key '%.*s' to chunks failed\n",
                    (uint32_t)CSTRKV_KEY_LEN(cstrkv), (char *)CSTRKV_KEY_STR(cstrkv));
        return (EC_FALSE);                         
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN(":")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_kv: append '\r\n' to chunks failed\n");
        return (EC_FALSE);                         
    }
 
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CSTRKV_VAL_STR(cstrkv), (uint32_t)CSTRKV_VAL_LEN(cstrkv)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_kv: append val '%.*s' to chunks failed\n",
                    (uint32_t)CSTRKV_VAL_LEN(cstrkv), (char *)CSTRKV_VAL_STR(cstrkv));
        return (EC_FALSE);                         
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN("\r\n")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_kv: append '\r\n' to chunks failed\n");
        return (EC_FALSE);                         
    }

    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_kvs(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == cstrkv_mgr_walk(CHTTP_NODE_HEADER_OUT_KVS(chttp_node), (void *)chttp_node, (CSTRKV_MGR_WALKER)chttp_make_response_header_kv))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_kvs: append kvs to chunks failed\n");
        return (EC_FALSE);                         
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_end(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CONST_UINT8_STR_AND_LEN("\r\n")))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_end: append '\r\n' to chunks failed\n");
        return (EC_FALSE);                         
    }
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_body(CHTTP_NODE *chttp_node, const uint8_t *data, const uint32_t size)
{
    if(NULL_PTR == data || 0 == size)
    {
        return (EC_TRUE);
    }

    //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_make_response_body: body: '%.*s'\n", size, data);
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), data, size))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_body: append %d bytes to chunks failed\n", size);
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

/*make response body without data copying but data transfering*/
EC_BOOL chttp_make_response_body_ext(CHTTP_NODE *chttp_node, const uint8_t *data, const uint32_t size)
{
    if(NULL_PTR == data || 0 == size)
    {
        return (EC_TRUE);
    }
 
    if(EC_FALSE == chunk_mgr_mount_data(CHTTP_NODE_SEND_BUF(chttp_node), data, size))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_body_ext: mount %d bytes to chunks failed\n", size);
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_make_response_header_common(CHTTP_NODE *chttp_node, const uint64_t body_len)
{
    if(EC_FALSE == chttp_make_response_header_protocol(chttp_node,
                                                          CHTTP_VERSION_MAJOR, CHTTP_VERSION_MINOR,
                                                          CHTTP_NODE_RSP_STATUS(chttp_node)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header protocol failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_date(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header date failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_expires(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header expires failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_elapsed(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header elapsed failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_content_type(chttp_node, NULL_PTR, 0))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header content type failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_make_response_header_content_length(chttp_node, body_len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_response_header_common: make header content length failed\n");     
        return (EC_FALSE);
    }   

    return (EC_TRUE);
}

EC_BOOL chttp_make_error_response(CHTTP_NODE *chttp_node)
{
    if(EC_FALSE == chttp_make_response_header_common(chttp_node, (uint64_t)0))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_error_response: make error response header failed\n");
     
        return (EC_FALSE);
    }

    /*note: not send keepalive header*/

    if(EC_FALSE == chttp_make_response_header_end(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_make_error_response: make header end failed\n");
        return (EC_FALSE);
    } 

    return (EC_TRUE);
}

/*---------------------------------------- HTTP SERVER ----------------------------------------*/
CSRV * chttp_srv_start(const UINT32 srv_ipaddr, const UINT32 srv_port, const UINT32 md_id,
                     const uint32_t timeout_nsec,
                     CSRV_INIT_CSOCKET_CNODE       init_csocket_cnode,
                     CSRV_ADD_CSOCKET_CNODE        add_csocket_cnode,
                     CSRV_DEL_CSOCKET_CNODE        del_csocket_cnode,
                     CSRV_RD_HANDLER_FUNC          rd_handler,
                     CSRV_WR_HANDLER_FUNC          wr_handler,
                     CSRV_TIMEOUT_HANDLER_FUNC     timeout_handler,
                     CSRV_COMPLETE_HANDLER_FUNC    complete_handler,
                     CSRV_CLOSE_HANDLER_FUNC       close_handler
                     )
{
    CSRV *csrv;
    int srv_sockfd;
    int srv_unix_sockfd;
#if 0
    if(ERR_MODULE_ID == md_id)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_start: md id is invalid\n");
        return (NULL_PTR);
    }
#endif

#if (SWITCH_OFF == NGX_BGN_SWITCH)
    if(EC_FALSE == csocket_listen(srv_ipaddr, srv_port, &srv_sockfd))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDERR, "error:chttp_srv_start: failed to listen on port %s:%ld\n",
                            c_word_to_ipv4(srv_ipaddr), srv_port);
        return (NULL_PTR);
    }
#endif/*(SWITCH_OFF == NGX_BGN_SWITCH)*/

#if (SWITCH_ON == NGX_BGN_SWITCH)
    while(EC_FALSE == csocket_listen(srv_ipaddr, srv_port, &srv_sockfd))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDERR, "error:chttp_srv_start: failed to listen on port %s:%ld, retry again\n",
                            c_word_to_ipv4(srv_ipaddr), srv_port);
        c_usleep(1, LOC_CHTTP_0017);
    }
#endif/*(SWITCH_ON == NGX_BGN_SWITCH)*/

    srv_unix_sockfd = ERR_FD;
#if 0
    if(EC_FALSE == csocket_unix_listen(srv_ipaddr, srv_port, &srv_unix_sockfd))
    {
        dbg_log(SEC_0112_CSRV, 0)(LOGSTDERR, "error:chttp_srv_start: failed to listen on unix@%s:%ld\n",
                            c_word_to_ipv4(srv_ipaddr), srv_port);

        csocket_close(srv_sockfd);
        return (NULL_PTR);
    } 
#endif
    csrv = csrv_new();
    if(NULL_PTR == csrv)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_start: new csrv failed, close srv sockfd %d and unix sockfd %d\n",
                        srv_sockfd, srv_unix_sockfd);
        csocket_close(srv_sockfd);
        csocket_close(srv_unix_sockfd);
        return (NULL_PTR);
    }

    CSRV_IPADDR(csrv)               = srv_ipaddr;
    CSRV_PORT(csrv)                 = srv_port;
    CSRV_SOCKFD(csrv)               = srv_sockfd;
    CSRV_UNIX_SOCKFD(csrv)          = srv_unix_sockfd;
 
    CSRV_MD_ID(csrv)                = md_id;
    CSRV_TIMEOUT_NSEC(csrv)         = timeout_nsec;
 
    CSRV_INIT_CSOCKET_CNODE(csrv)   = init_csocket_cnode;
    CSRV_ADD_CSOCKET_CNODE(csrv)    = add_csocket_cnode;
    CSRV_DEL_CSOCKET_CNODE(csrv)    = del_csocket_cnode;
 
    CSRV_RD_EVENT_HANDLER(csrv)     = rd_handler;
    CSRV_WR_EVENT_HANDLER(csrv)     = wr_handler;
    CSRV_TIMEOUT_HANDLER(csrv)      = timeout_handler;
    CSRV_COMPLETE_HANDLER(csrv)     = complete_handler;
    CSRV_CLOSE_HANDLER(csrv)        = close_handler;

    CSRV_CSSL_NODE(csrv)            = NULL_PTR;
 
    cepoll_set_event(task_brd_default_get_cepoll(),
                      CSRV_SOCKFD(csrv),
                      CEPOLL_RD_EVENT,
                      (CEPOLL_EVENT_HANDLER)chttp_srv_accept,
                      (void *)csrv);

    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "[DEBUG] chttp_srv_start: start srv sockfd %d on %s:%ld\n",
                       srv_sockfd, c_word_to_ipv4(srv_ipaddr), srv_port);
#if 0
    cepoll_set_event(task_brd_default_get_cepoll(),
                      CSRV_UNIX_SOCKFD(csrv),
                      CEPOLL_RD_EVENT,
                      (CEPOLL_EVENT_HANDLER)chttp_srv_unix_accept,
                      (void *)csrv);
                   

    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "chttp_srv_start: start srv sockfd %d on unix@%s:%ld\n",
                       srv_sockfd, c_word_to_ipv4(srv_ipaddr), srv_port);                    
#endif                    
    return (csrv);
}

EC_BOOL chttp_srv_end(CSRV *csrv)
{
    return csrv_free(csrv);
}

EC_BOOL chttp_srv_bind_modi(CSRV *csrv, const UINT32 modi)
{
    CSRV_MD_ID(csrv) = modi;

    return (EC_TRUE);
}

EC_BOOL chttp_srv_accept_once(CSRV *csrv, EC_BOOL *continue_flag)
{
    UINT32  client_ipaddr; 
    EC_BOOL ret;
    int     client_conn_sockfd; 

    ret = csocket_accept(CSRV_SOCKFD(csrv), &(client_conn_sockfd), CSOCKET_IS_NONBLOCK_MODE, &(client_ipaddr));
    if(EC_TRUE == ret)
    {
        CSOCKET_CNODE *csocket_cnode;
        CHTTP_NODE    *chttp_node;
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_srv_accept_once: handle new sockfd %d\n", client_conn_sockfd);

        csocket_cnode = csocket_cnode_new(CMPI_ERROR_TCID, client_conn_sockfd, CSOCKET_TYPE_TCP, client_ipaddr, CMPI_ERROR_SRVPORT);/*here do not know the remote client srv port*/
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_accept_once:failed to alloc csocket cnode for sockfd %d, hence close it\n", client_conn_sockfd);
            csocket_close(client_conn_sockfd);
            return (EC_FALSE);
        }

        chttp_node = chttp_node_new(CHTTP_TYPE_HANDLE_REQ);
        if(NULL_PTR == chttp_node)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_accept_once: new chttp_node for sockfd %d failed\n", client_conn_sockfd);
            csocket_cnode_force_close(csocket_cnode);
            return (EC_FALSE);
        }
     
        CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/
     
        /*mount csrv*/
        CHTTP_NODE_CSRV(chttp_node) = (void *)csrv;

        /*bind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = chttp_node;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = csocket_cnode;
     
        if(NULL_PTR != CSRV_INIT_CSOCKET_CNODE(csrv))
        {
            if(EC_FALSE == CSRV_INIT_CSOCKET_CNODE(csrv)(CSRV_MD_ID(csrv), csocket_cnode))
            {
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_accept_once: csrv init sockfd %d failed\n", client_conn_sockfd);

                /*unbind csocket_cnode and chttp_node*/
                CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
                CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

                csocket_cnode_force_close(csocket_cnode);
                chttp_node_free(chttp_node);

                return (EC_FALSE);
            }
        }
     
        if(SWITCH_ON == HIGH_PRECISION_TIME_SWITCH)
        {
            task_brd_update_time_default();
        }
        CTMV_CLONE(task_brd_default_get_daytime(), CHTTP_NODE_START_TMV(chttp_node));     

        if(NULL_PTR != CSRV_ADD_CSOCKET_CNODE(csrv))
        {
            CSRV_ADD_CSOCKET_CNODE(csrv)(CSRV_MD_ID(csrv), csocket_cnode);
        }

        CSOCKET_CNODE_MODI(csocket_cnode) = CSRV_MD_ID(csrv);
     
        cepoll_set_event(task_brd_default_get_cepoll(),
                          CSOCKET_CNODE_SOCKFD(csocket_cnode),
                          CEPOLL_RD_EVENT,
                          (CEPOLL_EVENT_HANDLER)CSRV_RD_EVENT_HANDLER(csrv),
                          (void *)csocket_cnode);

        cepoll_set_complete(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_COMPLETE_HANDLER(csrv),
                           (void *)csocket_cnode);
                        
        cepoll_set_shutdown(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_CLOSE_HANDLER(csrv),
                           (void *)csocket_cnode);

        cepoll_set_timeout(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CSRV_TIMEOUT_NSEC(csrv),
                           (CEPOLL_EVENT_HANDLER)CSRV_TIMEOUT_HANDLER(csrv),
                           (void *)csocket_cnode);

        csocket_cnode_stat_inc();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_srv_accept_once: accept sockfd %d done\n", client_conn_sockfd);
    }

    (*continue_flag) = ret;
 
    return (EC_TRUE);
}

EC_BOOL chttp_srv_accept(CSRV *csrv)
{
    UINT32   idx;
    UINT32   num;
    EC_BOOL  continue_flag;

    num = CSRV_ACCEPT_MAX_NUM;
    for(idx = 0; idx < num; idx ++)
    {
        if(EC_FALSE == chttp_srv_accept_once(csrv, &continue_flag))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_accept: accept No. %ld client failed where expect %ld clients\n", idx, num);
            return (EC_FALSE);
        }

        if(EC_FALSE == continue_flag)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_srv_accept: accept No. %ld client terminate where expect %ld clients\n", idx, num);
            break;
        }     
    }

    return (EC_TRUE);
}

EC_BOOL chttp_srv_unix_accept_once(CSRV *csrv, EC_BOOL *continue_flag)
{
    UINT32  client_ipaddr; 
    EC_BOOL ret;
    int     client_conn_sockfd; 

    ret = csocket_unix_accept(CSRV_UNIX_SOCKFD(csrv), &(client_conn_sockfd), CSOCKET_IS_NONBLOCK_MODE);
    if(EC_TRUE == ret)
    {
        CSOCKET_CNODE *csocket_cnode;
        CHTTP_NODE    *chttp_node;

        client_ipaddr = c_ipv4_to_word((const char *)"127.0.0.1");
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_srv_unix_accept_once: handle new sockfd %d\n", client_conn_sockfd);

        csocket_cnode = csocket_cnode_unix_new(CMPI_ERROR_TCID, client_conn_sockfd, CSOCKET_TYPE_TCP, client_ipaddr, CMPI_ERROR_SRVPORT);/*here do not know the remote client srv port*/
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_unix_accept_once:failed to alloc csocket cnode for sockfd %d, hence close it\n", client_conn_sockfd);
            csocket_close(client_conn_sockfd);
            return (EC_FALSE);
        }

        chttp_node = chttp_node_new(CHTTP_TYPE_HANDLE_REQ);
        if(NULL_PTR == chttp_node)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_unix_accept_once: new chttp_node for sockfd %d failed\n", client_conn_sockfd);
            csocket_cnode_try_close(csocket_cnode);
            return (EC_FALSE);
        }
     
        CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/

        /*bind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = chttp_node;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = csocket_cnode;
     
        if(NULL_PTR != CSRV_INIT_CSOCKET_CNODE(csrv))
        {
            if(EC_FALSE == CSRV_INIT_CSOCKET_CNODE(csrv)(CSRV_MD_ID(csrv), csocket_cnode))
            {
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_unix_accept_once: csrv init sockfd %d failed\n", client_conn_sockfd);

                /*unbind csocket_cnode and chttp_node*/
                CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
                CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

                csocket_cnode_try_close(csocket_cnode);
                chttp_node_free(chttp_node);

                return (EC_FALSE);
            }
        }
     
        if(SWITCH_ON == HIGH_PRECISION_TIME_SWITCH)
        {
            task_brd_update_time_default();
        }
        CTMV_CLONE(task_brd_default_get_daytime(), CHTTP_NODE_START_TMV(chttp_node));     

        if(NULL_PTR != CSRV_ADD_CSOCKET_CNODE(csrv))
        {
            CSRV_ADD_CSOCKET_CNODE(csrv)(CSRV_MD_ID(csrv), csocket_cnode);
        }

        CSOCKET_CNODE_MODI(csocket_cnode) = CSRV_MD_ID(csrv);
     
        cepoll_set_event(task_brd_default_get_cepoll(),
                          CSOCKET_CNODE_SOCKFD(csocket_cnode),
                          CEPOLL_RD_EVENT,
                          (CEPOLL_EVENT_HANDLER)CSRV_RD_EVENT_HANDLER(csrv),
                          (void *)csocket_cnode);

       cepoll_set_complete(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_COMPLETE_HANDLER(csrv),
                           (void *)csocket_cnode);
                        
       cepoll_set_shutdown(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           (CEPOLL_EVENT_HANDLER)CSRV_CLOSE_HANDLER(csrv),
                           (void *)csocket_cnode);

       cepoll_set_timeout(task_brd_default_get_cepoll(),
                           CSOCKET_CNODE_SOCKFD(csocket_cnode),
                           CSRV_TIMEOUT_NSEC(csrv),
                           (CEPOLL_EVENT_HANDLER)CSRV_TIMEOUT_HANDLER(csrv),
                           (void *)csocket_cnode);

        csocket_cnode_stat_inc();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_srv_unix_accept_once: accept sockfd %d done\n", client_conn_sockfd);
    }

    (*continue_flag) = ret;
 
    return (EC_TRUE);
}

EC_BOOL chttp_srv_unix_accept(CSRV *csrv)
{
    UINT32   idx;
    UINT32   num;
    EC_BOOL  continue_flag;

    num = CSRV_ACCEPT_MAX_NUM;
    for(idx = 0; idx < num; idx ++)
    {
        if(EC_FALSE == chttp_srv_unix_accept_once(csrv, &continue_flag))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_srv_unix_accept: accept No. %ld client failed where expect %ld clients\n", idx, num);
            return (EC_FALSE);
        }

        if(EC_FALSE == continue_flag)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_srv_unix_accept: accept No. %ld client terminate where expect %ld clients\n", idx, num);
            break;
        }     
    }

    return (EC_TRUE);
}
/*---------------------------------------- COMMIT RESPONSE FOR EMITTING  ----------------------------------------*/
EC_BOOL chttp_commit_error_response(CHTTP_NODE *chttp_node)
{
    CSOCKET_CNODE * csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    if(NULL_PTR == csocket_cnode)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_commit_error_response: csocket_cnode of chttp_node %p is null\n", chttp_node);
        return (EC_FALSE);
    }
 
    cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT);

    /*exception would be handled in cepoll*/
    return cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                 (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_send_rsp, csocket_cnode);
}

EC_BOOL chttp_commit_error_request(CHTTP_NODE *chttp_node)
{ 
    /*cleanup request body and response body*/
    chttp_node_recv_clean(chttp_node);
    cbytes_clean(CHTTP_NODE_CONTENT_CBYTES(chttp_node));

    if(EC_FALSE == chttp_make_error_response(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_commit_error_request: make error response failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_commit_error_response(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_commit_error_request: commit error response failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

/*---------------------------------------- CONNECTION INIT and CLOSE HANDLER ----------------------------------------*/
EC_BOOL chttp_csocket_cnode_init(const UINT32 UNUSED(md_id), CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE    *chttp_node;
    http_parser_t *http_parser;

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_init: sockfd %d -> chttp_node is null\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    http_parser = CHTTP_NODE_PARSER(chttp_node);
    http_parser->data = (void *)chttp_node;
    return (EC_TRUE);
}

EC_BOOL chttp_csocket_cnode_close(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE *chttp_node;
    int sockfd;

    if(NULL_PTR == csocket_cnode)
    {
        return (EC_TRUE);
    }

    sockfd = CSOCKET_CNODE_SOCKFD(csocket_cnode);

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: no chttp_node, force to close socket %d\n", sockfd);
        csocket_cnode_force_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: no chttp_node, force to close socket %d done\n", sockfd);
        return (EC_TRUE); 
    }
 
    if(CHTTP_TYPE_HANDLE_REQ == CHTTP_NODE_TYPE(chttp_node))/*on server side*/
    {
        /*umount from defer request queue if necessary*/
        chttp_defer_request_queue_erase(chttp_node);

        if(BIT_TRUE == CHTTP_NODE_KEEPALIVE(chttp_node))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: [server] keep-alive, resume socket %d\n", sockfd);
            /*resume*/
            chttp_node_wait_resume(chttp_node);
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: [server] keep-alive, resume socket %d done\n", sockfd);

            return (EC_TRUE);
        }

        /* unbind */
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /*free*/
        chttp_node_free(chttp_node);
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: [server] not keep-alive, force to close socket %d\n", sockfd);
        csocket_cnode_force_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: [server] not keep-alive, force to close socket %d done\n", sockfd);

        return (EC_TRUE);
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node))/*on client side*/
    {
        /* unbind */
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /**
         * not free chttp_node but release ccond
         * which will pull routine to the starting point of sending http request
         **/
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0018);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: has chttp_node, try to close socket %d\n", sockfd);
        csocket_cnode_try_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_close: has chttp_node, try to close socket %d done\n", sockfd);
     
        return (EC_TRUE);
     
    }

    if(CHTTP_TYPE_HANDLE_CHECK == CHTTP_NODE_TYPE(chttp_node))/*on client side*/
    {
        /*not unbind*/
     
        /**
         * not free chttp_node but release ccond
         * which will pull routine to the starting point of sending http request
         **/
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0019);
        }

        return (EC_TRUE);
    }
 
    /*should never reacher here!*/

    /* unbind */
    CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
    CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_close:should never reach here, release chttp_node and try to close socket %d\n", sockfd);
    /*free*/
    chttp_node_free(chttp_node);
   
    csocket_cnode_force_close(csocket_cnode);
    csocket_cnode_stat_dec();
    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_close: should never reach here, release chttp_node and  try to close socket %d done\n", sockfd);
 
    return (EC_TRUE);
}

EC_BOOL chttp_csocket_cnode_force_close(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE *chttp_node;
    int sockfd;

    if(NULL_PTR == csocket_cnode)
    {
        return (EC_TRUE);
    }

    sockfd = CSOCKET_CNODE_SOCKFD(csocket_cnode);

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: no chttp_node, try to close socket %d\n", sockfd);
        csocket_cnode_force_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: no chttp_node, try to close socket %d done\n", sockfd);
        return (EC_TRUE);
    }
 
    if(CHTTP_TYPE_HANDLE_REQ == CHTTP_NODE_TYPE(chttp_node))
    {
        /*umount from defer request queue if necessary*/
        chttp_defer_request_queue_erase(chttp_node);

        /* unbind */
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /*free*/
        chttp_node_free(chttp_node);

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: released chttp_node, try to close socket %d\n", sockfd);
        csocket_cnode_force_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: released chttp_node, try to close socket %d done\n", sockfd);     

        return (EC_TRUE);
    }

    if(CHTTP_TYPE_HANDLE_RSP == CHTTP_NODE_TYPE(chttp_node))
    {
        /* unbind */
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /**
         * not free chttp_node but release ccond
         * which will pull routine to the starting point of sending http request
         **/
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0020);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: has chttp_node, try to close socket %d\n", sockfd);
        csocket_cnode_force_close(csocket_cnode);
        csocket_cnode_stat_dec();
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_force_close: has chttp_node, try to close socket %d done\n", sockfd);
        return (EC_TRUE);
     
    }

    if(CHTTP_TYPE_HANDLE_CHECK == CHTTP_NODE_TYPE(chttp_node))
    {
        /*not unbind*/
     
        /**
         * not free chttp_node but release ccond
         * which will pull routine to the starting point of sending http request
         **/
        if(NULL_PTR != CHTTP_NODE_CROUTINE_COND(chttp_node) && BIT_FALSE == CHTTP_NODE_COROUTINE_RESTORE(chttp_node))
        {
            CHTTP_NODE_COROUTINE_RESTORE(chttp_node) = BIT_TRUE;
            croutine_cond_release(CHTTP_NODE_CROUTINE_COND(chttp_node), LOC_CHTTP_0021);
        }

        return (EC_TRUE);
    }
 
    /*should never reacher here!*/

    /* unbind */
    CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
    CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_force_close:should never reach here, release chttp_node and try to close socket %d\n", sockfd);
    /*free*/
    chttp_node_free(chttp_node);
 
    csocket_cnode_force_close(csocket_cnode);
    csocket_cnode_stat_dec();
    dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_force_close: should never reach here, release chttp_node and  try to close socket %d done\n", sockfd);
    return (EC_TRUE);
}

/*---------------------------------------- SEND AND RECV MANAGEMENT  ----------------------------------------*/
EC_BOOL chttp_csocket_cnode_recv_req(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE   *chttp_node;
    EC_BOOL       ret;
 
    if(EC_FALSE == CSOCKET_CNODE_IS_CONNECTED(csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_req: sockfd %d is not connected\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_req: sockfd %d -> chttp_node is null\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    if(EC_TRUE == CSOCKET_CNODE_WORK_WAIT_RESUME(csocket_cnode))
    {
        CSOCKET_CNODE_WORK_WAIT_RESUME(csocket_cnode) = EC_FALSE;
        CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/
    }

    ret = chttp_node_recv(chttp_node, csocket_cnode);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_req: recv req on sockfd %d failed\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);                         
    }

    if(EC_DONE == ret)
    {
        if(BIT_FALSE == CHTTP_NODE_RECV_COMPLETE(chttp_node))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_req: sockfd %d, recv not completed => false\n",
                                CSOCKET_CNODE_SOCKFD(csocket_cnode));
     
            return (EC_FALSE);
        }
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_recv_req: sockfd %d, no more data to recv or parse\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));

        /*note: for http request, recv done means the whole request ready, and MUST NOT close the connection. hence return true*/
        return (EC_TRUE);
    }

    if(EC_FALSE == chttp_parse(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_req: parse on sockfd %d failed\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);                         
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_recv_req: sockfd %d, recv and parse done\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
    return (EC_TRUE);
}

EC_BOOL chttp_csocket_cnode_send_rsp(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE               *chttp_node;
 
    if(EC_FALSE == CSOCKET_CNODE_IS_CONNECTED(csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_rsp: sockfd %d is not connected\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_rsp: sockfd %d -> chttp_node is null\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    if(0)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGUSER07, "[DEBUG] sockfd %d, to send len: %ld, uri: %.*s\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode),
                       chttp_node_send_len(chttp_node),
                       CBUFFER_USED(CHTTP_NODE_URI(chttp_node)), (char *)CBUFFER_DATA(CHTTP_NODE_URI(chttp_node))); 
    }

    if(EC_FALSE == chttp_node_send(chttp_node, csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_rsp: sockfd %d send rsp failed\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    if(EC_TRUE == chttp_node_need_send(chttp_node))
    {
        /*wait for next writing*/

        /**********************************************************************************
        *   note:
        *       assume caller would set WR event when return EC_AGAIN.
        *       we cannot determine who will trigger send op:
        *         if cepoll trigger send op, WR event should not be set,
        *         if chttp trigger send op before WR event was set, chttp should set later
        *       thus, return EC_AGAIN is more safe.
        *
        ***********************************************************************************/
        return (EC_AGAIN);
    }

    if(NULL_PTR != CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node))/*for big file*/
    {
        /*unbind csocket_cnode and chttp_node*/
        cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                     (CEPOLL_EVENT_HANDLER)CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node), (void *)csocket_cnode);         
        return (EC_TRUE);/*wait for writing more*/
    }

    CHTTP_NODE_LOG_TIME_WHEN_END(chttp_node);
    CHTTP_NODE_LOG_PRINT(chttp_node);
 
    /*now all data had been sent out, del WR event and set RD event*/
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_send_rsp: sockfd %d had sent out all rsp data\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));  

    cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT);
 
    return (EC_DONE);/*return EC_DONE will trigger CEPOLL cleanup*/
}
/*---------------------------------------- REQUEST REST LIST MANAGEMENT ----------------------------------------*/
CHTTP_REST *chttp_rest_new()
{
    CHTTP_REST *chttp_rest;
    alloc_static_mem(MM_CHTTP_REST, &chttp_rest, LOC_CHTTP_0022);
    if(NULL_PTR == chttp_rest)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rest_new: new chttp_rest failed\n");
        return (NULL_PTR);
    }
    chttp_rest_init(chttp_rest);
    return (chttp_rest);
}
EC_BOOL chttp_rest_init(CHTTP_REST *chttp_rest)
{           
    CHTTP_REST_NAME(chttp_rest)   = NULL_PTR;
    CHTTP_REST_LEN(chttp_rest)    = 0;
    CHTTP_REST_COMMIT(chttp_rest) = NULL_PTR;
 
    return (EC_TRUE);
}

EC_BOOL chttp_rest_clean(CHTTP_REST *chttp_rest)
{
    CHTTP_REST_NAME(chttp_rest)   = NULL_PTR;
    CHTTP_REST_LEN(chttp_rest)    = 0;
    CHTTP_REST_COMMIT(chttp_rest) = NULL_PTR;
    return (EC_TRUE);
}

EC_BOOL chttp_rest_free(CHTTP_REST *chttp_rest)
{
    if(NULL_PTR != chttp_rest)
    {
        chttp_rest_clean(chttp_rest);
        free_static_mem(MM_CHTTP_REST, chttp_rest, LOC_CHTTP_0023);
    }
 
    return (EC_TRUE);
}

void chttp_rest_print(LOG *log, const CHTTP_REST *chttp_rest)
{
    sys_log(log, "chttp_rest_print: chttp_rest: %p, name '%s', commit %p\n", 
                 chttp_rest, 
                 CHTTP_REST_NAME(chttp_rest), 
                 CHTTP_REST_COMMIT(chttp_rest));

    return;
}

EC_BOOL chttp_rest_cmp(const CHTTP_REST *chttp_rest_1st, const CHTTP_REST *chttp_rest_2nd)
{   
    if(CHTTP_REST_LEN(chttp_rest_1st) != CHTTP_REST_LEN(chttp_rest_2nd))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_cmp: cmp: '%.*s' <----> '%.*s' [len not equal]\n",
                        CHTTP_REST_LEN(chttp_rest_1st), CHTTP_REST_NAME(chttp_rest_1st),
                        CHTTP_REST_LEN(chttp_rest_2nd), CHTTP_REST_NAME(chttp_rest_2nd));    
        return (EC_FALSE);
    }

    if(0 != STRNCASECMP(CHTTP_REST_NAME(chttp_rest_1st), CHTTP_REST_NAME(chttp_rest_2nd), CHTTP_REST_LEN(chttp_rest_1st)))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_cmp: cmp: '%.*s' <----> '%.*s' [name not equal]\n",
                        CHTTP_REST_LEN(chttp_rest_1st), CHTTP_REST_NAME(chttp_rest_1st),
                        CHTTP_REST_LEN(chttp_rest_2nd), CHTTP_REST_NAME(chttp_rest_2nd));    
        return (EC_FALSE);
    }
  
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_cmp: cmp: '%.*s' <----> '%.*s' [equal]\n",
                    CHTTP_REST_LEN(chttp_rest_1st), CHTTP_REST_NAME(chttp_rest_1st),
                    CHTTP_REST_LEN(chttp_rest_2nd), CHTTP_REST_NAME(chttp_rest_2nd));   

    return (EC_TRUE);
}

EC_BOOL chttp_rest_list_push(const char *name, EC_BOOL (*commit)(CHTTP_NODE *))
{
    CHTTP_REST *chttp_rest;
    CHTTP_REST *chttp_rest_t;

    if(NULL_PTR == g_chttp_rest_list)
    {
        g_chttp_rest_list = clist_new(MM_CHTTP_REST, LOC_CHTTP_0024);
        if(NULL_PTR == g_chttp_rest_list)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rest_list_push: new rest list failed\n");
            return (NULL_PTR);        
        }
    }

    chttp_rest = chttp_rest_new();
    if(NULL_PTR == chttp_rest)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rest_list_push: new chttp_rest failed\n");
        return (NULL_PTR);
    }

    CHTTP_REST_NAME(chttp_rest)   = name;
    CHTTP_REST_LEN(chttp_rest)    = strlen(name);
    CHTTP_REST_COMMIT(chttp_rest) = commit;

    chttp_rest_t = (CHTTP_REST *)clist_search_data_back(g_chttp_rest_list, (void *)chttp_rest, 
                                                    (CLIST_DATA_DATA_CMP)chttp_rest_cmp);
    if(NULL_PTR != chttp_rest_t)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_push: already exist rest %p ('%.*s', %p)\n",
                    chttp_rest_t,
                    CHTTP_REST_LEN(chttp_rest), CHTTP_REST_NAME(chttp_rest), 
                    CHTTP_REST_COMMIT(chttp_rest));
        chttp_rest_free(chttp_rest);            
        return (EC_TRUE);
    }
    
    clist_push_back(g_chttp_rest_list, (void *)chttp_rest);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_push: push rest %p ('%s', %p) done\n",
                    chttp_rest, name, commit);
        
    return (EC_TRUE);
}

CHTTP_REST *chttp_rest_list_pop(const char *name, const uint32_t len)
{
    CHTTP_REST   chttp_rest_t;
    CHTTP_REST  *chttp_rest;
    
    if(NULL_PTR == g_chttp_rest_list)
    {
        dbg_log(SEC_0149_CHTTP, 5)(LOGSTDOUT, "warn:chttp_rest_list_pop: rest list is null\n");
        return (NULL_PTR);
    }

    CHTTP_REST_NAME(&chttp_rest_t)   = name;
    CHTTP_REST_LEN(&chttp_rest_t)    = len;
    CHTTP_REST_COMMIT(&chttp_rest_t) = NULL_PTR;    

    chttp_rest = (CHTTP_REST *)clist_del(g_chttp_rest_list, (void *)&chttp_rest_t, 
                                            (CLIST_DATA_DATA_CMP)chttp_rest_cmp);

    if(NULL_PTR == chttp_rest)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_pop: not found rest of '%.*s'\n",
                    len, name);
        return (NULL_PTR);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_pop: found rest %p of ('%.*s', %p)\n",
                    chttp_rest,
                    CHTTP_REST_LEN(chttp_rest), CHTTP_REST_NAME(chttp_rest), 
                    CHTTP_REST_COMMIT(chttp_rest));

    return (chttp_rest);
}

CHTTP_REST *chttp_rest_list_find(const char *name, const uint32_t len)
{
    CHTTP_REST   chttp_rest_t;
    CHTTP_REST  *chttp_rest;
    
    if(NULL_PTR == g_chttp_rest_list)
    {
        dbg_log(SEC_0149_CHTTP, 5)(LOGSTDOUT, "warn:chttp_rest_list_find: rest list is null\n");
        return (NULL_PTR);
    }

    CHTTP_REST_NAME(&chttp_rest_t)   = name;
    CHTTP_REST_LEN(&chttp_rest_t)    = len;
    CHTTP_REST_COMMIT(&chttp_rest_t) = NULL_PTR;    

    chttp_rest = (CHTTP_REST *)clist_search_data_back(g_chttp_rest_list, (void *)&chttp_rest_t, 
                                                        (CLIST_DATA_DATA_CMP)chttp_rest_cmp);

    if(NULL_PTR == chttp_rest)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_find: not found rest of '%.*s'\n",
                    len, name);
        return (NULL_PTR);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rest_list_find: found rest %p of ('%.*s', %p)\n",
                    chttp_rest,
                    CHTTP_REST_LEN(chttp_rest),CHTTP_REST_NAME(chttp_rest), 
                    CHTTP_REST_COMMIT(chttp_rest));

    return (chttp_rest);
}

/*---------------------------------------- REQUEST DEFER QUEUE MANAGEMENT ----------------------------------------*/
EC_BOOL chttp_defer_request_queue_init()
{
    if(EC_FALSE == g_chttp_defer_request_queue_init_flag)
    {
        cqueue_init(&g_chttp_defer_request_queue, MM_CHTTP_NODE, LOC_CHTTP_0025);

        if(EC_FALSE == cepoll_set_loop_handler(task_brd_default_get_cepoll(),
                                               (CEPOLL_LOOP_HANDLER)chttp_defer_request_queue_launch,
                                               chttp_defer_request_commit))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_defer_request_queue_init: set cepoll loop handler failed\n");
            return (EC_FALSE);
        }
    }

    g_chttp_defer_request_queue_init_flag = EC_TRUE;
    return (EC_TRUE);
}

EC_BOOL chttp_defer_request_queue_clean()
{
    cqueue_clean(&g_chttp_defer_request_queue, (CQUEUE_DATA_DATA_CLEANER)chttp_node_free);
    return (EC_TRUE);
}

/**
*
* WARNING:
*
*   chttp_defer_request_queue_init is called in RFS module,
*   but chttp_defer_request_queue_is_empty is checked in task_brd_need_slow_down
*   where maybe chttp_defer_request_queue_init is not called or never be called!
*
*   Thus, one cannot call cqueue_is_empty is check whether g_chttp_defer_request_queue
*   is empty or not.
*
**/
EC_BOOL chttp_defer_request_queue_is_empty()
{
    if(EC_FALSE == g_chttp_defer_request_queue_init_flag)
    {
        return (EC_TRUE);
    }
    return cqueue_is_empty(&g_chttp_defer_request_queue);
}

EC_BOOL chttp_defer_request_queue_push(CHTTP_NODE *chttp_node)
{
    CQUEUE_DATA *cqueue_data;

    cqueue_data = cqueue_push(&g_chttp_defer_request_queue, (void *)chttp_node);
    CHTTP_NODE_CQUEUE_DATA(chttp_node) = cqueue_data;
    return (EC_TRUE);
}

EC_BOOL chttp_defer_request_queue_erase(CHTTP_NODE *chttp_node)
{
    CQUEUE_DATA *cqueue_data;
 
    cqueue_data = CHTTP_NODE_CQUEUE_DATA(chttp_node);
    if(NULL_PTR != cqueue_data)
    {
        cqueue_erase(&g_chttp_defer_request_queue, cqueue_data);
        CHTTP_NODE_CQUEUE_DATA(chttp_node) = NULL_PTR;
    }
    return (EC_TRUE);
}

CHTTP_NODE *chttp_defer_request_queue_pop()
{
    CHTTP_NODE *chttp_node;

    chttp_node = (CHTTP_NODE *)cqueue_pop(&g_chttp_defer_request_queue);
    CHTTP_NODE_CQUEUE_DATA(chttp_node) = NULL_PTR;
    return (chttp_node);
}

CHTTP_NODE *chttp_defer_request_queue_peek()
{
    return (CHTTP_NODE *)cqueue_front(&g_chttp_defer_request_queue);
}

EC_BOOL chttp_defer_request_commit(CHTTP_NODE *chttp_node)
{
    CBUFFER *uri_cbuffer;
    CHTTP_REST    *chttp_rest;
    const char    *rest_name;
    uint32_t       rest_len;
 
    uri_cbuffer  = CHTTP_NODE_URI(chttp_node);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_defer_request_commit: uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));

    rest_name = (char *)CBUFFER_DATA(uri_cbuffer);
    if('/' != rest_name[ 0 ])
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_defer_request_commit: invalid url '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));
        return (EC_FALSE);                    
    }
    
    for(rest_len = 1; rest_len < CBUFFER_USED(uri_cbuffer); rest_len ++)
    {
        if('/' == rest_name[ rest_len ])
        {
            break;
        }
    }

    if('/' != rest_name[ rest_len ])
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_defer_request_commit: invalid rest '%.*s' [len %d]\n",
                            rest_len, rest_name, rest_len);
        return (EC_FALSE);                    
    }

    chttp_rest = chttp_rest_list_find(rest_name, rest_len);
    if(NULL_PTR == chttp_rest)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_defer_request_commit: not support rest '%.*s' [len %d]\n",
                            rest_len, rest_name, rest_len);
        return (EC_FALSE);                    
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_defer_request_commit: found rest %p of ('%.*s', %p)\n",
                    chttp_rest,
                    CHTTP_REST_LEN(chttp_rest),CHTTP_REST_NAME(chttp_rest), 
                    CHTTP_REST_COMMIT(chttp_rest));

    /*shift out rest tag*/
    cbuffer_left_shift_out(uri_cbuffer, NULL_PTR, rest_len);  
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_defer_request_commit: after left shift out rest tag, uri: '%.*s' [len %d]\n",
                        CBUFFER_USED(uri_cbuffer),
                        CBUFFER_DATA(uri_cbuffer),
                        CBUFFER_USED(uri_cbuffer));
                    
    return CHTTP_REST_COMMIT(chttp_rest)(chttp_node);
}

EC_BOOL chttp_defer_request_queue_launch(CHTTP_NODE_COMMIT_REQUEST chttp_node_commit_request)
{
    CHTTP_NODE *chttp_node;
    uint32_t    http_req_max_num; /*max http requests could be handled in one loop. 0 means handle all*/
    uint32_t    http_req_idx;

    if(SWITCH_ON == NGX_BGN_SWITCH)
    {
        http_req_max_num = NGX_HTTP_REQ_NUM_PER_LOOP;
    }
    else
    {
        http_req_max_num = RFS_HTTP_REQ_NUM_PER_LOOP;
    }

    if(0 == http_req_max_num)
    {
        http_req_max_num = (uint32_t)~0; /*max uint32_t value*/
    }

    /*
    * note: loop_switch control the loop times:
    *       for NGX BGN (SWITCH_ON == NGX_BGN_SWITCH), handle all http requests in one loop
    *       for RFS (SWITCH_OFF == NGX_BGN_SWITCH), handle one http request only in one loop
    */
    for(http_req_idx = 0; http_req_idx < http_req_max_num; http_req_idx ++)
    {
        EC_BOOL ret;
     
        chttp_node = chttp_defer_request_queue_peek();
        if(NULL_PTR == chttp_node)/*no more*/
        {
            break;
        }

        ret = chttp_defer_request_commit(chttp_node);

        /*ret = chttp_node_commit_request(chttp_node);*//*call back*/
        if(EC_BUSY == ret)/*okay, no routine resource to load this task, terminate and wait for next time try*/
        {
            break;
        }

        /*pop it when everything ok or some unknown scenario happen*/
        chttp_defer_request_queue_pop();

        if(EC_FALSE == ret)/*Oops! found unknown request, dicard it now*/
        {
            CSOCKET_CNODE *csocket_cnode;
        
            csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
            if(NULL_PTR == csocket_cnode)
            {
                /*actually, should never reach here*/
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                        "error:chttp_defer_request_queue_launch: csocket_cnode in chttp_node %p is null\n",
                        chttp_node);         
             
                chttp_node_free(chttp_node);
            }
            else
            {
                int sockfd;

                sockfd = CSOCKET_CNODE_SOCKFD(csocket_cnode);/*csocket_cnode will be cleanup, save sockfd at first*/
                dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_defer_request_queue_launch: try to close socket %d\n", sockfd);

                CHTTP_NODE_KEEPALIVE(chttp_node) = BIT_FALSE; /*force to close the http connection*/

                /*warning: do not remove cepoll_del_all due to chttp_csocket_cnode_close may be called out side of cepoll.c*/
                cepoll_del_all(task_brd_default_get_cepoll(), sockfd);/*del all before csocket_cnode closing*/
                chttp_csocket_cnode_close(csocket_cnode);/*hence we have to call cepoll_del_all in chttp_csocket_cnode_close*/
                cepoll_clear_node(task_brd_default_get_cepoll(), sockfd);
            }
        }

        /*handle next request*/
    }
    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * CHTTP_REQ and CHTTP_RSP interfaces
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/
CHTTP_REQ *chttp_req_new()
{
    CHTTP_REQ *chttp_req;
    alloc_static_mem(MM_CHTTP_REQ, &chttp_req, LOC_CHTTP_0026);
    if(NULL_PTR == chttp_req)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_req_new: new chttp_req failed\n");
        return (NULL_PTR);
    }
    chttp_req_init(chttp_req);
    return (chttp_req);
}
EC_BOOL chttp_req_init(CHTTP_REQ *chttp_req)
{           
    CHTTP_REQ_IPADDR(chttp_req) = CMPI_ERROR_IPADDR;
    CHTTP_REQ_PORT(chttp_req)   = CMPI_ERROR_SRVPORT;

    CHTTP_REQ_SSL_FLAG(chttp_req) = EC_FALSE;

    cstring_init(CHTTP_REQ_METHOD(chttp_req), NULL_PTR);
    cstring_init(CHTTP_REQ_URI(chttp_req), NULL_PTR);

    cstrkv_mgr_init(CHTTP_REQ_PARAM(chttp_req));
    cstrkv_mgr_init(CHTTP_REQ_HEADER(chttp_req));

    cbytes_init(CHTTP_REQ_BODY(chttp_req));
 
    return (EC_TRUE);
}

EC_BOOL chttp_req_clean(CHTTP_REQ *chttp_req)
{
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_req_clean: chttp_req: %p\n", chttp_req);
    CHTTP_REQ_IPADDR(chttp_req) = CMPI_ERROR_IPADDR;
    CHTTP_REQ_PORT(chttp_req)   = CMPI_ERROR_SRVPORT;

    CHTTP_REQ_SSL_FLAG(chttp_req) = EC_FALSE;

    cstring_clean(CHTTP_REQ_METHOD(chttp_req));
    cstring_clean(CHTTP_REQ_URI(chttp_req));

    cstrkv_mgr_clean(CHTTP_REQ_PARAM(chttp_req));
    cstrkv_mgr_clean(CHTTP_REQ_HEADER(chttp_req));

    cbytes_clean(CHTTP_REQ_BODY(chttp_req));
    return (EC_TRUE);
}

EC_BOOL chttp_req_free(CHTTP_REQ *chttp_req)
{
    if(NULL_PTR != chttp_req)
    {
        chttp_req_clean(chttp_req);
        free_static_mem(MM_CHTTP_REQ, chttp_req, LOC_CHTTP_0027);
    }
 
    return (EC_TRUE);
}

void chttp_req_print(LOG *log, const CHTTP_REQ *chttp_req)
{
    sys_log(log, "chttp_req_print: chttp_req: %p\n", chttp_req);
    sys_log(log, "chttp_req_print: ipaddr: %s\n", CHTTP_REQ_IPADDR_STR(chttp_req));
    sys_log(log, "chttp_req_print: port: %ld\n" , CHTTP_REQ_PORT(chttp_req));
    sys_log(log, "chttp_req_print: ssl: %s\n" , c_bool_str(CHTTP_REQ_SSL_FLAG(chttp_req)));

    sys_log(log, "chttp_req_print: method: %.*s\n", cstring_get_len(CHTTP_REQ_METHOD(chttp_req)), cstring_get_str(CHTTP_REQ_METHOD(chttp_req)));
    sys_log(log, "chttp_req_print: uri: %.*s\n"   , cstring_get_len(CHTTP_REQ_URI(chttp_req)), cstring_get_str(CHTTP_REQ_URI(chttp_req)));

    sys_log(log, "chttp_req_print: param: \n");
    cstrkv_mgr_print(log, CHTTP_REQ_PARAM(chttp_req));

    sys_log(log, "chttp_req_print: header: \n");
    cstrkv_mgr_print(log, CHTTP_REQ_HEADER(chttp_req));

    sys_log(log, "chttp_req_print: body: len = %ld\n", cbytes_len(CHTTP_REQ_BODY(chttp_req)));

    //cbytes_print_chars(log, CHTTP_REQ_BODY(chttp_req));

    return;
}

static void __chttp_req_header_print_plain(LOG *log, const CHTTP_REQ *chttp_req)
{
    CLIST_DATA *clist_data;
    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(CHTTP_REQ_HEADER(chttp_req)), clist_data)
    {
        CSTRKV *cstrkv;

        cstrkv = (CSTRKV *)CLIST_DATA_DATA(clist_data);
        if(EC_FALSE == cstring_is_empty(CSTRKV_VAL(cstrkv)))
        {
            sys_print(log, "%s: %s\n", CSTRKV_KEY_STR(cstrkv), CSTRKV_VAL_STR(cstrkv));
        }
        else
        {
            sys_print(log, "%s: \n", CSTRKV_KEY_STR(cstrkv));
        }
    }
    return;
}

static void __chttp_req_param_print_plain(LOG *log, const CHTTP_REQ *chttp_req)
{
    CLIST_DATA *clist_data;
    EC_BOOL flag;

    flag = EC_FALSE;

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(CHTTP_REQ_PARAM(chttp_req)), clist_data)
    {
        CSTRKV *cstrkv;
        char   *prefix;

        cstrkv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == flag)
        {
            flag = EC_TRUE;
            prefix = "?";
        }
        else
        {
            prefix = "&";
        }

        if(EC_FALSE == cstring_is_empty(CSTRKV_VAL(cstrkv)))
        {
            sys_print(log, "%s%s=%s", prefix, CSTRKV_KEY_STR(cstrkv), CSTRKV_VAL_STR(cstrkv));
        }
        else
        {
            sys_print(log, "%s%s", prefix, CSTRKV_KEY_STR(cstrkv));
        }
    }
    return;
}

void chttp_req_print_plain(LOG *log, const CHTTP_REQ *chttp_req)
{
    sys_log(log, "chttp_req_print_plain: chttp_req: %p => %s:%ld\n", chttp_req, 
                 CHTTP_REQ_IPADDR_STR(chttp_req), CHTTP_REQ_PORT(chttp_req));

    sys_print(log, "%s", (const char *)cstring_get_str(CHTTP_REQ_METHOD(chttp_req)));
                
    sys_print(log, " %s", (const char *)cstring_get_str(CHTTP_REQ_URI(chttp_req)));

    __chttp_req_param_print_plain(log, chttp_req);

    sys_print(log, " HTTP/1.1\n");

    __chttp_req_header_print_plain(log, chttp_req);
    
    sys_print(log, "%.*s\n", 
                 (uint32_t)cbytes_len(CHTTP_REQ_BODY(chttp_req)), 
                 (char *)cbytes_buf(CHTTP_REQ_BODY(chttp_req))
                 );            
    return;
}

static EC_BOOL __chttp_req_resolve_host(const char *host, UINT32 *ip)
{
    EC_BOOL ret;

    CDNS_REQ cdns_req;
    CDNS_RSP cdns_rsp;

    CDNS_RSP_NODE *cdns_rsp_node;
 
    if(EC_TRUE == c_ipv4_is_ok(host))
    {
        (*ip) = c_ipv4_to_word(host);
        return (EC_TRUE);
    }

    cdns_req_init(&cdns_req);
    cdns_rsp_init(&cdns_rsp);

    CDNS_REQ_IPADDR(&cdns_req) = c_ipv4_to_word("127.0.0.1");/*default*/
    CDNS_REQ_PORT(&cdns_req)   = 53; /*default*/

    cstring_set_str(CDNS_REQ_HOST(&cdns_req), (UINT8 *)host);

    ret = cdns_request(&cdns_req, &cdns_rsp);/*block here*/

    cstring_unset(CDNS_REQ_HOST(&cdns_req));
 
    if(EC_FALSE == ret)
    {
        cdns_req_clean(&cdns_req);
        cdns_rsp_clean(&cdns_rsp);
        return (EC_FALSE);
    }

    cdns_rsp_node = clist_first_data(&cdns_rsp);
    (*ip) = c_ipv4_to_word((char *)CDNS_RSP_NODE_IPADDR_STR(cdns_rsp_node));
 
    cdns_req_clean(&cdns_req);
    cdns_rsp_clean(&cdns_rsp);
    return (EC_TRUE);
}

EC_BOOL chttp_req_set_server(CHTTP_REQ *chttp_req, const char *server)
{
    char   server_saved[128];
    char  *fields[2];
    UINT32 ipaddr;
    size_t len;

    len = strlen(server);
    if(len >= sizeof(server_saved)/sizeof(server_saved[0]))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_req_set_server: server '%s' too long\n",
                            server);
        return (EC_FALSE);
    }
 
    BCOPY(server, (char *)server_saved, len + 1);
 
    if(2 != c_str_split(server_saved, ":", fields, 2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_req_set_server: invalid server '%s'\n",
                            server_saved);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_req_set_server: try to resolve host '%s'\n", fields[0]);
    if(EC_FALSE == __chttp_req_resolve_host(fields[0], &ipaddr))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_req_set_server: resolve host '%s' failed\n",
                            fields[0]);
        return (EC_FALSE);
    }
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_req_set_server: resolve host '%s' => %s\n", fields[0], c_word_to_ipv4(ipaddr));
 
    CHTTP_REQ_IPADDR(chttp_req) = ipaddr;
    CHTTP_REQ_PORT(chttp_req)   = c_str_to_word(fields[1]);

    return (EC_TRUE);
}

EC_BOOL chttp_req_set_ipaddr_word(CHTTP_REQ *chttp_req, const UINT32 ipaddr)
{
    CHTTP_REQ_IPADDR(chttp_req) = ipaddr;
    return (EC_TRUE);
}

EC_BOOL chttp_req_set_port_word(CHTTP_REQ *chttp_req, const UINT32 port)
{
    CHTTP_REQ_PORT(chttp_req) = port;
    return (EC_TRUE);
}

EC_BOOL chttp_req_set_ipaddr(CHTTP_REQ *chttp_req, const char *ipaddr)
{
    UINT32 ip;

    if(EC_FALSE == __chttp_req_resolve_host(ipaddr, &ip))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_req_set_ipaddr: resolve host '%s' failed\n",
                            ipaddr);
        return (EC_FALSE);
    }
 
    CHTTP_REQ_IPADDR(chttp_req) = ip;

    return (EC_TRUE);
}

EC_BOOL chttp_req_set_port(CHTTP_REQ *chttp_req, const char *port)
{
    CHTTP_REQ_PORT(chttp_req) = c_str_to_word(port);

    return (EC_TRUE);
}

EC_BOOL chttp_req_set_method(CHTTP_REQ *chttp_req, const char *method)
{
    cstring_append_str(CHTTP_REQ_METHOD(chttp_req), (UINT8 *)method);
    return (EC_TRUE);
}

EC_BOOL chttp_req_set_uri(CHTTP_REQ *chttp_req, const char *uri)
{
    cstring_append_str(CHTTP_REQ_URI(chttp_req), (UINT8 *)uri);
    return (EC_TRUE);
}

EC_BOOL chttp_req_add_param(CHTTP_REQ *chttp_req, const char *k, const char *v)
{
    return cstrkv_mgr_add_kv_str(CHTTP_REQ_PARAM(chttp_req), k, v);
}

EC_BOOL chttp_req_add_header(CHTTP_REQ *chttp_req, const char *k, const char *v)
{
    return cstrkv_mgr_add_kv_str(CHTTP_REQ_HEADER(chttp_req), k, v);
}

EC_BOOL chttp_req_add_header_chars(CHTTP_REQ *chttp_req, const char *k, const uint32_t klen, const char *v, const uint32_t vlen)
{
    return cstrkv_mgr_add_kv_chars(CHTTP_REQ_HEADER(chttp_req), k, klen, v, vlen);
}

EC_BOOL chttp_req_del_header(CHTTP_REQ *chttp_req, const char *k)
{
    return cstrkv_mgr_del_key_str(CHTTP_REQ_HEADER(chttp_req), k);
}

EC_BOOL chttp_req_renew_header(CHTTP_REQ *chttp_req, const char *k, const char *v)
{
    if(NULL_PTR == k)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_req_renew_header: k is null => no header renewed\n");
        return (EC_FALSE);
    }
 
    chttp_req_del_header(chttp_req, k);

    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_req_renew_header: v is null => header ['%s'] was deleted only\n");
        return (EC_FALSE);
    }
 
    chttp_req_add_header(chttp_req, k, v);

    return (EC_TRUE);
}

EC_BOOL chttp_req_set_body(CHTTP_REQ *chttp_req, const uint8_t *data, const uint32_t len)
{
    return cbytes_set(CHTTP_REQ_BODY(chttp_req), data, len);
}

EC_BOOL chttp_req_clone(CHTTP_REQ *chttp_req_des, const CHTTP_REQ *chttp_req_src)
{
    CHTTP_REQ_IPADDR(chttp_req_des) = CHTTP_REQ_IPADDR(chttp_req_src);
    CHTTP_REQ_PORT(chttp_req_des)   = CHTTP_REQ_PORT(chttp_req_src);
 
    CHTTP_REQ_SSL_FLAG(chttp_req_des) = CHTTP_REQ_SSL_FLAG(chttp_req_src);

    cstring_clone(CHTTP_REQ_METHOD(chttp_req_src), CHTTP_REQ_METHOD(chttp_req_des));
    cstring_clone(CHTTP_REQ_URI(chttp_req_src), CHTTP_REQ_URI(chttp_req_des));

    cstrkv_mgr_clone(CHTTP_REQ_PARAM(chttp_req_des), CHTTP_REQ_PARAM(chttp_req_src));
    cstrkv_mgr_clone(CHTTP_REQ_HEADER(chttp_req_des), CHTTP_REQ_HEADER(chttp_req_src));

    cbytes_clone(CHTTP_REQ_BODY(chttp_req_src), CHTTP_REQ_BODY(chttp_req_des));

    return (EC_TRUE);
}

EC_BOOL chttp_req_has_body(const CHTTP_REQ *chttp_req)
{
    if(EC_TRUE == cbytes_is_empty(CHTTP_REQ_BODY(chttp_req)))
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

CHTTP_RSP *chttp_rsp_new()
{
    CHTTP_RSP *chttp_rsp;

    alloc_static_mem(MM_CHTTP_RSP, &chttp_rsp, LOC_CHTTP_0028);
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_new: new chttp_rsp failed\n");
        return (NULL_PTR);
    }

    chttp_rsp_init(chttp_rsp);
    return (chttp_rsp);
}
EC_BOOL chttp_rsp_init(CHTTP_RSP *chttp_rsp)
{
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rsp_init: chttp_rsp: %p\n", chttp_rsp);
    CHTTP_RSP_STATUS(chttp_rsp) = CHTTP_STATUS_NONE;
 
    cstrkv_mgr_init(CHTTP_RSP_HEADER(chttp_rsp));

    cbytes_init(CHTTP_RSP_BODY(chttp_rsp)); 

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_clean(CHTTP_RSP *chttp_rsp)
{
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rsp_clean: chttp_rsp: %p\n", chttp_rsp);
    CHTTP_RSP_STATUS(chttp_rsp) = CHTTP_STATUS_NONE;
 
    cstrkv_mgr_clean(CHTTP_RSP_HEADER(chttp_rsp));

    cbytes_clean(CHTTP_RSP_BODY(chttp_rsp));
    return (EC_TRUE);
}

EC_BOOL chttp_rsp_free(CHTTP_RSP *chttp_rsp)
{
    if(NULL_PTR != chttp_rsp)
    {
        chttp_rsp_clean(chttp_rsp);
        free_static_mem(MM_CHTTP_RSP, chttp_rsp, LOC_CHTTP_0029);
    }
 
    return (EC_TRUE);
}

void chttp_rsp_print(LOG *log, const CHTTP_RSP *chttp_rsp)
{
    sys_log(log, "chttp_rsp_print: chttp_rsp: %p\n", chttp_rsp);
    sys_log(log, "chttp_rsp_print: status: %u\n", CHTTP_RSP_STATUS(chttp_rsp));
 
    sys_log(log, "chttp_rsp_print: header: \n");
    cstrkv_mgr_print(log, CHTTP_RSP_HEADER(chttp_rsp));

    sys_log(log, "chttp_rsp_print: body: \n");
    sys_log(log, "chttp_rsp_print: body: len = %ld\n", cbytes_len(CHTTP_RSP_BODY(chttp_rsp)));
    //cbytes_print_chars(log, CHTTP_RSP_BODY(chttp_rsp));

    return;
}

static void __chttp_rsp_header_print_plain(LOG *log, const CHTTP_RSP *chttp_rsp)
{
    CLIST_DATA *clist_data;
    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(CHTTP_RSP_HEADER(chttp_rsp)), clist_data)
    {
        CSTRKV *cstrkv;

        cstrkv = (CSTRKV *)CLIST_DATA_DATA(clist_data);
        if(EC_FALSE == cstring_is_empty(CSTRKV_VAL(cstrkv)))
        {
            sys_print(log, "%s: %s\n", CSTRKV_KEY_STR(cstrkv), CSTRKV_VAL_STR(cstrkv));
        }
        else
        {
            sys_print(log, "%s: \n", CSTRKV_KEY_STR(cstrkv));
        }
    }
    return;
}

void chttp_rsp_print_plain(LOG *log, const CHTTP_RSP *chttp_rsp)
{
    sys_log(log, "chttp_rsp_print_plain: chttp_rsp: %p\n", chttp_rsp);
    sys_print(log, "HTTP/1.1 status %u %s\n", 
                     CHTTP_RSP_STATUS(chttp_rsp),
                     chttp_status_str_get(CHTTP_RSP_STATUS(chttp_rsp)));
    
    __chttp_rsp_header_print_plain(log, chttp_rsp);

    sys_print(log, "... [body len %ld] ...\n", (uint32_t)cbytes_len(CHTTP_RSP_BODY(chttp_rsp)));
#if 0    
    sys_print(log, "%.*s\n", 
                 (uint32_t)cbytes_len(CHTTP_RSP_BODY(chttp_rsp)), 
                 (char *)cbytes_buf(CHTTP_RSP_BODY(chttp_rsp))
                 );
#endif                 
    return;
}

EC_BOOL chttp_rsp_is_chunked(const CHTTP_RSP *chttp_rsp)
{
    char *transfer_encoding;
    transfer_encoding = cstrkv_mgr_get_val_str_ignore_case(CHTTP_RSP_HEADER(chttp_rsp), (const char *)"Transfer-Encoding");
    if(NULL_PTR == transfer_encoding)
    {
        return (EC_FALSE);
    }

    if(0 != STRCASECMP(transfer_encoding, (const char *)"chunked"))
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_add_header(CHTTP_RSP *chttp_rsp, const char *k, const char *v)
{
    if(NULL_PTR == k)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_rsp_add_header: k is null => no header added\n");
        return (EC_FALSE);
    }

    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_rsp_add_header: v is null => header ['%s'] = '' not added\n");
        return (EC_FALSE);
    } 
    return cstrkv_mgr_add_kv_str(CHTTP_RSP_HEADER(chttp_rsp), k, v);
}

char *chttp_rsp_get_header(const CHTTP_RSP *chttp_rsp, const char *k)
{
    return cstrkv_mgr_get_val_str_ignore_case(CHTTP_RSP_HEADER(chttp_rsp), k);
}

EC_BOOL chttp_rsp_del_header(CHTTP_RSP *chttp_rsp, const char *k)
{
    return cstrkv_mgr_del_key_str(CHTTP_RSP_HEADER(chttp_rsp), k);
}

EC_BOOL chttp_rsp_renew_header(CHTTP_RSP *chttp_rsp, const char *k, const char *v)
{
    if(NULL_PTR == k)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_rsp_renew_header: k is null => no header renewed\n");
        return (EC_FALSE);
    }
 
    chttp_rsp_del_header(chttp_rsp, k);

    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_rsp_renew_header: v is null => header ['%s'] was deleted only\n");
        return (EC_FALSE);
    }
 
    chttp_rsp_add_header(chttp_rsp, k, v);

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_has_header_key(CHTTP_RSP *chttp_rsp, const char *k)
{
    char *val;

    val = cstrkv_mgr_get_val_str_ignore_case(CHTTP_RSP_HEADER(chttp_rsp), k);
    if(NULL_PTR == val)
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_has_header(CHTTP_RSP *chttp_rsp, const char *k, const char *v)
{
    char *val;

    val = cstrkv_mgr_get_val_str_ignore_case(CHTTP_RSP_HEADER(chttp_rsp), k);
    if(NULL_PTR == val)
    {
        return (EC_FALSE);
    }

    if(NULL_PTR == v || 0 == STRCASECMP(val, v))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL chttp_rsp_fetch_header(CHTTP_RSP *chttp_rsp, const char *k, CSTRKV_MGR *cstrkv_mgr)
{
    char   * v;
 
    v = chttp_rsp_get_header(chttp_rsp, k);
    if(NULL_PTR != v)
    {
        cstrkv_mgr_add_kv_str(cstrkv_mgr, k, v);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_rsp_fetch_headers(CHTTP_RSP *chttp_rsp, const char *keys, CSTRKV_MGR *cstrkv_mgr)
{
    char    *s;
    char    *k[ 16 ];
 
    UINT32   num;
    UINT32   idx;

    s = c_str_dup(keys);
    if(NULL_PTR == s)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_fetch_headers: dup '%s' failed\n", keys);
        return (EC_FALSE);
    }

    num = c_str_split(s, ":;", k, sizeof(k)/sizeof(k[0]));
    for(idx = 0; idx < num; idx ++)
    {
        char   * v;
     
        v = chttp_rsp_get_header(chttp_rsp, k[ idx ]);
        if(NULL_PTR != v)
        {
            cstrkv_mgr_add_kv_str(cstrkv_mgr, k[ idx ], v);
        } 
    }

    safe_free(s, LOC_CHTTP_0030);

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_has_body(const CHTTP_RSP *chttp_rsp)
{
    if(EC_TRUE == cbytes_is_empty(CHTTP_RSP_BODY(chttp_rsp)))
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}
/*---------------------------------------- HTTP RESPONSE PASER INTERFACE ----------------------------------------*/
static int __chttp_rsp_on_message_begin(http_parser_t* http_parser)
{
    CHTTP_RSP *chttp_rsp;

    chttp_rsp= (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_message_begin: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_rsp_on_message_begin: chttp_rsp %p, ***MESSAGE BEGIN***\n",
                    chttp_rsp);
    return (0);
}

/**
*
* refer http parser case s_headers_almost_done
*
* if return 0, succ
* if return 1, SKIP BODY
* otherwise, error
*
**/
static int __chttp_rsp_on_headers_complete(http_parser_t* http_parser, const char* last, size_t length)
{
    CHTTP_RSP    *chttp_rsp;

    chttp_rsp = (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_headers_complete: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    return (0);/*succ*/
}

static int __chttp_rsp_on_message_complete(http_parser_t* http_parser)
{
    CHTTP_RSP    *chttp_rsp;

    chttp_rsp = (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_message_complete: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_rsp_on_message_complete: ***MESSAGE COMPLETE***\n");
    return (0);
}

static int __chttp_rsp_on_url(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_RSP    *chttp_rsp;

    chttp_rsp= (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_url: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    return (0);
}

/*only for http response*/
static int __chttp_rsp_on_status(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_RSP     *chttp_rsp;

    chttp_rsp = (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_status: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    CHTTP_RSP_STATUS(chttp_rsp) = http_parser->status_code;

    return (0);
}

static int __chttp_rsp_on_header_field(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_RSP    *chttp_rsp;
    CSTRKV *cstrkv;

    chttp_rsp= (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_header_field: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    cstrkv = cstrkv_new(NULL_PTR, NULL_PTR);
    if(NULL_PTR == cstrkv)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_header_field: new cstrkv failed where header field: %.*s\n",
                           (int)length, at);
        return (-1);
    }

    cstrkv_set_key_bytes(cstrkv, (const uint8_t *)at, (uint32_t)length);
    cstrkv_mgr_add_kv(CHTTP_RSP_HEADER(chttp_rsp), cstrkv);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_rsp_on_header_field: chttp_rsp %p, Header field: '%.*s'\n", chttp_rsp, (int)length, at);
    return (0);
}

static int __chttp_rsp_on_header_value(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_RSP    *chttp_rsp;
    CSTRKV       *cstrkv;

    chttp_rsp= (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_header_value: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }
 
    cstrkv = cstrkv_mgr_last_kv(CHTTP_RSP_HEADER(chttp_rsp));
    if(NULL_PTR == cstrkv)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_header_value: no cstrkv existing where value field: %.*s\n",
                           (int)length, at);
        return (-1);
    }

    cstrkv_set_val_bytes(cstrkv, (const uint8_t *)at, (uint32_t)length);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_rsp_on_header_value: chttp_rsp %p, Header value: '%.*s'\n", chttp_rsp, (int)length, at);
 
    return (0);
}

static int __chttp_rsp_on_body(http_parser_t* http_parser, const char* at, size_t length)
{
    CHTTP_RSP    *chttp_rsp;

    chttp_rsp= (CHTTP_RSP *)http_parser->data;
    if(NULL_PTR == chttp_rsp)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_body: http_parser %p -> chttp_rsp is null\n", http_parser);
        return (-1);/*error*/
    }

    if(EC_FALSE == cbytes_append(CHTTP_RSP_BODY(chttp_rsp), (uint8_t *)at, length))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_rsp_on_body: append %d bytes failed\n", length);
        return (-1);
    }
 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_rsp_on_body: chttp_rsp %p, len %d => body parsed %ld\n",
                    chttp_rsp, length, CBYTES_LEN(CHTTP_RSP_BODY(chttp_rsp)));
    return (0);
}


/*
*   decode http response from cbytes.
*/
EC_BOOL chttp_rsp_decode(CHTTP_RSP *chttp_rsp, const uint8_t *data, const uint32_t data_len)
{
    http_parser_t           http_parser;
    http_parser_settings_t  http_parser_setting;

    uint8_t                *ch;

    uint32_t                header_len;
    uint32_t                body_len;
    uint32_t                parsed_len;
    uint32_t                flag;

    /*check validity*/
    if(4 >= data_len)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_decode: invalid data_len %u\n", data_len);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rsp_decode: data_len %u\n", data_len);

    flag = BIT_FALSE;
    for(ch = (uint8_t *)data, header_len = 0; header_len <= data_len - 4; ch ++, header_len ++)
    {
        if('\r' == ch[ 0 ] && '\n' == ch[ 1 ] && '\r' == ch[ 2 ] && '\n' == ch[ 3 ])
        {
            flag = BIT_TRUE;
            break;
        }
    }

    if(BIT_FALSE == flag)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_decode: invalid header '%.*s'\n", data_len, (char *)data);
        return (EC_FALSE);
    }
 
    header_len += 4;
    body_len    = data_len - header_len;

    http_parser_init(&http_parser, HTTP_RESPONSE);
    http_parser.data  = (void *)chttp_rsp;

    if(5 < data_len
    && 'H' == data[0] && 'T' == data[1] && 'T' == data[2] && 'P' == data[3] && '/' == data[4])
    {
        http_parser.state = s_start_res;
    }
    else
    {
        http_parser.state = s_header_field; /*no heading line likes as 'HTTP/1.1 200 OK'*/
    }

    http_parser_setting.on_message_begin    = __chttp_rsp_on_message_begin;
    http_parser_setting.on_url              = __chttp_rsp_on_url;
    http_parser_setting.on_status           = __chttp_rsp_on_status;
    http_parser_setting.on_header_field     = __chttp_rsp_on_header_field;
    http_parser_setting.on_header_value     = __chttp_rsp_on_header_value;
    http_parser_setting.on_headers_complete = __chttp_rsp_on_headers_complete;
    http_parser_setting.on_body             = __chttp_rsp_on_body;
    http_parser_setting.on_message_complete = __chttp_rsp_on_message_complete;


    parsed_len = http_parser_execute(&http_parser, &http_parser_setting, (char *)data, header_len);
    /*check parser error*/
    if(HPE_OK != HTTP_PARSER_ERRNO(&http_parser)
    && HPE_PAUSED != HTTP_PARSER_ERRNO(&http_parser)
    && HPE_CLOSED_CONNECTION != HTTP_PARSER_ERRNO(&http_parser)
    )
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT,
                            "error:chttp_rsp_decode: http parser encounter error where errno = %d, name = %s, description = %s, [%.*s]\n",
                            HTTP_PARSER_ERRNO(&http_parser),
                            http_errno_name(HTTP_PARSER_ERRNO(&http_parser)),
                            http_errno_description(HTTP_PARSER_ERRNO(&http_parser)),
                            DMIN(data_len, 300), (char *)data
                            );
        return (EC_FALSE);
    }

    if(0 < body_len)
    {
        cbytes_append(CHTTP_RSP_BODY(chttp_rsp), data + header_len, body_len);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_rsp_decode: parsed_len = %u, header_len %u, body_len %u\n",
                    parsed_len, header_len, body_len);
                         
    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_rsp_decode: decoded rsp:\n");
        chttp_rsp_print(LOGSTDOUT, chttp_rsp);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_rsp_encode_header_kv(const CHTTP_RSP *chttp_rsp, const CSTRKV *kv, CBYTES *cbytes)
{
    const CSTRING *key;
    const CSTRING *val;

    key = CSTRKV_KEY(kv);
    val = CSTRKV_VAL(kv);
 
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(key), cstring_get_len(key)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header_kv: encode key of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)":",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header_kv: encode seperator of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(val), cstring_get_len(val)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header_kv: encode val of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)"\r\n",  2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header_kv: encode EOF failed\n");
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_rsp_encode_header_end(const CHTTP_RSP *chttp_rsp, CBYTES *cbytes)
{
    return cbytes_append(cbytes, (uint8_t *)"\r\n",  2);
}

EC_BOOL chttp_rsp_encode_header(const CHTTP_RSP *chttp_rsp, const CSTRKV_MGR *header, CBYTES *cbytes)
{
    CLIST_DATA *clist_data;

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(header), clist_data)
    {
        CSTRKV *kv;
     
        kv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == chttp_rsp_encode_header_kv(chttp_rsp, kv, cbytes))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header: encode kv failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_rsp_encode_header_end(chttp_rsp, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode_header: encode header EOF failed\n");
        return (EC_FALSE);
    }
#if 0
    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_rsp_encode_header: encoded header is \n");
        cbytes_print_str(LOGSTDOUT, cbytes);
    }
#endif 
    return (EC_TRUE);
}

EC_BOOL chttp_rsp_encode_body(const CHTTP_RSP *chttp_rsp, const CBYTES *rsp_body, CBYTES *cbytes)
{
    return cbytes_append(cbytes, CBYTES_BUF(rsp_body),  CBYTES_LEN(rsp_body));
}

EC_BOOL chttp_rsp_encode(const CHTTP_RSP *chttp_rsp, CBYTES *cbytes)
{
    /*ignore status*/
    if(EC_FALSE == chttp_rsp_encode_header(chttp_rsp, CHTTP_RSP_HEADER(chttp_rsp), cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode: encode header failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_rsp_encode_body(chttp_rsp, CHTTP_RSP_BODY(chttp_rsp), cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_rsp_encode: encode body failed\n");
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * Send Http Request and Handle Http Response
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/

EC_BOOL chttp_node_encode_req_const_str(CHTTP_NODE *chttp_node, const uint8_t *str, const UINT32 len)
{
    return chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), str, len);
}

EC_BOOL chttp_node_encode_req_protocol(CHTTP_NODE *chttp_node, const uint16_t version_major, const uint16_t version_minor)
{
    char protocol[16];
    UINT32 len;

    len = snprintf(protocol, sizeof(protocol)/sizeof(protocol[0]), "HTTP/%d.%d", version_major, version_minor);
    return chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)protocol, len);
}

EC_BOOL chttp_node_encode_req_method(CHTTP_NODE *chttp_node, const CSTRING *method)
{
    return chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(method), cstring_get_len(method));
}

EC_BOOL chttp_node_encode_req_header_kv(CHTTP_NODE *chttp_node, const CSTRKV *kv)
{
    const CSTRING *key;
    const CSTRING *val;

    key = CSTRKV_KEY(kv);
    val = CSTRKV_VAL(kv);
 
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(key), cstring_get_len(key)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_kv: encode key of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)":",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_kv: encode seperator of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(val), cstring_get_len(val)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_kv: encode val of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)"\r\n",  2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_kv: encode EOF failed\n");
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_param_kv(CHTTP_NODE *chttp_node, const CSTRKV *kv)
{
    const CSTRING *key;
    const CSTRING *val;

    key = CSTRKV_KEY(kv);
    val = CSTRKV_VAL(kv);

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)"&",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_param_kv: encode prefix of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(key), cstring_get_len(key)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_param_kv: encode key of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)"=",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_param_kv: encode seperator of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(val), cstring_get_len(val)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_param_kv: encode val of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_param(CHTTP_NODE *chttp_node, const CSTRKV_MGR *param)
{
    CLIST_DATA *clist_data;

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(param), clist_data)
    {
        CSTRKV *kv;
     
        kv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == chttp_node_encode_req_param_kv(chttp_node, kv))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_param: encode kv failed\n");
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_uri(CHTTP_NODE *chttp_node , const CSTRING *uri, const CSTRKV_MGR *param)
{
    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), cstring_get_str(uri), cstring_get_len(uri)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_uri: encode uri '%s' failed\n",
                                               cstring_get_str(uri));
        return (EC_FALSE);
    }
 
    if(NULL_PTR != param)
    {
        if(EC_FALSE == chttp_node_encode_req_param(chttp_node, param))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_uri: encode param failed\n");
            return (EC_FALSE);
        }
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_header_line(CHTTP_NODE *chttp_node, const CSTRING *method, const CSTRING *uri, const CSTRKV_MGR *param,
                                            const uint16_t version_major, const uint16_t version_minor)
{
    if(EC_FALSE == chttp_node_encode_req_method(chttp_node, method))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_line: encode method '%s' failed\n",
                                              cstring_get_str(method));
        return (EC_FALSE);
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)" ",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_uri: encode prefix space failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_req_uri(chttp_node, uri, param))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_line: encode uri failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)" ",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_uri: encode prefix space failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_req_protocol(chttp_node, CHTTP_VERSION_MAJOR, CHTTP_VERSION_MINOR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_line: encode protocol failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)"\r\n",  2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header_line: encode EOF failed\n");
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_header_end(CHTTP_NODE *chttp_node)
{
    return chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), (uint8_t *)"\r\n",  2);
}

EC_BOOL chttp_node_encode_req_header(CHTTP_NODE *chttp_node, const CSTRING *method, const CSTRING *uri, const CSTRKV_MGR *param, const CSTRKV_MGR *header)
{
    CLIST_DATA *clist_data;

    if(EC_FALSE == chttp_node_encode_req_header_line(chttp_node, method, uri, param, CHTTP_VERSION_MAJOR, CHTTP_VERSION_MINOR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header: encode header line failed\n");
        return (EC_FALSE);
    }

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(header), clist_data)
    {
        CSTRKV *kv;
     
        kv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == chttp_node_encode_req_header_kv(chttp_node, kv))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header: encode kv failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_node_encode_req_header_end(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_req_header: encode header EOF failed\n");
        return (EC_FALSE);
    }

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_node_encode_req_header: encoded header is \n");
        chunk_mgr_print_str(LOGSTDOUT, CHTTP_NODE_SEND_BUF(chttp_node));
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_req_body(CHTTP_NODE *chttp_node, const CBYTES *req_body)
{
    return chunk_mgr_append_data(CHTTP_NODE_SEND_BUF(chttp_node), CBYTES_BUF(req_body),  CBYTES_LEN(req_body));
}

EC_BOOL chttp_node_encode_rsp_const_str(CHTTP_NODE *chttp_node, const uint8_t *str, const UINT32 len, CBYTES *cbytes)
{
    return cbytes_append(cbytes, str, len);
}

EC_BOOL chttp_node_encode_rsp_protocol(CHTTP_NODE *chttp_node, const uint16_t version_major, const uint16_t version_minor, CBYTES *cbytes)
{
    char protocol[16];
    UINT32 len;

    len = snprintf(protocol, sizeof(protocol)/sizeof(protocol[0]), "HTTP/%d.%d", version_major, version_minor);
    return cbytes_append(cbytes, (uint8_t *)protocol, len);
}

EC_BOOL chttp_node_encode_rsp_method(CHTTP_NODE *chttp_node, const CSTRING *method, CBYTES *cbytes)
{
    return cbytes_append(cbytes, cstring_get_str(method), cstring_get_len(method));
}

EC_BOOL chttp_node_encode_rsp_status(CHTTP_NODE *chttp_node, const uint32_t status_code, CBYTES *cbytes)
{
    char status_str[32];
    uint32_t len;

    len = snprintf(status_str, sizeof(status_str)/sizeof(status_str[0]), "Response-Status:%u\r\n", status_code);
 
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)status_str, len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_kv: encode '%s' failed\n", status_str);
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_header_kv(CHTTP_NODE *chttp_node, const CSTRKV *kv, CBYTES *cbytes)
{
    const CSTRING *key;
    const CSTRING *val;

    key = CSTRKV_KEY(kv);
    val = CSTRKV_VAL(kv);
 
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(key), cstring_get_len(key)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_kv: encode key of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)":",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_kv: encode seperator of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(val), cstring_get_len(val)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_kv: encode val of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)"\r\n",  2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_kv: encode EOF failed\n");
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_param_kv(CHTTP_NODE *chttp_node, const CSTRKV *kv, CBYTES *cbytes)
{
    const CSTRING *key;
    const CSTRING *val;

    key = CSTRKV_KEY(kv);
    val = CSTRKV_VAL(kv);

    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)"&",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_param_kv: encode prefix of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(key), cstring_get_len(key)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_param_kv: encode key of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)"=",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_param_kv: encode seperator of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(val), cstring_get_len(val)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_param_kv: encode val of ('%s', '%s') failed\n",
                            cstring_get_str(key), cstring_get_str(val));
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_param(CHTTP_NODE *chttp_node, const CSTRKV_MGR *param, CBYTES *cbytes)
{
    CLIST_DATA *clist_data;

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(param), clist_data)
    {
        CSTRKV *kv;
     
        kv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == chttp_node_encode_rsp_param_kv(chttp_node, kv, cbytes))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_param: encode kv failed\n");
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_uri(CHTTP_NODE *chttp_node , const CSTRING *uri, const CSTRKV_MGR *param, CBYTES *cbytes)
{
    if(EC_FALSE == cbytes_append(cbytes, cstring_get_str(uri), cstring_get_len(uri)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_uri: encode uri '%s' failed\n",
                                               cstring_get_str(uri));
        return (EC_FALSE);
    }
 
    if(NULL_PTR != param)
    {
        if(EC_FALSE == chttp_node_encode_rsp_param(chttp_node, param, cbytes))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_uri: encode param failed\n");
            return (EC_FALSE);
        }
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_header_line(CHTTP_NODE *chttp_node, const CSTRING *method, const CSTRING *uri, const CSTRKV_MGR *param,
                                            const uint16_t version_major, const uint16_t version_minor, CBYTES *cbytes)
{
    if(EC_FALSE == chttp_node_encode_rsp_method(chttp_node, method, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_line: encode method '%s' failed\n",
                                              cstring_get_str(method));
        return (EC_FALSE);
    }

    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)" ",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_uri: encode prefix space failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_rsp_uri(chttp_node, uri, param, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_line: encode uri failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)" ",  1))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_uri: encode prefix space failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_rsp_protocol(chttp_node, CHTTP_VERSION_MAJOR, CHTTP_VERSION_MINOR, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_line: encode protocol failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == cbytes_append(cbytes, (uint8_t *)"\r\n",  2))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header_line: encode EOF failed\n");
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_header_end(CHTTP_NODE *chttp_node, CBYTES *cbytes)
{
    return cbytes_append(cbytes, (uint8_t *)"\r\n",  2);
}

EC_BOOL chttp_node_encode_rsp_header(CHTTP_NODE *chttp_node, const UINT32 status_code, const CSTRKV_MGR *header, CBYTES *cbytes)
{
    CLIST_DATA *clist_data;
#if 0
    if(EC_FALSE == chttp_node_encode_rsp_header_line(chttp_node, method, uri, param, CHTTP_VERSION_MAJOR, CHTTP_VERSION_MINOR, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header: encode header line failed\n");
        return (EC_FALSE);
    }
#endif 

    if(EC_FALSE == chttp_node_encode_rsp_status(chttp_node, (uint32_t)status_code, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header: encode status code failed\n");
        return (EC_FALSE);
    }

    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(header), clist_data)
    {
        CSTRKV *kv;
     
        kv = (CSTRKV *)CLIST_DATA_DATA(clist_data);

        if(EC_FALSE == chttp_node_encode_rsp_header_kv(chttp_node, kv, cbytes))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header: encode kv failed\n");
            return (EC_FALSE);
        }
    }

    if(EC_FALSE == chttp_node_encode_rsp_header_end(chttp_node, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_header: encode header EOF failed\n");
        return (EC_FALSE);
    }

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_node_encode_rsp_header: encoded header is \n");
        cbytes_print_str(LOGSTDOUT, cbytes);
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp_body(CHTTP_NODE *chttp_node, CBYTES *cbytes)
{
    CHUNK_MGR     *recv_chunks;
    uint64_t       body_len;

    recv_chunks = CHTTP_NODE_RECV_BUF(chttp_node);
    body_len    = chunk_mgr_total_length(recv_chunks);
    if(0 < body_len)
    {
        UINT32         len;
        UINT32         size;
 
        len  = CBYTES_LEN(cbytes);
        size = len + (UINT32)body_len;

        if(EC_FALSE == cbytes_expand_to(cbytes, size))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_body: cbytes expand to size %ld failed\n", size);
            return (EC_FALSE);
        }

        if(EC_FALSE == chunk_mgr_export(recv_chunks, CBYTES_BUF(cbytes) + len , CBYTES_LEN(cbytes), NULL_PTR))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp_body: export recv chunks failed\n", size);
            return (EC_FALSE);
        }
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_encode_rsp(CHTTP_NODE *chttp_node, CBYTES *cbytes)
{
    if(EC_FALSE == chttp_node_encode_rsp_header(chttp_node, CHTTP_NODE_STATUS_CODE(chttp_node), CHTTP_NODE_HEADER_IN_KVS(chttp_node), cbytes)
     )
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp: encode header failed\n");
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_rsp_body(chttp_node, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_encode_rsp: encode body failed\n");
        return (EC_FALSE);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_connect(CHTTP_NODE *chttp_node, const UINT32 ipaddr, const UINT32 port)
{
    CSOCKET_CNODE *csocket_cnode;

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_connect: connect server %s:%ld >>>\n",
                        c_word_to_ipv4(ipaddr), port); 
#if 0
    if(EC_TRUE == __chttp_need_unix_domain_ipc(ipaddr, port))
    {
        if(EC_FALSE == csocket_unix_connect( ipaddr, port , CSOCKET_IS_NONBLOCK_MODE, &sockfd ))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_connect: connect server unix@%s:%ld failed\n",
                                c_word_to_ipv4(ipaddr), port);
            return (EC_FALSE);
        }
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_connect: socket %d connecting to server unix@%s:%ld\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);

        csocket_cnode = csocket_cnode_unix_new(CMPI_ERROR_TCID, sockfd, CSOCKET_TYPE_TCP, ipaddr, port);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_connect:new csocket cnode for socket %d to server unix@%s:%ld failed\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }                         
    }
    else
#endif
    csocket_cnode = cconnp_mgr_try_reserve(task_brd_default_get_http_cconnp_mgr(), CMPI_ANY_TCID, ipaddr, port);
    if(NULL_PTR != csocket_cnode)
    {
        /*optimize for the latest loaed config*/
        csocket_optimize(CSOCKET_CNODE_SOCKFD(csocket_cnode));
    }
    else
    {
        int sockfd;
     
        if(EC_FALSE == csocket_connect( ipaddr, port , CSOCKET_IS_NONBLOCK_MODE, &sockfd ))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_connect: connect server %s:%ld failed\n",
                                c_word_to_ipv4(ipaddr), port);
            return (EC_FALSE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_connect: socket %d connecting to server %s:%ld\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);

        if(EC_FALSE == csocket_is_connected(sockfd))/*not adaptive to unix domain socket*/
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_connect: socket %d to server %s:%ld is not connected\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }

        if(do_log(SEC_0149_CHTTP, 5))
        {
            sys_log(LOGSTDOUT, "[DEBUG] chttp_connect: client tcp stat:\n");
            csocket_tcpi_stat_print(LOGSTDOUT, sockfd);
        }

        csocket_cnode = csocket_cnode_new(CMPI_ERROR_TCID, sockfd, CSOCKET_TYPE_TCP, ipaddr, port);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_connect:new csocket cnode for socket %d to server %s:%ld failed\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }     
    }

    /* bind */
    CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = csocket_cnode;
    CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = chttp_node;

    cepoll_set_event(task_brd_default_get_cepoll(),
                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                    CEPOLL_WR_EVENT,
                    (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_send_req,
                    (void *)csocket_cnode);

    cepoll_set_complete(task_brd_default_get_cepoll(),
                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                    (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_close,
                    (void *)csocket_cnode);

    cepoll_set_shutdown(task_brd_default_get_cepoll(),
                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                    (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_force_close,
                    (void *)csocket_cnode);

    cepoll_set_timeout(task_brd_default_get_cepoll(),
                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                    (uint32_t)CONN_TIMEOUT_NSEC,
                    (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_force_close,
                    (void *)csocket_cnode);
                 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_connect: sockfd %d set event WR\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode)); 
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_store_header_after_ddir(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint32_t max_store_size, uint32_t *has_stored_size, const CSTRING *path, const UINT32 store_srv_tcid, const UINT32 store_srv_ipaddr, const UINT32 store_srv_port)
{
    CBYTES        *cbytes;

    cbytes = cbytes_new(0);
    if(NULL_PTR == cbytes)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_store_header_after_ddir: new cbytes failed\n"); 
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_node_encode_rsp(chttp_node, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_store_header_after_ddir: encode rsp failed\n"); 
        cbytes_free(cbytes);
        return (EC_FALSE);
    }

    if(0 < CBYTES_LEN(cbytes))
    {

        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_node_store_header_after_ddir: p2p: path '%.*s', data %p [len %ld] => %s:%ld\n",
                    CSTRING_LEN(path), CSTRING_STR(path), CBYTES_BUF(cbytes), CBYTES_LEN(cbytes),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
                 
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_http_store_after_ddir, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, path, cbytes,
                        CHTTP_STORE_AUTH_TOKEN(chttp_store), chttp_store);
    }

    if(NULL_PTR != has_stored_size)
    {
        (*has_stored_size) = CBYTES_LEN(cbytes);
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

static EC_BOOL __chttp_node_store_header(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint32_t max_store_size, uint32_t *has_stored_size, const CSTRING *path, const UINT32 store_srv_tcid, const UINT32 store_srv_ipaddr, const UINT32 store_srv_port)
{
    CBYTES        *cbytes;

    cbytes = cbytes_new(0);
    if(NULL_PTR == cbytes)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_store_header: new cbytes failed\n"); 
        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_node_encode_rsp(chttp_node, cbytes))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_store_header: encode rsp failed\n"); 
        cbytes_free(cbytes);
        return (EC_FALSE);
    }

    if(0 < CBYTES_LEN(cbytes))
    {

        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_node_store_header: p2p: [token %s] path '%.*s', data %p [len %ld] => %s:%ld\n",
                    (char *)cstring_get_str(CHTTP_STORE_AUTH_TOKEN(chttp_store)),
                    CSTRING_LEN(path), CSTRING_STR(path), CBYTES_BUF(cbytes), CBYTES_LEN(cbytes),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
                 
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_http_store, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, path, cbytes,
                        CHTTP_STORE_AUTH_TOKEN(chttp_store));
    }

    if(NULL_PTR != has_stored_size)
    {
        (*has_stored_size) = CBYTES_LEN(cbytes);
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

static EC_BOOL __chttp_node_renew_header(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const CSTRING *path, const char *k, const char *v, const UINT32 store_srv_tcid, const UINT32 store_srv_ipaddr, const UINT32 store_srv_port)
{
    TASK_BRD      *task_brd;
    MOD_NODE       recv_mod_node;
    CSTRING        key;
    CSTRING        val;

    /*make receiver*/
    task_brd = task_brd_default_get();
    MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
    MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
    MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
    MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_renew_header: renew_header '%s:%s' of '%.*s' on %s:%ld\n",
                    k,v,
                    CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

    cstring_set_str(&key, (UINT8 *)k);
    cstring_set_str(&val, (UINT8 *)v);
                 
    task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                    &recv_mod_node,
                    NULL_PTR, FI_super_renew_header, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, path, &key, &val);
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_renew_headers(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const CSTRING *path, const CSTRKV_MGR *cstrkv_mgr, const UINT32 store_srv_tcid, const UINT32 store_srv_ipaddr, const UINT32 store_srv_port)
{
    TASK_BRD      *task_brd;
    MOD_NODE       recv_mod_node;

    /*make receiver*/
    task_brd = task_brd_default_get();
    MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
    MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
    MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
    MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_renew_header: renew headers of '%.*s' on %s:%ld\n",
                    CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
                 
    task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                    &recv_mod_node,
                    NULL_PTR, FI_super_renew_headers, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, path, cstrkv_mgr,
                    CHTTP_STORE_AUTH_TOKEN(chttp_store));
                 
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_renew_content_length(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const CSTRING *path, const CSTRKV_MGR *cstrkv_mgr, const UINT32 store_srv_tcid, const UINT32 store_srv_ipaddr, const UINT32 store_srv_port)
{
    TASK_BRD      *task_brd;
    MOD_NODE       recv_mod_node;
    CSTRING        auth_token_null;

    /*make receiver*/
    task_brd = task_brd_default_get();
    MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
    MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
    MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
    MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_renew_content_length: renew headers of '%.*s' on %s:%ld\n",
                    CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

    cstring_init(&auth_token_null, NULL_PTR);
    task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                    &recv_mod_node,
                    NULL_PTR, FI_super_renew_headers, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, path, cstrkv_mgr,
                    &auth_token_null);
                 
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_delete_dir(CHTTP_NODE *chttp_node, const CSTRING *path)
{
    CHTTP_STORE *chttp_store;
    TASK_BRD    *task_brd;
    TASK_MGR    *task_mgr;

    UINT32       crfsmon_md_id;
    UINT32       pos;
    UINT32       num;
    EC_BOOL      ret;
 
    chttp_store = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_delete_dir: store is null\n");
        return (EC_FALSE);
    }

    crfsmon_md_id = task_brd_default_get_crfsmon_id();
    if(CMPI_ERROR_MODI == crfsmon_md_id)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_delete_dir: no crfsmon started\n");
        return (EC_FALSE);
    }

    crfsmon_crfs_node_num(crfsmon_md_id, &num);
    if(0 == num)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_node_delete_dir: no rfs\n");
        return (EC_FALSE);
    }

    task_brd = task_brd_default_get();

    task_mgr = task_new(NULL_PTR, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
 
    for(pos = 0; pos < num; pos ++)
    {
        CRFS_NODE      crfs_node;
        MOD_NODE       recv_mod_node;

        crfs_node_init(&crfs_node);
        if(EC_FALSE == crfsmon_crfs_node_get_by_pos(crfsmon_md_id, pos, &crfs_node))
        {
            crfs_node_clean(&crfs_node);
            continue;
        }

        if(EC_FALSE == crfs_node_is_up(&crfs_node))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_delete_dir: delete '%.*s' skip rfs %s which is not up\n",
                    CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(CRFS_NODE_TCID(&crfs_node))
                    );
            crfs_node_clean(&crfs_node);
            continue;
        }

        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        task_p2p_inc(task_mgr, 0, &recv_mod_node,
                &ret, FI_super_delete_dir, ERR_MODULE_ID,
                CRFS_NODE_TCID(&crfs_node), CRFS_NODE_IPADDR(&crfs_node), CRFS_NODE_PORT(&crfs_node), path);
             
        crfs_node_clean(&crfs_node);             
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_delete_dir: delete '%.*s' done\n",
                    CSTRING_LEN(path), CSTRING_STR(path));
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_filter_header_check_etag(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    char *etag;

    if(EC_TRUE == cstring_is_empty(CHTTP_STORE_ETAG(chttp_store)))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: store etag is empty\n");
        return (EC_TRUE);
    }
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: store etag: %.*s\n",
                CHTTP_STORE_ETAG_LEN(chttp_store), CHTTP_STORE_ETAG_STR(chttp_store));

    etag = chttp_node_get_header(chttp_node, (const char *)"ETag");
    if(NULL_PTR == etag)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: header etag is null\n");
        return (EC_TRUE);
    }
 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: header etag: %s\n", etag);

    if(EC_TRUE == cstring_is_str_ignore_case(CHTTP_STORE_ETAG(chttp_store), (UINT8 *)etag))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: store etag and header etag same as '%s'\n", etag);
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_etag: ETag: '%.*s' != '%s' => del '%.*s'\n",
                        CHTTP_STORE_ETAG_LEN(chttp_store), CHTTP_STORE_ETAG_STR(chttp_store), etag,
                        CHTTP_STORE_BASEDIR_LEN(chttp_store), CHTTP_STORE_BASEDIR_STR(chttp_store));

    return (EC_FALSE);
}

static EC_BOOL __chttp_node_filter_header_check_lsmd(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    char  *last_modified;
    time_t lsmd;

    if(CHTTP_STORE_ERR_LAST_MODIFIED == CHTTP_STORE_LAST_MODIFIED(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_lsmd: store last-modified is not set\n");
        return (EC_TRUE);
    }

    last_modified = chttp_node_get_header(chttp_node, (const char *)"Last-Modified");
    if(NULL_PTR == last_modified)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_lsmd: header last-modified is empty\n");
        return (EC_TRUE);
    }

    lsmd = c_parse_http_time((uint8_t *)last_modified, strlen(last_modified));

    if(CHTTP_STORE_LAST_MODIFIED(chttp_store) == lsmd)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_lsmd: store last-modified and header last-modified same as %"PRId64"\n", lsmd);
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_lsmd: Last-Modified: %"PRId64" != %"PRId64"('%s') => del '%.*s'\n",
                        CHTTP_STORE_LAST_MODIFIED(chttp_store), (uint64_t)lsmd, last_modified,
                        CHTTP_STORE_BASEDIR_LEN(chttp_store), CHTTP_STORE_BASEDIR_STR(chttp_store));

    return (EC_FALSE);
}

static EC_BOOL __chttp_node_filter_header_check_expired(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE    *chttp_store;
    uint64_t        content_length;

    chttp_store = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: store is null\n");
        return (EC_FALSE);/*not expired*/
    }

    if(EC_TRUE == CHTTP_STORE_LAST_MODIFIED_SWITCH(chttp_store))
    {
        if(EC_FALSE == __chttp_node_filter_header_check_etag(chttp_node, chttp_store))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: found etag mismatched\n");
            /*__chttp_node_delete_dir(chttp_node, CHTTP_STORE_BASEDIR(chttp_store));*//*this will blocking the main coroutine! remove it!*/
            return (EC_TRUE);/*expired*/
        }

        if(EC_FALSE == __chttp_node_filter_header_check_lsmd(chttp_node, chttp_store))
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: found last-modified mismatched\n");
            /*__chttp_node_delete_dir(chttp_node, CHTTP_STORE_BASEDIR(chttp_store));*//*this will blocking the main coroutine! remove it!*/
            return (EC_TRUE);/*expired*/
        }
    }

    content_length = chttp_node_fetch_file_size(chttp_node);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: content-length from header: %"PRId64"\n", content_length);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: content-length from store : %"PRId64"\n", CHTTP_STORE_CONTENT_LENGTH(chttp_store));
    if(0 < content_length
    && 0 < CHTTP_STORE_CONTENT_LENGTH(chttp_store)
    && content_length != CHTTP_STORE_CONTENT_LENGTH(chttp_store)
    )
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: found content-length mismatched\n");
        return (EC_TRUE);/*expired*/
    }

    if(EC_OBSCURE != CHTTP_STORE_USE_GZIP_FLAG(chttp_store)
    && chttp_node_check_use_gzip(chttp_node) != CHTTP_STORE_USE_GZIP_FLAG(chttp_store)
    )
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: found content-encoding mismatched\n");
        return (EC_TRUE);/*expired*/
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_expired: not expired\n");
    return (EC_FALSE);/*not expired*/
}

static EC_BOOL __chttp_node_filter_header_check_modified(CHTTP_NODE *chttp_node)
{
    uint32_t status_code;
 
    status_code = (uint32_t)CHTTP_NODE_STATUS_CODE(chttp_node);
    if(CHTTP_NOT_MODIFIED == status_code)
    {
        CSTRKV_MGR *cstrkv_mgr;
     
        cstrkv_mgr = CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node);
     
        chttp_node_fetch_header(chttp_node, (char *)"Expires"      , cstrkv_mgr);
        chttp_node_fetch_header(chttp_node, (char *)"Date"         , cstrkv_mgr);
        chttp_node_fetch_header(chttp_node, (char *)"Cache-Control", cstrkv_mgr);

        if(do_log(SEC_0149_CHTTP, 9))
        {
            CSOCKET_CNODE *csocket_cnode;
            csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_check_modified: socket %d, seg_id %u, status %u => fetch headers\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode),
                        CHTTP_STORE_SEG_ID(CHTTP_NODE_STORE(chttp_node)),
                        status_code);
            cstrkv_mgr_print(LOGSTDOUT, cstrkv_mgr);
        }
        return (EC_TRUE);
    }
 
    return (EC_FALSE);
}

static EC_BOOL __chttp_node_filter_header_check_chunked(CHTTP_NODE *chttp_node)
{
    if(EC_TRUE == chttp_node_is_chunked(chttp_node))
    {
        if(NULL_PTR == chttp_node_get_header(chttp_node, (const char *)"Response-Status")
        && NULL_PTR == chttp_node_get_header(chttp_node, (const char *)"status"))
        {
            uint32_t status_code;

            status_code = (uint32_t)CHTTP_NODE_STATUS_CODE(chttp_node);
            chttp_node_add_header(chttp_node, (const char *)"Response-Status", c_uint32_to_str(status_code));
        }
     
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

static EC_BOOL __chttp_node_filter_header_set_cc_cache_control(CHTTP_NODE *chttp_node)
{
    if(NULL_PTR != CHTTP_NODE_STORE(chttp_node))
    {
        UINT32         cache_control;
        CSOCKET_CNODE *csocket_cnode;
     
        cache_control = CHTTP_STORE_CACHE_CTRL(CHTTP_NODE_STORE(chttp_node));

        if(CHTTP_STORE_CACHE_ERR == cache_control || CHTTP_STORE_CACHE_NONE == cache_control)
        {
            chttp_node_add_header(chttp_node, (const char *)"X_CACHE_CONTROL", (const char *)"no-cache");
        }
        else
        {
            chttp_node_add_header(chttp_node, (const char *)"X_CACHE_CONTROL", (const char *)"cache");
        }

        csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node); 
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_set_cc_cache_control: socket %d, seg_id %u, cache_ctrl: 0x%lx\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode),
                        CHTTP_STORE_SEG_ID(CHTTP_NODE_STORE(chttp_node)),
                        cache_control);         
    }
    return (EC_TRUE);
}

static EC_BOOL __chttp_node_filter_header_set_override_expires(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE    *chttp_store;

    chttp_store = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_set_override_expires: store is null\n");
        return (EC_FALSE);
    }

    if(EC_TRUE == CHTTP_STORE_OVERRIDE_EXPIRES_FLAG(chttp_store))
    {
        if(0 < CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store))
        {
            char *expires_str;
            /*note: time_t unit is second but not mico-second*/
            expires_str = c_http_time(task_brd_default_get_time() + (time_t)CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store));
            chttp_node_renew_header(chttp_node, (const char *)"Expires", expires_str);
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_set_override_expires: renew header: 'Expires':'%s'\n", expires_str);
        }
    }
    else
    {
        if(0 < CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store)
        && EC_FALSE == chttp_node_has_header_key(chttp_node, (const char *)"Expires"))
        {
            char *expires_str;
            /*note: time_t unit is second but not mico-second*/
            expires_str = c_http_time(task_brd_default_get_time() + (time_t)CHTTP_STORE_OVERRIDE_EXPIRES_NSEC(chttp_store));
            chttp_node_add_header(chttp_node, (const char *)"Expires", expires_str);
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_set_override_expires: add header: 'Expires':'%s'\n", expires_str);
        }
    }

    return (EC_TRUE);
}

static EC_BOOL __chttp_node_filter_header_set_content_range(CHTTP_NODE *chttp_node)
{
    char           *v;
    char            content_range[64];

    uint64_t        content_length;
    uint32_t        status_code;
 
    v = chttp_node_get_header(chttp_node, (const char *)"Content-Range");
    if(NULL_PTR != v)
    {
        return (EC_TRUE);
    }

    status_code = (uint32_t)CHTTP_NODE_STATUS_CODE(chttp_node);
    if(CHTTP_OK != status_code)
    {
        return (EC_FALSE);
    }

    v = chttp_node_get_header(chttp_node, (const char *)"Content-Length");
    if(NULL_PTR == v)
    {
        return (EC_FALSE);
    }

    content_length = c_str_to_uint64(v);

    snprintf(content_range, sizeof(content_range)/sizeof(content_range[0]), "0-%"PRId64"/%"PRId64, content_length - 1, content_length);
    chttp_node_add_header(chttp_node, (const char *)"Content-Range", content_range);
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_node_filter_header_set_content_range: add header: 'Content-Range':'%s'\n", (char *)content_range);

    return (EC_TRUE);
}

EC_BOOL chttp_node_filter_on_header_complete(CHTTP_NODE *chttp_node)
{
    __chttp_node_filter_header_set_cc_cache_control(chttp_node);
    __chttp_node_filter_header_set_override_expires(chttp_node);

    if(EC_TRUE == __chttp_node_filter_header_check_modified(chttp_node))
    {
        CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node) = EC_TRUE;
        return (EC_TRUE);
    }

    if(EC_TRUE == __chttp_node_filter_header_check_expired(chttp_node))
    {
        CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node) = EC_TRUE;
        return (EC_TRUE);
    }
 
    if(EC_TRUE == __chttp_node_filter_header_check_chunked(chttp_node))
    {
        if(NULL_PTR != CHTTP_NODE_STORE(chttp_node))
        {
            CHTTP_STORE_CHUNK_FLAG(CHTTP_NODE_STORE(chttp_node)) = EC_TRUE;
        }
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_store_header(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint32_t max_store_size, uint32_t *has_stored_size)
{
    CSTRING        path;
 
    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store, &path);

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_header: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    if(EC_TRUE == CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node))
    {
        CSTRKV_MGR *cstrkv_mgr;

        cstrkv_mgr = CHTTP_NODE_HEADER_MODIFIED_KVS(chttp_node);
        __chttp_node_renew_headers(chttp_node, chttp_store, &path, cstrkv_mgr, store_srv_tcid, store_srv_ipaddr, store_srv_port);
     
        CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;

        cstring_clean(&path);
        return (EC_TRUE);
    }

    __chttp_node_filter_header_set_content_range(chttp_node);

    if(EC_FALSE == CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node))
    {
        /*not expired*/
        __chttp_node_store_header(chttp_node, chttp_store, max_store_size, has_stored_size, &path, store_srv_tcid, store_srv_ipaddr, store_srv_port);
    }
    else
    {
        /*expired, then => del basedir => store header*/
        __chttp_node_store_header_after_ddir(chttp_node, chttp_store, max_store_size, has_stored_size, &path, store_srv_tcid, store_srv_ipaddr, store_srv_port);
    }
    CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;
 
    cstring_clean(&path);

    return (EC_TRUE);
}

EC_BOOL chttp_node_store_body(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint32_t max_store_size, uint32_t *has_stored_size)
{
    CSTRING        path;
    CBYTES         body_cbytes;

    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    uint32_t       store_size;
    uint32_t       content_len;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store, &path);

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_body: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_body: store '%.*s' to server %s:%ld\n",
                        CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 
 
    /*make body*/
    cbytes_init(&body_cbytes);
    store_size = DMIN(CHTTP_STORE_SEG_SIZE(chttp_store), max_store_size);
 
    if(EC_FALSE == cbytes_expand_to(&body_cbytes, store_size))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_body: expand body cbytes to store size %u failed\n", store_size);
        cstring_clean(&path);
        return (EC_FALSE);
    }
 
    chunk_mgr_shift(CHTTP_NODE_RECV_BUF(chttp_node), store_size, CBYTES_BUF(&body_cbytes), &content_len);
    CBYTES_LEN(&body_cbytes) = content_len;

    rlog(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_body: min(%u, %u) => %u, final content len %u\n",
                CHTTP_STORE_SEG_SIZE(chttp_store), max_store_size, store_size, content_len);

    if(0 < content_len)
    {
        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_body: p2p: [token %s] path '%.*s', data %p [len %ld] => %s:%ld\n",
                    (char *)cstring_get_str(CHTTP_STORE_AUTH_TOKEN(chttp_store)),
                    CSTRING_LEN(&path), CSTRING_STR(&path), CBYTES_BUF(&body_cbytes), CBYTES_LEN(&body_cbytes),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
     
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_http_store, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, &path, &body_cbytes,
                        CHTTP_STORE_AUTH_TOKEN(chttp_store));

        CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;
    }

    if(NULL_PTR != has_stored_size)
    {
        (*has_stored_size) = content_len;
    }
 
    cstring_clean(&path);
    cbytes_clean(&body_cbytes);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_body: seg size %u, store size %u, content len %u, seg %u\n",
                    CHTTP_STORE_SEG_SIZE(chttp_store), store_size, content_len, CHTTP_STORE_SEG_ID(chttp_store));
 
    return (EC_TRUE);
}

/*store zero-length body*/
EC_BOOL chttp_node_store_no_body(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    CSTRING        path;
    CBYTES         body_cbytes;

    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store, &path);

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_no_body: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_no_body: store '%.*s' to server %s:%ld\n",
                        CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 
 
    /*make body*/
    cbytes_init(&body_cbytes);

    if(1)
    {
        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_no_body: p2p: path '%.*s', data %p [len %ld] => %s:%ld\n",
                    CSTRING_LEN(&path), CSTRING_STR(&path), CBYTES_BUF(&body_cbytes), CBYTES_LEN(&body_cbytes),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
     
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_http_store, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, &path, &body_cbytes,
                        CHTTP_STORE_AUTH_TOKEN(chttp_store));
                     
        CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;                     
    }
 
    cstring_clean(&path);
    cbytes_clean(&body_cbytes);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_no_body: seg size %u, seg %u\n",
                    CHTTP_STORE_SEG_SIZE(chttp_store), CHTTP_STORE_SEG_ID(chttp_store));
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_store_whole(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint32_t max_store_size, uint32_t *has_stored_size)
{
    CSTRING        path;
 
    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store, &path);

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_whole: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    if(EC_FALSE == CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node))
    {
        /*not expired*/
        __chttp_node_store_header(chttp_node, chttp_store, max_store_size, has_stored_size, &path, store_srv_tcid, store_srv_ipaddr, store_srv_port);
    }
    else
    {
        /*expired, then => del basedir => store header*/
        __chttp_node_store_header_after_ddir(chttp_node, chttp_store, max_store_size, has_stored_size, &path, store_srv_tcid, store_srv_ipaddr, store_srv_port);
    }

    CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;
 
    cstring_clean(&path);

    return (EC_TRUE);
}

/*for chunked*/
EC_BOOL chttp_node_renew_content_length(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store, const uint64_t content_length)
{
    CSTRING        path;
 
    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    uint32_t       seg_id_saved;
    uint32_t       buf_size;
    char           buf_str[ 64 ];

    CSTRKV_MGR    *cstrkv_mgr;

    seg_id_saved = CHTTP_STORE_SEG_ID(chttp_store); /*save*/

    /*make path*/
    cstring_init(&path, NULL_PTR);
    CHTTP_STORE_SEG_ID(chttp_store) = 0;
    chttp_store_path_get(chttp_store, &path);

    CHTTP_STORE_SEG_ID(chttp_store) = seg_id_saved; /*restore*/

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_renew_content_length: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    cstrkv_mgr = cstrkv_mgr_new();
    if(NULL_PTR == cstrkv_mgr)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_renew_content_length: new cstrkv_mgr failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    buf_size = sizeof(buf_str)/sizeof(buf_str[0]);
 
    snprintf(buf_str, buf_size - 1, "%"PRId64, content_length);
    cstrkv_mgr_add_kv_str(cstrkv_mgr, (const char *)"Content-Length", (char *)buf_str);/*add content length header*/

    snprintf(buf_str, buf_size - 1, "0-%"PRId64"/%"PRId64, content_length - 1, content_length);
    cstrkv_mgr_add_kv_str(cstrkv_mgr, (const char *)"Content-Range", (char *)buf_str);/*add content range header*/

    cstrkv_mgr_add_kv_str(cstrkv_mgr, (const char *)"Transfer-Encoding", NULL_PTR); /*remove chunk header*/
 
    __chttp_node_renew_content_length(chttp_node, chttp_store, &path, cstrkv_mgr, store_srv_tcid, store_srv_ipaddr, store_srv_port);

    cstrkv_mgr_free(cstrkv_mgr);
    cstring_clean(&path);

    return (EC_TRUE);
}


/*notify the waiters: no next data. used for chunked scenario*/
EC_BOOL chttp_node_store_no_next(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    CSTRING        path;

    UINT32         store_srv_tcid;
    UINT32         store_srv_ipaddr;
    UINT32         store_srv_port;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store, &path);

    /*select storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_no_next: select storage server for '%.*s' failed\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path));
        cstring_clean(&path);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_no_next: notify no '%.*s' to server %s:%ld\n",
                        CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 

    if(1)
    {
        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_no_next: p2p: path '%.*s'[NONE] => %s:%ld\n",
                    CSTRING_LEN(&path), CSTRING_STR(&path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
     
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_file_notify, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, &path);
    }

    cstring_clean(&path);
 
    return (EC_TRUE);
}

/*no more to store*/
EC_BOOL chttp_node_store_done_blocking(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store && EC_TRUE == CHTTP_STORE_LOCKED_FLAG(chttp_store))
    {
        CSTRING        path;
        CSTRING       *auth_token;

        UINT32         store_srv_tcid;
        UINT32         store_srv_ipaddr;
        UINT32         store_srv_port;

        /*make path*/
        cstring_init(&path, NULL_PTR);
        chttp_store_path_get(chttp_store, &path);

        auth_token = (CSTRING *)CHTTP_STORE_AUTH_TOKEN(chttp_store);

        /*select storage server*/
        if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_done_blocking: determine storage server for '%.*s' failed\n",
                                CSTRING_LEN(&path), CSTRING_STR(&path));
            cstring_clean(&path);
            return (EC_FALSE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_done_blocking: unlock '%.*s' to server %s:%ld\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
    
     
        /*case HPCC-343: the 1st orig procedure failed with range header, then trigger 2nd orig procedure without range header*/
        /*but found the unlock operation emitted in the 1st orig procedure was not sent out yet which cause the lock operation */
        /*in the 2nd orig procedure, and then abnormal result happen. thus have to unlock in blocking mode*/
        {
            UINT32 super_md_id;

            super_md_id = 0;
            super_unlock(super_md_id, store_srv_tcid, store_srv_ipaddr, store_srv_port, &path, auth_token);
        }

        CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;
  
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_done_blocking: p2p: unlock '%.*s' to server %s:%ld => done\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        cstring_clean(&path);                         
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_store_done_nonblocking(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store && EC_TRUE == CHTTP_STORE_LOCKED_FLAG(chttp_store))
    {
        CSTRING        path;
        CSTRING       *auth_token;

        UINT32         store_srv_tcid;
        UINT32         store_srv_ipaddr;
        UINT32         store_srv_port;

        /*make path*/
        cstring_init(&path, NULL_PTR);
        chttp_store_path_get(chttp_store, &path);

        auth_token = (CSTRING *)CHTTP_STORE_AUTH_TOKEN(chttp_store);

        /*select storage server*/
        if(EC_FALSE == chttp_store_srv_get(chttp_store, &path, &store_srv_tcid, &store_srv_ipaddr, &store_srv_port))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_done_nonblocking: determine storage server for '%.*s' failed\n",
                                CSTRING_LEN(&path), CSTRING_STR(&path));
            cstring_clean(&path);
            return (EC_FALSE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_done_nonblocking: unlock '%.*s' to server %s:%ld\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
    
        {
            TASK_BRD      *task_brd;
            MOD_NODE       recv_mod_node;

            /*make receiver*/
            task_brd = task_brd_default_get();
            MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
            MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
            MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
            MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

            rlog(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_done_nonblocking: p2p: '%.*s' => done\n",
                        CSTRING_LEN(&path), CSTRING_STR(&path));
         
            task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                            &recv_mod_node,
                            NULL_PTR, FI_super_unlock, ERR_MODULE_ID, store_srv_tcid, store_srv_ipaddr, store_srv_port, &path, auth_token);
        }
     
        CHTTP_STORE_LOCKED_FLAG(chttp_store) = EC_FALSE;
  
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_store_done_nonblocking: p2p: unlock '%.*s' to server %s:%ld => done\n",
                            CSTRING_LEN(&path), CSTRING_STR(&path), c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        cstring_clean(&path);                         
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_store_on_headers_complete(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE   *chttp_store;
    CSOCKET_CNODE *csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    chttp_store   = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_headers_complete: socket %d, store is null\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode));
 
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_headers_complete: socket %d, check seg_id %u and cache ctrl 0x%x\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), CHTTP_STORE_CACHE_CTRL(chttp_store));
 
    /*if need to store recved data to storage and the starting seg is 0, i.e., header*/
    if(0 == CHTTP_STORE_SEG_ID(chttp_store) /*for range request, not need to store header*/
    && (CHTTP_STORE_CACHE_HEADER & CHTTP_STORE_CACHE_CTRL(chttp_store))
    )
    {
        uint32_t stored_size;

        /*WARNING: when store header to storage failed, the whole received header data would be shift out from buffer*/
        if(EC_FALSE == chttp_node_store_header(chttp_node, chttp_store, CHTTP_NODE_HEADER_PARSED_LEN(chttp_node), &stored_size))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_on_headers_complete: socket %d, store header failed and would be lost\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));

            CHTTP_STORE_SEG_ID(chttp_store) ++; /*fix: skip header seg*/
        }
        else
        {
            CHTTP_STORE_SEG_ID(chttp_store) ++; /*prepare for body store*/
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_headers_complete: socket %d, store header size %u done => header parsed %u\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode), stored_size, CHTTP_NODE_HEADER_PARSED_LEN(chttp_node));
        }

        /*clear corresponding cache ctrl flag*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) &= (UINT32)(~CHTTP_STORE_CACHE_HEADER);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_node_store_on_message_complete(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE   *chttp_store;
    CSOCKET_CNODE *csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    chttp_store   = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, store is null\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode));
 
        return (EC_FALSE);
    }
 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, [1] seg_id %u, check cache ctrl 0x%x\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), CHTTP_STORE_CACHE_CTRL(chttp_store));
 
    /*after body data was appended to recv_chunks, store recv_chunks to storage*/
    if(CHTTP_STORE_CACHE_BODY & CHTTP_STORE_CACHE_CTRL(chttp_store))
    {
        /*store all left data to storage*/
        while(CHTTP_NODE_BODY_STORED_LEN(chttp_node) < CHTTP_NODE_BODY_PARSED_LEN(chttp_node))
        {
            uint32_t stored_size;

            /*WARNING: when store body to storage failed, the received data would not be shift out from recv_chunks*/
            if(EC_FALSE == chttp_node_store_body(chttp_node, chttp_store, CHTTP_STORE_SEG_SIZE(chttp_store), &stored_size))
            {
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_on_message_complete: socket %d, store body seg %u failed\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store));
                /*break;*/
                /*fix: give up storing this seg */
                CHTTP_NODE_BODY_STORED_LEN(chttp_node) += CHTTP_STORE_SEG_SIZE(chttp_store);/*update stored len*/
                CHTTP_STORE_SEG_ID(chttp_store) ++;/*skip it*/
            }
            else
            {
                CHTTP_NODE_BODY_STORED_LEN(chttp_node) += stored_size;/*update stored len*/
             
                dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, store body seg %u size %u done => stored %"PRId64"\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), stored_size, CHTTP_NODE_BODY_STORED_LEN(chttp_node));
                             
                if(stored_size == CHTTP_STORE_SEG_SIZE(chttp_store))
                {
                    CHTTP_STORE_SEG_ID(chttp_store) ++;/*move to next seg*/
                }
            }
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, now store body seg %u, stored %"PRId64", parsed %"PRId64", chunk_flag %s, is_chunked %s\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store),
                    CHTTP_NODE_BODY_STORED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node),
                    c_bool_str(CHTTP_STORE_CHUNK_FLAG(chttp_store)), c_bool_str(chttp_node_is_chunked(chttp_node)));

        /*error happend, have to discard all left data*/
        if(CHTTP_NODE_BODY_STORED_LEN(chttp_node) < CHTTP_NODE_BODY_PARSED_LEN(chttp_node))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_on_message_complete: socket %d, now store body seg %u, stored %"PRId64", parsed %"PRId64" => discard %"PRId64" chunk left %ld\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store),
                        CHTTP_NODE_BODY_STORED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node),
                        CHTTP_NODE_BODY_PARSED_LEN(chttp_node) - CHTTP_NODE_BODY_STORED_LEN(chttp_node),
                        chunk_mgr_total_length(CHTTP_NODE_RECV_BUF(chttp_node)));
                     
            chunk_mgr_clean(CHTTP_NODE_RECV_BUF(chttp_node)); /*cleanup left data to prevent it from dirty data*/
        }
        else if(0 == (CHTTP_NODE_BODY_PARSED_LEN(chttp_node) % CHTTP_STORE_SEG_SIZE(chttp_store)))
        {
            if(EC_TRUE == CHTTP_STORE_CHUNK_FLAG(chttp_store) || EC_TRUE == chttp_node_is_chunked(chttp_node))
            {
                /*notify all chunked-file waiters: no more data*/
                chttp_node_store_no_body(chttp_node, chttp_store);
                //chttp_node_store_no_next(chttp_node, chttp_store);
            }
        }

        /*clear corresponding cache ctrl flag*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) &= (UINT32)(~CHTTP_STORE_CACHE_BODY);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, [2] check seg_id %u and cache ctrl 0x%x\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), CHTTP_STORE_CACHE_CTRL(chttp_store));

    /*if need to store recved data to storage and the starting seg is 0, i.e., header*/
    if(0 == CHTTP_STORE_SEG_ID(chttp_store) /*for range request, not need to store header*/
    && (CHTTP_STORE_CACHE_WHOLE & CHTTP_STORE_CACHE_CTRL(chttp_store))
    )
    {
        uint32_t max_store_size;
        uint32_t stored_size;

        max_store_size = CHTTP_NODE_HEADER_PARSED_LEN(chttp_node) + (uint32_t)CHTTP_NODE_BODY_PARSED_LEN(chttp_node);

        /*WARNING: when store header to storage failed, the whole received header data would be shift out from buffer*/
        if(EC_FALSE == chttp_node_store_whole(chttp_node, chttp_store, max_store_size, &stored_size))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_on_message_complete: socket %d, store whole failed and would be lost\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode));
                     
            CHTTP_STORE_SEG_ID(chttp_store) ++; /*fix: give up storing*/
        }
        else
        {
            CHTTP_STORE_SEG_ID(chttp_store) ++;/*move to next seg*/
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_message_complete: socket %d, store whole size %u done => whole parsed %u\n",
                        CSOCKET_CNODE_SOCKFD(csocket_cnode), stored_size, max_store_size);
        }

        /*clear corresponding cache ctrl flag*/
        CHTTP_STORE_CACHE_CTRL(chttp_store) &= (UINT32)(~CHTTP_STORE_CACHE_WHOLE);
    }
 
    return (EC_TRUE);
}

EC_BOOL chttp_node_store_on_body(CHTTP_NODE *chttp_node)
{
    CHTTP_STORE   *chttp_store;
    CSOCKET_CNODE *csocket_cnode;

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
 
    chttp_store   = CHTTP_NODE_STORE(chttp_node);
    if(NULL_PTR == chttp_store)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_body: socket %d, store is null\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode));
 
        return (EC_FALSE);
    }
 
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_body: socket %d, seg_id %u, check cache ctrl 0x%x\n",
                CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), CHTTP_STORE_CACHE_CTRL(chttp_store));
             
    if(CHTTP_STORE_CACHE_BODY & CHTTP_STORE_CACHE_CTRL(chttp_store))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_body: socket %d, stored %"PRId64", body parsed %"PRId64"\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_NODE_BODY_STORED_LEN(chttp_node), CHTTP_NODE_BODY_PARSED_LEN(chttp_node));
                 
        while(CHTTP_NODE_BODY_STORED_LEN(chttp_node) + CHTTP_STORE_SEG_SIZE(chttp_store) <= CHTTP_NODE_BODY_PARSED_LEN(chttp_node))
        {
            uint32_t stored_size;

            /*WARNING: when store body to storage failed, the received data would not be shift out from recv_chunks*/
            if(EC_FALSE == chttp_node_store_body(chttp_node, chttp_store, CHTTP_STORE_SEG_SIZE(chttp_store), &stored_size))
            {
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_store_on_body: socket %d, store body seg %u failed, skip it!\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store));

                /*fix: give up storing this seg */
                CHTTP_NODE_BODY_STORED_LEN(chttp_node) += CHTTP_STORE_SEG_SIZE(chttp_store);/*update stored len*/
                CHTTP_STORE_SEG_ID(chttp_store) ++; /*skip*/
            }
            else
            {
                CHTTP_NODE_BODY_STORED_LEN(chttp_node) += stored_size;/*update stored len*/
             
                dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_store_on_body: socket %d, store body seg %u size %u done => stored %"PRId64"\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode), CHTTP_STORE_SEG_ID(chttp_store), stored_size, CHTTP_NODE_BODY_STORED_LEN(chttp_node));
                             
                if(stored_size == CHTTP_STORE_SEG_SIZE(chttp_store))
                {
                    CHTTP_STORE_SEG_ID(chttp_store) ++; /*move to next seg*/
                }
            }
        }

        /*note: do not clear corresponding cache ctrl flag due to body data may have more left*/
    }
    return (EC_TRUE);
}

EC_BOOL chttp_node_set_billing(CHTTP_NODE *chttp_node, CHTTP_STORE *chttp_store)
{
    if(NULL_PTR != chttp_store
    && EC_FALSE == cstring_is_empty(CHTTP_STORE_BILLING_DOMAIN(chttp_store))
    && EC_FALSE == cstring_is_empty(CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store))
    )
    {
        TASK_BRD      *task_brd;
        MOD_NODE       recv_mod_node;

        UINT32         billing_srv_ipaddr;
        UINT32         billing_srv_port;

        CSTRING       *billing_flags;
        CSTRING       *billing_domain;
        CSTRING       *billing_client_type;
     
        UINT32         send_len;
        UINT32         recv_len;

        billing_flags       = CHTTP_STORE_BILLING_FLAGS(chttp_store);
        billing_domain      = CHTTP_STORE_BILLING_DOMAIN(chttp_store);
        billing_client_type = CHTTP_STORE_BILLING_CLIENT_TYPE(chttp_store);
     
        send_len    = CHTTP_NODE_S_SEND_LEN(chttp_node);
        recv_len    = CHTTP_NODE_S_RECV_LEN(chttp_node);

        /*note: when reach here, csocket_cnode in chttp_node may be null*/
    
        if(0 == send_len && 0 == recv_len)
        {
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_set_billing: basedir '%.*s', seg_id %u, not data send or recv\n",
                            CHTTP_STORE_BASEDIR_LEN(chttp_store), CHTTP_STORE_BASEDIR_STR(chttp_store), CHTTP_STORE_SEG_ID(chttp_store));
                     
            return (EC_TRUE);
        }

        billing_srv_ipaddr = c_ipv4_to_word((const char *)"127.0.0.1");
        billing_srv_port   = 888;

        /*make receiver*/
        task_brd = task_brd_default_get();
        MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
        MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
        MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
        MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_node_set_billing: basedir '%.*s', domain '%.*s', send_len %ld, recv_len %ld\n",
                    CHTTP_STORE_BASEDIR_LEN(chttp_store), CHTTP_STORE_BASEDIR_STR(chttp_store),
                    CSTRING_LEN(billing_domain), CSTRING_STR(billing_domain), send_len, recv_len);
                 
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                        &recv_mod_node,
                        NULL_PTR, FI_super_set_billing, ERR_MODULE_ID, billing_srv_ipaddr, billing_srv_port,
                        billing_flags, billing_domain, billing_client_type, send_len, recv_len);

        return (EC_TRUE);
    }

    return (EC_TRUE);
}

EC_BOOL chttp_csocket_cnode_send_req(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE *chttp_node;

    if(EC_FALSE == CSOCKET_CNODE_IS_CONNECTED(csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_req: sockfd %d is not connected\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode)); 
     
        return (EC_FALSE);
    }
 
    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_req: sockfd %d -> chttp_node is null\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);
    }

    if(EC_FALSE == chttp_node_send(chttp_node, csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_send_req: sockfd %d send req failed\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);
    }

    if(EC_TRUE == chttp_node_need_send(chttp_node))
    {
        /*wait for next writing*/

        /**********************************************************************************
        *   note:
        *       assume caller would set WR event when return EC_AGAIN.
        *       we cannot determine who will trigger send op:
        *         if cepoll trigger send op, WR event should not be set,
        *         if chttp trigger send op before WR event was set, chttp should set later
        *       thus, return EC_AGAIN is more safe.
        *
        ***********************************************************************************/
        return (EC_AGAIN);
    }

    chunk_mgr_clean(CHTTP_NODE_SEND_BUF(chttp_node));/*clean up asap*/
 
    if(NULL_PTR != CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node))/*for big file*/
    {
        cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT,
                     (CEPOLL_EVENT_HANDLER)CHTTP_NODE_SEND_DATA_MORE_FUNC(chttp_node), (void *)csocket_cnode);         
        return (EC_TRUE);/*wait for writing more*/
    }

    //CHTTP_NODE_LOG_TIME_WHEN_SENT(chttp_node);
 
    /*now all data had been sent out, del WR event and set RD event*/
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_send_req: sockfd %d had sent out all req data\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));  

    cepoll_del_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_WR_EVENT);

    cepoll_set_event(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(csocket_cnode), CEPOLL_RD_EVENT,
                     (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_recv_rsp, (void *)csocket_cnode);

    return (EC_TRUE);
}

EC_BOOL chttp_csocket_cnode_recv_rsp(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE   *chttp_node;
    EC_BOOL       ret;
 
    if(EC_FALSE == CSOCKET_CNODE_IS_CONNECTED(csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_rsp: sockfd %d is not connected\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_rsp: sockfd %d -> chttp_node is null\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);
    }

    ret = chttp_node_recv(chttp_node, csocket_cnode);
    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_rsp: recv rsp on sockfd %d failed\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);                         
    }

    if(EC_DONE == ret)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_recv_rsp: sockfd %d, no more data to recv or parse\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));

        /*note: when origin return 502, e.g., here is the last chance to save to storage*/
        chttp_node_store_on_message_complete(chttp_node);
     
        return (EC_DONE); /*fix*/
    }

    if(EC_FALSE == chttp_parse(chttp_node))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_recv_rsp: parse on sockfd %d failed\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
        return (EC_FALSE);                         
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_csocket_cnode_recv_rsp: sockfd %d, recv and parse done\n",
                            CSOCKET_CNODE_SOCKFD(csocket_cnode));
    return (EC_TRUE);
}

/*basic http flow*/
EC_BOOL chttp_request_basic(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    CHTTP_NODE    *chttp_node;
    CROUTINE_COND *croutine_cond;
    CSOCKET_CNODE *csocket_cnode;
    uint64_t       rsp_body_len;
    UINT8         *data;
    UINT32         data_len;

    __COROUTINE_IF_EXCEPTION() {/*exception*/
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: found exception, refuse to launch http\n");
        chttp_node_store_done_nonblocking(NULL_PTR, chttp_store);/*for merge orign exception*/
        return (EC_FALSE);
    }

    chttp_node = chttp_node_new(CHTTP_TYPE_HANDLE_RSP);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: new chttp_node failed\n");
        return (EC_FALSE);
    }

    if(NULL_PTR != chttp_store)/*store data to storage as long as http recving*/
    {
        CHTTP_STORE *chttp_store_t;

        chttp_store_t = chttp_store_new();
        if(NULL_PTR == chttp_store_t)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: new chttp_store failed\n");

            chttp_node_store_done_nonblocking(chttp_node, chttp_store);/*for merge orign exception*/
            chttp_node_free(chttp_node);
            return (EC_FALSE);
        }

        CHTTP_NODE_STORE(chttp_node) = chttp_store_t;
        chttp_store_clone(chttp_store, CHTTP_NODE_STORE(chttp_node));
    }

    croutine_cond = croutine_cond_new(0/*never timeout*/, LOC_CHTTP_0031);
    if(NULL_PTR == croutine_cond)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: new croutine_cond failed\n");
     
        chttp_node_store_done_nonblocking(chttp_node, chttp_store);/*for merge orign exception*/
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
    CHTTP_NODE_CROUTINE_COND(chttp_node) = croutine_cond;

    CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/

    if(EC_FALSE == chttp_node_connect(chttp_node, CHTTP_REQ_IPADDR(chttp_req), CHTTP_REQ_PORT(chttp_req)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: connect server %s:%ld failed\n",
                            CHTTP_REQ_IPADDR_STR(chttp_req), CHTTP_REQ_PORT(chttp_req));

        chttp_node_store_done_nonblocking(chttp_node, chttp_store);/*for merge orign exception*/
     
        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    chttp_csocket_cnode_init(CMPI_ANY_MODI, csocket_cnode);
 
    if(EC_FALSE == chttp_node_encode_req_header(chttp_node,
                            CHTTP_REQ_METHOD(chttp_req), CHTTP_REQ_URI(chttp_req),
                            CHTTP_REQ_PARAM(chttp_req), CHTTP_REQ_HEADER(chttp_req))
     )
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: encode header failed\n");

        /*unbind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /*close http connection*/
        csocket_cnode_try_close(csocket_cnode);
        csocket_cnode_stat_dec();

        chttp_node_store_done_nonblocking(chttp_node, chttp_store);/*for merge orign exception*/
     
        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
 
    if(EC_FALSE == chttp_node_encode_req_body(chttp_node, CHTTP_REQ_BODY(chttp_req)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: encode body failed\n");

        /*unbind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        /*close http connection*/
        csocket_cnode_try_close(csocket_cnode);
        csocket_cnode_stat_dec();

        chttp_node_store_done_nonblocking(chttp_node, chttp_store);/*for merge orign exception*/
     
        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
 
    csocket_cnode_stat_inc();

    //dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_basic: croutine_cond %p reserved\n", croutine_cond);

    croutine_cond_reserve(croutine_cond, 1, LOC_CHTTP_0032);
    croutine_cond_wait(croutine_cond, LOC_CHTTP_0033);

    __COROUTINE_IF_EXCEPTION() {/*exception*/
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: coroutine was cancelled\n"); 
        if(NULL_PTR != CHTTP_NODE_CSOCKET_CNODE(chttp_node))
        {
            cepoll_del_all(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(CHTTP_NODE_CSOCKET_CNODE(chttp_node)));
        }

        /*when current coroutine was cancelled, blocking-mode is prohibitted*/
        chttp_node_store_done_nonblocking(chttp_node, chttp_store);  /*for merge orign termination in nonblocking mode*/
    } else {/*normal*/
        chttp_node_store_done_blocking(chttp_node, chttp_store);  /*for merge orign termination in blocking mode*/
    }
 
    chttp_node_set_billing(chttp_node, chttp_store); /*set billing in non-blocking mode*/
 
    /**
     *  when come back, check CHTTP_NODE_RECV_COMPLETE flag.
     *  if false, exception happened. and return false
     **/
    if(BIT_FALSE == CHTTP_NODE_RECV_COMPLETE(chttp_node))/*exception happened*/
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: exception happened\n");

        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);

        /*socket should not be used by others ...*/
        if(NULL_PTR != CHTTP_NODE_CSOCKET_CNODE(chttp_node))
        {
            /*unbind csocket_cnode and chttp_node*/
            CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
            CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

            /*close http connection*/
            csocket_cnode_force_close(csocket_cnode);
            csocket_cnode_stat_dec();   
        }
     
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
 
    if(NULL_PTR != CHTTP_NODE_CSOCKET_CNODE(chttp_node))
    {
        /*unbind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        dbg_log(SEC_0149_CHTTP, 5)(LOGSTDOUT, "[DEBUG] chttp_request_basic: try close socket %d\n", CSOCKET_CNODE_SOCKFD(csocket_cnode));
        /*close http connection*/
        csocket_cnode_try_close(csocket_cnode);
        csocket_cnode_stat_dec();   
    }

    /*get and check body len/content-length*/
    /*rsp_body_len = chttp_node_recv_len(chttp_node);*/
    rsp_body_len = CHTTP_NODE_BODY_PARSED_LEN(chttp_node);
    if(0 < rsp_body_len && 0 < CHTTP_NODE_CONTENT_LENGTH(chttp_node))
    {
        uint64_t content_len;
        content_len = CHTTP_NODE_CONTENT_LENGTH(chttp_node);
     
        if(content_len != rsp_body_len)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: body len %"PRId64" != content len %"PRId64"\n",
                            rsp_body_len, content_len);

            chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
            chttp_node_free(chttp_node);
            return (EC_FALSE);
        }
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_basic: body len %"PRId64", content len %"PRId64"\n",
                    rsp_body_len, CHTTP_NODE_CONTENT_LENGTH(chttp_node)); 

    /*handover http response*/
    CHTTP_RSP_STATUS(chttp_rsp) = (uint32_t)CHTTP_NODE_STATUS_CODE(chttp_node);

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request_basic: before handover, chttp_node: %p\n", chttp_node);
        chttp_node_print(LOGSTDOUT, chttp_node);
    }

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request_basic: before handover, chttp_rsp: %p\n", chttp_rsp);
        chttp_rsp_print(LOGSTDOUT, chttp_rsp);
    }
 
    cstrkv_mgr_handover(CHTTP_NODE_HEADER_IN_KVS(chttp_node), CHTTP_RSP_HEADER(chttp_rsp));

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request_basic: after handover, chttp_node: %p\n", chttp_node);
        chttp_node_print(LOGSTDOUT, chttp_node);
    }
 
    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request_basic: after handover, chttp_rsp: \n");
        chttp_rsp_print(LOGSTDOUT, chttp_rsp);
    }
 
    if(EC_TRUE == CHTTP_NODE_HEADER_MODIFIED_FLAG(chttp_node))
    {
        chttp_rsp_add_header(chttp_rsp, (const char *)"X_CACHE", (const char *)"TCP_REFRESH_HIT");
    }

    if(EC_TRUE == CHTTP_NODE_HEADER_EXPIRED_FLAG(chttp_node))
    {
        chttp_rsp_add_header(chttp_rsp, (const char *)"X_CACHE", (const char *)"TCP_REFRESH_MISS");
    } 

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request_basic: chttp_rsp: %p\n", chttp_rsp);
        chttp_rsp_print(LOGSTDOUT, chttp_rsp);
    } 

    /*Transfer-Encoding: chunked*/
    if(0 == CHTTP_NODE_CONTENT_LENGTH(chttp_node) && EC_TRUE == chttp_rsp_is_chunked(chttp_rsp))
    {
        CSTRKV *cstrkv;
     
        cstrkv = cstrkv_new((const char *)"Content-Length", c_word_to_str((UINT32)CHTTP_NODE_BODY_PARSED_LEN(chttp_node)));
        if(NULL_PTR == cstrkv)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: new cstrkv for chunked rsp failed\n");
            /*ignore this exception*/
        }
        else
        {
            cstrkv_mgr_add_kv(CHTTP_RSP_HEADER(chttp_rsp), cstrkv);
        }
     
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_basic: add %s:%s to rsp\n",
                        (char *)CSTRKV_KEY_STR(cstrkv), (char *)CSTRKV_VAL_STR(cstrkv));
    }

    /*dump body*/
    if(EC_FALSE == chunk_mgr_dump(CHTTP_NODE_RECV_BUF(chttp_node), &data, &data_len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_basic: dump response body failed\n");

        cstrkv_mgr_clean(CHTTP_RSP_HEADER(chttp_rsp));

        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_basic: dump response body len %ld\n", data_len);
    cbytes_mount(CHTTP_RSP_BODY(chttp_rsp), data_len, data);

    chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
    chttp_node_free(chttp_node);

    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * Try to connect remote http server to check connectivity (HEALTH CHECKER)
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/
EC_BOOL chttp_csocket_cnode_check(CSOCKET_CNODE *csocket_cnode)
{
    CHTTP_NODE *chttp_node;

    if(EC_FALSE == CSOCKET_CNODE_IS_CONNECTED(csocket_cnode))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_check: sockfd %d is not connected\n",
                           CSOCKET_CNODE_SOCKFD(csocket_cnode)); 
     
        return (EC_FALSE);
    }
 
    chttp_node = CSOCKET_CNODE_CHTTP_NODE(csocket_cnode);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_csocket_cnode_check: sockfd %d -> chttp_node is null\n",
                       CSOCKET_CNODE_SOCKFD(csocket_cnode));

        return (EC_FALSE);
    }

    __chttp_on_send_complete(chttp_node);

    /*note: return EC_DONE will trigger connection shutdown*/
    return (EC_DONE);
}

EC_BOOL chttp_node_check(CHTTP_NODE *chttp_node, const UINT32 ipaddr, const UINT32 port)
{
    CSOCKET_CNODE *csocket_cnode;
    int sockfd;
#if 0 
    if(EC_TRUE == __chttp_need_unix_domain_ipc(ipaddr, port))
    {
        if(EC_FALSE == csocket_unix_connect( ipaddr, port , CSOCKET_IS_NONBLOCK_MODE, &sockfd ))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_check: connect server unix@%s:%ld failed\n",
                                c_word_to_ipv4(ipaddr), port);
            return (EC_FALSE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check: socket %d connecting to server unix@%s:%ld\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);

        csocket_cnode = csocket_cnode_unix_new(CMPI_ERROR_TCID, sockfd, CSOCKET_TYPE_TCP, ipaddr, port);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_check:new csocket cnode for socket %d to server unix@%s:%ld failed\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }
    }
    else
#endif 
    {
        if(EC_FALSE == csocket_connect( ipaddr, port , CSOCKET_IS_NONBLOCK_MODE, &sockfd ))
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_check: connect server %s:%ld failed\n",
                                c_word_to_ipv4(ipaddr), port);
            return (EC_FALSE);
        }

        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check: socket %d connecting to server %s:%ld\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);

        if(EC_FALSE == csocket_is_connected(sockfd))/*not adaptive to unix domain socket*/
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_check: socket %d to server %s:%ld is not connected\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }

        if(do_log(SEC_0149_CHTTP, 5))
        {
            sys_log(LOGSTDOUT, "[DEBUG] chttp_node_check: client tcp stat:\n");
            csocket_tcpi_stat_print(LOGSTDOUT, sockfd);
        }

        csocket_cnode = csocket_cnode_new(CMPI_ERROR_TCID, sockfd, CSOCKET_TYPE_TCP, ipaddr, port);
        if(NULL_PTR == csocket_cnode)
        {
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_node_check:new csocket cnode for socket %d to server %s:%ld failed\n",
                            sockfd, c_word_to_ipv4(ipaddr), port);
            csocket_close(sockfd);
            return (EC_FALSE);
        }     
    }

    /* bind */
    CHTTP_NODE_CSOCKET_CNODE(chttp_node) = csocket_cnode;
    CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = chttp_node;

    cepoll_set_event(task_brd_default_get_cepoll(),
                    CSOCKET_CNODE_SOCKFD(csocket_cnode),
                    CEPOLL_WR_EVENT,
                    (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_check,
                    (void *)csocket_cnode);

    cepoll_set_complete(task_brd_default_get_cepoll(),
                   CSOCKET_CNODE_SOCKFD(csocket_cnode),
                   (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_close,
                   (void *)csocket_cnode);
                    
    cepoll_set_shutdown(task_brd_default_get_cepoll(),
                   CSOCKET_CNODE_SOCKFD(csocket_cnode),
                   (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_close,
                   (void *)csocket_cnode);

    cepoll_set_timeout(task_brd_default_get_cepoll(),
                   CSOCKET_CNODE_SOCKFD(csocket_cnode),
                   (uint32_t)CONN_TIMEOUT_NSEC,
                   (CEPOLL_EVENT_HANDLER)chttp_csocket_cnode_close,
                   (void *)csocket_cnode);
                    
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_node_check: sockfd %d set event WR\n",
                    CSOCKET_CNODE_SOCKFD(csocket_cnode)); 
    return (EC_TRUE);
}

EC_BOOL chttp_check(const CHTTP_REQ *chttp_req, CHTTP_STAT *chttp_stat)
{
    CHTTP_NODE    *chttp_node;
    CROUTINE_COND *croutine_cond;
    CSOCKET_CNODE *csocket_cnode;

    __COROUTINE_IF_EXCEPTION() {/*exception*/
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: found exception, refuse to launch http\n");
        return (EC_FALSE);
    }
 
    chttp_node = chttp_node_new(CHTTP_TYPE_HANDLE_CHECK);
    if(NULL_PTR == chttp_node)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: new chttp_node failed\n");
        return (EC_FALSE);
    }

    croutine_cond = croutine_cond_new(0/*never timeout*/, LOC_CHTTP_0034);
    if(NULL_PTR == croutine_cond)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: new croutine_cond failed\n");
     
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }
    CHTTP_NODE_CROUTINE_COND(chttp_node) = croutine_cond;

    //CHTTP_NODE_LOG_TIME_WHEN_START(chttp_node); /*record start time*/

    if(EC_FALSE == chttp_node_check(chttp_node, CHTTP_REQ_IPADDR(chttp_req), CHTTP_REQ_PORT(chttp_req)))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: check server %s:%ld failed\n",
                            CHTTP_REQ_IPADDR_STR(chttp_req), CHTTP_REQ_PORT(chttp_req));

        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }

    csocket_cnode = CHTTP_NODE_CSOCKET_CNODE(chttp_node);
    chttp_csocket_cnode_init(CMPI_ANY_MODI, csocket_cnode);

    csocket_cnode_stat_inc();

    croutine_cond_reserve(croutine_cond, 1, LOC_CHTTP_0035);
    croutine_cond_wait(croutine_cond, LOC_CHTTP_0036);

    __COROUTINE_CATCH_EXCEPTION() { /*exception*/
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: coroutine was cancelled\n"); 
        if(NULL_PTR != CHTTP_NODE_CSOCKET_CNODE(chttp_node))
        {
            cepoll_del_all(task_brd_default_get_cepoll(), CSOCKET_CNODE_SOCKFD(CHTTP_NODE_CSOCKET_CNODE(chttp_node)));
        }
    }__COROUTINE_HANDLE_EXCEPTION();
 
    /**
     *  when come back, check CHTTP_NODE_SEND_COMPLETE flag.
     *  if so, exception happened. and return false
     **/
    if(BIT_FALSE == CHTTP_NODE_SEND_COMPLETE(chttp_node))/*exception happened*/
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_check: exception happened\n");

        chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
        chttp_node_free(chttp_node);
        return (EC_FALSE);
    }

    if(NULL_PTR != CHTTP_NODE_CSOCKET_CNODE(chttp_node))
    {
        /*unbind csocket_cnode and chttp_node*/
        CSOCKET_CNODE_CHTTP_NODE(csocket_cnode) = NULL_PTR;
        CHTTP_NODE_CSOCKET_CNODE(chttp_node)    = NULL_PTR;

        dbg_log(SEC_0149_CHTTP, 5)(LOGSTDOUT, "[DEBUG] chttp_check: try close socket %d\n", CSOCKET_CNODE_SOCKFD(csocket_cnode));
     
        /*close http connection*/
        csocket_cnode_try_close(csocket_cnode);
        csocket_cnode_stat_dec();
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_check: OK\n"); 

    chttp_stat_clone(CHTTP_NODE_STAT(chttp_node), chttp_stat);
    chttp_node_free(chttp_node);

    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * Merge Http Request (MERGE ORIGIN FLOW)
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/
static EC_BOOL __chttp_request_merge_file_lock_over_http(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;
 
    char        *v;
 
    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    cstring_append_str(CHTTP_REQ_URI(&chttp_req_t), (uint8_t *)CHTTP_RFS_PREFIX"/lock_req");
    cstring_append_cstr(CHTTP_REQ_URI(&chttp_req_t), path);

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"Expires", (char *)c_word_to_str(expire_nsec));
    chttp_req_add_header(&chttp_req_t, (const char *)"tcid", (char *)c_word_to_ipv4(task_brd_default_get_tcid()));
 
    (*locked_already) = EC_FALSE;

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_INTERNAL_SERVER_ERROR == CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_FORBIDDEN == CHTTP_RSP_STATUS(&chttp_rsp_t))/*locked already*/
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => locked by other\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);

        (*locked_already) = EC_TRUE;
        return (EC_TRUE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    } 
 
    v = chttp_rsp_get_header(&chttp_rsp_t, (const char *)"auth-token");
    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u but not found auth-token\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    cstring_append_str(CHTTP_STORE_AUTH_TOKEN(chttp_store), (const UINT8 *)v);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_lock_over_bgn(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    UINT32       tcid;
    MOD_NODE     mod_node;
    EC_BOOL      ret;
 
    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, &tcid, NULL_PTR, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_lock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = 0;
    MOD_NODE_MODI(&mod_node) = 0;/*crfs_md_id = 0*/
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_lock, ERR_MODULE_ID, tcid, path, expire_nsec, CHTTP_STORE_AUTH_TOKEN(chttp_store), locked_already);

    if(EC_FALSE == ret)
    {
        return (EC_FALSE);
    }

    if(EC_TRUE == (*locked_already))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s => locked by other\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(tcid));
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_lock: [No.%ld] lock_req '%.*s' on %s => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(tcid));

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_lock(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    if(SWITCH_ON == NGX_BGN_OVER_HTTP_SWITCH)
    {
        return __chttp_request_merge_file_lock_over_http(chttp_req, chttp_store, path, expire_nsec, locked_already);
    }

    return __chttp_request_merge_file_lock_over_bgn(chttp_req, chttp_store, path, expire_nsec, locked_already);
}

static EC_BOOL __chttp_request_merge_file_unlock_over_http(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;
 
    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_unlock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    cstring_append_str(CHTTP_REQ_URI(&chttp_req_t), (uint8_t *)CHTTP_RFS_PREFIX"/unlock_req");
    cstring_append_cstr(CHTTP_REQ_URI(&chttp_req_t), path);

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"auth-token", (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store));

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_unlock_over_bgn(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    UINT32       tcid;
    MOD_NODE     mod_node;
    EC_BOOL      ret;
 
    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, &tcid, NULL_PTR, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_unlock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = 0;
    MOD_NODE_MODI(&mod_node) = 0;/*crfs_md_id = 0*/
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_unlock, ERR_MODULE_ID, path, CHTTP_STORE_AUTH_TOKEN(chttp_store));

    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_unlock: [No.%ld] unlock_req '%.*s' on %s => failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(tcid));
                 
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_unlock: [No.%ld] unlock_req '%.*s' on %s => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(tcid));

    return (EC_TRUE);
}
static EC_BOOL __chttp_request_merge_file_unlock(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    if(SWITCH_ON == NGX_BGN_OVER_HTTP_SWITCH)
    {
        return __chttp_request_merge_file_unlock_over_http(chttp_req, chttp_store, path);
    }

    return __chttp_request_merge_file_unlock_over_bgn(chttp_req, chttp_store, path);
}

static EC_BOOL __chttp_request_merge_file_read(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    CHTTP_REQ    chttp_req_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;

    CSTRING     *uri;
    //uint8_t     *data;
    //UINT32       data_len;
 
    chttp_req_init(&chttp_req_t);

    uri = CHTTP_REQ_URI(&chttp_req_t);
    cstring_append_str(uri, (uint8_t *)CHTTP_RFS_PREFIX"/getsmf");
    cstring_append_cstr(uri, path);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_read: [No.%ld] uri '%.*s'\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(uri), CSTRING_STR(uri));

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_read: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_read: [No.%ld] read '%.*s' from %s:%ld start ...\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
                     
    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");

    if(CHTTP_SEG_ERR_OFFSET != CHTTP_STORE_SEG_S_OFFSET(chttp_store)
    && CHTTP_SEG_ERR_OFFSET != CHTTP_STORE_SEG_E_OFFSET(chttp_store)
    && CHTTP_STORE_SEG_E_OFFSET(chttp_store) >= CHTTP_STORE_SEG_S_OFFSET(chttp_store))
    {
        uint32_t store_offset;
        uint32_t store_size;

        store_offset = CHTTP_STORE_SEG_S_OFFSET(chttp_store);
        store_size   = CHTTP_STORE_SEG_E_OFFSET(chttp_store) - CHTTP_STORE_SEG_S_OFFSET(chttp_store) + 1;
     
        chttp_req_add_header(&chttp_req_t, (const char *)"store-offset", (char *)c_uint32_to_str(store_offset));
        chttp_req_add_header(&chttp_req_t, (const char *)"store-size"  , (char *)c_uint32_to_str(store_size));
    }
 

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, chttp_rsp, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_read: [No.%ld] read '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_read: [No.%ld] read '%.*s' on %s:%ld back\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 

    if(CHTTP_OK != CHTTP_RSP_STATUS(chttp_rsp))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_read: [No.%ld] read '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(chttp_rsp));

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }
#if 0
    cbytes_umount(CHTTP_RSP_BODY(chttp_rsp), &data_len, &data);
    chttp_rsp_clean(chttp_rsp);
 
    if(EC_FALSE == chttp_rsp_decode(chttp_rsp, data, (uint32_t)data_len))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_read: [No.%ld] read '%.*s' on %s:%ld, decode failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);
                     
        cbytes_mount(CHTTP_RSP_BODY(chttp_rsp), data_len, data);
        chttp_rsp_clean(chttp_rsp);
     
        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }
#endif
    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_read: [No.%ld] read '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 

    chttp_req_clean(&chttp_req_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_retire(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;

    CSTRING     *uri;

    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    uri = CHTTP_REQ_URI(&chttp_req_t);
    cstring_append_str(uri, (uint8_t *)CHTTP_RFS_PREFIX"/dsmf");
    cstring_append_cstr(uri, path);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_retire: [No.%ld] uri '%.*s'\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(uri), CSTRING_STR(uri));

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_retire: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
 
    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_retire: [No.%ld] file_retire '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_retire: [No.%ld] file_retire '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_retire: [No.%ld] file_retire '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_wait(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat, UINT32 *data_ready)
{
    CHTTP_REQ    chttp_req_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;

    CSTRING     *uri;

    char        *v;

    chttp_req_init(&chttp_req_t);

    uri = CHTTP_REQ_URI(&chttp_req_t);
    cstring_append_str(uri, (uint8_t *)CHTTP_RFS_PREFIX"/file_wait");
    cstring_append_cstr(uri, path);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_wait: [No.%ld] uri '%.*s'\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(uri), CSTRING_STR(uri));

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    (*data_ready) = EC_FALSE;

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"tcid", (char *)c_word_to_ipv4(task_brd_default_get_tcid()));
    chttp_req_add_header(&chttp_req_t, (const char *)"wait-data", (char *)"yes");
 
    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, chttp_rsp, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait: [No.%ld] file_wait '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(chttp_rsp))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait: [No.%ld] file_wait '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(chttp_rsp));

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    v = chttp_rsp_get_header(chttp_rsp, (const char *)"data-ready");
    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait: [No.%ld] file_wait '%.*s' on %s:%ld => status %u but not found data-ready\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(chttp_rsp));

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    (*data_ready) = c_str_to_bool(v);

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_wait: [No.%ld] file_wait '%.*s' on %s:%ld => OK, data_ready: '%s' [%ld]\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                    v, (*data_ready));

    chttp_req_clean(&chttp_req_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_merge_file_wait_ready(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    CHTTP_REQ    chttp_req_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;

    CSTRING     *uri;

    chttp_req_init(&chttp_req_t);

    uri = CHTTP_REQ_URI(&chttp_req_t);
    cstring_append_str(uri, (uint8_t *)CHTTP_RFS_PREFIX"/file_wait");
    cstring_append_cstr(uri, path);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_merge_file_wait_ready: [No.%ld] uri '%.*s'\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(uri), CSTRING_STR(uri));

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait_ready: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path));
        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"tcid", (char *)c_word_to_ipv4(task_brd_default_get_tcid()));
    chttp_req_add_header(&chttp_req_t, (const char *)"wait-data", (char *)"no");

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, chttp_rsp, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait_ready: [No.%ld] file_wait '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(chttp_rsp))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_merge_file_wait_ready: [No.%ld] file_wait '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(chttp_rsp));

        chttp_req_clean(&chttp_req_t);
        return (EC_FALSE);
    }

    chttp_req_clean(&chttp_req_t);

    return (EC_TRUE);
}

/*(NO WAIT)*/
static EC_BOOL __chttp_request_merge_file_orig(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    TASK_BRD      *task_brd;
    MOD_NODE       recv_mod_node;

    CHTTP_STORE   *chttp_store_t;
    EC_BOOL        merge_flag_saved;

    /*make receiver: the current rank, i.e. send to itself*/
    task_brd = task_brd_default_get();
    MOD_NODE_TCID(&recv_mod_node) = TASK_BRD_TCID(task_brd);
    MOD_NODE_COMM(&recv_mod_node) = TASK_BRD_COMM(task_brd);
    MOD_NODE_RANK(&recv_mod_node) = TASK_BRD_RANK(task_brd);
    MOD_NODE_MODI(&recv_mod_node) = 0;/*only one super*/

    /*trick: we cannot send merge flag to super and we want to reduce chttp_store clone, so ...*/
    chttp_store_t = (CHTTP_STORE *)chttp_store; /*save*/
    merge_flag_saved = CHTTP_STORE_MERGE_FLAG(chttp_store_t);
    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = EC_FALSE; /*clean*/
#if 0
    if(0 == CHTTP_STORE_SEG_ID(chttp_store) && EC_TRUE == CHTTP_STORE_EXPIRED_FLAG(chttp_store))
    {
        /*blocking origin flow to prevent wait-data from returning dirty data (old seg-0)*/
        task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                &recv_mod_node,
                NULL_PTR, FI_super_http_request, ERR_MODULE_ID, chttp_req, chttp_store_t, chttp_rsp, chttp_stat);
    }
    else
#endif 
    {
        task_p2p_no_wait(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP,
                    &recv_mod_node,
                    NULL_PTR, FI_super_http_request_merge, ERR_MODULE_ID, chttp_req, chttp_store_t, chttp_rsp, chttp_stat);
    }

    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = merge_flag_saved; /*restore*/
 
    return (EC_TRUE);
}

EC_BOOL chttp_request_merge(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    UINT32         locked_already;
 
    UINT32         timeout_msec;
    UINT32         expire_nsec;

    UINT32         tag;

    CHTTP_STORE   *chttp_store_t;
    CSTRING        path;

    CHTTP_STAT     chttp_stat_t; /*only for merge procedure statistics*/

    ASSERT(NULL_PTR != chttp_store);
    ASSERT(EC_TRUE == CHTTP_STORE_MERGE_FLAG(chttp_store));

    chttp_store_t = chttp_store_new();
    if(NULL_PTR == chttp_store_t)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: new chttp_store failed\n");
        return (EC_FALSE);
    }
    chttp_store_clone(chttp_store, chttp_store_t);
    CHTTP_STORE_SEQ_NO_GEN(chttp_store_t);

    chttp_stat_init(&chttp_stat_t);
    CHTTP_STAT_LOG_MERGE_TIME_WHEN_START(&chttp_stat_t); /*record merge start time*/

    timeout_msec = 60 * 1000;
    expire_nsec  = 60;
    tag          = MD_CRFS;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store_t, &path);
 
    /*s1. file lock: acquire auth-token*/
    locked_already = EC_FALSE;

    if(EC_FALSE == __chttp_request_merge_file_lock(chttp_req, chttp_store_t, &path, expire_nsec, &locked_already))
    {
        CHTTP_STAT_LOG_MERGE_TIME_WHEN_LOCKED(&chttp_stat_t); /*record merge locked done time*/
        CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_LOCKED_ERR [No.%ld]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] file lock '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);
    }

    CHTTP_STAT_LOG_MERGE_TIME_WHEN_LOCKED(&chttp_stat_t); /*record merge locked done time*/

    if(EC_TRUE == locked_already)
    {
        /*[N] means this is not the auth-token owner*/
        CHTTP_STORE_LOCKED_FLAG(chttp_store_t) = EC_FALSE;
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] file lock '%.*s' => auth-token: (null)\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 

        if(0 == CHTTP_STORE_SEG_ID(chttp_store_t) && EC_TRUE == CHTTP_STORE_EXPIRED_FLAG(chttp_store_t))
        {
            if(EC_FALSE == __chttp_request_merge_file_wait_ready(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat))
            {
                CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_ready done time*/
                CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_READY_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [N] wait_ready '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
                return (EC_FALSE);                     
            }

            CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_ready done time*/

            chttp_rsp_clean(chttp_rsp);
         
            dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] wait_ready '%.*s' done\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));     
        }
        else
        {
            UINT32         data_ready;
         
            data_ready = EC_FALSE;
            if(EC_FALSE == __chttp_request_merge_file_wait(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat, &data_ready))
            {
                CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/
                CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_DATA_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [N] wait '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
                return (EC_FALSE);                     
            }

            if(EC_TRUE == data_ready)
            {
                CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/
                CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_DATA_SUCC [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] wait '%.*s' done and data ready\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
                return (EC_TRUE);
            }

            CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/

            chttp_rsp_clean(chttp_rsp);
         
            dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] wait '%.*s' done and data not ready\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));     
        }

        /*s3: wait being waken up or timeout*/ 
        if(EC_FALSE == super_cond_wait(0, tag, &path, timeout_msec))
        {
            CHTTP_STAT_LOG_MERGE_TIME_WHEN_CONDED(&chttp_stat_t); /*record merge cond_wait done time*/
            CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_COND_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [N] cond wait '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);
        }

        CHTTP_STAT_LOG_MERGE_TIME_WHEN_CONDED(&chttp_stat_t); /*record merge cond_wait done time*/

        /*after come back*/
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] cond wait '%.*s' => back\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));

        /*s5: read file from storage*/
        if(EC_FALSE == __chttp_request_merge_file_read(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat))
        {
            CHTTP_STAT_LOG_MERGE_TIME_WHEN_READ(&chttp_stat_t); /*record merge file_read done time*/
            CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_READ_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [N] read '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);                     
        }

        CHTTP_STAT_LOG_MERGE_TIME_WHEN_READ(&chttp_stat_t); /*record merge file_read done time*/
        CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_OK [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_MERGE_NO_PRINT(&chttp_stat_t);

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [N] read '%.*s' => succ\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_TRUE);
    }

    CHTTP_STORE_LOCKED_FLAG(chttp_store_t) = EC_TRUE;

    /*[Y] means this is the auth-token owner*/
    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] file lock '%.*s' => auth-token: %.*s\n",
                CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path),
                CHTTP_STORE_AUTH_TOKEN_LEN(chttp_store_t), (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store_t));

    if(0 == CHTTP_STORE_SEG_ID(chttp_store_t) && EC_TRUE == CHTTP_STORE_EXPIRED_FLAG(chttp_store_t))
    {
        if(EC_FALSE == __chttp_request_merge_file_wait_ready(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat))
        {
            CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_ready done time*/
            CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_READY_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [Y] wait_ready '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);                     
        }

        CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_ready done time*/

        chttp_rsp_clean(chttp_rsp);
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] wait_ready '%.*s' done\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    }
    else
    {
        UINT32         data_ready;
     
        data_ready = EC_FALSE;
        if(EC_FALSE == __chttp_request_merge_file_wait(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat, &data_ready))
        {
            CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/
            CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_DATA_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
     
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [Y] wait '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);                     
        }

        if(EC_TRUE == data_ready)
        {
            CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/
            CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_WAIT_DATA_SUCC [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
     
            dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] wait '%.*s' done and data ready\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_TRUE);
        }

        CHTTP_STAT_LOG_MERGE_TIME_WHEN_WAITED(&chttp_stat_t); /*record merge wait_data done time*/
     
        chttp_rsp_clean(chttp_rsp);
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] wait '%.*s' done and data not ready\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    }                 
    /*s2: http orig (NO WAIT and only happen on local in order to wakeup later)*/
    if(EC_FALSE == __chttp_request_merge_file_orig(chttp_req, chttp_store_t, chttp_rsp, chttp_stat))
    {
        CHTTP_STAT_LOG_MERGE_TIME_WHEN_ORIGED(&chttp_stat_t); /*record merge orig done time*/
        CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_ORIG_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [Y] http orig '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);
    }

    CHTTP_STAT_LOG_MERGE_TIME_WHEN_ORIGED(&chttp_stat_t); /*record merge orig done time*/

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] http orig '%.*s' => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 

    /*s3: wait being waken up or timeout*/ 
    if(EC_FALSE == super_cond_wait(0, tag, &path, timeout_msec))
    {
        CHTTP_STAT_LOG_MERGE_TIME_WHEN_CONDED(&chttp_stat_t); /*record merge cond_wait done time*/
        CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_COND_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
 
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [Y] cond wait '%.*s' failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);                     
    }

    CHTTP_STAT_LOG_MERGE_TIME_WHEN_CONDED(&chttp_stat_t); /*record merge cond_wait done time*/

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] cond wait '%.*s' => back\n",
                CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 
#if 0
    /*s4: file unlock: remove the locked-file from remote. despite of response status*/
    if(EC_FALSE == __chttp_request_merge_file_unlock(chttp_req, chttp_store_t, &path))
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "warn:chttp_request_merge: [No.%ld] [Y] file unlock '%.*s', auth-token: '%.*s' => failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), CSTRING_LEN(&path), (char *)CSTRING_STR(&path),
                    CHTTP_STORE_AUTH_TOKEN_LEN(chttp_store_t), (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store_t));
    }
    else
    {
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] file unlock '%.*s', auth-token: '%.*s' => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), CSTRING_LEN(&path), (char *)CSTRING_STR(&path),
                    CHTTP_STORE_AUTH_TOKEN_LEN(chttp_store_t), (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store_t));
    }
#endif
    /*s5: read file from storage*/
    if(EC_FALSE == __chttp_request_merge_file_read(chttp_req, chttp_store_t, &path, chttp_rsp, chttp_stat))
    {
        CHTTP_STAT_LOG_MERGE_TIME_WHEN_READ(&chttp_stat_t); /*record merge file_read done time*/
        CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_READ_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_merge: [No.%ld] [Y] read '%.*s' failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);                     
    }

    CHTTP_STAT_LOG_MERGE_TIME_WHEN_READ(&chttp_stat_t); /*record merge file_read done time*/
    CHTTP_STAT_LOG_MERGE_STAT_WHEN_DONE(&chttp_stat_t, "MERGE_OK [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
    CHTTP_STAT_LOG_MERGE_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_merge: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    CHTTP_STAT_LOG_MERGE_YES_PRINT(&chttp_stat_t);

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_merge: [No.%ld] [Y] read '%.*s' => succ\n",
                CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
             
    chttp_store_free(chttp_store_t);
    cstring_clean(&path);
    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * Header Http Request (only token owner would store header to storage)
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/

static EC_BOOL __chttp_request_header_file_lock_over_http(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;
 
    char        *v;
 
    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    cstring_append_str(CHTTP_REQ_URI(&chttp_req_t), (uint8_t *)CHTTP_RFS_PREFIX"/lock_req");
    cstring_append_cstr(CHTTP_REQ_URI(&chttp_req_t), path);

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"Expires", (char *)c_word_to_str(expire_nsec));
    chttp_req_add_header(&chttp_req_t, (const char *)"tcid", (char *)c_word_to_ipv4(task_brd_default_get_tcid()));
 
    (*locked_already) = EC_FALSE;

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_INTERNAL_SERVER_ERROR == CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_FORBIDDEN == CHTTP_RSP_STATUS(&chttp_rsp_t))/*locked already*/
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => locked by other\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);

        (*locked_already) = EC_TRUE;
        return (EC_TRUE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    } 
 
    v = chttp_rsp_get_header(&chttp_rsp_t, (const char *)"auth-token");
    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => status %u but not found auth-token\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    cstring_append_str(CHTTP_STORE_AUTH_TOKEN(chttp_store), (const UINT8 *)v);

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_header_file_lock_over_bgn(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    UINT32       tcid;
    MOD_NODE     mod_node;
    EC_BOOL      ret;
 
    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, &tcid, NULL_PTR, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_lock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = 0;
    MOD_NODE_MODI(&mod_node) = 0;/*crfs_md_id = 0*/
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_lock, ERR_MODULE_ID, tcid, path, expire_nsec, CHTTP_STORE_AUTH_TOKEN(chttp_store), locked_already);

    if(EC_FALSE == ret)
    {
        return (EC_FALSE);
    }

    if(EC_TRUE == (*locked_already))
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s => locked by other\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(tcid));
        return (EC_TRUE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_lock: [No.%ld] lock_req '%.*s' on %s => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(tcid));

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_header_file_lock(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, const CSTRING *path, const UINT32 expire_nsec, UINT32 *locked_already)
{
    if(SWITCH_ON == NGX_BGN_OVER_HTTP_SWITCH)
    {
        return __chttp_request_header_file_lock_over_http(chttp_req, chttp_store, path, expire_nsec, locked_already);
    }

    return __chttp_request_header_file_lock_over_bgn(chttp_req, chttp_store, path, expire_nsec, locked_already);
}

static EC_BOOL __chttp_request_header_file_unlock_over_http(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;
 
    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_unlock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    cstring_append_str(CHTTP_REQ_URI(&chttp_req_t), (uint8_t *)CHTTP_RFS_PREFIX"/unlock_req");
    cstring_append_cstr(CHTTP_REQ_URI(&chttp_req_t), path);

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"auth-token", (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store));

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_unlock: [No.%ld] unlock_req '%.*s' on %s:%ld => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port); 

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

static EC_BOOL __chttp_request_header_file_unlock_over_bgn(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    UINT32       tcid;
    MOD_NODE     mod_node;
    EC_BOOL      ret;
 
    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, &tcid, NULL_PTR, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_unlock: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = 0;
    MOD_NODE_MODI(&mod_node) = 0;/*crfs_md_id = 0*/
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_unlock, ERR_MODULE_ID, path, CHTTP_STORE_AUTH_TOKEN(chttp_store));

    if(EC_FALSE == ret)
    {
        dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_unlock: [No.%ld] unlock_req '%.*s' on %s => failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(tcid));
                 
        return (EC_FALSE);
    }

    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_unlock: [No.%ld] unlock_req '%.*s' on %s => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                    c_word_to_ipv4(tcid));

    return (EC_TRUE);
}
static EC_BOOL __chttp_request_header_file_unlock(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path)
{
    if(SWITCH_ON == NGX_BGN_OVER_HTTP_SWITCH)
    {
        return __chttp_request_header_file_unlock_over_http(chttp_req, chttp_store, path);
    }

    return __chttp_request_header_file_unlock_over_bgn(chttp_req, chttp_store, path);
}

static EC_BOOL __chttp_request_header_file_orig_cache(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    CHTTP_STORE   *chttp_store_t;
    EC_BOOL        merge_flag_saved;
    UINT32         super_md_id;
    EC_BOOL        ret;

    /*trick: we cannot send merge flag to super and we want to reduce chttp_store clone, so ...*/
    chttp_store_t = (CHTTP_STORE *)chttp_store; /*save*/
    merge_flag_saved = CHTTP_STORE_MERGE_FLAG(chttp_store_t);
    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = EC_FALSE; /*clean*/
    /*note: filter determine to cache or not*/
    super_md_id = 0;
    ret = super_http_request(super_md_id, chttp_req, chttp_store_t, chttp_rsp, chttp_stat);
 
    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = merge_flag_saved; /*restore*/
 
    return (ret);
}

static EC_BOOL __chttp_request_header_file_orig_no_cache(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    CHTTP_STORE   *chttp_store_t;
    EC_BOOL        merge_flag_saved;
    UINT32         cache_ctrl_saved;
    UINT32         super_md_id;
    EC_BOOL        ret;

    /*trick: we cannot send merge flag to super and we want to reduce chttp_store clone, so ...*/
    chttp_store_t = (CHTTP_STORE *)chttp_store; /*save*/
    merge_flag_saved = CHTTP_STORE_MERGE_FLAG(chttp_store_t);
    cache_ctrl_saved = CHTTP_STORE_CACHE_CTRL(chttp_store_t);
 
    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = EC_FALSE; /*clean*/
    CHTTP_STORE_CACHE_CTRL(chttp_store_t) = CHTTP_STORE_CACHE_NONE;/*force not to cache*/
    dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_orig_no_cache: set CHTTP_STORE_CACHE_NONE\n");

    super_md_id = 0;
    ret = super_http_request(super_md_id, chttp_req, chttp_store_t, chttp_rsp, chttp_stat);
 
    CHTTP_STORE_MERGE_FLAG(chttp_store_t) = merge_flag_saved; /*restore*/
    CHTTP_STORE_CACHE_CTRL(chttp_store_t) = cache_ctrl_saved; /*resotre*/
 
    return (ret);
}

static EC_BOOL __chttp_request_header_file_wait_header(const CHTTP_REQ *chttp_req, const CHTTP_STORE *chttp_store, const CSTRING *path, const CSTRKV_MGR *cstrkv_mgr, UINT32 *header_ready)
{
    CHTTP_REQ    chttp_req_t;
    CHTTP_RSP    chttp_rsp_t;

    UINT32       store_srv_ipaddr;
    UINT32       store_srv_port;

    CLIST_DATA  *clist_data;
    uint32_t     idx;

    char        *v;

    /*determine storage server*/
    if(EC_FALSE == chttp_store_srv_get(chttp_store, path, NULL_PTR, &store_srv_ipaddr, &store_srv_port))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_wait_header: [No.%ld] determine storage server for '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path));
        return (EC_FALSE);
    }

    chttp_req_init(&chttp_req_t);
    chttp_rsp_init(&chttp_rsp_t);

    cstring_append_str(CHTTP_REQ_URI(&chttp_req_t), (uint8_t *)CHTTP_RFS_PREFIX"/wait_header");
    cstring_append_cstr(CHTTP_REQ_URI(&chttp_req_t), path);

    chttp_req_set_ipaddr_word(&chttp_req_t, store_srv_ipaddr);
    chttp_req_set_port_word(&chttp_req_t, store_srv_port);
    chttp_req_set_method(&chttp_req_t, (const char *)"GET");

    chttp_req_add_header(&chttp_req_t, (const char *)"Host", (char *)"127.0.0.1");
    chttp_req_add_header(&chttp_req_t, (const char *)"Connection", (char *)"Keep-Alive");
    chttp_req_add_header(&chttp_req_t, (const char *)"Content-Length", (char *)"0");
    chttp_req_add_header(&chttp_req_t, (const char *)"tcid", (char *)c_word_to_ipv4(task_brd_default_get_tcid()));

    idx = 0;
    CLIST_LOOP_NEXT(CSTRKV_MGR_LIST(cstrkv_mgr), clist_data)
    {
        CSTRKV   *cstrkv;

        char     wait_key_tag[ 16 ];
        char     wait_val_tag[ 16 ];
     
        cstrkv = CLIST_DATA_DATA(clist_data);
        if(NULL_PTR == cstrkv)
        {
            continue;
        }

        idx ++;
        snprintf(wait_key_tag, sizeof(wait_key_tag)/sizeof(wait_key_tag[ 0 ]), "wait-key-%u", idx);
        snprintf(wait_val_tag, sizeof(wait_val_tag)/sizeof(wait_val_tag[ 0 ]), "wait-val-%u", idx);

        chttp_req_add_header(&chttp_req_t, (const char *)wait_key_tag, (char *)CSTRKV_KEY_STR(cstrkv));
        chttp_req_add_header(&chttp_req_t, (const char *)wait_val_tag, (char *)CSTRKV_VAL_STR(cstrkv));
    }

    chttp_req_add_header(&chttp_req_t, (const char *)"wait-num", (char *)c_uint32_to_str(idx));

    if(EC_FALSE == chttp_request_basic(&chttp_req_t, NULL_PTR, &chttp_rsp_t, NULL_PTR))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_wait_header: [No.%ld] wait headers of '%.*s' on %s:%ld failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port);

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    if(CHTTP_OK != CHTTP_RSP_STATUS(&chttp_rsp_t))
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_wait_header: [No.%ld] wait headers of '%.*s' on %s:%ld => status %u\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), (char *)CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);

        return (EC_FALSE);
    }

    v = chttp_rsp_get_header(&chttp_rsp_t, (const char *)"header-ready");
    if(NULL_PTR == v)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:__chttp_request_header_file_wait_header: [No.%ld] wait headers '%.*s' on %s:%ld => status %u but not found header-ready\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                        c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                        CHTTP_RSP_STATUS(&chttp_rsp_t));

        chttp_req_clean(&chttp_req_t);
        chttp_rsp_clean(&chttp_rsp_t);
        return (EC_FALSE);
    }

    (*header_ready) = c_str_to_bool(v);

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] __chttp_request_header_file_wait_header: [No.%ld]wait  headers '%.*s' on %s:%ld => OK, header_ready: '%s' [%ld]\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store), (uint32_t)CSTRING_LEN(path), CSTRING_STR(path),
                    c_word_to_ipv4(store_srv_ipaddr), store_srv_port,
                    v, (*header_ready));

    chttp_req_clean(&chttp_req_t);
    chttp_rsp_clean(&chttp_rsp_t);

    return (EC_TRUE);
}

/*request header only*/
EC_BOOL chttp_request_header(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    UINT32         locked_already;
 
    UINT32         timeout_msec;
    UINT32         expire_nsec;

    UINT32         tag;

    CHTTP_STORE   *chttp_store_t;
    CSTRING        path;

    CHTTP_STAT     chttp_stat_t; /*only for merge procedure statistics*/

    ASSERT(NULL_PTR != chttp_store);
    ASSERT(EC_TRUE == CHTTP_STORE_MERGE_FLAG(chttp_store));
    ASSERT(0 == CHTTP_STORE_SEG_ID(chttp_store));

    chttp_store_t = chttp_store_new();
    if(NULL_PTR == chttp_store_t)
    {
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: new chttp_store failed\n");
        return (EC_FALSE);
    }
    chttp_store_clone(chttp_store, chttp_store_t);
    CHTTP_STORE_SEQ_NO_GEN(chttp_store_t);

    chttp_stat_init(&chttp_stat_t);
    CHTTP_STAT_LOG_HEADER_TIME_WHEN_START(&chttp_stat_t); /*record header start time*/

    timeout_msec = 60 * 1000;
    expire_nsec  = 60;
    tag          = MD_CRFS;

    /*make path*/
    cstring_init(&path, NULL_PTR);
    chttp_store_path_get(chttp_store_t, &path);
 
    /*s1. file lock: acquire auth-token*/
    locked_already = EC_FALSE;

    if(EC_FALSE == __chttp_request_header_file_lock(chttp_req, chttp_store_t, &path, expire_nsec, &locked_already))
    {
        CHTTP_STAT_LOG_HEADER_TIME_WHEN_LOCKED(&chttp_stat_t); /*record header locked done time*/
        CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_LOCKED_ERR [No.%ld]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] file lock '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);
    }

    CHTTP_STAT_LOG_HEADER_TIME_WHEN_LOCKED(&chttp_stat_t); /*record header locked done time*/

    if(EC_TRUE == locked_already)
    {
        /*[N] means this is not the auth-token owner*/
        CHTTP_STORE_LOCKED_FLAG(chttp_store_t) = EC_FALSE;
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [N] file lock '%.*s' => auth-token: (null)\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path)); 

        if(EC_FALSE == __chttp_request_header_file_orig_no_cache(chttp_req, chttp_store_t, chttp_rsp, chttp_stat))
        {
            CHTTP_STAT_LOG_HEADER_TIME_WHEN_ORIGED(&chttp_stat_t); /*record header orig done time*/
            CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_ORIG_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_HEADER_NO_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [N] http orig '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);
        }

        CHTTP_STAT_LOG_HEADER_TIME_WHEN_ORIGED(&chttp_stat_t); /*record header orig done time*/
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [N] http orig '%.*s' done\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));

        /*wait being waken up or timeout if old dir would be deleted*/ 
        if(EC_TRUE == chttp_rsp_has_header(chttp_rsp, (const char *)"X_CACHE", (const char *)"TCP_REFRESH_MISS"))
        {
            CSTRKV_MGR cstrkv_mgr;
            UINT32     header_ready;

            cstrkv_mgr_init(&cstrkv_mgr);

            chttp_rsp_fetch_header(chttp_rsp, (char *)"Expires"      , &cstrkv_mgr);
            chttp_rsp_fetch_header(chttp_rsp, (char *)"Date"         , &cstrkv_mgr);
            chttp_rsp_fetch_header(chttp_rsp, (char *)"Cache-Control", &cstrkv_mgr);

            header_ready = EC_TRUE;

            if(EC_FALSE == __chttp_request_header_file_wait_header(chttp_req, chttp_store_t, &path, &cstrkv_mgr, &header_ready))
            {
                CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
                CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_WAIT_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_HEADER_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [N] wait header '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
             
                cstrkv_mgr_clean(&cstrkv_mgr);
                return (EC_FALSE);
            }

            cstrkv_mgr_clean(&cstrkv_mgr);

            if(EC_TRUE == header_ready)
            {
                CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
                CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_WAIT_SUCC [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_HEADER_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [N] wait '%.*s' done and header ready\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
                return (EC_TRUE);
            }
         
            CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
     
            /* dangerous! */
            /* if orig procedure owner had run faster and end the whole procedure, */
            /* then nobody would wake up it, 60 seconds would be wasted :-(        */
            if(EC_FALSE == super_cond_wait(0, tag, &path, timeout_msec))
            {
                CHTTP_STAT_LOG_HEADER_TIME_WHEN_CONDED(&chttp_stat_t); /*record header orig cond_wait done time*/
                CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_COND_ERR [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
                CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                CHTTP_STAT_LOG_HEADER_NO_PRINT(&chttp_stat_t);
             
                dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [N] cond wait '%.*s' failed\n",
                            CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
                chttp_store_free(chttp_store_t);
                cstring_clean(&path);
                return (EC_FALSE);
            }
         
            dbg_log(SEC_0149_CHTTP, 9)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [N] cond wait '%.*s' done\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        }     
     
        CHTTP_STAT_LOG_HEADER_TIME_WHEN_CONDED(&chttp_stat_t); /*record header orig cond_wait done time*/
        CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_OK [No.%ld] [N]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_HEADER_NO_PRINT(&chttp_stat_t);

        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [N] '%.*s' => OK\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_TRUE);
    }

    CHTTP_STORE_LOCKED_FLAG(chttp_store_t) = EC_TRUE;

    /*[Y] means this is the auth-token owner*/
    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [Y] file lock '%.*s' => auth-token: %.*s\n",
                CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path),
                CHTTP_STORE_AUTH_TOKEN_LEN(chttp_store_t), (char *)CHTTP_STORE_AUTH_TOKEN_STR(chttp_store_t));
                    
    /*http orig and store header to cache if need*/
    if(EC_FALSE == __chttp_request_header_file_orig_cache(chttp_req, chttp_store_t, chttp_rsp, chttp_stat))
    {
        CHTTP_STAT_LOG_HEADER_TIME_WHEN_ORIGED(&chttp_stat_t); /*record header orig done time*/
        CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_ORIG_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
        CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);
     
        dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [Y] http orig '%.*s' failed\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
        chttp_store_free(chttp_store_t);
        cstring_clean(&path);
        return (EC_FALSE);
    }

    CHTTP_STAT_LOG_HEADER_TIME_WHEN_ORIGED(&chttp_stat_t); /*record header orig done time*/

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [Y] http orig '%.*s' done\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));

    /*wait being waken up or timeout if old dir would be deleted*/ 
    if(EC_TRUE == chttp_rsp_has_header(chttp_rsp, (const char *)"X_CACHE", (const char *)"TCP_REFRESH_MISS"))
    {
        CSTRKV_MGR cstrkv_mgr;
        UINT32     header_ready;

        cstrkv_mgr_init(&cstrkv_mgr);

        chttp_rsp_fetch_header(chttp_rsp, (char *)"Expires"      , &cstrkv_mgr);
        chttp_rsp_fetch_header(chttp_rsp, (char *)"Date"         , &cstrkv_mgr);
        chttp_rsp_fetch_header(chttp_rsp, (char *)"Cache-Control", &cstrkv_mgr);

        header_ready = EC_TRUE;

        if(EC_FALSE == __chttp_request_header_file_wait_header(chttp_req, chttp_store_t, &path, &cstrkv_mgr, &header_ready))
        {
            CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
            CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_WAIT_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [Y] wait header '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
         
            cstrkv_mgr_clean(&cstrkv_mgr);
            return (EC_FALSE);
        }

        cstrkv_mgr_clean(&cstrkv_mgr);

        if(EC_TRUE == header_ready)
        {
            CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
            CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_WAIT_SUCC [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [Y] wait '%.*s' done and header ready\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_TRUE);
        }
     
        CHTTP_STAT_LOG_HEADER_TIME_WHEN_WAITED(&chttp_stat_t); /*record header wait_header done time*/
 
        if(EC_FALSE == super_cond_wait(0, tag, &path, timeout_msec))
        {
            CHTTP_STAT_LOG_HEADER_TIME_WHEN_CONDED(&chttp_stat_t); /*record header orig cond_wait done time*/
            CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_COND_ERR [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
            CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "error:chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);
         
            dbg_log(SEC_0149_CHTTP, 0)(LOGSTDOUT, "error:chttp_request_header: [No.%ld] [Y] cond wait '%.*s' failed\n",
                        CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
            chttp_store_free(chttp_store_t);
            cstring_clean(&path);
            return (EC_FALSE);
        }
     
        dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [Y] cond wait '%.*s' done\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    }

    CHTTP_STAT_LOG_HEADER_TIME_WHEN_CONDED(&chttp_stat_t); /*record header orig cond_wait done time*/
    CHTTP_STAT_LOG_HEADER_STAT_WHEN_DONE(&chttp_stat_t, "HEADER_OK [No.%ld] [Y]", CHTTP_STORE_SEQ_NO_GET(chttp_store_t));
    CHTTP_STAT_LOG_HEADER_INFO_WHEN_DONE(&chttp_stat_t, "[DEBUG] chttp_request_header: %.*s", (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    CHTTP_STAT_LOG_HEADER_YES_PRINT(&chttp_stat_t);

    dbg_log(SEC_0149_CHTTP, 1)(LOGSTDOUT, "[DEBUG] chttp_request_header: [No.%ld] [Y] '%.*s' => OK\n",
                    CHTTP_STORE_SEQ_NO_GET(chttp_store_t), (uint32_t)CSTRING_LEN(&path), (char *)CSTRING_STR(&path));
    chttp_store_free(chttp_store_t);
    cstring_clean(&path);
    return (EC_TRUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------\
 *
 * General Http Request Entry
 *
\*-------------------------------------------------------------------------------------------------------------------------------------------*/

EC_BOOL chttp_request(const CHTTP_REQ *chttp_req, CHTTP_STORE *chttp_store, CHTTP_RSP *chttp_rsp, CHTTP_STAT *chttp_stat)
{
    if(NULL_PTR == chttp_store)
    {
        return chttp_request_basic(chttp_req, chttp_store, chttp_rsp, chttp_stat); /*normal http request*/
    }

    if(do_log(SEC_0149_CHTTP, 9))
    {
        sys_log(LOGSTDOUT, "[DEBUG] chttp_request: chttp_store %p:\n", chttp_store);
        chttp_store_print(LOGSTDOUT, chttp_store);
    }
 
    if(EC_FALSE == CHTTP_STORE_MERGE_FLAG(chttp_store))
    {
        return chttp_request_basic(chttp_req, chttp_store, chttp_rsp, chttp_stat); /*need store only*/
    }

    if(0 == CHTTP_STORE_SEG_ID(chttp_store))
    {
        return chttp_request_header(chttp_req, chttp_store, chttp_rsp, chttp_stat); /*need store but not merge http request*/
    }

#if (SWITCH_ON == NGX_BGN_SWITCH)
    if(EC_TRUE == task_brd_default_is_ngx_exiting())
    {
        /*when ngx is exiting*/
        return (EC_FALSE);
    }
#endif/*(SWITCH_ON == NGX_BGN_SWITCH)*/

    /*note: merge must store data in flow*/
    return chttp_request_merge(chttp_req, chttp_store, chttp_rsp, chttp_stat); /*need store and merge http request*/
}


#ifdef __cplusplus
}
#endif/*__cplusplus*/

