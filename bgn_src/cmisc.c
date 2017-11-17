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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include <memory.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <execinfo.h>

#include <libgen.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <arpa/inet.h>

#include <errno.h>

#include "type.h"

#include "mm.h"
#include "log.h"

#include "cmisc.h"
#include "cmd5.h"

#include "ccode.h"

#define CMISC_BUFF_NUM ((UINT32) 256)
#define CMISC_BUFF_LEN ((UINT32) 128)

#define CMISC_TM_NUM   ((UINT32) 256)

#define CMISC_CMD_OUTPUT_LINE_MAX_SIZE       ((UINT32) 1 * 1024 * 1024)

#define CMISC_WRITE_ONCE_MAX_BYTES           ((UINT32)0x04000000)/*64MB*/
#define CMISC_READ_ONCE_MAX_BYTES            ((UINT32)0x04000000)/*64MB*/


/*NOTE: use naked/original mutex BUT NOT CMUTEX*/
/*to avoid scenario such as sys_log need c_localtime_r but c_localtime_r need mutex*/
static pthread_mutex_t g_cmisc_str_cmutex;
static char g_str_buff[CMISC_BUFF_NUM][CMISC_BUFF_LEN];
static UINT32 g_str_idx = 0;

static pthread_mutex_t g_cmisc_tm_cmutex;
static CTM g_tm_tbl[CMISC_TM_NUM];
static UINT32 g_tm_idx = 0;

static char  g_hex_char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static UINT8 g_char_hex[] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*  0 -  15*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/* 16 -  31*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/* 32 -  47*/
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,/* 48 -  63*/
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,/* 64 -  79*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/* 80 -  95*/
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,/* 96 - 111*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*112 - 127*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*128 - 143*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*144 - 159*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*160 - 175*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*176 - 191*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*192 - 207*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*208 - 223*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*224 - 239*/
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,/*240 - 255*/
};

/*note: crc algorithm is copied from nginx*/
static uint32_t  g_crc32_table16[] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

static uint32_t  g_crc32_table256[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t *g_crc32_table_short = g_crc32_table16;

static uint32_t  g_mday[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static char  *g_week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static char  *g_months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
                        
EC_BOOL cmisc_init(UINT32 location)
{
    c_mutex_init(&g_cmisc_str_cmutex, CMUTEX_PROCESS_PRIVATE, location);
    c_mutex_init(&g_cmisc_tm_cmutex, CMUTEX_PROCESS_PRIVATE, location);
    return (EC_TRUE);
}

EC_BOOL c_chars_are_digit(const char *chars, const UINT32 len)
{
    UINT32 idx;

    if(NULL_PTR == chars || 0 == len)
    {
        return (EC_FALSE);
    }

    for(idx = 0; idx < len; idx ++)
    {
        if('0' > chars[ idx ] || '9' < chars[ idx ])
        {
            return (EC_FALSE);
        }
    }

    return (EC_TRUE);
}

UINT32 c_chars_to_word(const char *chars, const UINT32 len)
{
    UINT32  c;            /* current char */
    UINT32  total;        /* current total */
    UINT32  pos;
    UINT32  negs;

    if( NULL_PTR == chars)
    {
        return ((UINT32)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < len && '\0' != chars[ pos ]; pos ++)
    {
        if(0 == pos && '-' == chars[ pos ])
        {
            negs *= ((UINT32)-1);
            continue;
        }

        c = (UINT32)(chars[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_word: str %.*s found not digit char at pos %ld\n", len, chars, pos);
            return ((UINT32)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

UINT32 c_str_to_word(const char *str)
{
    UINT32  c;            /* current char */
    UINT32  total;        /* current total */
    UINT32  pos;
    UINT32  negs;

    if( NULL_PTR == str)
    {
        return ((UINT32)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < strlen(str); pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((UINT32)-1);
            continue;
        }

        c = (UINT32)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_word: str %s found not digit char at pos %ld\n", str, pos);
            return ((UINT32)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

char *c_word_to_str(const UINT32 num)
{
    char *str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0001);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0002);

    snprintf(str_cache, CMISC_BUFF_LEN, "%ld", num);

    return (str_cache);
}

int c_str_to_int(const char *str)
{
    int  c;            /* current char */
    int  total;        /* current total */
    int  pos;
    int  negs;

    if( NULL_PTR == str)
    {
        return ((int)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < strlen(str); pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((int)-1);
            continue;
        }

        c = (int)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_word: str %s found not digit char at pos %ld\n", str, pos);
            return ((int)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

char *c_int_to_str(const int num)
{
    char *str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0003);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0004);

    snprintf(str_cache, CMISC_BUFF_LEN, "%d", num);

    return (str_cache);
}

char *c_word_to_hex_str(const UINT32 num)
{
    char *str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0005);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0006);

    snprintf(str_cache, CMISC_BUFF_LEN, "%lx", num);

    return (str_cache);
}

UINT32 c_xmlchar_to_word(const xmlChar *xmlchar)
{
    return c_str_to_word((const char *)xmlchar);
}

EC_BOOL c_ipv4_is_ok(const char *ipv4_str)
{
    size_t   len;
    size_t   idx;
    uint32_t num;
    uint32_t segs;

    if(NULL_PTR == ipv4_str)
    {
        return (EC_FALSE);
    }

    len = strlen(ipv4_str);
    if(16 <= len)
    {
        return (EC_FALSE);
    }

    segs = 0;
    num  = 0;
    for(idx = 0; idx < len; idx ++)
    {
        char ch;

        ch = ipv4_str[ idx ];
        if('.' == ch)
        {
            segs ++;
            if(255 < num || 3 < segs)
            {
                return (EC_FALSE);
            }

            num = 0; /*reset*/
            continue;
        }
     
        if('0' > ch || '9' < ch)
        {
            return (EC_FALSE);
        }
        num = num * 10 + ch - '0';
    }

    segs ++;
    if(255 < num)
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

/*return host order*/
UINT32 c_ipv4_to_word(const char *ipv4_str)
{
    /*network order is big endian*/
    /*network order to host order. e.g., "1.2.3.4" -> 0x01020304*/

    UINT32 a,b,c,d;
    sscanf(ipv4_str, "%ld.%ld.%ld.%ld",&a, &b, &c, &d);
    if(!(256 > a && 256 > b && 256 > c && 256 > d))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_ipv4_to_word: invalid ipv4 str '%s'\n", ipv4_str);
        ASSERT( 256 > a);
        ASSERT( 256 > b);
        ASSERT( 256 > c);
        ASSERT( 256 > d);
    }
 
    return ((a << 24) | (b << 16) | (c << 8) | (d));
    //return (ntohl(inet_addr(ipv4_str)));
}


/*ipv4_num is host order*/
char *c_word_to_ipv4(const UINT32 ipv4_num)
{
    char *ipv4_str_cache;
    UINT32 a,b,c,d;

    a = ((ipv4_num >> 24) & 0xFF);/*high bits*/
    b = ((ipv4_num >> 16) & 0xFF);
    c = ((ipv4_num >>  8) & 0xFF);
    d = ((ipv4_num >>  0) & 0xFF);/*low bits*/

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0007);
    ipv4_str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0008);

    snprintf(ipv4_str_cache, CMISC_BUFF_LEN, "%ld.%ld.%ld.%ld", a, b, c, d);
    return (ipv4_str_cache);
}

uint32_t c_chars_to_uint32(const char *str, const uint32_t len)
{
    uint32_t  c;            /* current char */
    uint32_t  total;        /* current total */
    uint32_t  negs;
    uint32_t  pos;

    if( NULL_PTR == str)
    {
        return ((uint32_t)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < len; pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((uint32_t)-1);
            continue;
        }

        c = (uint32_t)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_chars_to_uint32: str %s found not digit char at pos %ld\n", str, pos);
            return ((uint32_t)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

uint32_t c_str_to_uint32(const char *str)
{
    uint32_t  c;            /* current char */
    uint32_t  total;        /* current total */
    uint32_t  negs;
    uint32_t  pos;
    uint32_t  len;

    if( NULL_PTR == str)
    {
        return ((uint32_t)0);
    }

    total = 0;
    negs  = 1;

    len = (uint32_t)strlen(str);
    for(pos = 0; pos < len; pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((uint32_t)-1);
            continue;
        }

        c = (uint32_t)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_uint32: str %s found not digit char at pos %ld\n", str, pos);
            return ((uint32_t)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

char *c_uint32_to_str(const uint32_t num)
{
    char *str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0009);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0010);

    snprintf(str_cache, CMISC_BUFF_LEN, "%d", num);

    return (str_cache);
}

uint16_t c_str_to_uint16(const char *str)
{
    uint16_t  c;            /* current char */
    uint16_t  total;        /* current total */
    uint16_t  negs;
    uint16_t  pos;
    uint16_t  len;

    if( NULL_PTR == str)
    {
        return ((uint16_t)0);
    }

    total = 0;
    negs  = 1;

    len = (uint16_t)strlen(str);
    for(pos = 0; pos < len; pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((uint16_t)-1);
            continue;
        }

        c = (uint16_t)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_uint16: str %s found not digit char at pos %ld\n", str, pos);
            return ((uint16_t)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

char *c_uint16_to_str(const uint16_t num)
{
    char *str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0011);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0012);

    snprintf(str_cache, CMISC_BUFF_LEN, "%d", num);

    return (str_cache);
}

char *c_uint8_to_bin_str(const uint8_t num)
{
    char *str_cache;
    char *pch;
    uint8_t e;
    uint8_t len;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0013);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0014);

    len = sizeof(uint8_t) * BYTESIZE;
    e = (uint8_t)(1 << (len - 1));
 
    for(pch = str_cache; len > 0; len --, e >>= 1)
    {
        if(num & e)
        {
            *pch ++ = '1';
        }
        else
        {
            *pch ++ = '0';
        }
    }

    *pch = '\0';
    return (str_cache);
}

char *c_uint16_to_bin_str(const uint16_t num)
{
    char *str_cache;
    char *pch;
    uint16_t e;
    uint16_t len;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0015);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0016);

    len = sizeof(uint16_t) * BYTESIZE;
    e = (uint16_t)(1 << (len - 1));
 
    for(pch = str_cache; len > 0; len --, e >>= 1)
    {
        if(num & e)
        {
            *pch ++ = '1';
        }
        else
        {
            *pch ++ = '0';
        }
    }

    *pch = '\0';
    return (str_cache);
}

char *c_uint32_to_bin_str(const uint32_t num)
{
    char *str_cache;
    char *pch;
    uint32_t e;
    uint32_t len;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0017);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0018);

    len = sizeof(uint32_t) * BYTESIZE;
    e = (uint32_t)(1 << (len - 1));
 
    for(pch = str_cache; len > 0; len --, e >>= 1)
    {
        if(num & e)
        {
            *pch ++ = '1';
        }
        else
        {
            *pch ++ = '0';
        }
    }

    *pch = '\0';
    return (str_cache);
}

char *c_word_to_bin_str(const word_t num)
{
    char *str_cache;
    char *pch;
    word_t e;
    word_t len;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0019);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0020);

    len = sizeof(word_t) * BYTESIZE;
    e = (word_t)(1 << (len - 1));
 
    for(pch = str_cache; len > 0; len --, e >>= 1)
    {
        if(num & e)
        {
            *pch ++ = '1';
        }
        else
        {
            *pch ++ = '0';
        }
    }

    *pch = '\0';
    return (str_cache);
}

uint64_t c_chars_to_uint64(const char *str, const uint32_t len)
{
    uint64_t  c;            /* current char */
    uint64_t  negs; 
    uint64_t  total;     /* current total */
    UINT32    pos;

    if( NULL_PTR == str)
    {
        return ((UINT32)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < len; pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((uint64_t)-1);
            continue;
        }

        c = (uint64_t)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_chars_to_uint64: str %.*s found not digit char at pos %ld\n", len, str, pos);
            return ((UINT32)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

uint64_t c_str_to_uint64(const char *str)
{
    uint64_t  c;            /* current char */
    uint64_t  negs; 
    uint64_t  total;     /* current total */
    UINT32    pos;

    if( NULL_PTR == str)
    {
        return ((uint64_t)0);
    }

    total = 0;
    negs  = 1;

    for(pos = 0; pos < strlen(str); pos ++)
    {
        if(0 == pos && '-' == str[ pos ])
        {
            negs *= ((uint64_t)-1);
            continue;
        }

        c = (uint64_t)(str[ pos ]);

        if(c < '0' || c > '9')
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_str_to_uint64: str %s found not digit char at pos %ld\n", str, pos);
            return ((uint64_t)0);
        }
        total = 10 * total + (c - '0');
    }
    return (total * negs);
}

/*long to string*/
int c_long_to_str_buf(const long num, char *buf)
{
    long val;
    char swap;
    char *end;
    int len;

    val = num;
    len = 1;

    if (0 > val)
    {
        len++;
        *(buf++) = '-';
        val = -val;
    }

    end = buf;
    while (9 < val)
    {
        *(end++) = '0' + (val % 10);
        val = val / 10;
    }
    *(end) = '0' + val;
    *(end + 1) = '\0';
    len += end - buf;

    while (buf < end)
    {
        swap = *end;
        *end = *buf;
        *buf = swap;

        buf++;
        end--;
    }

    return (len);
}

char *c_inet_ntos(const struct in_addr *in)
{
    char *ipv4_str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0021);
    ipv4_str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0022);

    inet_ntop(AF_INET, in, ipv4_str_cache, CMISC_BUFF_LEN);
    return (ipv4_str_cache);
}

/*
   inet_aton()  converts  the  Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order) and stores it in the structure
   that inp points to.  inet_aton() returns non-zero if the address is valid, zero if not.  The address supplied in cp can have one of the following forms:

   a.b.c.d   Each of the four numeric parts specifies a byte of the address; the bytes are assigned in left-to-right order to produce the binary address.

   a.b.c     Parts a and b specify the first two bytes of the binary address.  Part c is interpreted as a 16-bit value that defines the  rightmost  two  bytes  of  the
             binary address.  This notation is suitable for specifying (outmoded) Class B network addresses.

   a.b       Part  a  specifies  the  first  byte  of the binary address.  Part b is interpreted as a 24-bit value that defines the rightmost three bytes of the binary
             address.  This notation is suitable for specifying (outmoded) Class C network addresses.

   a         The value a is interpreted as a 32-bit value that is stored directly into the binary address without any byte rearrangement.

   In all of the above forms, components of the dotted address can be specified in decimal, octal (with a leading 0), or hexadecimal, with a leading 0X).  Addresses in
   any  of  these  forms are collectively termed IPV4 numbers-and-dots notation.  The form that uses exactly four decimal numbers is referred to as IPv4 dotted-decimal
   notation (or sometimes: IPv4 dotted-quad notation).
*/
EC_BOOL c_inet_ston(const char *ipv4_str, struct in_addr *in)
{
    if(inet_aton(ipv4_str, in))
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}


char *c_uint32_ntos(const uint32_t ipv4)
{
    struct in_addr in;
    in.s_addr = ipv4;
    return c_inet_ntos(&in);
}

uint32_t  c_uint32_ston(const char *ipv4_str)
{
    struct in_addr in;
    if(EC_FALSE == c_inet_ston(ipv4_str, &in))
    {
        return (0);
    }
    return (in.s_addr);
}

UINT32 c_port_to_word(const char *port_str)
{
    return ((UINT32)(atoi(port_str)));
}

char *c_word_to_port(const UINT32 port_num)
{
    char *port_str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0023);
    port_str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0024);

    snprintf(port_str_cache, CMISC_BUFF_LEN, "%ld", port_num);

    return (port_str_cache);
}

/*note: subnet_mask is in host order*/
uint32_t ipv4_subnet_mask_prefix(uint32_t subnet_mask)
{
    uint32_t e;
    uint32_t prefix;

    if(0 == subnet_mask)
    {
        return ((uint32_t)0);
    }

    for(prefix = 32, e = 1; 0 == (e & subnet_mask); prefix --, e <<= 1)
    {
        /*do nothing*/
    }
    return (prefix);
}

EC_BOOL c_check_is_uint8_t(const UINT32 num)
{
    if(0 == (num >> 8))
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_check_is_uint16_t(const UINT32 num)
{
    if(0 == (num >> 16))
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_check_is_uint32_t(const UINT32 num)
{
#if (32 == WORDSIZE)
    return (EC_TRUE);
#endif /*(32 == WORDSIZE)*/

#if (64 == WORDSIZE)
    if(32 == WORDSIZE || 0 == (num >> 32))
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
#endif /*(64 == WORDSIZE)*/
}

void str_to_lower(UINT8 *str, const UINT32 len)
{
    UINT32 pos;
    for(pos = 0; pos < len; pos ++)
    {
        if('A' <= (str[ pos ]) && (str[ pos ]) <= 'Z')
        {
            //str[ pos ] = (str[ pos ] - 'A' + 'a');
            str[ pos ] |= 32;
        }
    }
    return;
}

void str_to_upper(UINT8 *str, const UINT32 len)
{
    UINT32 pos;
    for(pos = 0; pos < len; pos ++)
    {
        if('a' <= (str[ pos ]) && (str[ pos ]) <= 'z')
        {
            //str[ pos ] = (str[ pos ] - 'a' + 'A');
            str[ pos ] &= (~32);
        }
    }
    return;
}


char *mac_addr_to_str(const UINT8 *mac)
{
    char *mac_str_cache;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0025);
    mac_str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0026);

    snprintf(mac_str_cache, CMISC_BUFF_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
                            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return (mac_str_cache);
}

EC_BOOL str_to_mac_addr(const char *mac_str, UINT8 *mac_addr)
{
    char mac_str_tmp[32];
    char *fields[16];
    UINT32 len;
    UINT32 pos;

    len = DMIN(32, strlen(mac_str) + 1);
    dbg_log(SEC_0013_CMISC, 9)(LOGSTDNULL, "[DEBUG] str_to_mac_addr: len %ld\n", len);

    BCOPY(mac_str, mac_str_tmp, len);
    if(6 != c_str_split(mac_str_tmp, ":", fields, sizeof(fields)/sizeof(fields[0])))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDNULL, "error:str_to_mac_addr:invalid mac addr %s\n", mac_str);
        return (EC_FALSE);
    }

    for(pos = 0; pos < 6; pos ++)
    {
        mac_addr[ pos ] = (UINT8)(0xff & strtol(fields[ pos ], NULL_PTR, 16));
    }
    return (EC_TRUE);
}

UINT32 c_str_to_switch(const char *str)
{
    if(0 == strcasecmp(str, (char *)"SWITCH_ON") || 0 == strcasecmp(str, (char *)"ON"))
    {
        return (SWITCH_ON);
    }
    return (SWITCH_OFF);
}

char *c_switch_to_str(const UINT32 switch_choice)
{
    if(SWITCH_ON == switch_choice)
    {
        return (SWITCH_ON_STR);
    }
    if(SWITCH_OFF == switch_choice)
    {
        return (SWITCH_OFF_STR);
    }

    return (SWITCH_UNDEF_STR);
}

EC_BOOL c_str_split_to_cstr_list(const char *str, const int len, const char *delim, CLIST *clist)
{
    char    *string;
    char    *saveptr;
    char    *field;

    string = safe_malloc(len + 1, LOC_CMISC_0027);
    if(NULL_PTR == string)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_split_to_cstr_list: malloc %d bytes failed\n", len);
        return (EC_FALSE);
    }

    BCOPY(str, string, len);
    string[ len ] = '\0';

    saveptr = string;
    while ((field = strtok_r(NULL_PTR, delim, &saveptr)) != NULL_PTR)
    {
        CSTRING *cstr;
     
        cstr = cstring_new((UINT8 *)field, LOC_CMISC_0028);
        if(NULL_PTR == cstr)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_split_to_cstr_list: new cstring '%s' failed\n",
                        field);
                     
            safe_free(string, LOC_CMISC_0029);
            return (EC_FALSE);
        }

        if(NULL_PTR == clist_push_back(clist, (void *)cstr))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_split_to_cstr_list: push '%s' to list failed\n",
                        field);
                     
            cstring_free(cstr);
            safe_free(string, LOC_CMISC_0030);
            return (EC_FALSE);
        }
    }

    safe_free(string, LOC_CMISC_0031);
    return (EC_TRUE);
}

EC_BOOL c_cstr_list_join_to_str(char *str, const int max_len, int *len, const char *delim, const CLIST *clist)
{
    CLIST_DATA      *clist_data;
    CLIST_DATA      *last_node;

    int              idx;
    int              left_len;
    int              delim_len;
    char            *cur;

    idx       = 0;
    left_len  = max_len;
    cur       = str;

    delim_len = strlen(delim);

    last_node = CLIST_LAST_NODE(clist);

    CLIST_LOOP_NEXT(clist, clist_data)
    {
        CSTRING     *tmp_cstr;
        int          tmp_len;

        if(NULL_PTR == CLIST_DATA_DATA(clist_data))
        {
            continue;
        }

        tmp_cstr = (CSTRING *)CLIST_DATA_DATA(clist_data);
        tmp_len  = (int)cstring_get_len(tmp_cstr);
        if(tmp_len > left_len)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_cstr_list_join_to_str: "
                                                  "join [%d] cstring '%s' but no more space\n",
                                                  idx, (char *)cstring_get_str(tmp_cstr));

         
            return (EC_FALSE);                                   
        }
        dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_cstr_list_join_to_str: "
                                              "join [%d] cstring '%s' done\n",
                                              idx, (char *)cstring_get_str(tmp_cstr));     

        BCOPY(cstring_get_str(tmp_cstr), cur, tmp_len);
        cur      += tmp_len;
        left_len -= tmp_len;     

        if(last_node != clist_data)
        {
            if(delim_len > left_len)
            {
                dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_cstr_list_join_to_str: "
                                                      "no more space to accept next after [%d] cstring'\n",
                                                      idx);         
                return (EC_FALSE);
            }

            BCOPY(delim, cur, delim_len);
            cur      += delim_len;
            left_len -= delim_len;
        }

        idx ++;
    }

    (*len) = max_len - left_len;

    return (EC_TRUE);
}

UINT32 c_str_split (char *string, const char *delim, char **fields, const UINT32 size)
{
    UINT32 idx;
    char *saveptr;

    idx = 0;
    saveptr = string;
    while ((fields[ idx ] = strtok_r(NULL_PTR, delim, &saveptr)) != NULL_PTR)
    {
        idx ++;

        if (idx >= size)
        {
            break;
        }
    }

    return (idx);
}

char *c_str_join(const char *delim, const char **fields, const UINT32 size)
{
    UINT32 total_len;
    UINT32 delim_len;
    UINT32 idx;
    char  *ptr;
    char  *str;

    if( 0 == size)
    {
        return (NULL_PTR);
    }

    delim_len = strlen(delim);

    for(idx = 0, total_len = 0; idx < size; idx ++)
    {
        total_len += strlen(fields[ idx ]);
    }

    total_len += delim_len * (size - 1);
    total_len ++; /*string terminate char*/

    str = (char *)safe_malloc(total_len, LOC_CMISC_0032);
    if(NULL_PTR == str)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_join: malloc str with len %ld failed\n", total_len);
        return (NULL_PTR);
    }

    BSET(str, 0, total_len);

    ptr = str;
    for(idx = 0; idx < size; idx ++)
    {
        UINT32 cur_len;

        cur_len = strlen(fields[ idx ]);

        BCOPY(fields[ idx ], ptr, cur_len);
        ptr += cur_len;

        if(idx + 1 < size)
        {
            BCOPY(delim, ptr, delim_len);
            ptr += delim_len;
        }
    }

    (*ptr) = '\0';

    return (str);
}

char *c_str_cat(const char *src_str_1st, const char *src_str_2nd)
{
    char *des_str;
    UINT32 des_str_len;
 
    char *src;
    char *des; 

    des_str_len = strlen(src_str_1st) + strlen(src_str_2nd) + 1;
    des_str = safe_malloc(des_str_len, LOC_CMISC_0033);
    if(NULL_PTR == des_str)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_cat: malloc %ld bytes failed\n", des_str_len);
        return (NULL_PTR);
    }

    des = des_str;
 
    src = (char *)src_str_1st;
    while( '\0' != (*src))
    {
        (*des ++) = (*src ++);
    }

    src = (char *)src_str_2nd;
    while( '\0' != (*src))
    {
        (*des ++) = (*src ++);
    }

    (*des) = '\0';

    return (des_str); 
}

char *c_str_dup(const char *str)
{
    char *dup_str;
 
    dup_str = (char *)safe_malloc(strlen(str) + 1, LOC_CMISC_0034);
    if(NULL_PTR == dup_str)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_dup: dup str %s failed\n", str);
        return (NULL_PTR);
    }
    BCOPY(str, dup_str, strlen(str) + 1);
    return (dup_str);
}

EC_BOOL c_str_free(char *str)
{
    if(NULL_PTR != str)
    {
        safe_free(str, LOC_CMISC_0035);
        return (EC_TRUE);
    }
    return (EC_TRUE);
}

EC_BOOL c_str_is_in(const char *string, const char *delim, const char *tags_str)
{
    char *str_tmp;
    char *tag[32];
    UINT32 tag_num;
    UINT32 tag_idx;

    if(NULL_PTR == string)
    {
        return (EC_FALSE);
    }
 
    str_tmp = c_str_dup(tags_str);
    if(NULL_PTR == str_tmp)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_str_is_in: dup %s failed\n", tags_str);
        return (EC_FALSE);
    }
    tag_num = c_str_split(str_tmp, delim, tag, sizeof(tag)/sizeof(tag[0]));

    for(tag_idx = 0; tag_idx < tag_num; tag_idx ++)
    {
        if(0 == strcasecmp(string, tag[ tag_idx ]))
        {
            safe_free(str_tmp, LOC_CMISC_0036);
            return (EC_TRUE);
        }
    }
    safe_free(str_tmp, LOC_CMISC_0037);
    return (EC_FALSE);
}

char *c_str_skip_space(const char *start, const char *end)
{
    const char *cur;
    for(cur = start;cur < end; cur ++)
    {
        if(!ISSPACE(*cur))
        {
            return ((char *)cur);
        }
    }

    return (NULL_PTR);
}

char *c_str_ltrim(char *str, const char ch)
{
    char *t;
 
    for(t = str; '\0' != (*t) && ch == (*t); t ++ )
    {
        /*do nothing*/
    }

    if(str != t)
    {
        char *s;
        for(s = str; '\0' != (*t);)
        {
            (*s ++) = (*t ++);
        }

        (*s) = '\0';
    }

    return (str);
}

char *c_str_rtrim(char *str, const char ch)
{
    char *t;

    for(t = str; '\0' != (*t); t ++)
    {
        /*do nothing*/
    }
 
    for(-- t; t >= str && ch == (*t); t -- )
    {
        /*do nothing*/
    }

    (* ++ t) = '\0';

    return (str);
}


char *c_str_trim(char *str, const char ch)
{
    c_str_ltrim(str, ch);
    c_str_rtrim(str, ch);

    return (str);
}

char *c_str_del(char *str, const char ch)
{
    char *s;
    char *t;

    for(s = t = str; '\0' != (*t);)
    {
        if(ch == (*t))
        {
            t ++;
            continue;
        }

        (*s ++) = (*t ++);
    }

    (*s) = '\0';
 
    return (str);
}

char c_str_first_char(const char *str)
{
    return ((char)str[0]);
}

char c_str_last_char(const char *str)
{
    uint32_t len;
    len = strlen(str);
    if(0 == len)
    {
        return '\0';
    }
    return ((char)str[ len - 1]);
}

/*sub chars or sub string*/
char *c_str_sub(const char *str, const char *sub, const char sub_terminate_char, UINT32 *sub_len)
{
    char      *s;
 
    s = strstr(str, sub);
    if(NULL_PTR == s)
    {
        if(NULL_PTR != sub_len)
        {
            (*sub_len) = 0;
        }
        return (NULL_PTR);
    }

    if(NULL_PTR != sub_len)
    {
        char      *p;

        for(p = s; '\0' != (*p) && sub_terminate_char != (*p); p ++)
        {
            /*do nothing*/
        }

        (*sub_len) = (p - s);
    }
 
    return (s);
}

char *c_str_fetch_line(char *str)
{
    char *pch;

    if('\0' == *str)
    {
        return (NULL_PTR);
    }

    for(pch = str; '\0' != (*pch); pch ++)
    {
        if('\n' == (*pch))
        {
            (*pch) = '\0';
            break;
        }
    }
    return (str);
}

char *c_str_fetch_next_line(char *str)
{
    char *pch;
    char *next;

    if('\0' == *str)
    {
        return (NULL_PTR);
    }

    next = (str + strlen(str) + 1);/*xxx*/
    for(pch = next; '\0' != (*pch); pch ++)
    {
        if('\n' == (*pch))
        {
            (*pch) = '\0';
            break;
        }
    }
    return (next);
}

char *c_str_move_next(char *str)
{
    return (str + strlen(str) + 1);
}

/* Searches the next delimiter (char listed in DELIM) starting at *STRINGP.
   If one is found, it is overwritten with a NUL, and *STRINGP is advanced
   to point to the next char after it.  Otherwise, *STRINGP is set to NULL.
   If *STRINGP was already NULL, nothing happens.
   Returns the old value of *STRINGP.

   This is a variant of strtok() that is multithread-safe and supports
   empty fields.

   Caveat: It modifies the original string.
   Caveat: These functions cannot be used on constant strings.
   Caveat: The identity of the delimiting character is lost.
   Caveat: It doesn't work with multibyte strings unless all of the delimiter
           characters are ASCII characters < 0x30.

   See also strtok_r().
*/
char *c_str_seperate (char **stringp, const char *delim)
{
    char *start;
    char *ptr;

    start = *stringp;

    if (NULL_PTR == start)
    {
        return (NULL_PTR);
    }
 
    if (NULL_PTR == *delim)
    {
        ptr = start + strlen (start);
    }
    else
    {
        /**
         *   char *strpbrk(const char *s, const char *accept)
         *   The strpbrk() function returns a pointer to the character in s that
         *   matches 'one' of the characters in accept, or NULL if no such character is found.
         **/     
        ptr = strpbrk (start, delim);
        if (NULL_PTR == ptr)
        {
            *stringp = NULL_PTR;
            return (start);
        }
    }

    *ptr = '\0';
    *stringp = ptr + 1;

    return (start);
}

char *c_chars_dup(const char *str_chars, const uint32_t len)
{
    char *dup_str;
 
    dup_str = (char *)safe_malloc(len + 1, LOC_CMISC_0038);
    if(NULL_PTR == dup_str)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_chars_dup: dup chars %.*s failed\n", len, str_chars);
        return (NULL_PTR);
    }
    BCOPY(str_chars, dup_str, len);
    dup_str[ len ] = '\0';
    return (dup_str);
}


/*
*   location format e.g.:
*       http://www.cc.com:8080/flv/a.mp4
*       http://www.cc.com/flv/a.mp4
*       /flv/a.mp4
*
*   note: not support format:
*       /www.cc.com/flv/a.mp4
*/
EC_BOOL c_parse_location(const char *v, char **host, char **port, char **uri)
{
    char *s;
    char *e;
    char *p;

    uint32_t vlen;
    uint32_t hlen;
    uint32_t plen;
    uint32_t ulen;

    vlen = strlen(v);
    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_parse_location: v = '%s', [vlen %u]\n", v, vlen);

    if(vlen >= sizeof("http://") - 1 && 0 == STRNCASECMP(v, (const char *)"http://", sizeof("http://") - 1))
    {
        s = (char *)v + sizeof("http://") - 1;

        for(e = s; '\0' != (*e) && '/' != (*e); e ++)
        {
            /*do nothing*/
        }

        /*Host: [s, e)*/
        for(p = s; p < e && ':' != (*p); p ++)
        {
            /*do nothing*/
        }

        hlen = p - s;
        (*host) = c_chars_dup(s, hlen);
        if(NULL_PTR == *host)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_parse_location: dup host failed\n");
            return (EC_FALSE);
        }
        dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_parse_location: host = '%s', [hlen %u]\n", (*host), hlen);
     
        if(p != e)
        {
            p ++;
            plen = e - p;
         
            (*port) = c_chars_dup(p, plen);
            if(NULL_PTR == *port)
            {
                dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_parse_location: dup port failed\n");
                return (EC_FALSE);
            }
            dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_parse_location: port = '%s', [plen %u]\n", (*port), plen);
        }
        else
        {
            (*port) = NULL_PTR;
        }         

        ulen = vlen - (e - s);
        (*uri) = c_chars_dup(e, ulen);
        if(NULL_PTR == *uri)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_parse_location: dup uri failed\n");
            return (EC_FALSE);
        }
        dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_parse_location: uri = '%s', [plen %u]\n", (*uri), ulen);

        return (EC_TRUE);
    }

    (*host) = NULL_PTR;
    (*port) = NULL_PTR;

    (*uri)  = c_chars_dup(v, vlen);
    if(NULL_PTR == *uri)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_parse_location: dup uri failed\n");
        return (EC_FALSE);
    }
    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_parse_location: uri = '%s', [plen %u]\n", (*uri), vlen);
 
    return (EC_TRUE);
}

UINT32 c_line_len(const char *str)
{
    char *pch;
    UINT32 len;

    if('\0' == *str || '\n' == *str)
    {
        return (0);
    }

    for(pch = (char *)str, len = 0; '\0' != (*pch) && '\n' != (*pch); pch ++, len ++)
    {
        /*do nothing*/
    }
    return (len);
}

char *uint32_vec_to_str(const CVECTOR *uint32_vec)
{
    char  *buff;
    UINT32 len;
    UINT32 char_pos;
 
    UINT32 beg_pos;
    UINT32 cur_pos;

    UINT32 cvec_size; /* compute size only once to avoid invoking cvector_size() in every loop */
 
    if(EC_TRUE == cvector_is_empty(uint32_vec))
    {
        return (NULL_PTR);
    }

    len = 1 * 1024;
    buff = (char *)safe_malloc(len, LOC_CMISC_0039);
    if(NULL_PTR == buff)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:uint32_vec_to_str: malloc %ld bytes failed\n", len);
        return (NULL_PTR);
    }

    char_pos = 0;

    CVECTOR_LOCK(uint32_vec, LOC_CMISC_0040);

    cvec_size = cvector_size(uint32_vec);
 
    for(beg_pos = 0; beg_pos < cvec_size; beg_pos ++)
    {
        UINT32 beg_num;
        UINT32 end_num;
     
        beg_num = (UINT32)cvector_get_no_lock(uint32_vec, beg_pos);
        end_num = beg_num;

        for(cur_pos = beg_pos + 1; cur_pos < cvec_size; cur_pos ++)
        {
            UINT32 cur_num;
            cur_num = (UINT32)cvector_get_no_lock(uint32_vec, cur_pos);

            if(end_num + 1 == cur_num)
            {
                end_num = cur_num;
            }
            else
            {
                -- cur_pos;
                break;/*terminate inner loop*/
            }         
        }

        /*okay, we obtain the [beg_num, end_num]*/
        if(0 < char_pos)
        {
            char_pos += snprintf(buff + char_pos, len - char_pos, ",");
        }

        if(beg_num == end_num)
        {
            char_pos += snprintf(buff + char_pos, len - char_pos, "%ld", beg_num);
        }
        else if(beg_num + 1 == end_num)
        {
            char_pos += snprintf(buff + char_pos, len - char_pos, "%ld,%ld", beg_num, end_num);
        }     
        else
        {
            char_pos += snprintf(buff + char_pos, len - char_pos, "%ld-%ld", beg_num, end_num);
        }

        /*move forward*/
        beg_pos = cur_pos;
    }
    CVECTOR_UNLOCK(uint32_vec, LOC_CMISC_0041);

    return (buff);
}

char *c_bytes_to_hex_str(const UINT8 *bytes, const UINT32 len)
{ 
    char *str;
    UINT32 byte_pos;
    UINT32 char_pos;

    str = (char *)safe_malloc(2 * len + 1, LOC_CMISC_0042);
    if(NULL_PTR == str)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_bytes_to_hex_str: malloc %ld bytes failed\n", 2 * len + 1);
        return (NULL_PTR);
    }

    char_pos = 0;
    for(byte_pos = 0; byte_pos < len; byte_pos ++)
    {
        str[ char_pos ++ ] = g_hex_char[(bytes[ byte_pos ] >> 4) & 0xF];/*high 4 bytes*/
        str[ char_pos ++ ] = g_hex_char[(bytes[ byte_pos ]) & 0xF];/*high 4 bytes*/
    }
    str[ char_pos ] = '\0';

    return (str);
}

EC_BOOL c_hex_str_to_bytes(const char *str, UINT8 **bytes, UINT32 *len)
{
    UINT32 byte_pos;
    UINT32 char_pos;
    UINT32 str_len;
    UINT32 bytes_len;
    UINT8  *buff;

    str_len = strlen(str);

    if(str_len & 1)/*should never be odd*/
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_hex_str_to_bytes: str len %ld, but it should never is odd!\n", str_len);
        return (EC_FALSE);
    }

    bytes_len = (str_len >> 1);
    buff = (UINT8 *)safe_malloc(bytes_len, LOC_CMISC_0043);
    if(NULL_PTR == buff)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_hex_str_to_bytes: malloc %ld bytes failed\n", bytes_len);
        return (EC_FALSE);
    }

    char_pos = 0;
    for(byte_pos = 0; byte_pos < bytes_len; byte_pos ++)
    {
        UINT8 hi;
        UINT8 lo;
        UINT8 des;

        hi = g_char_hex[ (UINT8)(str[ char_pos ++ ]) ];
        lo = g_char_hex[ (UINT8)(str[ char_pos ++ ]) ];

        //ASSERT((UINT8)-1 != hi);
        //ASSERT((UINT8)-1 != lo);

        des = ((hi << 4) | lo);
        buff[ byte_pos ] = des;
    }

    (*bytes) = buff;
    (*len)   = bytes_len;
    return (EC_TRUE);
}

char *c_md5_to_hex_str(const uint8_t *md5, char *str, const uint32_t max_len)
{ 
    uint32_t byte_pos;
    uint32_t char_pos;
    uint32_t end_pos;

    ASSERT(3 <= max_len);
    end_pos  = max_len - 3;
    char_pos = 0;
    for(byte_pos = 0; byte_pos < CMD5_DIGEST_LEN && char_pos < end_pos; byte_pos ++)
    {
        str[ char_pos ++ ] = g_hex_char[(md5[ byte_pos ] >> 4) & 0xF];/*high 4 bytes*/
        str[ char_pos ++ ] = g_hex_char[(md5[ byte_pos ]) & 0xF];/*high 4 bytes*/
    }
    str[ char_pos ] = '\0';

    return (str);
}

uint32_t c_md5_to_hex_chars(const uint8_t *md5, char *chars, const uint32_t max_len)
{ 
    uint32_t byte_pos;
    uint32_t char_pos;
    uint32_t end_pos;

    ASSERT(2 * CMD5_DIGEST_LEN <= max_len);
    end_pos  = max_len;
    char_pos = 0;
    for(byte_pos = 0; byte_pos < CMD5_DIGEST_LEN && char_pos < end_pos; byte_pos ++)
    {
        chars[ char_pos ++ ] = g_hex_char[(md5[ byte_pos ] >> 4) & 0xF];/*high 4 bytes*/
        chars[ char_pos ++ ] = g_hex_char[(md5[ byte_pos ]) & 0xF];     /*high 4 bytes*/
    }
    return (char_pos);
}

EC_BOOL c_md5_hex_chars_is_valid(const char *md5, const uint32_t len)
{
    uint32_t idx;

    for(idx = 0; idx < len; idx ++)
    {
        char c;

        c = md5[ idx ];
        if(('0' <= c && '9' >= c) || ('a' <= c && 'f' >= c) || ('A' <= c && 'F' >= c))
        {
            continue;
        }
     
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

char *c_dirname(const char *path_name)
{
    char *dir_name;
    char *dir_name_t;
    const char *ptr;

    ptr = path_name;
    while('/' != (*ptr) && '\0' != (*ptr))
    {
        ptr ++;
    }

    if('\0' == (*ptr))
    {
        return c_str_dup((const char *)".");
    }

    dir_name = c_str_dup(path_name);
    //dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_dirname: dir_name = %p, %s\n", dir_name, dir_name);
    dir_name_t = dirname(dir_name);
    //dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_dirname: dir_name_t = %p, %s\n", dir_name_t, dir_name_t);
    return (dir_name_t);
}

EC_BOOL c_dir_create(const char *dir_name)
{
    char *pstr;

    int   len;
    int   pos;

    if(0 == access(dir_name, F_OK))/*exist*/
    {
        return (EC_TRUE);
    }

    pstr = strdup(dir_name);
    if(NULL_PTR == pstr)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_dir_create: strdup failed\n");
        return (EC_FALSE);
    }

    len  = strlen(pstr);

    for(pos = 1; pos < len; pos ++)
    {
        UINT32 loop;

        if('/' != pstr[ pos ])
        {
            continue;
        }

        pstr[ pos ] = '\0';

        for(loop = 0; loop < 3 && 0 != access(pstr, F_OK) && 0 != mkdir(pstr, 0755); loop ++)/*try 3 times*/
        {
            /*do nothing*/
        }

        if(3 <= loop)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_dir_create: create dir %s failed\n", pstr);
            pstr[ pos ] = '/';

            free(pstr);
            return (EC_FALSE);
        }
        pstr[ pos ] = '/';
    }

    if(0 != access(dir_name, F_OK) && 0 != mkdir(dir_name, 0755))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_dir_create: create dir %s failed\n", dir_name);
        free(pstr);
        return (EC_FALSE);
    }

    free(pstr);
    return (EC_TRUE);
}

EC_BOOL c_basedir_create(const char *file_name)
{
    char *dir_name;
    EC_BOOL ret;

    dir_name = c_dirname(file_name);
    ret      = c_dir_create(dir_name);
    safe_free(dir_name, LOC_CMISC_0044);
    return (ret);
}

EC_BOOL c_dir_exist(const char *pathname)
{
    struct stat filestat;

    if(0 != stat(pathname, &filestat))
    {
        return (EC_FALSE);
    }

    /************************************************************
       S_ISREG(m)  is it a regular file?

       S_ISDIR(m)  directory?

       S_ISCHR(m)  character device?

       S_ISBLK(m)  block device?

       S_ISFIFO(m) FIFO (named pipe)?

       S_ISLNK(m)  symbolic link? (Not in POSIX.1-1996.)

       S_ISSOCK(m) socket? (Not in POSIX.1-1996.)
    ************************************************************/
    if(S_ISDIR(filestat.st_mode))
    {
        return (EC_TRUE);
    }

    return (EC_FALSE);
}

EC_BOOL exec_shell(const char *cmd_str, char *cmd_output, const UINT32 max_size)
{
    FILE    *rstream;
    char    *cmd_ostream;
    char    *cmd_ostr;
    UINT32   cmd_osize;

    //dbg_log(SEC_0013_CMISC, 5)(LOGSTDNULL, "exec_shell beg: %s\n", cmd_str);

    if(NULL_PTR == cmd_output)
    {
        cmd_osize   = CMISC_CMD_OUTPUT_LINE_MAX_SIZE;
        cmd_ostream = (char *)SAFE_MALLOC(cmd_osize, LOC_CMISC_0045);
    }
    else
    {
        cmd_osize   = max_size;
        cmd_ostream = cmd_output;
    }

    rstream = popen(cmd_str, "r");
    if(NULL_PTR == rstream)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:exec_shell: popen %s failed\n", cmd_str);
        return (EC_FALSE);
    }
    for(cmd_ostr = cmd_ostream;
        1 < cmd_osize && NULL_PTR != (cmd_ostr = fgets(cmd_ostr, cmd_osize, rstream));
        cmd_osize -= strlen(cmd_ostr), cmd_ostr += strlen(cmd_ostr))
    {
        /*do nothing*/
    }
    pclose( rstream );

    if(cmd_ostream != cmd_output)
    {
        SAFE_FREE(cmd_ostream, LOC_CMISC_0046);
    }
    //dbg_log(SEC_0013_CMISC, 5)(LOGSTDNULL, "exec_shell end: %s\n", cmd_output);
    return (EC_TRUE);
}

EC_BOOL c_file_flush(int fd, UINT32 *offset, const UINT32 wsize, const UINT8 *buff)
{
    UINT32 csize;/*write completed size*/
    UINT32 osize;/*write once size*/

    if(ERR_SEEK == lseek(fd, (*offset), SEEK_SET))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_flush: seek offset %ld failed\n", (*offset));
        return (EC_FALSE);
    }

    for(csize = 0, osize = CMISC_WRITE_ONCE_MAX_BYTES; csize < wsize; csize += osize)
    {
        if(csize + osize > wsize)
        {
            osize = wsize - csize;
        }

        if((ssize_t)osize != write(fd, buff + csize, osize))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_flush: flush buff to offset %ld failed where wsize %ld, csize %ld, osize %ld, errno %d, errstr %s\n",
                                (*offset), wsize, csize, osize, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == wsize);
 
    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_write(int fd, UINT32 *offset, const UINT32 wsize, const UINT8 *buff)
{
    UINT32  csize;/*write completed size*/
    UINT32  osize;/*write once size*/
    ssize_t wsize_t;

    if(ERR_SEEK == lseek(fd, (*offset), SEEK_SET))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_write: seek offset %ld failed\n", (*offset));
        return (EC_FALSE);
    }

    for(csize = 0, osize = CMISC_WRITE_ONCE_MAX_BYTES; csize < wsize; csize += wsize_t)
    {
        if(csize + osize > wsize)
        {
            osize = wsize - csize;
        }

        wsize_t = write(fd, buff + csize, osize);
     
        if(0 > wsize_t)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_write: write data to offset %ld failed where wsize %ld, csize %ld, osize %ld, wsize_t %ld, errno %d, errstr %s\n",
                                (*offset), wsize, csize, osize, wsize_t, errno, strerror(errno));
            (*offset) += csize;
            return (EC_FALSE);
        }
        if(0 == wsize_t)
        {
            (*offset) += csize;
            return (EC_TRUE);
        }
    }

    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_pad(int fd, UINT32 *offset, const UINT32 wsize, const UINT8 ch)
{
    UINT32 csize;/*write completed size*/
    UINT32 osize;/*write once size*/
    UINT8  buff[64];
    UINT32 len;

    len = sizeof(buff)/sizeof(buff[ 0 ]);
    BSET(buff, ch, len);

    if(ERR_SEEK == lseek(fd, (*offset), SEEK_SET))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_pad: seek offset %ld failed\n", (*offset));
        return (EC_FALSE);
    }

    for(csize = 0, osize = len; csize < wsize; csize += osize)
    {
        if(csize + osize > wsize)
        {
            osize = wsize - csize;
        }

        if((ssize_t)osize != write(fd, buff, osize))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_pad: flush buff to offset %ld failed where wsize %ld, csize %ld, osize %ld, errno %d, errstr %s\n",
                                (*offset), wsize, csize, osize, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == wsize);
 
    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_load(int fd, UINT32 *offset, const UINT32 rsize, UINT8 *buff)
{
    UINT32  csize;/*read completed size*/
    UINT32  osize;/*read once size*/
    ssize_t rsize_t;

    //dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_file_load: fd %d, offset %u, rsize %u\n", fd, (*offset), rsize);

    if(ERR_SEEK == lseek(fd, (*offset), SEEK_SET))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_load: seek offset %ld failed, errno %d, errstr %s\n", (*offset), errno, strerror(errno));
        return (EC_FALSE);
    }

    for(csize = 0, osize = CMISC_READ_ONCE_MAX_BYTES; csize < rsize; csize += rsize_t)
    {
        if(csize + osize > rsize)
        {
            osize = rsize - csize;
        }

        rsize_t = read(fd, buff + csize, osize);
        if(0 >= rsize_t)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_load: load buff from offset %ld failed where rsize %ld, csize %ld, osize %ld, rsize_t %ld, errno %d, errstr %s\n",
                                (*offset), rsize, csize, osize, rsize_t, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == rsize);
 
    (*offset) += csize;
    return (EC_TRUE);
}

/*try to read as more as possible. caller should check offset and make sure what happen*/
EC_BOOL c_file_read(int fd, UINT32 *offset, const UINT32 rsize, UINT8 *buff)
{
    UINT32  csize;/*read completed size*/
    UINT32  osize;/*read once size*/
    ssize_t rsize_t;

    if(ERR_SEEK == lseek(fd, (*offset), SEEK_SET))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_read: seek offset %ld failed, errno %d, errstr %s\n", (*offset), errno, strerror(errno));
        return (EC_FALSE);
    }

    for(csize = 0, osize = CMISC_READ_ONCE_MAX_BYTES; csize < rsize; csize += rsize_t)
    {
        if(csize + osize > rsize)
        {
            osize = rsize - csize;
        }

        rsize_t = read(fd, buff + csize, osize);
        if(0 > rsize_t)
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_read: read data from offset %ld failed where rsize %ld, csize %ld, osize %ld, rsize_t %ld, errno %d, errstr %s\n",
                                (*offset), rsize, csize, osize, rsize_t, errno, strerror(errno));
            (*offset) += csize;
            return (EC_FALSE);
        }

        if(0 == rsize_t)
        {
            (*offset) += csize;
            return (EC_TRUE);     
        }
    }

    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_pwrite(int fd, UINT32 *offset, const UINT32 wsize, const UINT8 *buff)
{
    UINT32 csize;/*write completed size*/
    UINT32 osize;/*write once size*/

    for(csize = 0, osize = CMISC_WRITE_ONCE_MAX_BYTES; csize < wsize; csize += osize)
    {
        if(csize + osize > wsize)
        {
            osize = wsize - csize;
        }

        if((ssize_t)osize != pwrite(fd, buff + csize, osize, (*offset) + csize))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_pwrite: flush buff to offset %ld failed where "
                               "wsize %ld, csize %ld, osize %ld, errno %d, errstr %s\n",
                                (*offset), wsize, csize, osize, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == wsize);
 
    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_ppad(int fd, UINT32 *offset, const UINT32 wsize, const UINT8 ch)
{
    UINT32 csize;/*write completed size*/
    UINT32 osize;/*write once size*/
    UINT8  buff[64];
    UINT32 len;

    len = sizeof(buff)/sizeof(buff[ 0 ]);
    BSET(buff, ch, len);

    for(csize = 0, osize = len; csize < wsize; csize += osize)
    {
        if(csize + osize > wsize)
        {
            osize = wsize - csize;
        }

        if((ssize_t)osize != pwrite(fd, buff, osize, (*offset) + csize))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_ppad: flush buff to offset %ld failed where "
                               "wsize %ld, csize %ld, osize %ld, errno %d, errstr %s\n",
                                (*offset), wsize, csize, osize, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == wsize);
 
    (*offset) += csize;
    return (EC_TRUE);
}

EC_BOOL c_file_pread(int fd, UINT32 *offset, const UINT32 rsize, UINT8 *buff)
{
    UINT32 csize;/*read completed size*/
    UINT32 osize;/*read once size*/

    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_file_pread: fd %d, offset %u, rsize %u\n", fd, (*offset), rsize);

    for(csize = 0, osize = CMISC_READ_ONCE_MAX_BYTES; csize < rsize; csize += osize)
    {
        if(csize + osize > rsize)
        {
            osize = rsize - csize;
        }

        if((ssize_t)osize != pread(fd, buff + csize, osize, (*offset) + csize))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_pread: load buff from offset %ld failed where "
                               "rsize %ld, csize %ld, osize %ld, errno %d, errstr %s\n",
                                (*offset), rsize, csize, osize, errno, strerror(errno));
            /*(*offset) += csize;*//*give up offset adjustment*/
            return (EC_FALSE);
        }
    }

    ASSERT(csize == rsize);
 
    (*offset) += csize;

    //dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "cdfsnp_buff_load: load %ld bytes\n", rsize);
    return (EC_TRUE);
}

EC_BOOL c_file_size(int fd, UINT32 *fsize)
{
    (*fsize) = lseek(fd, 0, SEEK_END);
    if(((UINT32)-1) == (*fsize))
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL c_file_size_b(int fd, uint64_t *fsize)
{
    (*fsize) = lseek(fd, 0, SEEK_END);
    if(((UINT32)-1) == (*fsize))
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL c_file_pos(int fd, UINT32 *fpos)
{
    (*fpos) = lseek(fd, 0, SEEK_CUR);
    if(((UINT32)-1) == (*fpos))
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL c_file_pos_b(int fd, uint64_t *fpos)
{
    (*fpos) = lseek(fd, 0, SEEK_CUR);
    if(((UINT32)-1) == (*fpos))
    {
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL c_file_access(const char *pathname, int mode)
{
    if(0 != access(pathname, mode))
    {
        //dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_access: access %s with mode %d failed\n", pathname, mode);
        return (EC_FALSE);
    }
    //dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_file_access: access %s with mode %d done\n", pathname, mode);
    return (EC_TRUE);
}

EC_BOOL c_file_truncate(int fd, const UINT32 fsize)
{
    /**
        -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
        will enable 32bit os support file size more than 2G due to off_t extented from 4B to 8B
    **/
    if(0 != ftruncate(fd, fsize))
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_truncate: fd %d truncate %ld bytes failed where , errno %d, errstr %s\n",
                            fd, fsize, errno, strerror(errno));
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

EC_BOOL c_file_unlink(const char *filename)
{
    if (NULL_PTR == filename)
    {
        return (EC_FALSE);
    }

    if(0 != unlink(filename))
    {
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

int c_mem_ncmp(const UINT8 *src, const UINT32 slen, const UINT8 *des, const UINT32 dlen)
{
    UINT32 len;

    int result;

    len = DMIN(slen, dlen);
    result = BCMP(src, des, len);
    if(0 != result)
    {
        return (result);
    }

    if(slen < dlen)
    {
        return (-1);
    }

    if(slen > dlen)
    {
        return (1);
    }
    return (0);
}

void c_ident_print(LOG *log, const UINT32 level)
{
    UINT32 idx;

    for(idx = 0; idx < level; idx ++)
    {
        sys_print(log, "    ");
    }
    return;
}

void c_usage_print(LOG *log, const char **usage, const int size)
{
    int pos;
    for(pos = 0; pos < size; pos ++)
    {
        sys_print(log, "usage: %s\n", usage[ pos ]);
    }
    return;
}

void c_history_init(char **history, const int max, int *size)
{
    int pos;

    for(pos = 0; pos < max; pos ++)
    {
        history[ pos ] = NULL_PTR;
    }
    (*size) = 0;
    return;
}

void c_history_push(char **history, const int max, int *size, const char *str)
{
    int pos;
    if(NULL_PTR == str)
    {
        return;
    }

    if((*size) < max)
    {
        history[ (*size) ] = (char *)str;
        (*size) ++;
        return;
    }

    if(NULL_PTR != history[ 0 ])
    {
        safe_free(history[ 0 ], LOC_CMISC_0047);
        history[ 0 ] = NULL_PTR;
    }

    for(pos = 1; pos < max; pos ++)
    {
        history[ pos - 1 ] = history[ pos ];
    }
    history[ pos - 1 ] = (char *)str;
 
    return;
}

void c_history_clean(char **history, const int max, const int size)
{
    int pos;

    for(pos = 0; pos < DMIN(max, size); pos ++)
    {
        safe_free(history[ pos ], LOC_CMISC_0048);
    }
    return;
}

void c_history_print(LOG *log, char **history, const int max, const int size)
{
    int pos;

    for(pos = 0; pos < DMIN(max, size); pos ++)
    {
        sys_print(log, "    %4d: %s\n", pos, history[ pos ]);
    }
    return;
}

void c_uint16_lo2hi_header_print(LOG *log)
{
    sys_print(log, "Lo                     --->                  Hi \n");
    sys_print(log, "00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 \n");
    return;
}

void c_uint16_lo2hi_bits_print(LOG *log, const uint16_t num)
{
    uint16_t bit_nth;
    uint16_t t;

    for(bit_nth = 0, t = num; bit_nth < sizeof(uint16_t) * BYTESIZE; bit_nth ++, t >>= 1)
    {
        sys_print(log, "%2d ", (t & 1));
    }
    sys_print(log, "\n");
 
    return;
}

void c_uint16_hi2lo_header_print(LOG *log)
{
        sys_print(log, "Hi                     --->                  Lo \n");
        sys_print(log, "15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 \n");
    return;
}

void c_uint16_hi2lo_bits_print(LOG *log, const uint16_t num)
{
    uint16_t bit_nth;
    uint16_t e;
    uint16_t t;

    bit_nth = sizeof(uint16_t) * BYTESIZE;
    e = (uint16_t)(~(((uint16_t)~0) >> 1));
 
    for(t = num; 0 < bit_nth; bit_nth --, t <<= 1)
    {
        sys_print(log, "%2d ", (t & e)?1:0);
    }
    sys_print(log, "\n");
 
    return;
}

void c_buff_print_char(LOG *log, const UINT8 *buff, const UINT32 len)
{
    UINT32 pos;

    for(pos = 0; pos < len; pos ++)
    {
        sys_print(log, "%c", buff[ pos ]);
    }
    sys_print(log, "\n");
    return;
}

void c_buff_print_hex(LOG *log, const UINT8 *buff, const UINT32 len)
{
    UINT32 pos;

    for(pos = 0; pos < len; pos ++)
    {
        sys_print(log, "%02x,", buff[ pos ]);
    }
    return;
}

void c_buff_print_str(LOG *log, const UINT8 *buff, const UINT32 len)
{
    sys_print(log, "%.*s", len, (char *)buff);
    return;
}

EC_BOOL c_isdigit(int c)
{
    if(c >= '0' && c <= '9')
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_isxdigit(int c)
{
    if (EC_TRUE == c_isdigit(c))
    {
        return (EC_TRUE);
    }

    c |= 32;
    if(c >= 'a' && c <= 'f')
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_isalpha(int c)
{
    c |= 32;
    if(c >= 'a' && c <= 'z')
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_isalnum(int c)
{
    if(EC_TRUE == c_isdigit(c) || EC_TRUE == c_isalpha(c))
    {
        return (EC_TRUE);
    }
    return (EC_FALSE);
}

EC_BOOL c_memcmp(const uint8_t *s1, const uint8_t *s2, const uint32_t len)
{
    uint32_t idx;

    for(idx = 0; idx < len; idx ++)
    {
        //dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_memcmp: idx %d, len %d, %c vs %c\n", idx, len, s1[ idx ], s2[ idx ]);
        if(s1[ idx ] != s2[ idx ])
        {
            return (EC_FALSE);
        }
    }
    return (EC_TRUE);
}

const char *c_bool_str(const EC_BOOL flag)
{
    if(EC_TRUE == flag)
    {
        return ((const char *)"true");
    }
    return ((const char *)"false");
}

EC_BOOL c_str_to_bool(const char *str)
{
    return c_str_is_in(str,  ",", "TRUE,true,EC_TRUE,yes");
}

int c_file_open(const char *pathname, const int flags, const mode_t mode)
{
    int fd;

    if(flags & O_CREAT)
    {
        if(EC_FALSE == c_basedir_create(pathname))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_open: create basedir of file %s failed\n", pathname);
            return (-1);
        }
    }

    fd = open(pathname, flags, mode);
    if(-1 == fd)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT,"error:c_file_open: open %s failed\n", pathname);
        return (-1);
    }

    if(1)
    {
        if( 0 > fcntl(fd, F_SETFD, FD_CLOEXEC))
        {
            dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_file_open: set fd %d to FD_CLOEXEC failed, errno = %d, errstr = %s\n",
                               fd, errno, strerror(errno));
            close(fd);
            return (-1);
        }
    }
    return (fd);
}

int c_file_close(int fd)
{
    if(-1 != fd)
    {
        return close(fd);
    }
    return (0);
}

CTM *c_localtime_r(const time_t *timestamp)
{
    CTM *ptime;
    c_mutex_lock(&g_cmisc_tm_cmutex, LOC_CMISC_0049);
    ptime = &(g_tm_tbl[g_tm_idx]);
    g_tm_idx = ((g_tm_idx + 1) % (CMISC_TM_NUM));
    c_mutex_unlock(&g_cmisc_tm_cmutex, LOC_CMISC_0050);

    if(NULL_PTR != timestamp)
    {
        localtime_r(timestamp, ptime);
    }
    else
    {
        time_t cur_timestamp;
        cur_timestamp = c_time(&cur_timestamp);
        localtime_r(&cur_timestamp, ptime);
    }
    return (ptime);
}

ctime_t c_time(ctime_t *timestamp)
{
    for(;;)
    {
        ctime_t t;
     
        t = time(timestamp);
        if(0 < t)
        {
            return (t);
        }

        /*error happen*/
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDERR, "error:c_time: time return %d, errno = %d, errstr = %s\n", t, errno, strerror(errno));
    }
    return (ctime_t)(-1);
}

EC_BOOL c_usleep(const UINT32 msec, const UINT32 location)
{
    struct timeval tv;

    tv.tv_sec  = (msec / 1000);
    tv.tv_usec = (msec % 1000) * 1000;

    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_usleep: now sleep %ld.%03ld seconds at %s:%ld\n",
                        msec / 1000, msec % 1000,
                        MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));

    select(0, NULL, NULL, NULL, &tv);

    return (EC_TRUE);
}

EC_BOOL c_sleep(const UINT32 nsec, const UINT32 location)
{
    struct timeval tv;

    tv.tv_sec  = nsec;
    tv.tv_usec = 0;

    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_sleep: now sleep %ld seconds at %s:%ld\n",
                        nsec,
                        MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
    select(0, NULL, NULL, NULL, &tv);

    return (EC_TRUE);
}

EC_BOOL c_checker_default(const void * val)
{
    return ((EC_BOOL)val);
}

void c_mutex_print(pthread_mutex_t *mutex)
{
#if 0
    fprintf(stdout, "c_mutex_print: mutex %lx: __m_lock = %d, __m_reserved = %d, __m_count = %d, __m_owner = %d, __m_kind = %d\n",
                    mutex,
                    mutex->__m_lock,
                    mutex->__m_reserved,
                    mutex->__m_count,
                    mutex->__m_owner,
                    mutex->__m_kind
            );
    fflush(stdout);         
#endif 
#if 1
    fprintf(stdout, "c_mutex_print: mutex %lx: __m_lock = %d, __m_reserved = %d, __m_count = %d, __m_owner = %d, __m_kind = %d\n",
                    (UINT32)((void *)mutex),
                    mutex->__data.__lock,
                    mutex->__data.__nusers,
                    mutex->__data.__count,
                    mutex->__data.__owner,
                    mutex->__data.__kind
            );
    fflush(stdout);         
#endif 

    return;
}

#if (SWITCH_OFF == CROUTINE_SUPPORT_SINGLE_CTHREAD_SWITCH)
pthread_mutex_t *c_mutex_new(const UINT32 flag, const UINT32 location)
{
    pthread_mutex_t *mutex;

    mutex = (pthread_mutex_t *)safe_malloc(sizeof(pthread_mutex_t), location);
    if(NULL_PTR == mutex)
    {
        fprintf(stdout, "error:c_mutex_new: malloc mutex failed, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
        fflush(stdout);
        return (NULL_PTR);
    }

    if(EC_FALSE == c_mutex_init(mutex, flag, location))
    {
        fprintf(stdout, "error:c_mutex_new: init mutex failed, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
        fflush(stdout);
        safe_free(mutex, location);
        return (NULL_PTR);
    }

    return (mutex);
}

EC_BOOL c_mutex_clean(pthread_mutex_t *mutex, const UINT32 location)
{
    int ret_val;

    ret_val = pthread_mutex_destroy(mutex);
    if( 0 != ret_val )
    {
        switch( ret_val )
        {
            case EINVAL:
            {
                fprintf(stdout, "error:c_mutex_clean - EINVAL: mutex doesn't refer to an initialized mutex, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EBUSY:
            {
                fprintf(stdout, "error:c_mutex_clean - EBUSY: mutex is locked or in use by another thread, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            default:
            {
                /* Unknown error */
                fprintf(stdout, "error:c_mutex_clean - UNKNOWN: mutex detect error, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }
        return (EC_FALSE);
    }
    return (EC_TRUE);
}

void c_mutex_free(pthread_mutex_t *mutex, const UINT32 location)
{
    if(NULL_PTR != mutex)
    {
        if(EC_FALSE == c_mutex_clean(mutex, location))
        {
            fprintf(stdout, "error:c_mutex_free: clean mutex failed, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
            fflush(stdout);
        }
        safe_free(mutex, location);
    }
    return;
}

EC_BOOL c_mutex_lock(pthread_mutex_t *mutex, const UINT32 location)
{
    int ret_val;

    ret_val = pthread_mutex_lock(mutex);
    if(0 != ret_val)
    {
        switch(ret_val)
        {
            case EINVAL:
            {
                fprintf(stdout, "error:c_mutex_lock - EINVAL: mutex NOT an initialized object, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EDEADLK:
            {
                fprintf(stdout, "error:c_mutex_lock - EDEADLK: deadlock is detected or current thread already owns the mutex, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case ETIMEDOUT:
            {
                fprintf(stdout, "error:c_mutex_lock - ETIMEDOUT: failed to lock mutex before the specified timeout expired, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EBUSY:
            {
                fprintf(stdout, "error:c_mutex_lock - EBUSY: failed to lock mutex due to busy, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            default:
            {
                fprintf(stdout, "error:c_mutex_lock - UNKNOWN: error detected, errno %d, errstr %s, called at %s:%ld\n", ret_val, strerror(ret_val), MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL c_mutex_unlock(pthread_mutex_t *mutex, const UINT32 location)
{
    int ret_val;

    ret_val = pthread_mutex_unlock(mutex);
    if(0 != ret_val)
    {
        switch(ret_val)
        {
            case EINVAL:
            {
                fprintf(stdout, "error:c_mutex_unlock - EINVAL: mutex NOT an initialized object, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EPERM:
            {
                fprintf(stdout, "error:c_mutex_unlock - EPERM: current thread does not hold a lock on mutex, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            default:
            {
                fprintf(stdout, "error:c_mutex_unlock - UNKNOWN: error detected, errno %d, errstr %s, called at %s:%ld\n", ret_val, strerror(ret_val), MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }
        return (EC_FALSE);
    }

    return (EC_TRUE);
}

EC_BOOL c_mutex_init(pthread_mutex_t *mutex, const UINT32 flag, const UINT32 location)
{
    pthread_mutexattr_t  mutex_attr;
    int ret_val;

    ret_val = pthread_mutexattr_init(&mutex_attr);
    if( 0 != ret_val )
    {
        switch( ret_val )
        {
            case ENOMEM:
            {
                fprintf(stdout, "error:c_mutex_init - ENOMEM: Insufficient memory to create the mutex attributes object, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
            default:
            {
                /* Unknown error */
                fprintf(stdout, "error:c_mutex_init - UNKNOWN: Error detected when mutexattr init, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }
        return (EC_FALSE);
    }

    if(CMUTEX_PROCESS_PRIVATE == flag)
    {
        ret_val = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_PRIVATE);
        if( 0 != ret_val )
        {
            switch( ret_val )
            {
                case EINVAL:
                {
                    fprintf(stdout, "error:c_mutex_init - EINVAL: value specified for argument -pshared- is INCORRECT, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                    fflush(stdout);
                    break;
                }

                default:
                {
                    fprintf(stdout, "error:c_mutex_init - UNKNOWN: error detected when setpshared, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                    fflush(stdout);
                    break;
                }
            }
            return (EC_FALSE);
        }
    }

    if(CMUTEX_PROCESS_SHARED == flag)
    {
        ret_val = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        if( 0 != ret_val )
        {
            switch( ret_val )
            {
                case EINVAL:
                {
                    fprintf(stdout, "error:c_mutex_init - EINVAL: value specified for argument -pshared- is INCORRECT, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                    fflush(stdout);
                    break;
                }

                default:
                {
                    fprintf(stdout, "error:c_mutex_init - UNKNOWN: error detected when setpshared, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                    fflush(stdout);
                    break;
                }
            }

            return (ret_val);
        }
    }

    /*Initialize the mutex attribute called 'type' to PTHREAD_MUTEX_RECURSIVE_NP,
    so that a thread can recursively lock a mutex if needed. */
    ret_val = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    if( 0 != ret_val )
    {
        switch( ret_val )
        {
            case EINVAL:
            {
                fprintf(stdout, "error:c_mutex_init - EINVAL: value specified for argument -type- is INCORRECT, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            default:
            {
                fprintf(stdout, "error:c_mutex_init - UNKNOWN: error detected when settype, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }
        return (EC_FALSE);
    }

    /* Creating and Initializing the mutex with the above stated mutex attributes */
    ret_val = pthread_mutex_init(mutex, &mutex_attr);
    if( 0 != ret_val )
    {
        switch( ret_val )
        {
            case EAGAIN:
            {
                fprintf(stdout, "error:mutex_new - EAGAIN: System resources(other than memory) are unavailable, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EPERM:
            {
                fprintf(stdout, "error:mutex_new - EPERM: Doesn't have privilige to perform this operation, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EINVAL:
            {
                fprintf(stdout, "error:mutex_new - EINVAL: mutex_attr doesn't refer a valid condition variable attribute object, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case EFAULT:
            {
                fprintf(stdout, "error:mutex_new - EFAULT: Mutex or mutex_attr is an invalid pointer, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            case ENOMEM:
            {
                fprintf(stdout, "error:mutex_new - ENOMEM: Insufficient memory exists to initialize the mutex, called at %s:%ld\n", MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }

            default:
            {
                /* Unknown error */
                fprintf(stdout, "error:mutex_new - UNKNOWN: Error detected when mutex init, error no: %d, called at %s:%ld\n", ret_val, MM_LOC_FILE_NAME(location), MM_LOC_LINE_NO(location));
                fflush(stdout);
                break;
            }
        }

        return (EC_FALSE);
    }

    return (EC_TRUE);
}
#endif /*(SWITCH_OFF == CROUTINE_SUPPORT_SINGLE_CTHREAD_SWITCH)*/

#if (SWITCH_ON == CROUTINE_SUPPORT_SINGLE_CTHREAD_SWITCH)
pthread_mutex_t *c_mutex_new(const UINT32 flag, const UINT32 location)
{
    return (NULL_PTR);
}

EC_BOOL c_mutex_clean(pthread_mutex_t *mutex, const UINT32 location)
{
    return (EC_FALSE);
}

void c_mutex_free(pthread_mutex_t *mutex, const UINT32 location)
{
    return;
}

EC_BOOL c_mutex_lock(pthread_mutex_t *mutex, const UINT32 location)
{
    return (EC_FALSE);
}

EC_BOOL c_mutex_unlock(pthread_mutex_t *mutex, const UINT32 location)
{
    return (EC_FALSE);
}

EC_BOOL c_mutex_init(pthread_mutex_t *mutex, const UINT32 flag, const UINT32 location)
{
    return (EC_FALSE);
}
#endif /*(SWITCH_ON == CROUTINE_SUPPORT_SINGLE_CTHREAD_SWITCH)*/

#define __X86          (1)
#define __IA64         (2)
#define __GENERIC      (3)

#if defined(REG_RIP)
#define CMISC_PLATFORM __IA64
#define CMISC_REG_FORMAT "%016lx"
#define CMISC_REG_RIP   REG_RIP
#define CMISC_REG_RBP   REG_RBP
#elif defined(REG_EIP)
#define CMISC_PLATFORM __X86
#define CMISC_REG_FORMAT "%08x"
#define CMISC_REG_RIP   REG_EIP
#define CMISC_REG_RBP   REG_EBP
#else
#define CMISC_PLATFORM __GENERIC
#define CMISC_REG_FORMAT "%x"
#endif

void c_backtrace_dump_details(LOG *log, ucontext_t *ucontext)
{
#if (__X86 == CMISC_PLATFORM || __IA64 == CMISC_PLATFORM)
    int      frame_idx = 0;
    Dl_info  dlinfo;
    void   **bp = 0;
    void    *ip = 0;
#endif /*(__X86 == CMISC_PLATFORM || __IA64 == CMISC_PLATFORM)*/

#if (__X86 != CMISC_PLATFORM && __IA64 != CMISC_PLATFORM)
    void *bt[128];
    char **strings;
    size_t size;
    size_t idx;
#endif /*(__X86 != CMISC_PLATFORM && __IA64 != CMISC_PLATFORM)*/

    if(1)
    {
        size_t reg_idx;
        for(reg_idx = 0; reg_idx < NGREG; reg_idx ++)
        {
            dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "reg[%02d]       = 0x" CMISC_REG_FORMAT "\n",
                               reg_idx,
                               ucontext->uc_mcontext.gregs[reg_idx]);
        }
    }

#if (__X86 == CMISC_PLATFORM || __IA64 == CMISC_PLATFORM)
    ip = (void*)ucontext->uc_mcontext.gregs[CMISC_REG_RIP];
    bp = (void**)ucontext->uc_mcontext.gregs[CMISC_REG_RBP];

    dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "c_backtrace_dump_details: stac trace:\n");
    while(NULL_PTR != bp && NULL_PTR != ip)
    {
        const char *symname;
        if(0 == dladdr(ip, &dlinfo))
        {
            break;
        }

        symname = dlinfo.dli_sname;
        dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "% 2d: %p <%s+%u> (%s)\n",
                            ++ frame_idx,
                            ip,
                            symname,
                            (unsigned)(ip - dlinfo.dli_saddr),
                            dlinfo.dli_fname);


        if(NULL_PTR != dlinfo.dli_sname && 0 == strcmp(dlinfo.dli_sname, "main"))
        {
            break;
        }

        ip = bp[1];
        bp = (void **)bp[0];
    } 
#endif /*(__X86 == CMISC_PLATFORM || __IA64 == CMISC_PLATFORM)*/

#if (__X86 != CMISC_PLATFORM && __IA64 != CMISC_PLATFORM)
    dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "c_backtrace_dump_details: stack trace (non-dedicated) beg\n");
 
    size    = backtrace(bt, sizeof(bt)/sizeof(bt[0]));
    strings = backtrace_symbols(bt, size);

    for(idx = 0; idx < size; ++ idx)
    {
        dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "%s\n", strings[idx]);
    }
#endif /*(__X86 != CMISC_PLATFORM && __IA64 != CMISC_PLATFORM)*/

  dbg_log(SEC_0013_CMISC, 5)(LOGSTDOUT, "c_backtrace_dump_details: stack trace end\n");

  return;
}

void c_backtrace_dump(LOG *log)
{
    void *bt[128];
    char **strings;
    size_t size;
    size_t idx;

    sys_log(log, "c_backtrace_dump: stack trace beg\n");
 
    size    = backtrace(bt, sizeof(bt)/sizeof(bt[0]));
    strings = backtrace_symbols(bt, size);

    for(idx = 0; idx < size; ++ idx)
    {
        sys_log(log, "c_backtrace_dump: %s\n", strings[idx]);
    } 

    sys_log(log, "c_backtrace_dump: stack trace end\n");
    return;
}

void c_indent_print(LOG *log, const UINT32 level)
{
    UINT32 idx;

    for(idx = 0; idx < level; idx ++)
    {
        sys_print(log, "    ");
    }
    return;
}

/*note: crc algorithm is copied from nginx*/
uint32_t c_crc32_short(uint8_t *p, size_t len)
{
    uint8_t   c;
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--)
    {
        c = *p++;
        crc = g_crc32_table_short[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
        crc = g_crc32_table_short[(crc ^ (c >>  4)) & 0xf] ^ (crc >> 4);
    }

    return (crc ^ 0xffffffff);
}
/*note: crc algorithm is copied from nginx*/
uint32_t c_crc32_long(uint8_t *p, size_t len)
{
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--)
    {
        crc = g_crc32_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    }

    return (crc ^ 0xffffffff);
}
/*note: crc algorithm is copied from nginx*/
void c_crc32_update(uint32_t *crc, uint8_t *p, size_t len)
{
    uint32_t  c;

    c = *crc;

    while (len--) {
        c = g_crc32_table256[(c ^ *p++) & 0xff] ^ (c >> 8);
    }

    *crc = c;
    return;
}


/*note: copied from nginx*/
time_t c_parse_http_time(uint8_t *value, size_t len)
{
    uint8_t      *p, *end;
    int           month;
    uint32_t      day = 0, year = 0, hour, min, sec;
    uint64_t      time;
    enum {
        no = 0,
        rfc822,   /* Tue, 10 Nov 2002 23:50:13   */
        rfc850,   /* Tuesday, 10-Dec-02 23:50:13 */
        isoc      /* Tue Dec 10 23:50:13 2002    */
    } fmt;

    fmt = 0;
    end = value + len;

    //day = 32;
    //year = 2038;

    for (p = value; p < end; p++) {
        if (*p == ',') {
            break;
        }

        if (*p == ' ') {
            fmt = isoc;
            break;
        }
    }

    for (p++; p < end; p++)
        if (*p != ' ') {
            break;
        }

    if (end - p < 18) {
        return ((time_t) -1);
        }

    if (fmt != isoc) {
        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
            return ((time_t) -1);
        }

        day = (*p - '0') * 10 + *(p + 1) - '0';
        p += 2;

        if (*p == ' ') {
            if (end - p < 18) {
                return ((time_t) -1);
            }
            fmt = rfc822;

        } else if (*p == '-') {
            fmt = rfc850;

        } else {
            return ((time_t) -1);
        }

        p++;
    }

    switch (*p) {

    case 'J':
        month = *(p + 1) == 'a' ? 0 : *(p + 2) == 'n' ? 5 : 6;
        break;

    case 'F':
        month = 1;
        break;

    case 'M':
        month = *(p + 2) == 'r' ? 2 : 4;
        break;

    case 'A':
        month = *(p + 1) == 'p' ? 3 : 7;
        break;

    case 'S':
        month = 8;
        break;

    case 'O':
        month = 9;
        break;

    case 'N':
        month = 10;
        break;

    case 'D':
        month = 11;
        break;

    default:
        return ((time_t) -1);
    }

    p += 3;

    if ((fmt == rfc822 && *p != ' ') || (fmt == rfc850 && *p != '-')) {
        return ((time_t) -1);
    }

    p++;

    if (fmt == rfc822) {
        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9'
            || *(p + 2) < '0' || *(p + 2) > '9'
            || *(p + 3) < '0' || *(p + 3) > '9')
        {
            return ((time_t) -1);
        }

        year = (*p - '0') * 1000 + (*(p + 1) - '0') * 100
               + (*(p + 2) - '0') * 10 + *(p + 3) - '0';
        p += 4;

    } else if (fmt == rfc850) {
        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
            return ((time_t) -1);
        }

        year = (*p - '0') * 10 + *(p + 1) - '0';
        year += (year < 70) ? 2000 : 1900;
        p += 2;
    }

    if (fmt == isoc) {
        if (*p == ' ') {
            p++;
        }

        if (*p < '0' || *p > '9') {
            return ((time_t) -1);
        }

        day = *p++ - '0';

        if (*p != ' ') {
            if (*p < '0' || *p > '9') {
                return ((time_t) -1);
            }

            day = day * 10 + *p++ - '0';
        }

        if (end - p < 14) {
            return ((time_t) -1);
        }
    }

    if (*p++ != ' ') {
        return ((time_t) -1);
    }

    if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
        return ((time_t) -1);
    }

    hour = (*p - '0') * 10 + *(p + 1) - '0';
    p += 2;

    if (*p++ != ':') {
        return ((time_t) -1);
    }

    if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
        return ((time_t) -1);
    }

    min = (*p - '0') * 10 + *(p + 1) - '0';
    p += 2;

    if (*p++ != ':') {
        return ((time_t) -1);
    }

    if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9') {
        return ((time_t) -1);
    }

    sec = (*p - '0') * 10 + *(p + 1) - '0';

    if (fmt == isoc) {
        p += 2;

        if (*p++ != ' ') {
            return ((time_t) -1);
        }

        if (*p < '0' || *p > '9' || *(p + 1) < '0' || *(p + 1) > '9'
            || *(p + 2) < '0' || *(p + 2) > '9'
            || *(p + 3) < '0' || *(p + 3) > '9')
        {
            return ((time_t) -1);
        }

        year = (*p - '0') * 1000 + (*(p + 1) - '0') * 100
               + (*(p + 2) - '0') * 10 + *(p + 3) - '0';
    }

    if (hour > 23 || min > 59 || sec > 59) {
        return ((time_t) -1);
    }

    if (day == 29 && month == 1) {
        if ((year & 3) || ((year % 100 == 0) && (year % 400) != 0)) {
            return ((time_t) -1);
        }

    } else if (day > g_mday[month]) {
        return ((time_t) -1);
    }

    /*
     * shift new year to March 1 and start months from 1 (not 0),
     * it is needed for Gauss' formula
     */

    if (--month <= 0) {
        month += 12;
        year -= 1;
    }

    /* Gauss' formula for Gregorian days since March 1, 1 BC */

    time = (uint64_t) (
            /* days in years including leap years since March 1, 1 BC */

            365 * year + year / 4 - year / 100 + year / 400

            /* days before the month */

            + 367 * month / 12 - 30

            /* days before the day */

            + day - 1

            /*
             * 719527 days were between March 1, 1 BC and March 1, 1970,
             * 31 and 28 days were in January and February 1970
             */

            - 719527 + 31 + 28) * 86400 + hour * 3600 + min * 60 + sec;

    return (time_t) time;
}

/*note: copied from nginx*/
void c_gmtime(time_t t, CTM *tp)
{
    uint32_t   yday;
    uint32_t  n, sec, min, hour, mday, mon, year, wday, days, leap;

    /* the calculation is valid for positive time_t only */

    n = (uint32_t) t;

    days = n / 86400;

    /* January 1, 1970 was Thursday */

    wday = (4 + days) % 7;

    n %= 86400;
    hour = n / 3600;
    n %= 3600;
    min = n / 60;
    sec = n % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/http_parse_time.c
     */

    /* days since March 1, 1 BC */
    days = days - (31 + 28) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last February days to next year, but we catch the case when "yday"
     * becomes negative.
     */

    year = (days + 2) * 400 / (365 * 400 + 100 - 4 + 1);

    yday = days - (365 * year + year / 4 - year / 100 + year / 400);

    if (yday < 0) {
        leap = (year % 4 == 0) && (year % 100 || (year % 400 == 0));
        yday = 365 + leap + yday;
        year--;
    }

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    mon = (yday + 31) * 10 / 306;

    /* the Gauss' formula that evaluates days before the month */

    mday = yday - (367 * mon / 12 - 30) + 1;

    if (yday >= 306) {

        year++;
        mon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        mon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }

    tp->tm_sec  = (int) sec;
    tp->tm_min  = (int) min;
    tp->tm_hour = (int) hour;
    tp->tm_mday = (int) mday;
    tp->tm_mon  = (int) mon;
    tp->tm_year = (int) year;
    tp->tm_wday = (int) wday;

    return;
}

char *c_http_time(time_t t)
{
    char *str_cache;
    CTM  ctm;

    c_mutex_lock(&g_cmisc_str_cmutex, LOC_CMISC_0051);
    str_cache = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));
    c_mutex_unlock(&g_cmisc_str_cmutex, LOC_CMISC_0052);
 
    c_gmtime(t, &ctm);

    snprintf(str_cache, CMISC_BUFF_LEN, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                       g_week[ctm.tm_wday],
                       ctm.tm_mday,
                       g_months[ctm.tm_mon - 1],
                       ctm.tm_year,
                       ctm.tm_hour,
                       ctm.tm_min,
                       ctm.tm_sec);
    return (str_cache);
}

UINT32 c_hash_strlow(const uint8_t *src, const uint32_t slen, uint8_t **des)
{
    UINT32    hash;
    uint8_t  *dst;
    uint32_t  n;

    if(CMISC_BUFF_LEN < slen) /*note: must left at least one space for possible '\0'*/
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_hash_strlow: fatal error: '%.*s' [len = %u] overflow\n",
                    slen, (char *)src, slen);

        if(NULL_PTR != des)
        {
            (*des) = NULL_PTR;
        }
                 
        return ((UINT32)~0);
    }

    dst = (uint8_t *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));

    if(NULL_PTR != des)
    {
        (*des) = dst;
    }

    hash = 0;
    n   = slen;

    while(n--)
    {
        *dst = c_tolower(*src);

        hash = c_hash(hash, *dst);
        dst ++;
        src ++;
    }

    return (hash);
}

CTMV *c_get_day_time()
{
    static CTMV cur_timev;
    gettimeofday(&cur_timev, NULL_PTR);

    return (&cur_timev);
}

char *c_get_day_time_str()
{
    CTMV            *cur_timev;
    CTM             *cur_time;

    int              tv_msec;
    int              tv_usec;
    char            *time_str;

    cur_timev = c_get_day_time();
    cur_time  = c_localtime_r(&(cur_timev->tv_sec));


    tv_msec = (int)(cur_timev->tv_usec / 1000);
    tv_usec = (int)(cur_timev->tv_usec % 1000);
    
    time_str = (char *)(g_str_buff[g_str_idx]);
    g_str_idx = ((g_str_idx + 1) % (CMISC_BUFF_NUM));

    snprintf(time_str, CMISC_BUFF_LEN, "%4d%02d%02d%02d%02d%02d%03d%03d",
                                       TIME_IN_YMDHMS(cur_time),
                                       tv_msec, tv_usec);
    
    return (time_str);
}

/*note: host_name is domain or ipv4 string*/
EC_BOOL c_dns_resolve(const char *host_name, UINT32 *ipv4)
{
    struct hostent *host;
    char           *ipv4_str;

    host = gethostbyname(host_name);
    if(NULL_PTR == host)
    {
        dbg_log(SEC_0013_CMISC, 0)(LOGSTDOUT, "error:c_dns_resolve: resolve host '%s' failed\n", host_name);
        return (EC_FALSE);
    }

    ipv4_str = inet_ntoa(*((struct in_addr *)host->h_addr));
    (*ipv4)  = c_ipv4_to_word(ipv4_str);

    dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_dns_resolve: resolve host '%s' => %s\n", host_name, ipv4_str);

    if(do_log(SEC_0013_CMISC, 9))
    {
        uint32_t idx;
        for(idx = 0; ; idx ++)
        {
            struct in_addr addr;
         
            if(0 == host->h_addr_list[ idx ])
            {
                break;
            }

            addr.s_addr = *(uint32_t *)host->h_addr_list[ idx ];

            dbg_log(SEC_0013_CMISC, 9)(LOGSTDOUT, "[DEBUG] c_dns_resolve:: [%u] host '%s': %s\n",
                            idx, host_name, inet_ntoa(addr));
        }
    }

    return (EC_TRUE);
}

#ifdef __cplusplus
}
#endif/*__cplusplus*/

