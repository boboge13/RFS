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
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include "type.h"

#include "clist.h"
#include "cvector.h"
#include "cstring.h"

#include "mm.h"

#include "cmisc.h"
#include "task.inc"
#include "task.h"
#include "tcnode.h"
#include "ctimer.h"

#include "taskcfg.inc"
#include "taskcfg.h"
#include "taskcfgchk.h"
#include "cxml.h"

#include "cdfs.h"
#include "cdfsnp.h"
#include "cdfsdn.h"

#include "crfs.h"
#include "crfsnp.h"
#include "crfsdn.h"
#include "crfsc.h"

#include "chfs.h"
#include "chfsnp.h"

#include "csfs.h"

#include "cbgt.h"
#include "csession.h"
#include "cbytes.h"

#include "log.h"
#include "chashalgo.h"

#include "cparacfg.h"

#include "findex.inc"

#include "cconsole.h"

#include "api_cmd.inc"
#include "api_cmd.h"
#include "api_cmd_ui.h"
#include "api_ui_util.h"

static char     api_cmd_line_buff[ API_CMD_LINE_BUFF_SIZE ];
static uint32_t api_cmd_line_buff_size = sizeof(api_cmd_line_buff) / sizeof(api_cmd_line_buff[0]);

static const char *api_cmd_prompt = "bgn> ";

EC_BOOL api_cmd_ui_init(CMD_ELEM_VEC *cmd_elem_vec, CMD_TREE *cmd_tree, CMD_HELP_VEC *cmd_help_vec)
{
    CMD_ELEM *where    = NULL_PTR;
    CMD_ELEM *tcid     = NULL_PTR;
    CMD_ELEM *rank     = NULL_PTR;
    CMD_ELEM *times    = NULL_PTR;
    CMD_ELEM *maski    = NULL_PTR;
    CMD_ELEM *maske    = NULL_PTR;
    CMD_ELEM *maskr    = NULL_PTR;
    CMD_ELEM *ipaddr   = NULL_PTR;
    CMD_ELEM *hops     = NULL_PTR;
    CMD_ELEM *remotes  = NULL_PTR;
    CMD_ELEM *ttl      = NULL_PTR;
    CMD_ELEM *on_off   = NULL_PTR;
    CMD_ELEM *cmd      = NULL_PTR;
    CMD_ELEM *oid      = NULL_PTR;
    CMD_ELEM *offset64 = NULL_PTR;

    where  = api_cmd_elem_create_cstring("<console|log>");
    api_cmd_elem_vec_add(cmd_elem_vec, where);

    tcid   = api_cmd_elem_create_tcid("<tcid>");
    api_cmd_elem_vec_add(cmd_elem_vec, tcid);

    rank   = api_cmd_elem_create_uint32("<rank>");
    api_cmd_elem_vec_add(cmd_elem_vec, rank);

    times  = api_cmd_elem_create_uint32("<times>");
    api_cmd_elem_vec_add(cmd_elem_vec, times);

    maski  = api_cmd_elem_create_mask("<maski>");
    api_cmd_elem_vec_add(cmd_elem_vec, maski);

    maske  = api_cmd_elem_create_mask("<maske>");
    api_cmd_elem_vec_add(cmd_elem_vec, maske);

    maskr  = api_cmd_elem_create_mask("<maskr>");
    api_cmd_elem_vec_add(cmd_elem_vec, maskr);

    ipaddr  = api_cmd_elem_create_ipaddr("<ipaddr>");
    api_cmd_elem_vec_add(cmd_elem_vec, ipaddr);

    hops  = api_cmd_elem_create_uint32("<max hops>");
    api_cmd_elem_vec_add(cmd_elem_vec, hops);

    ttl  = api_cmd_elem_create_uint32("<time to live in seconds>");
    api_cmd_elem_vec_add(cmd_elem_vec, ttl);

    remotes  = api_cmd_elem_create_uint32("<max remotes>");
    api_cmd_elem_vec_add(cmd_elem_vec, remotes);

    on_off = api_cmd_elem_create_list("<on|off>");
    api_cmd_elem_create_list_item(on_off, "on" , SWITCH_ON);
    api_cmd_elem_create_list_item(on_off, "off", SWITCH_OFF);
    api_cmd_elem_vec_add(cmd_elem_vec, on_off);

    cmd  = api_cmd_elem_create_cstring("cmd>");
    api_cmd_elem_vec_add(cmd_elem_vec, cmd);

    oid  = api_cmd_elem_create_uint32("<oid>");
    api_cmd_elem_vec_add(cmd_elem_vec, oid);

    api_cmd_help_vec_create(cmd_help_vec, "help"         , "help");
    //api_cmd_help_vec_create(cmd_help_vec, "version"      , "show version on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "run script"   , "script <file name>");

    api_cmd_help_vec_create(cmd_help_vec, "act sysconfig" , "act sysconfig on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "show sysconfig", "show sysconfig on {all | tcid <tcid> rank <rank>} at <console|log>");

    //api_cmd_help_vec_create(cmd_help_vec, "add route"    , "add route des_tcid <tcid> maskr <maskr> next_tcid <tcid> on tcid <tcid>");
    //api_cmd_help_vec_create(cmd_help_vec, "del route"    , "del route des_tcid <tcid> maskr <maskr> next_tcid <tcid> on tcid <tcid>");

    //api_cmd_help_vec_create(cmd_help_vec, "add conn"    , "add <num> conn to tcid <tcid> ipaddr <ipaddr> port <port> on {all | tcid <tcid>}");

    api_cmd_help_vec_create(cmd_help_vec, "diag mem"     , "diag mem {all | type <type>} on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "breathing mem", "breathing mem on {all | tcid <tcid> rank <rank>}");
    //api_cmd_help_vec_create(cmd_help_vec, "mon oid"      , "mon oid <oid> for <times> times on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "show client"  , "show client {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "show mem"     , "show mem {all | type <type>} on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "show queue"   , "show queue on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "check slowndown"   , "check slow down on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "show thread"  , "show thread {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "show route"   , "show route {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "show taskcomm", "show taskcomm {all | tcid <tcid>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "show load"    , "show rank load on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "sync load"    , "sync rank load from tcid <tcid> rank <rank> to {all | tcid <tcid> rank <rank>}");
    //api_cmd_help_vec_create(cmd_help_vec, "set  load"    , "set rank load of tcid <tcid> rank <rank> as load que <load> obj <load> cpu <load> mem <load> dsk <load> net <load> on {all | tcid <tcid> rank <rank>}");

    //api_cmd_help_vec_create(cmd_help_vec, "vendor show"   , "show vendor on {all | tcid <tcid>} at <console|log>");

    //api_cmd_help_vec_create(cmd_help_vec, "enable  brd"  , "enable task brd on {all | tcid <tcid> rank <rank>}");
    //api_cmd_help_vec_create(cmd_help_vec, "disable brd"  , "disable task brd on {all | tcid <tcid> rank <rank>}");

    //api_cmd_help_vec_create(cmd_help_vec, "shell"        , "shell <cmd> on {all | tcid <tcid>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "switch"       , "switch {all | tcid <tcid> rank <rank>} log <on|off>");
    //api_cmd_help_vec_create(cmd_help_vec, "switch"       , "switch ngx memc on {all | tcid <tcid> rank <rank>} to <on|off>");
    api_cmd_help_vec_create(cmd_help_vec, "shutdown"     , "shutdown <dbg | mon | work> {all | tcid <tcid>}");
    //api_cmd_help_vec_create(cmd_help_vec, "ping taskcomm", "ping taskcomm tcid <tcid> at <console|log>");

    //api_cmd_help_vec_create(cmd_help_vec, "sync taskcomm", "sync taskcomm hops <max hops> remotes <max remotes> ttl <time to live in seconds> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "taskcfgchk net"    , "taskcfgchk net {all | tcid <tcid>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "taskcfgchk route"  , "taskcfgchk route tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "taskcfgchk tracert", "taskcfgchk tracert src_tcid <tcid> des_tcid <tcid> hops <max hops> at <console|log>");
#if 0
#if (32 == WORDSIZE)
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs create"  , "hsdfs create npp <root dir> mode <4K|64K|1M|2M|128M|256M|512M|1G|2G> max <num> disk max <num> np on tcid <tcid> at <console|log>");
#endif/*(32 == WORDSIZE)*/
#if (64 == WORDSIZE)
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs create"  , "hsdfs create npp <root dir> mode <4K|64K|1M|2M|128M|256M|512M|1G|2G|4G> max <num> disk max <num> np on tcid <tcid> at <console|log>");
#endif/*(64 == WORDSIZE)*/
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs create"  , "hsdfs create dn <root dir> with <num> disk <max> GB on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs add"     , "hsdfs add {npp|dn} <tcid> to {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs reg"     , "hsdfs reg {npp|dn} <tcid> to {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs list"    , "hsdfs list {npp|dn} on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs show"    , "hsdfs show {npp|dn} on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs dbg"     , "hsdfs dbg show cached np on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs dbg"     , "hsdfs dbg show np <path layout> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs dbg"     , "hsdfs dbg write <num> files and replicas <replica num> in dir <dir> to npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs dbg"     , "hsdfs dbg import fnode file <log file name> on npp <tcid> to npp <tcid> timeout <num> minutes at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs dbg"     , "hsdfs dbg import replica file <log file name> on npp <tcid> to dn <tcid> timeout <num> minutes at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs path"    , "hsdfs show disk <num> block path layout <layout> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs open"    , "hsdfs open {npp|dn} <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs close"   , "hsdfs close [and flush] {npp|dn} on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs mkdir"   , "hsdfs mkdir <path> on {all | npp <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs read"    , "hsdfs read <file> from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs write"   , "hsdfs write <file> with content <text> and replicas <replica num> to npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs trunc"   , "hsdfs trunc <file> with size <file size> and replicas <replica num> to npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs update"  , "hsdfs update <file> with content <text> to npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs del"     , "hsdfs del {path|file|dir} <path> from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs trans"   , "hsdfs transfer <max> GB from dn <tcid> to dn <tcid> on npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs snapshot", "hsdfs make snapshot on {all | tcid <tcid>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qfile"   , "hsdfs qfile <file> from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qdir"    , "hsdfs qdir <dir> from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qlist"   , "hsdfs qlist <file or dir> {full | short} from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qcount"  , "hsdfs qcount <file or dir> files from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qsize"   , "hsdfs qsize <file or dir> file[s] from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs qblock"  , "hsdfs qblock tcid <tcid> pathlayout <path layout> from npp <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsdfs flush"   , "hsdfs flush {npp|dn} {all | tcid <tcid>} at <console|log>");
#endif

#if 0
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt create"  , "hsbgt create root table <name> with path <path> on tcid <tcid> rank <rank> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt create"  , "hsbgt create user table <name> column family <col[:col]> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt delete"  , "hsbgt delete user table <name> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt delete"  , "hsbgt delete colf table <table:colf> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt add"     , "hsbgt add colf table <table:colf> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt insert"  , "hsbgt insert user table <name> <row:colf:colq:val> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt delete"  , "hsbgt delete user table <name> <row:colf:colq> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt search"  , "hsbgt {search|fetch} user table <name> <row:colf:colq> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt select"  , "hsbgt select in [cached|all] user table <name> <(row regex)(colf regex)(colqregex)> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt select"  , "hsbgt select of [cached|all] user table <(table regex)(row regex)(colf regex)(colqregex)> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt select"  , "hsbgt select in [cached|all] colf <table:colf> <(row regex)(colqregex)> from path <path> at <console|log>");
//    api_cmd_help_vec_create(cmd_help_vec, "hsbgt open"    , "hsbgt open root table <name> from path <path> on tcid <tcid> rank <rank> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt open"    , "hsbgt open user table <name> colf <colf> from path <path> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt close"   , "hsbgt close module on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt show"    , "hsbgt show module on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt trav"    , "hsbgt {traversal|runthrough} [depth] on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt status"  , "hsbgt status on {all | tcid <tcid> rank <rank> modi <modi>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt flush"   , "hsbgt flush on {all | tcid <tcid> rank <rank> modi <modi>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsbgt debug"   , "hsbgt debug {merge|split} on tcid <tcid> rank <rank> modi <modi> at <console|log>");
#endif

#if 1
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs create"  , "hsrfs <id> create np model <model> max num <np mum> with hash algo <id> and root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs create"  , "hsrfs <id> create dn with root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs add"     , "hsrfs <id> add disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs del"     , "hsrfs <id> del disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs mount"   , "hsrfs <id> mount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs umount"  , "hsrfs <id> umount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs open"    , "hsrfs <id> open from root <dir> on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs close"   , "hsrfs <id> close on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs read"    , "hsrfs <id> read file <name> [from memcache] on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs write"   , "hsrfs <id> write file <name> with content <string> and expire <num> seconds on tcid <tcid> at <console|log>");

    /* write both memory cache and rfs & only write data to memory cache but NOT rfs */
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs write"   , "hsrfs <id> write file <name> with content <string> [to memcache] on tcid <tcid> at <console|log>");
    /* check whether file is in memory cache */
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs check"   , "hsrfs <id> check memcache file <name> on tcid <tcid> at <console|log>");
 
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs create"  , "hsrfs <id> create bigfile <name> with size <file size> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs read"    , "hsrfs <id> read bigfile <name> from offset <offset> max <max len> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs download", "hsrfs <id> download bigfile <name> to file <name> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs upload"  , "hsrfs <id> upload file <name> to bigfile <name> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs write"   , "hsrfs <id> write bigfile <name> with content <string> at offset <offset> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs delete"  , "hsrfs <id> delete bigfile <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs delete"  , "hsrfs <id> delete {file|dir|path} <name> [from memcache] on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs update"  , "hsrfs <id> update file <name> with content <string> to memcache on tcid <tcid> at <console|log>");

    /* for deleting root dir / only! */
    /* but do not exposure at api cmd ui */
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs delete root dir"  , "hsrfs <id> delete root dir / on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs recycle" , "hsrfs <id> recycle on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs retire"  , "hsrfs <id> retire max <num> files created before <nsec> seconds on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs flush"   , "hsrfs <id> flush on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs flush"   , "hsrfs <id> flush npp on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs flush"   , "hsrfs <id> flush dn on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs mkdir"   , "hsrfs <id> mkdir <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs search"  , "hsrfs <id> search <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs count"   , "hsrfs <id> count file num of <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs count"   , "hsrfs <id> count file size of <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs qfile"   , "hsrfs <id> qfile <file> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs qdir"    , "hsrfs <id> qdir <dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs qlist"   , "hsrfs <id> qlist <file or dir> {full | short | tree} [of np <np id>] on tcid <tcid> at <console|log>"); 
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs show"    , "hsrfs <id> show npp on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs show"    , "hsrfs <id> show dn on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs show"    , "hsrfs <id> show cached np on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs show"    , "hsrfs <id> show specific np <id> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfs show"    , "hsrfs <id> show locked files on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs md5sum"  , "hsrfs <id> md5sum file <name> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs md5sum"  , "hsrfs <id> md5sum bigfile <name> seg <no> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs snapshot", "hsrfs <id> snapshot {np <id> | npp | disk <no> | dn | vol | all} to path <dir> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs backup"  , "hsrfs <id> {create|open} backup np <dir> dn <dir> op <fname> on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs backup"  , "hsrfs <id> {show|close} backup on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs sync"    , "hsrfs <id> {start|end} sync on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hsrfs replay"  , "hsrfs <id> replay on tcid <tcid> at <console|log>");
#endif

#if 0
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc open"    , "hsrfsc open from root <dir> on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc close"   , "hsrfsc close on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc read"    , "hsrfsc read file <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc write"   , "hsrfsc write file <name> with content <string> and expire <num> seconds on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc create"  , "hsrfsc create bigfile <name> with size <file size> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc read"    , "hsrfsc read bigfile <name> from offset <offset> max <max len> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc download", "hsrfsc download bigfile <name> to file <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc upload"  , "hsrfsc upload file <name> to bigfile <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc write"   , "hsrfsc write bigfile <name> with content <string> at offset <offset> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc delete"  , "hsrfsc delete bigfile <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc delete"  , "hsrfsc delete {file|dir|path} <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc recycle" , "hsrfsc recycle on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc search"  , "hsrfsc search {file|dir} <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc count"   , "hsrfsc count file size of <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc qfile"   , "hsrfsc qfile <file> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc md5sum"  , "hsrfsc md5sum file <name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc md5sum"  , "hsrfsc md5sum bigfile <name> seg <no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc dir"     , "hsrfsc {add|del|has} dir <name> <tcid> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc dt"      , "hsrfsc {flush|load|clone|rollback|show} dt on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hsrfsc trans"   , "hsrfsc trans dir <name> {whole|prepare|handle|post|recycle} on tcid <tcid> at <console|log>");

#endif 
#if 0
    api_cmd_help_vec_create(cmd_help_vec, "hshfs create"  , "hshfs create np model <model> max num <np mum> with root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs create"  , "hshfs create dn with root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs add"     , "hshfs add disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs del"     , "hshfs del disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs mount"   , "hshfs mount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs umount"  , "hshfs umount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs open"    , "hshfs open from np root <dir> and dn root <dir> on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs close"   , "hshfs close on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs read"    , "hshfs read file <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs write"   , "hshfs write file <file name> with content <string> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs delete"  , "hshfs delete file <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs recycle" , "hshfs recycle on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs retire"  , "hshfs retire max <num> files created before <n> seconds on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs search"  , "hshfs search <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs qfile"   , "hshfs qfile <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs count"   , "hshfs count file num on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs count"   , "hshfs count file size of <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs show"    , "hshfs show npp on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hshfs show"    , "hshfs show dn on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hshfs show"    , "hshfs show cached np on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hshfs show"    , "hshfs show specific np <id> on tcid <tcid> at <console|log>"); 
#endif

#if 0
    api_cmd_help_vec_create(cmd_help_vec, "hssfs create"  , "hssfs create np model <model> max num <np mum> with root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs create"  , "hssfs create dn with root <root dir> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs add"     , "hssfs add disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs del"     , "hssfs del disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs mount"   , "hssfs mount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs umount"  , "hssfs umount disk <disk no> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs open"    , "hssfs open from np root <dir> and dn root <dir> on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs close"   , "hssfs close on tcid <tcid>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs read"    , "hssfs read file <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs write"   , "hssfs write file <file name> with content <string> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs delete"  , "hssfs delete file <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs search"  , "hssfs search <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs qfile"   , "hssfs qfile <file name> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs count"   , "hssfs count file num on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs count"   , "hssfs count file size of <path> on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs show"    , "hssfs show npp on tcid <tcid> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "hssfs show"    , "hssfs show dn on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hssfs show"    , "hssfs show cached np on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "hssfs show"    , "hssfs show specific np <id> on tcid <tcid> at <console|log>"); 
#endif


#if 0
    api_cmd_help_vec_create(cmd_help_vec, "session add"   , "session add name <name> expire <nsec> on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "session rmv"   , "session rmv [name[regex] <name> | id[regex] <id>] on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "session set"   , "session set [name <name> | id <id>] key <key path> val <val> on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "session get"   , "session get [nameregex <name> | idregex <id>] key <key path> on tcid <tcid> rank <rank> modi <modi> at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "session show"  , "session show on tcid <tcid> rank <rank> modi <modi> at <console|log>");
#endif
    //api_cmd_help_vec_create(cmd_help_vec, "exec download" , "exec download <file> on {all|tcid <tcid>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "exec upload"   , "exec upload <file> with <content> on {all|tcid <tcid>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "exec shell"    , "exec shell <cmd> on {all|tcid <tcid>} at <console|log>");

    //api_cmd_help_vec_create(cmd_help_vec, "udp server"    , "{start|stop|status} udp server on tcid <tcid> at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "download file" , "download file <src fname> to <des fname> from tcid <tcid>  at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "upload   file" , "upload file <src fname> to <des fname> on tcid <tcid>  at <console|log>");

    api_cmd_help_vec_create(cmd_help_vec, "log rotate"    , "rotate log <0|5|8> on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "log level"     , "show log level on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "log level"     , "set log level table to <level> on {all | tcid <tcid> rank <rank>} at <console|log>");
    api_cmd_help_vec_create(cmd_help_vec, "log level"     , "set log level sector <sector id> to <level> on {all | tcid <tcid> rank <rank>} at <console|log>");
    //api_cmd_help_vec_create(cmd_help_vec, "say hello"     , "say hello [loop <num>] to tcid <tcid> rank <rank> on {all | tcid <tcid> rank <rank>} at <console|log>");

    /*----------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    api_cmd_comm_define(cmd_tree, api_cmd_ui_add_route                   , "add route des_tcid %t maskr %m next_tcid %t on tcid %t", tcid, maskr, tcid, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_del_route                   , "del route des_tcid %t maskr %m next_tcid %t on tcid %t", tcid, maskr, tcid, tcid);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_add_conn                    , "add %n conn to tcid %t ipaddr %p port %n on tcid %t", rank, tcid, ipaddr, rank, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_add_conn_all                , "add %n conn to tcid %t ipaddr %p port %n on all"    , rank, tcid, ipaddr, rank);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_diag_mem_all                , "diag mem all on all at %s"                       , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_diag_mem                    , "diag mem all on tcid %t rank %n at %s"           , tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_diag_mem_all_of_type        , "diag mem type %n on all at %s"                   , rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_diag_mem_of_type            , "diag mem type %n on tcid %t rank %n at %s"       , rank, tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_breathing_mem               , "breathing mem on tcid %t rank %n"                , tcid, rank);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_breathing_mem_all           , "breathing mem on all");

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_client_all             , "show client all at %s"                           , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_client                 , "show client tcid %t at %s"                       , tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_activate_sys_cfg_all        , "act sysconfig on all at %s"                 , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_activate_sys_cfg            , "act sysconfig on tcid %t rank %n at %s"     , tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_sys_cfg_all            , "show sysconfig on all at %s"                 , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_sys_cfg                , "show sysconfig on tcid %t rank %n at %s"     , tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_mem_all                , "show mem all on all at %s"                       , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_mem                    , "show mem all on tcid %t rank %n at %s"           , tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_mem_all_of_type        , "show mem type %n on all at %s"                   , rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_mem_of_type            , "show mem type %n on tcid %t rank %n at %s"       , rank, tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_queue_all              , "show queue on all at %s"                            , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_queue                  , "show queue on tcid %t rank %n at %s"                , tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_check_slowdown_all          , "check slow down on all at %s"                    , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_check_slowdown              , "check slow down on tcid %t rank %n at %s"        , tcid, rank, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_thread_all            , "show thread all at %s"                           , where);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_thread                , "show thread tcid %t rank %n at %s"               , tcid, rank, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_route_all             , "show route all at %s"                            , where);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_route                 , "show route tcid %t at %s"                        , tcid, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_taskcomm_all          , "show taskcomm all at %s"                         , where);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_show_taskcomm              , "show taskcomm tcid %t at %s"                     , tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_version                , "show version on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_version_all            , "show version on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_vendor                 , "show vendor on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_vendor_all             , "show vendor on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_rank_load_all          , "show rank load on all at %s"                     , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_rank_load              , "show rank load on tcid %t rank %n at %s"         , tcid, rank, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_run_shell_all              , "shell %s on all at %s"                           , cmd, where);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_run_shell                  , "shell %s on tcid %t at %s"                       , cmd, tcid, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_switch_log_all             , "switch all log %l"                               , on_off);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_switch_log                 , "switch tcid %t rank %n log %l"                   , tcid, rank, on_off);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_switch_ngx_memc_all        , "switch ngx memc on all to %l"                    , on_off);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_switch_ngx_memc            , "switch ngx memc on tcid %t rank %n to %l"        , tcid, rank, on_off);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_rotate_log_all             , "rotate log %n on all at %s"                     , rank, where);
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_rotate_log                 , "rotate log %n on tcid %t rank %n at %s"         , rank, tcid, rank, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_do_test                    , "do test on tcid %t rank %n at %s"               , tcid, rank, where);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_dbg_all           , "shutdown dbg all"                                );
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_dbg               , "shutdown dbg tcid %t"                            , tcid);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_work_all          , "shutdown work all"                               );
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_work              , "shutdown work tcid %t"                           , tcid);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_mon_all           , "shutdown mon all"                                );
    api_cmd_comm_define(cmd_tree,  api_cmd_ui_shutdown_mon               , "shutdown mon tcid %t"                            , tcid);

    api_cmd_comm_define(cmd_tree,  api_cmd_ui_ping_taskcomm              , "ping taskcomm tcid %t at %s"                     , tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_sync_taskcomm_from_local    , "sync taskcomm hops %n remotes %n ttl %n on tcid %t at %s" , hops, remotes, ttl, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_sync_rank_load              , "sync rank load from tcid %t rank %n to tcid %t rank %n"   , tcid, rank, tcid, rank);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_sync_rank_load_to_all       , "sync rank load from tcid %t rank %n to all"   , tcid, rank);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_rank_load              , "set rank load of tcid %t rank %n as load que %n obj %n cpu %n mem %n dsk %n net %n on tcid %t rank %n"   , tcid, rank, rank, rank, rank, rank, rank, rank, tcid, rank);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_rank_load_on_all       , "set rank load of tcid %t rank %n as load %n on all"   , tcid, rank, rank);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_enable_task_brd           , "enable task brd on tcid %t rank %n", tcid, rank);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_enable_all_task_brd       , "enable task brd on all");

    api_cmd_comm_define(cmd_tree, api_cmd_ui_disable_task_brd           , "disable task brd on tcid %t rank %n", tcid, rank);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_disable_all_task_brd       , "disable task brd on all");

    api_cmd_comm_define(cmd_tree, api_cmd_ui_taskcfgchk_net_all          , "taskcfgchk net all at %s"                        , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_taskcfgchk_net              , "taskcfgchk net tcid %t at %s"                    , tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_taskcfgchk_route            , "taskcfgchk route tcid %t at %s"                  , tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_taskcfgchk_route_trace      , "taskcfgchk tracert src_tcid %t des_tcid %t hops %n at %s", tcid, tcid, hops, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_create_npp          , "hsdfs create npp %s mode %s max %n disk max %n np on tcid %t at %s", where, where, rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_open_npp            , "hsdfs open npp %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_npp           , "hsdfs close npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_with_flush_npp, "hsdfs close and flush npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_npp_all       , "hsdfs close npp on all at %s", where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_with_flush_npp_all, "hsdfs close and flush npp on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_create_dn          , "hsdfs create dn %s with %n disk %n GB on tcid %t at %s", where, times, times, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_open_dn            , "hsdfs open dn %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_dn           , "hsdfs close dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_with_flush_dn, "hsdfs close and flush dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_dn_all       , "hsdfs close dn on all at %s", where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_close_with_flush_dn_all, "hsdfs close and flush dn on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_mkdir                  , "hsdfs mkdir %s on npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_mkdir_all              , "hsdfs mkdir %s on all at %s", where, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_read                   , "hsdfs read %s from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_write                  , "hsdfs write %s with content %s and replicas %n to npp %t at %s", where, where, hops, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_trunc                  , "hsdfs trunc %s with size %n and replicas %n to npp %t at %s", where, rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_update                 , "hsdfs update %s with content %s to npp %t at %s", where, where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qfile                  , "hsdfs qfile %s from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qdir                   , "hsdfs qdir %s from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qlist_path             , "hsdfs qlist %s full from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qlist_seg              , "hsdfs qlist %s short from npp %t at %s", where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qcount_files           , "hsdfs qcount %s files from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qsize_files            , "hsdfs qsize %s files from npp %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qsize_one_file         , "hsdfs qsize %s file from npp %t at %s", where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_qblock                 , "hsdfs qblock tcid %t pathlayout %n from npp %t at %s", tcid, rank, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_flush_npp        , "hsdfs flush npp tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_flush_npp_all    , "hsdfs flush npp all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_flush_dn         , "hsdfs flush dn tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_flush_dn_all     , "hsdfs flush dn all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_add_npp          , "hsdfs add npp %t to tcid %t at %s", tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_add_npp_to_all   , "hsdfs add npp %t to all at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_add_dn           , "hsdfs add dn %t to tcid %t at %s", tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_add_dn_to_all    , "hsdfs add dn %t to all at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_reg_npp          , "hsdfs reg npp %t to tcid %t at %s", tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_reg_npp_to_all   , "hsdfs reg npp %t to all at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_reg_dn           , "hsdfs reg dn %t to tcid %t at %s", tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_reg_dn_to_all    , "hsdfs reg dn %t to all at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_list_npp         , "hsdfs list npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_list_npp_all     , "hsdfs list npp on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_list_dn          , "hsdfs list dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_list_dn_all      , "hsdfs list dn on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_npp         , "hsdfs show npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_npp_all     , "hsdfs show npp on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_dn          , "hsdfs show dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_dn_all      , "hsdfs show dn on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_delete_path_npp  , "hsdfs del path %s from npp %t at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_delete_file_npp  , "hsdfs del file %s from npp %t at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_delete_dir_npp   , "hsdfs del dir %s from npp %t at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_transfer_npp     , "hsdfs transfer %n GB from dn %t to dn %t on npp %t at %s", rank, tcid, tcid, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_make_snapshot    , "hsdfs make snapshot on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_make_snapshot_all, "hsdfs make snapshot on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_cached_np    , "hsdfs dbg show cached np on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_cached_np_all, "hsdfs dbg show cached np on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_showup_np         , "hsdfs dbg show np %n on tcid %t at %s", rank, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_write_files       , "hsdfs dbg write %n files and replicas %n in dir %s to npp %t at %s", rank, rank, where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_import_fnode_log  , "hsdfs dbg import fnode file %s on npp %t to npp %t timeout %n minutes at %s", where, tcid, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_import_replica_log, "hsdfs dbg import replica file %s on npp %t to dn %t timeout %n minutes at %s", where, tcid, tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cdfs_show_block_path  , "hsdfs show disk %n block path layout %n at %s", rank, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_create_root_table, "hsbgt create root table %s with path %s on tcid %t rank %n at %s", where, where, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_create_user_table, "hsbgt create user table %s column family %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_delete_user_table, "hsbgt delete user table %s from path %s at %s", where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_delete_colf_table, "hsbgt delete colf table %s from path %s at %s", where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_add_colf_table   , "hsbgt add colf table %s from path %s at %s", where, where, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_insert           , "hsbgt insert user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_delete           , "hsbgt delete user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_search           , "hsbgt search user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_fetch            , "hsbgt fetch user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_cached           , "hsbgt select of cached user table %s from path %s at %s", where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_in_cached_user   , "hsbgt select in cached user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_all           , "hsbgt select of all user table %s from path %s at %s", where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_in_all_user   , "hsbgt select in all user table %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_in_cached_colf, "hsbgt select in cached colf %s %s from path %s at %s", where, where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_select_in_all_colf   , "hsbgt select in all colf %s %s from path %s at %s", where, where, where, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_close_module     , "hsbgt close module on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);

    //api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_open_root_table  , "hsbgt open root table %s from path %s on tcid %t rank %n at %s", where, where, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_open_colf_table  , "hsbgt open user table %s colf %s from path %s at %s", where, where, where, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_show_module      , "hsbgt show module on tcid %t rank %n at %s", tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_show_module_all  , "hsbgt show module on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_traversal_module      , "hsbgt traversal on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_traversal_module_all  , "hsbgt traversal on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_traversal_depth_module      , "hsbgt traversal depth on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_traversal_depth_module_all  , "hsbgt traversal depth on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_runthrough_module      , "hsbgt runthrough on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    //api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_runthrough_module_all  , "hsbgt runthrough on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_runthrough_depth_module      , "hsbgt runthrough depth on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    //api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_runthrough_depth_module_all  , "hsbgt runthrough depth on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_print_status     , "hsbgt status on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_print_status_all , "hsbgt status on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_flush            , "hsbgt flush on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_flush_all        , "hsbgt flush on all at %s", where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_debug_merge      , "hsbgt debug merge on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_cbgt_debug_split      , "hsbgt debug split on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_add          , "session add name %s expire %n on tcid %t rank %n modi %n at %s", where, rank, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_rmv_by_name  , "session rmv name %s on tcid %t rank %n modi %n at %s", where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_rmv_by_id    , "session rmv id %n on tcid %t rank %n modi %n at %s", rank, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_rmv_by_name_regex  , "session rmv nameregex %s on tcid %t rank %n modi %n at %s", where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_rmv_by_id_regex    , "session rmv idregex %s on tcid %t rank %n modi %n at %s", where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_set_by_name  , "session set name %s key %s val %s on tcid %t rank %n modi %n at %s", where, where, where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_set_by_id    , "session set id %n key %s val %s on tcid %t rank %n modi %n at %s", where, where, where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_get_by_name  , "session get name %s key %s on tcid %t rank %n modi %n at %s", where, where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_get_by_id    , "session get id %n key %s on tcid %t rank %n modi %n at %s", rank, where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_show         , "session show on tcid %t rank %n modi %n at %s", tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_get_by_name_regex  , "session get nameregex %s key %s on tcid %t rank %n modi %n at %s", where, where, tcid, rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csession_get_by_id_regex    , "session get idregex %s key %s on tcid %t rank %n modi %n at %s", where, where, tcid, rank, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_download_all     , "exec download %s on all at %s"      , where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_download         , "exec download %s on tcid %t at %s"  , where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_upload_all       , "exec upload %s with %s on all at %s"          , where, where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_upload           , "exec upload %s with %s on tcid %t at %s"      , where, where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_shell_all        , "exec shell %s on all at %s"      , where, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_exec_shell            , "exec shell %s on tcid %t at %s"  , where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_start_mcast_udp_server, "start udp server on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_stop_mcast_udp_server , "stop udp server on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_status_mcast_udp_server , "status udp server on tcid %t at %s", tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_create_npp       , "hsrfs %n create np model %n max num %n with hash algo %n and root %s on tcid %t at %s", rank, rank, rank, rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_create_dn        , "hsrfs %n create dn with root %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_add_disk         , "hsrfs %n add disk %n on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_del_disk         , "hsrfs %n del disk %n on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_mount_disk       , "hsrfs %n mount disk %n on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_umount_disk      , "hsrfs %n umount disk %n on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_open             , "hsrfs %n open from root %s on tcid %t", rank, where, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_close            , "hsrfs %n close on tcid %t", rank, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_read             , "hsrfs %n read file %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_read_memc        , "hsrfs %n read file %s from memcache on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_write            , "hsrfs %n write file %s with content %s on tcid %t at %s", rank, where, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_write_memc       , "hsrfs %n write file %s with content %s to memcache on tcid %t at %s", rank, where, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_update_memc      , "hsrfs %n update file %s with content %s to memcache on tcid %t at %s", rank, where, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_check_memc       , "hsrfs %n check memcache file %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_create_b         , "hsrfs %n create bigfile %s with size %N on tcid %t at %s", rank, where, offset64, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_read_b           , "hsrfs %n read bigfile %s from offset %N max %n on tcid %t at %s", rank, where, offset64, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_write_b          , "hsrfs %n write bigfile %s with content %s at offset %N on tcid %t at %s", rank, where, where, offset64, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_b         , "hsrfs %n delete bigfile %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_download_b       , "hsrfs %n download bigfile %s to file %s on tcid %t at %s", rank, where, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_upload_b         , "hsrfs %n upload file %s to bigfile %s on tcid %t at %s", rank, where, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_mkdir            , "hsrfs %n mkdir %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_search           , "hsrfs %n search %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_file      , "hsrfs %n delete file %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_dir       , "hsrfs %n delete dir %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_path      , "hsrfs %n delete path %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_file_memc , "hsrfs %n delete file %s from memcache on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_dir_memc  , "hsrfs %n delete dir %s from memcache on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_path_memc , "hsrfs %n delete path %s from memcache on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_root_dir  , "hsrfs %n delete root dir %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_delete_root_dir_memc  , "hsrfs %n delete root dir %s from memcache on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_recycle          , "hsrfs %n recycle on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_retire           , "hsrfs %n retire max %n files created before %n seconds on tcid %t at %s", rank, rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_flush            , "hsrfs %n flush on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_flush_npp        , "hsrfs %n flush npp on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_flush_dn         , "hsrfs %n flush dn on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_npp         , "hsrfs %n show npp on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_dn          , "hsrfs %n show dn on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_cached_np   , "hsrfs %n show cached np on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_specific_np , "hsrfs %n show specific np %n on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_locked_files, "hsrfs %n show locked files on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_md5sum           , "hsrfs %n md5sum file %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_md5sum_b         , "hsrfs %n md5sum bigfile %s seg %n on tcid %t at %s", rank, where, rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_np_snapshot      , "hsrfs %n snapshot np %n to path %s on tcid %t at %s", rank, rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_npp_snapshot     , "hsrfs %n snapshot npp to path %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_disk_snapshot    , "hsrfs %n snapshot disk %n to path %s on tcid %t at %s", rank, rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_dn_snapshot      , "hsrfs %n snapshot dn to path %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_vol_snapshot     , "hsrfs %n snapshot vol to path %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_all_snapshot     , "hsrfs %n snapshot all to path %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_create_backup    , "hsrfs %n create backup np %s dn %s op %s on tcid %t at %s", rank, where, where, where,tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_open_backup      , "hsrfs %n open backup np %s dn %s op %s on tcid %t at %s", rank, where, where, where,tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_close_backup     , "hsrfs %n close backup on tcid %t at %s", rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_show_backup      , "hsrfs %n show backup on tcid %t at %s", rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_start_sync       , "hsrfs %n start sync on tcid %t at %s", rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_end_sync         , "hsrfs %n end sync on tcid %t at %s", rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_replay           , "hsrfs %n replay on tcid %t at %s", rank, tcid, where);   
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_count_file_num   , "hsrfs %n count file num of %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_count_file_size  , "hsrfs %n count file size of %s on tcid %t at %s", rank, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qfile            , "hsrfs %n qfile %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qdir             , "hsrfs %n qdir %s on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_path_of_np , "hsrfs %n qlist %s full of np %n on tcid %t at %s", rank, where, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_seg_of_np  , "hsrfs %n qlist %s short of np %n on tcid %t at %s", rank, where, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_tree_of_np , "hsrfs %n qlist %s tree of np %n on tcid %t at %s", rank, where, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_path       , "hsrfs %n qlist %s full on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_seg        , "hsrfs %n qlist %s short on tcid %t at %s", rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfs_qlist_tree       , "hsrfs %n qlist %s tree on tcid %t at %s", rank, where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_open            , "hsrfsc open from root %s on tcid %t", where, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_close           , "hsrfsc close on tcid %t", tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_read            , "hsrfsc read file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_write           , "hsrfsc write file %s with content %s and expire %n seconds on tcid %t at %s", where, where, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_create_b        , "hsrfsc create bigfile %s with size %N on tcid %t at %s", where, offset64, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_read_b          , "hsrfsc read bigfile %s from offset %N max %n on tcid %t at %s", where, offset64, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_write_b         , "hsrfsc write bigfile %s with content %s at offset %N on tcid %t at %s", where, where, offset64, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_delete_b        , "hsrfsc delete bigfile %s on tcid %t at %s", where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_download_b      , "hsrfsc download bigfile %s to file %s on tcid %t at %s", where, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_upload_b        , "hsrfsc upload file %s to bigfile %s on tcid %t at %s", where, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_search_file     , "hsrfsc search file %s on tcid %t at %s", where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_search_dir      , "hsrfsc search dir %s on tcid %t at %s", where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_delete_file     , "hsrfsc delete file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_delete_dir      , "hsrfsc delete dir %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_delete_path     , "hsrfsc delete path %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_recycle         , "hsrfsc recycle on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_md5sum          , "hsrfsc md5sum file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_md5sum_b        , "hsrfsc md5sum bigfile %s seg %n on tcid %t at %s", where, rank, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_count_file_size , "hsrfsc count file size of %s on tcid %t at %s", where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_qfile           , "hsrfsc qfile %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_add_dir         , "hsrfsc add dir %s %t on tcid %t at %s", where, tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_del_dir         , "hsrfsc del dir %s %t on tcid %t at %s", where, tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_has_dir         , "hsrfsc has dir %s %t on tcid %t at %s", where, tcid, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_clone_dt        , "hsrfsc clone dt on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_rollback_dt     , "hsrfsc rollback dt on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_flush_dt        , "hsrfsc flush dt on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_load_dt         , "hsrfsc load dt on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_show_dt         , "hsrfsc show dt on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_trans_dir_whole , "hsrfsc trans dir %s whole on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_trans_dir_prepare,"hsrfsc trans dir %s prepare on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_trans_dir_handle, "hsrfsc trans dir %s handle on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_trans_dir_post  , "hsrfsc trans dir %s post on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_crfsc_trans_dir_recycle,"hsrfsc trans dir %s recycle on tcid %t at %s", where, tcid, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_create_npp       , "hshfs create np model %n max num %n with root %s on tcid %t at %s", rank, rank, rank, rank, rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_create_dn        , "hshfs create dn with root %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_add_disk         , "hshfs add disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_del_disk         , "hshfs del disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_mount_disk       , "hshfs mount disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_umount_disk      , "hshfs umount disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_open             , "hshfs open from np root %s and dn root %s on tcid %t", where, where, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_close            , "hshfs close on tcid %t", tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_read             , "hshfs read file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_write            , "hshfs write file %s with content %s on tcid %t at %s", where, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_search           , "hshfs search %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_qfile            , "hshfs qfile %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_delete           , "hshfs delete file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_recycle          , "hshfs recycle on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_retire           , "hshfs retire max %n files created before %n seconds on tcid %t at %s", rank, rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_show_npp         , "hshfs show npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_show_dn          , "hshfs show dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_show_cached_np   , "hshfs show cached np on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_show_specific_np , "hshfs show specific np %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_count_file_num   , "hshfs count file num on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_chfs_count_file_size  , "hshfs count file size of %s on tcid %t at %s", where, tcid, where); 

    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_create_npp       , "hssfs create np model %n max num %n with root %s on tcid %t at %s", rank, rank, rank, rank, rank, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_create_dn        , "hssfs create dn with root %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_add_disk         , "hssfs add disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_del_disk         , "hssfs del disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_mount_disk       , "hssfs mount disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_umount_disk      , "hssfs umount disk %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_open             , "hssfs open from np root %s and dn root %s on tcid %t", where, where, tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_close            , "hssfs close on tcid %t", tcid);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_read             , "hssfs read file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_write            , "hssfs write file %s with content %s on tcid %t at %s", where, where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_search           , "hssfs search %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_qfile            , "hssfs qfile %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_delete           , "hssfs delete file %s on tcid %t at %s", where, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_show_npp         , "hssfs show npp on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_show_dn          , "hssfs show dn on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_show_cached_np   , "hssfs show cached np on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_show_specific_np , "hssfs show specific np %n on tcid %t at %s", rank, tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_count_file_num   , "hssfs count file num on tcid %t at %s", tcid, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_csfs_count_file_size  , "hssfs count file size of %s on tcid %t at %s", where, tcid, where); 

    api_cmd_comm_define(cmd_tree, api_cmd_ui_download_file         , "download file %s to %s from tcid %t at %s", where, where, tcid, where); 
    api_cmd_comm_define(cmd_tree, api_cmd_ui_upload_file           , "upload file %s to %s on tcid %t at %s", where, where, tcid, where); 

    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_log_level_all    , "show log level on all at %s"            , where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_show_log_level        , "show log level on tcid %t rank %n at %s", tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_log_level_tab_all , "set log level table to %n on all at %s", rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_log_level_tab     , "set log level table to %n on tcid %t rank %n at %s", rank, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_log_level_sec_all , "set log level sector %n to %n on all at %s", rank, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_set_log_level_sec     , "set log level sector %n to %n on tcid %t rank %n at %s", rank, rank, tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_say_hello             , "say hello to tcid %t rank %n on tcid %t rank %n at %s", tcid, rank, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_say_hello_all         , "say hello to tcid %t rank %n on all at %s", tcid, rank, where);

    api_cmd_comm_define(cmd_tree, api_cmd_ui_say_hello_loop        , "say hello loop %n to tcid %t rank %n on tcid %t rank %n at %s", rank, tcid, rank, tcid, rank, where);
    api_cmd_comm_define(cmd_tree, api_cmd_ui_say_hello_loop_all    , "say hello loop %n to tcid %t rank %n on all at %s", rank, tcid, rank, where);

    return (EC_TRUE);
}

void api_cmd_ui_do_script(CMD_TREE *cmd_tree, CMD_HELP_VEC *cmd_help_vec, char *script_name)
{
    FILE *script_fp;

    char   cmd_line[ 256 ];
    UINT32 cmd_line_len;

    script_fp = fopen(script_name, "r");
    if(NULL_PTR == script_fp)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_do_script: open script %s failed\n", script_name);
        return;
    }

    cmd_line_len = sizeof(cmd_line)/sizeof(cmd_line[0]);

    while(fgets(cmd_line, cmd_line_len, script_fp))
    {
        UINT8 *cmd_line_ptr;
        cmd_line_ptr = api_cmd_greedy_space((UINT8 *)cmd_line, ((UINT8 *)cmd_line) + strlen((char *)cmd_line));

        if(NULL_PTR != cmd_line_ptr && '#' != cmd_line_ptr[0])
        {
            cmd_line_ptr[strlen((char *)cmd_line_ptr) - 1] = '\0';
            dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "##################################################################################\n");
            dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "CMD [%s]\n", cmd_line_ptr);
            dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "##################################################################################\n");
            api_cmd_ui_do_once(cmd_tree, cmd_help_vec, (char *)cmd_line_ptr);
        }
    }

    fclose(script_fp);
    return;
}
void api_cmd_ui_do_once(CMD_TREE *cmd_tree, CMD_HELP_VEC *cmd_help_vec, char *cmd_line)
{
    CMD_PARA_VEC *cmd_para_vec;
    UINT8 *cmd_line_ptr;

    cmd_line_ptr = api_cmd_greedy_space((UINT8 *)cmd_line, ((UINT8 *)cmd_line) + strlen((char *)cmd_line));
    if(NULL_PTR == cmd_line_ptr)
    {
        return;
    }

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "CMD [%s]\n", cmd_line_ptr);

    if(0 == strncasecmp((char *)cmd_line_ptr, "help", 4))
    {
        api_cmd_comm_help(LOGCONSOLE, cmd_help_vec);
        return;
    }

    if(0 == strncasecmp((char *)cmd_line_ptr, "script", 6))
    {
        char *fields[8];
        if(2 != c_str_split((char *)cmd_line_ptr, " \r\n\t", fields, 8))
        {
            api_cmd_comm_help(LOGCONSOLE, cmd_help_vec);
            return;
        }
        api_cmd_ui_do_script(cmd_tree, cmd_help_vec, fields[1]);
        return;
    }

    cmd_para_vec = api_cmd_para_vec_new();
    api_cmd_comm_parse(cmd_tree, cmd_line_ptr, cmd_para_vec);
    api_cmd_para_vec_free(cmd_para_vec);

    return;
}

EC_BOOL api_cmd_ui_task(CMD_TREE *cmd_tree, CMD_HELP_VEC *cmd_help_vec)
{   
    const char  *prompt;
    char        *cmd;
    uint32_t     size;

    prompt = api_cmd_prompt;
    cmd    = (char *)api_cmd_line_buff;
    size   = api_cmd_line_buff_size;

    sys_print(LOGCONSOLE, "%32s------------------------------------------------------------\n", "");
    sys_print(LOGCONSOLE, "%32s|                                                          |\n", "");
    sys_print(LOGCONSOLE, "%32s|                WELCOME TO BGN CONSOLE UTILITY            |\n", "");
    sys_print(LOGCONSOLE, "%32s|                                                          |\n", "");
    sys_print(LOGCONSOLE, "%32s------------------------------------------------------------\n", "");
    
    for(;;)
    {
        uint32_t     len;
        EC_BOOL      ret;

        ret = cconsole_cmd_get(prompt, cmd, size, &len);
        
        if(EC_FALSE == ret)
        {
            dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_task: quit console due to get cmd failed\n");
            csig_stop(SIGHUP);
            break;
        }

        if(EC_AGAIN == ret)
        {
            continue;
        }
        
        /*quit*/
        if(EC_TRUE == c_str_is_in(cmd, (const char *)":", (const char *)"exit:quit"))
        {
            dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_task: quit console by cmd\n");
            csig_stop(SIGHUP);
            break;
        }
        
        /*add command to history*/
        cconsole_cmd_add_history(cmd);

        /*show history commands*/
        if(EC_TRUE == c_str_is_in(cmd, (const char *)":", (const char *)"history:his"))
        {
            cconsole_cmd_print_history(LOGCONSOLE);
            continue;
        }
        
        api_cmd_ui_do_once(cmd_tree, cmd_help_vec, (char *)cmd);
    }

    cconsole_cmd_clear_history();
   
    return (EC_FALSE);
}

static MOD_MGR *api_cmd_ui_gen_mod_mgr(const UINT32 incl_tcid, const UINT32 incl_rank, const UINT32 excl_tcid, const UINT32 excl_rank, const UINT32 modi)
{
    TASK_BRD  *task_brd;
    TASKC_MGR *taskc_mgr;

    MOD_MGR *mod_mgr;

    task_brd = task_brd_default_get();

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);

    taskc_mgr = taskc_mgr_new();
    task_brd_sync_taskc_mgr(task_brd, taskc_mgr);
    mod_mgr_gen_by_taskc_mgr(taskc_mgr, incl_tcid, incl_rank, modi, mod_mgr);
    taskc_mgr_free(taskc_mgr);

    mod_mgr_excl(excl_tcid, CMPI_ANY_COMM, excl_rank, modi, mod_mgr);
    return (mod_mgr);
}

static LOG *api_cmd_ui_get_log(const CSTRING *where)
{
    if(0 == strcmp("log", (char *)cstring_get_str(where)))
    {
        sys_log(LOGSTDOUT, "[DEBUG] api_cmd_ui_get_log: return LOGSTDOUT\n");
        return (LOGSTDOUT);
    }

    if(0 == strcmp("console", (char *)cstring_get_str(where)))
    {
        sys_log(LOGSTDOUT, "[DEBUG] api_cmd_ui_get_log: return LOGCONSOLE\n");
        return (LOGCONSOLE);
    }
    sys_log(LOGSTDOUT, "[DEBUG] api_cmd_ui_get_log: return LOGSTDNULL\n");
    return (LOGSTDNULL);
}

static UINT32 api_cmd_ui_get_cdfsnp_mode(const CSTRING *db_mode)
{
    UINT8  cdfsnp_mode_str[32];
    UINT32 cdfsnp_mode;

    snprintf((char *)cdfsnp_mode_str, sizeof(cdfsnp_mode_str)/sizeof(cdfsnp_mode_str[ 0 ]), "CDFSNP_%s_MODE", (char *)cstring_get_str(db_mode));
    cdfsnp_mode = cdfsnp_mode_get((char *)cdfsnp_mode_str);

    if(CDFSNP_ERR_MODE == cdfsnp_mode)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_get_cdfsnp_mode:invalid db mode %s, change automatically to default 1M mode\n",
                            (char *)cstring_get_str(db_mode));
        return (CDFSNP_ERR_MODE);
    }
    return (cdfsnp_mode);
}

EC_BOOL api_cmd_ui_activate_sys_cfg(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "act sysconfig on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_activate_sys_cfg beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_activate_sys_cfg end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_activate_sys_cfg, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);
    sys_log(des_log, "done\n");

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_activate_sys_cfg_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    LOG *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "act sysconfig on all at %s\n",
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_activate_sys_cfg_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_activate_sys_cfg_all end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_activate_sys_cfg, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);
    sys_log(des_log, "done\n");

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_sys_cfg(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show sysconfig on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_sys_cfg beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_sys_cfg end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0047);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_sys_cfg, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0048);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_sys_cfg_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show sysconfig on all at %s\n",
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_sys_cfg_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_sys_cfg_all end ----------------------------------\n");
    }
#endif
    report_vec = cvector_new(0, MM_LOG, LOC_API_0049);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_sys_cfg, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0050);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_show_mem(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show mem all on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0051);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_mem, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0052);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_mem_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show mem all on all at %s\n",
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0053);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_mem, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0054);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_show_mem_of_type(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 type;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_uint32(param, 0, &type);
    api_cmd_para_vec_get_tcid(param, 1, &tcid);
    api_cmd_para_vec_get_uint32(param, 2, &rank);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "type = %ld, tcid = %s, rank = %ld, where = %s\n",
                        type,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_of_type beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_of_type end ----------------------------------\n");
    }
#endif
    report_vec = cvector_new(0, MM_LOG, LOC_API_0055);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_mem_of_type, ERR_MODULE_ID, type, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0056);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_mem_all_of_type(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;
    UINT32   type;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param, 0, &type);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_show_mem_all_of_type type = %ld where = %s\n", type, (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_all_of_type beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_mem_all_of_type end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0057);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_mem_of_type, ERR_MODULE_ID, type, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0058);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_diag_mem(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "tcid = %s, rank = %ld, where = %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0059);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_diag_mem, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0060);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_diag_mem_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_diag_mem_all where = %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0061);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_diag_mem, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0062);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_diag_mem_of_type(CMD_PARA_VEC * param)
{
    UINT32 type;
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param, 0, &type);
    api_cmd_para_vec_get_tcid(param, 1, &tcid);
    api_cmd_para_vec_get_uint32(param, 2, &rank);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "type = %ld, tcid = %s, rank = %ld, where = %s\n",
                        type,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_of_type beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_of_type end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0063);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_diag_mem_of_type, ERR_MODULE_ID, type, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0064);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_diag_mem_all_of_type(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;
    UINT32 type;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param, 0, &type);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_diag_mem_all_of_type type = %ld where = %s\n", type, (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_all_of_type beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_diag_mem_all_of_type end ----------------------------------\n");
    }
#endif
    report_vec = cvector_new(0, MM_LOG, LOC_API_0065);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_diag_mem_of_type, ERR_MODULE_ID, type, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0066);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_clean_mem(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "tcid = %s, rank = %ld\n",
                       c_word_to_ipv4(tcid),
                       rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_clean_mem beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_clean_mem end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_clean_mem, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_clean_mem_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_clean_mem_all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_clean_mem_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_clean_mem_all end ----------------------------------\n");
    }
#endif 
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_clean_mem, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_breathing_mem(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "breathing mem on tcid %s, rank %ld\n",
                       c_word_to_ipv4(tcid),
                       rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_breathing_mem beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_breathing_mem end ----------------------------------\n");
    }
#endif 

#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_breathing_mem, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_breathing_mem_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "breathing mem on all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_breathing_mem_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_breathing_mem_all end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_breathing_mem, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_show_log_level(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show log level on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_log_level beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_log_level end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0067);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_log_level_tab, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0068);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_log_level_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show log level on all at %s\n",
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_log_level_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_log_level_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0069);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_log_level_tab, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0070);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_set_log_level_tab(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    level;
    UINT32    tcid;
    UINT32    rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &level);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    /*set log level tablele to %n on tcid %t rank %n at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set log level table to %ld on tcid %s rank %ld at %s\n",
                        level,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_tab beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_tab end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0071);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0072);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_set_log_level_tab, ERR_MODULE_ID, level);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] FAIL\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0073);
    }

    cvector_free(report_vec, LOC_API_0074);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_set_log_level_tab_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    level;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &level);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    /*set log level tablele to %n on all at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set log level table to %ld on all at %s\n",
                        level, (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_tab_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_tab_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0075);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0076);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_set_log_level_tab, ERR_MODULE_ID, level);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] FAIL\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0077);
    }

    cvector_free(report_vec, LOC_API_0078);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_set_log_level_sec(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    sector;
    UINT32    level;
    UINT32    tcid;
    UINT32    rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &sector);
    api_cmd_para_vec_get_uint32(param , 1, &level);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    /*set log level sector %n to %n on tcid %t rank %n at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set log level sector %ld to %ld on tcid %s rank %ld at %s\n",
                        sector,
                        level,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_sec beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_sec end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0079);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0080);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_set_log_level_sector, ERR_MODULE_ID, sector, level);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] FAIL\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0081);
    }

    cvector_free(report_vec, LOC_API_0082);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_set_log_level_sec_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    sector;
    UINT32    level;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &sector);
    api_cmd_para_vec_get_uint32(param , 1, &level);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    /*set log level sector %n to %n on all at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set log level sector %ld to %ld on all at %s\n",
                        sector,
                        level,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_sec_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_log_level_sec_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0083);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0084);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_set_log_level_sector, ERR_MODULE_ID, sector, level);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] FAIL\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0085);
    }

    cvector_free(report_vec, LOC_API_0086);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_say_hello(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    des_tcid;
    UINT32    des_rank;
    UINT32    tcid;
    UINT32    rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *hello_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &des_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &des_rank);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    /*say hello to tcid %t rank %n on tcid %t rank %n at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "say hello to tcid %s rank %ld on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(des_tcid),
                        des_rank,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0087);
    hello_vec = cvector_new(0, MM_CSTRING, LOC_API_0088); 

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CSTRING *cstring;
     
        cstring = cstring_new(NULL_PTR, LOC_API_0089);
        cvector_push(hello_vec, (void *)cstring);

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0090);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_say_hello, ERR_MODULE_ID, des_tcid, des_rank, cstring);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;
        CSTRING  *cstring;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cstring = (CSTRING *)cvector_get(hello_vec, remote_mod_node_idx);
        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC] %s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), cstring_get_str(cstring));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(hello_vec, remote_mod_node_idx, NULL_PTR);
        cstring_free(cstring);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0091);
     
    }

    cvector_free(hello_vec, LOC_API_0092);
    cvector_free(report_vec, LOC_API_0093);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_say_hello_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    des_tcid;
    UINT32    des_rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *hello_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &des_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &des_rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    /*say hello to tcid %t rank %n on tcid %t rank %n at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "say hello to tcid %s rank %ld on all at %s\n",
                        c_word_to_ipv4(des_tcid),
                        des_rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0094);
    hello_vec = cvector_new(0, MM_CSTRING, LOC_API_0095); 

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CSTRING *cstring;
     
        cstring = cstring_new(NULL_PTR, LOC_API_0096);
        cvector_push(hello_vec, (void *)cstring);

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0097);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_say_hello, ERR_MODULE_ID, des_tcid, des_rank, cstring);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;
        CSTRING  *cstring;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cstring = (CSTRING *)cvector_get(hello_vec, remote_mod_node_idx);
        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC] %s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), cstring_get_str(cstring));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(hello_vec, remote_mod_node_idx, NULL_PTR);
        cstring_free(cstring);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0098);
     
    }

    cvector_free(hello_vec, LOC_API_0099);
    cvector_free(report_vec, LOC_API_0100);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_say_hello_loop(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    loops;
    UINT32    des_tcid;
    UINT32    des_rank;
    UINT32    tcid;
    UINT32    rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &loops);
    api_cmd_para_vec_get_tcid(param   , 1, &des_tcid);
    api_cmd_para_vec_get_uint32(param , 2, &des_rank);
    api_cmd_para_vec_get_tcid(param   , 3, &tcid);
    api_cmd_para_vec_get_uint32(param , 4, &rank);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    /*say hello loop to tcid %t rank %n on tcid %t rank %n at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "say hello loop %ld to tcid %s rank %ld on tcid %s rank %ld at %s\n",
                        loops,
                        c_word_to_ipv4(des_tcid),
                        des_rank,
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_loop beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_loop end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0101);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
     
        alloc_static_mem(MM_UINT32, &ret, LOC_API_0102);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_say_hello_loop, ERR_MODULE_ID, loops, des_tcid, des_rank);
    }
    task_wait(task_mgr, TASK_ALWAYS_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0103);
     
    }

    cvector_free(report_vec, LOC_API_0104);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_say_hello_loop_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    UINT32    loops;
    UINT32    des_tcid;
    UINT32    des_rank;
    CSTRING  *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &loops);
    api_cmd_para_vec_get_tcid(param   , 1, &des_tcid);
    api_cmd_para_vec_get_uint32(param , 2, &des_rank);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    /*say hello loop to tcid %t rank %n on all at %s*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "say hello loop %ld to tcid %s rank %ld on all at %s\n",
                        loops,
                        c_word_to_ipv4(des_tcid),
                        des_rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_loop_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_say_hello_loop_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0105);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
     
        alloc_static_mem(MM_UINT32, &ret, LOC_API_0106);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_say_hello_loop, ERR_MODULE_ID, loops, des_tcid, des_rank);
    }
    task_wait(task_mgr, TASK_ALWAYS_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0107);     
    }

    cvector_free(report_vec, LOC_API_0108);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_switch_log(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 on_off;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_uint32(param, 2, &on_off);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "switch tcid %s, rank %ld log to %ld\n",
                        c_word_to_ipv4((tcid)),
                        rank,
                        on_off);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_log beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_log end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        if(SWITCH_OFF == on_off)/*off*/
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_log_off, ERR_MODULE_ID);
        }
        else
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_log_on, ERR_MODULE_ID);
        }
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_switch_log_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    UINT32 on_off;

    api_cmd_para_vec_get_uint32(param, 0, &on_off);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "switch all log to %ld\n", on_off);

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_log_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_log_all end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        if(SWITCH_OFF == on_off)/*off*/
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_log_off, ERR_MODULE_ID);
        }
        else
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_log_on, ERR_MODULE_ID);
        }
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_rotate_log(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 log_index;
    CSTRING *where;

    LOG      *des_log;
    EC_BOOL   ret;

    MOD_NODE  mod_node;

    api_cmd_para_vec_get_uint32(param , 0, &log_index);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_cstring(param, 3, &where);
 
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "rotate log %ld on tcid %s, rank %ld at %s\n",
                        log_index,
                        c_word_to_ipv4((tcid)),
                        rank,
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = rank;
    MOD_NODE_MODI(&mod_node) = 0;/*super_md_id = 0*/

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_super_rotate_log, ERR_MODULE_ID, log_index);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] log rotate done\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] log rotate failed\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_rotate_log_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    UINT32 log_index;
    CSTRING *where;

    LOG      *des_log;
    CVECTOR  *report_vec;

    api_cmd_para_vec_get_uint32(param , 0, &log_index);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "rotate log %ld on all at %s\n",
                        log_index,
                        (char *)cstring_get_str(where));

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0109);
 
    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0110);
        cvector_push(report_vec, (void *)ret);

        (*ret) = EC_FALSE;
     
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_rotate_log, ERR_MODULE_ID, log_index);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32   *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] FAIL\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0111);
    }

    cvector_free(report_vec, LOC_API_0112); 

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_do_test(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    LOG      *des_log;
    EC_BOOL   ret;

    MOD_NODE  mod_node;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);
 
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "do test on tcid %s, rank %ld at %s\n",
                        c_word_to_ipv4((tcid)),
                        rank,
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = rank;
    MOD_NODE_MODI(&mod_node) = 0;/*super_md_id = 0*/

    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_super_do_test, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] test done\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] test failed\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_switch_ngx_memc(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 on_off;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_uint32(param, 2, &on_off);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "switch ngx memc on tcid %s, rank %ld to %ld\n",
                        c_word_to_ipv4((tcid)),
                        rank,
                        on_off);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_ngx_memc beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_ngx_memc end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        if(SWITCH_OFF == on_off)/*off*/
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_ngx_memc_off, ERR_MODULE_ID);
        }
        else
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_ngx_memc_on, ERR_MODULE_ID);
        }
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_switch_ngx_memc_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    UINT32 on_off;

    api_cmd_para_vec_get_uint32(param, 0, &on_off);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "switch ngx memc on all to %ld\n", on_off);

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_ngx_memc_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_switch_ngx_memc_all end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        if(SWITCH_OFF == on_off)/*off*/
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_ngx_memc_off, ERR_MODULE_ID);
        }
        else
        {
            task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_ngx_memc_on, ERR_MODULE_ID);
        }
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_enable_to_rank_node(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 des_rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_uint32(param, 2, &des_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enable tcid %s rank %ld to rank %d\n",
                        c_word_to_ipv4((tcid)),
                        rank,
                        des_rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_to_rank_node beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_to_rank_node end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_rank_node_green, ERR_MODULE_ID, (des_rank));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_enable_all_to_rank_node(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    UINT32 des_rank;

    api_cmd_para_vec_get_uint32(param, 0, &des_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "disable all to rank %ld\n", des_rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_all_to_rank_node beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_all_to_rank_node end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_rank_node_green, ERR_MODULE_ID, (des_rank));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_disable_to_rank_node(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    UINT32 des_rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_uint32(param, 2, &des_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "disable tcid %s rank %ld to rank %ld\n", c_word_to_ipv4((tcid)), rank, des_rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_to_rank_node beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_to_rank_node end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_rank_node_red, ERR_MODULE_ID, (des_rank));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_disable_all_to_rank_node(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    UINT32 des_rank;

    api_cmd_para_vec_get_uint32(param, 0, &des_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "disable all to rank %ld\n", des_rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_all_to_rank_node beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_all_to_rank_node end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_switch_rank_node_red, ERR_MODULE_ID, (des_rank));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_queue(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show queue on tcid %s, rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_queue beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_queue end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0113);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_queues, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0114);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_queue_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show queue on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_queue_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_queue_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0115);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_queues, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0116);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_check_slowdown(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "check slow down on tcid %s, rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_check_slowdown beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_check_slowdown end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0117);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_check_slowdown, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0118);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_check_slowdown_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "check slow down on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_check_slowdown_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_check_slowdown_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0119);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_check_slowdown, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0120);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_client(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show client tcid %s where %s\n", c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_client beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_client end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0121);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_work_client, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0122);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_client_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show client all where = %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_client_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_client_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0123);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_work_client, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0124);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_thread(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "tcid = %s, rank = %ld, where = %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_thread beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_thread end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0125);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_thread_num, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0126);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_thread_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_show_thread_all where = %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_thread_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_thread_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0127);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_thread_num, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0128);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_show_route(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "tcid = %s, where = %s\n",
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_route beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_route end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0129);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_route_table, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0130);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_route_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_show_route_all where = %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_route_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_route_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0131);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_route_table, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0132);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_show_rank_node(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "tcid = %s, rank = %ld, where = %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_node beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_node end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0133);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_rank_node, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0134);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_rank_node_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enter api_cmd_ui_show_rank_node_all where = %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_node_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_node_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0135);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_rank_node, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0136);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);

}

EC_BOOL api_cmd_ui_show_rank_load(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show rank load on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_load beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_load end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0137);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_rank_load, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0138);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_rank_load_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show rank load on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_load_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_rank_load_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0139);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_rank_load, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0140);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_shutdown_work(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown work tcid %s\n", c_word_to_ipv4(tcid));

    if(EC_TRUE == task_brd_check_is_dbg_tcid(tcid))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:tcid = %s is debug taskcomm\n", c_word_to_ipv4(tcid));
        return (NULL_PTR);
    }

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_work beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_work end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_shutdown_work_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    //TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown work all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_work_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_work_all end ----------------------------------\n");
    }
#endif
#if 0
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif

#if 1
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_mono(mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP, remote_mod_node_idx,
                      NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_shutdown_dbg(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown dbg tcid %s\n", c_word_to_ipv4(tcid));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_dbg beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_dbg end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_shutdown_dbg_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown dbg all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_DBG_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ANY_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_dbg_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_dbg_all end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_shutdown_mon(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown mon tcid %s\n", c_word_to_ipv4(tcid));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_mon beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_mon end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_shutdown_mon_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "shutdown mon all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_MON_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ANY_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_mon_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_shutdown_mon_all end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_PREEMPT, TASK_NOT_NEED_RSP_FLAG, TASK_NEED_NONE_RSP);

    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_shutdown_taskcomm, ERR_MODULE_ID);
    }

    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);/*mod_mgr will be freed automatically if calling task_no_wait*/
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_taskcomm(CMD_PARA_VEC * param)
{
    UINT32  tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;
    UINT32 ret;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show taskcomm tcid %s where = %s\n", c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_taskcomm beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_taskcomm end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0141);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        TASKC_MGR *taskc_mgr;

        taskc_mgr = taskc_mgr_new();

        cvector_push(report_vec, (void *)taskc_mgr);
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_super_sync_taskc_mgr, 0, taskc_mgr);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        TASKC_MGR *taskc_mgr;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        taskc_mgr = (TASKC_MGR *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        taskc_mgr_print(des_log, taskc_mgr);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        taskc_mgr_free(taskc_mgr);
    }

    cvector_free(report_vec, LOC_API_0142);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_taskcomm_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;
    UINT32 ret;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show taskcomm all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_taskcomm_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_taskcomm_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0143);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        TASKC_MGR *taskc_mgr;

        taskc_mgr = taskc_mgr_new();

        cvector_push(report_vec, (void *)taskc_mgr);
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_super_sync_taskc_mgr, 0, taskc_mgr);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        TASKC_MGR *taskc_mgr;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        taskc_mgr = (TASKC_MGR  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        taskc_mgr_print(des_log, taskc_mgr);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        taskc_mgr_free(taskc_mgr);
    }

    cvector_free(report_vec, LOC_API_0144);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_add_route(CMD_PARA_VEC * param)
{
    UINT32 des_tcid;
    UINT32 maskr;
    UINT32 next_tcid;
    UINT32 on_tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &des_tcid);
    api_cmd_para_vec_get_mask(param, 1, &maskr);
    api_cmd_para_vec_get_tcid(param, 2, &next_tcid);
    api_cmd_para_vec_get_tcid(param, 3, &on_tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "add route des_tcid = %s maskr = %s next_tcid = %s on tcid = %s\n",
                        c_word_to_ipv4(des_tcid), c_word_to_ipv4(maskr), c_word_to_ipv4(next_tcid), c_word_to_ipv4(on_tcid));

    mod_mgr = api_cmd_ui_gen_mod_mgr(on_tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_route beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_route end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL, FI_super_add_route, 0, des_tcid, maskr, next_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_del_route(CMD_PARA_VEC * param)
{
    UINT32 des_tcid;
    UINT32 maskr;
    UINT32 next_tcid;
    UINT32 on_tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &des_tcid);
    api_cmd_para_vec_get_mask(param, 1, &maskr);
    api_cmd_para_vec_get_tcid(param, 2, &next_tcid);
    api_cmd_para_vec_get_tcid(param, 3, &on_tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "del route des_tcid = %s maskr = %s next_tcid = %s on tcid = %s\n",
                        c_word_to_ipv4(des_tcid), c_word_to_ipv4(maskr), c_word_to_ipv4(next_tcid), c_word_to_ipv4(on_tcid));

    mod_mgr = api_cmd_ui_gen_mod_mgr(on_tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_del_route beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_del_route end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL, FI_super_del_route, 0, des_tcid, maskr, next_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_add_conn(CMD_PARA_VEC * param)
{
    UINT32 conn_num;
    UINT32 des_tcid;
    UINT32 des_srv_ipaddr;
    UINT32 des_srv_port;
    UINT32 on_tcid;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_uint32(param, 0, &conn_num);
    api_cmd_para_vec_get_tcid(param  , 1, &des_tcid);
    api_cmd_para_vec_get_ipaddr(param, 2, &des_srv_ipaddr);
    api_cmd_para_vec_get_uint32(param, 3, &des_srv_port);
    api_cmd_para_vec_get_tcid(param  , 4, &on_tcid);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "add %ld conn to tcid = %s ipaddr = %s port = %ld on tcid = %s\n",
                        conn_num,
                        c_word_to_ipv4(des_tcid),
                        c_word_to_ipv4(des_srv_ipaddr),
                        des_srv_port,
                        c_word_to_ipv4(on_tcid));

    mod_mgr = api_cmd_ui_gen_mod_mgr(on_tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_conn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_conn end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL, FI_super_add_connection, ERR_MODULE_ID, des_tcid, des_srv_ipaddr, des_srv_port, conn_num);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_add_conn_all(CMD_PARA_VEC * param)
{
    UINT32 conn_num;
    UINT32 des_tcid;
    UINT32 des_srv_ipaddr;
    UINT32 des_srv_port;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_uint32(param, 0, &conn_num);
    api_cmd_para_vec_get_tcid(param  , 1, &des_tcid);
    api_cmd_para_vec_get_ipaddr(param, 2, &des_srv_ipaddr);
    api_cmd_para_vec_get_uint32(param, 3, &des_srv_port);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "add %ld conn to tcid = %s ipaddr = %s port = %ld on all\n",
                        conn_num,
                        c_word_to_ipv4(des_tcid),
                        c_word_to_ipv4(des_srv_ipaddr),
                        des_srv_port);

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_conn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_add_conn_all end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL, FI_super_add_connection, ERR_MODULE_ID, des_tcid, des_srv_ipaddr, des_srv_port, conn_num);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_run_shell(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    CSTRING *cmd_line;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    //cmd_line = cstring_new(NULL_PTR, LOC_API_0145);

    api_cmd_para_vec_get_cstring(param, 0, &cmd_line);
    api_cmd_para_vec_get_tcid(param, 1, &tcid);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "run shell %s on tcid %s where %s\n", (char *)cstring_get_str(cmd_line), c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_run_shell beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_run_shell end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0146);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_run_shell, ERR_MODULE_ID, cmd_line, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG  *log;
        char *str;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);
        str = (char *)cstring_get_str(LOG_CSTR(log));

        sys_log(des_log, "[rank_%s_%ld] %s\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(cmd_line),
                         NULL_PTR == str ? "(null)\n" : str);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0147);
    mod_mgr_free(mod_mgr);

    //cstring_free(cmd_line);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_run_shell_all(CMD_PARA_VEC * param)
{
    CSTRING *cmd_line;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    //cmd_line = cstring_new(NULL_PTR, LOC_API_0148);
    api_cmd_para_vec_get_cstring(param, 0, &cmd_line);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "run shell %s all where = %s\n", (char *)cstring_get_str(cmd_line), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_run_shell_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_run_shell_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0149);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_run_shell, ERR_MODULE_ID, cmd_line, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;
        char *str;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);
        str = (char *)cstring_get_str(LOG_CSTR(log));

        sys_log(des_log, "[rank_%s_%ld] %s\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(cmd_line),
                          NULL_PTR == str ? "(null)\n" : str);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0150);
    mod_mgr_free(mod_mgr);

    //cstring_free(cmd_line);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_ping_taskcomm(CMD_PARA_VEC * param)
{
    UINT32  tcid;
    CSTRING *where;

    TASK_MGR *task_mgr;

    MOD_NODE send_mod_node;
    MOD_NODE recv_mod_node;

    LOG   *des_log;
    EC_BOOL ret;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    des_log = api_cmd_ui_get_log(where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show taskcomm tcid %s where = %s\n", c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    ret = EC_FALSE; /*initialization*/

    sys_log(des_log, "ping tcid %s ....\n", c_word_to_ipv4(tcid));

    task_mgr = task_new(NULL_PTR, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);

    MOD_NODE_TCID(&send_mod_node) = CMPI_LOCAL_TCID;
    MOD_NODE_COMM(&send_mod_node) = CMPI_LOCAL_COMM;
    MOD_NODE_RANK(&send_mod_node) = CMPI_LOCAL_RANK;
    MOD_NODE_MODI(&send_mod_node) = 0;
    MOD_NODE_HOPS(&send_mod_node) = 0;
    MOD_NODE_LOAD(&send_mod_node) = 0;

    MOD_NODE_TCID(&recv_mod_node) = tcid;
    MOD_NODE_COMM(&recv_mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&recv_mod_node) = CMPI_FWD_RANK;
    MOD_NODE_MODI(&recv_mod_node) = 0;
    MOD_NODE_HOPS(&recv_mod_node) = 0;
    MOD_NODE_LOAD(&recv_mod_node) = 0;

    task_super_inc(task_mgr, &send_mod_node, &recv_mod_node, &ret, FI_super_ping_taskcomm, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "tcid %s is reachable\n", c_word_to_ipv4(tcid));
    }
    else
    {
        sys_log(des_log, "tcid %s is unreachable\n", c_word_to_ipv4(tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_taskcfgchk_net(CMD_PARA_VEC * param)
{
    xmlDocPtr  task_cfg_doc;
    xmlNodePtr task_cfg_root;

    UINT32 tcid;
    CSTRING *where;

    LOG *des_log;

    SYS_CFG *sys_cfg;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    des_log = api_cmd_ui_get_log(where);

    /*parse config xml*/
    task_cfg_doc  = cxml_new((UINT8 *)task_brd_default_sys_cfg_xml());
    task_cfg_root = cxml_get_root(task_cfg_doc);

    sys_cfg = sys_cfg_new();
    //cxml_parse_task_cfg(task_cfg_root, task_cfg);
    cxml_parse_sys_cfg(task_cfg_root, sys_cfg);
    cxml_free(task_cfg_doc);

    //task_cfg_check_all(task_cfg);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "api_cmd_ui_taskcfgchk_net: %s: \n", (char *)task_brd_default_sys_cfg_xml());
    sys_cfg_print_xml(LOGSTDOUT, sys_cfg, 0);

    taskcfgchk_net_print(des_log, sys_cfg_get_task_cfg(sys_cfg), tcid, CMPI_ANY_MASK, CMPI_ANY_MASK);

    sys_cfg_free(sys_cfg);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_taskcfgchk_net_all(CMD_PARA_VEC * param)
{
    xmlDocPtr  task_cfg_doc;
    xmlNodePtr task_cfg_root;

    CSTRING *where;

    LOG *des_log;

    SYS_CFG *sys_cfg;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    des_log = api_cmd_ui_get_log(where);

    /*parse config xml*/
    task_cfg_doc  = cxml_new((UINT8 *)task_brd_default_sys_cfg_xml());
    task_cfg_root = cxml_get_root(task_cfg_doc);

    sys_cfg = sys_cfg_new();
    //cxml_parse_task_cfg(task_cfg_root, task_cfg);
    cxml_parse_sys_cfg(task_cfg_root, sys_cfg);
    cxml_free(task_cfg_doc);

    taskcfgchk_net_all(des_log, sys_cfg_get_task_cfg(sys_cfg));

    sys_cfg_free(sys_cfg);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_taskcfgchk_route(CMD_PARA_VEC * param)
{
    xmlDocPtr  task_cfg_doc;
    xmlNodePtr task_cfg_root;

    UINT32 tcid;
    CSTRING *where;

    LOG *des_log;

    SYS_CFG *sys_cfg;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    des_log = api_cmd_ui_get_log(where);

    /*parse config xml*/
    task_cfg_doc  = cxml_new((UINT8 *)task_brd_default_sys_cfg_xml());
    task_cfg_root = cxml_get_root(task_cfg_doc);

    sys_cfg = sys_cfg_new();
    //cxml_parse_task_cfg(task_cfg_root, task_cfg);
    cxml_parse_sys_cfg(task_cfg_root, sys_cfg);
    cxml_free(task_cfg_doc);

    taskcfgchk_route_print(des_log, sys_cfg_get_task_cfg(sys_cfg), tcid, CMPI_ANY_MASK, CMPI_ANY_MASK);

    sys_cfg_free(sys_cfg);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_taskcfgchk_route_trace(CMD_PARA_VEC * param)
{
    xmlDocPtr  task_cfg_doc;
    xmlNodePtr task_cfg_root;

    UINT32 src_tcid;
    UINT32 des_tcid;
    UINT32 max_hops;
    CSTRING *where;

    LOG *des_log;

    SYS_CFG *sys_cfg;

    api_cmd_para_vec_get_tcid(param, 0, &src_tcid);
    api_cmd_para_vec_get_tcid(param, 1, &des_tcid);
    api_cmd_para_vec_get_uint32(param, 2, &max_hops);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    des_log = api_cmd_ui_get_log(where);

    /*parse config xml*/
    task_cfg_doc  = cxml_new((UINT8 *)task_brd_default_sys_cfg_xml());
    task_cfg_root = cxml_get_root(task_cfg_doc);

    sys_cfg = sys_cfg_new();
    //cxml_parse_task_cfg(task_cfg_root, task_cfg);
    cxml_parse_sys_cfg(task_cfg_root, sys_cfg);
    cxml_free(task_cfg_doc);

    taskcfgchk_route_trace(des_log, sys_cfg_get_task_cfg(sys_cfg), src_tcid, CMPI_ANY_MASK, CMPI_ANY_MASK, des_tcid, max_hops);

    sys_cfg_free(sys_cfg);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_sync_taskcomm_from_local(CMD_PARA_VEC * param)
{
    UINT32 hops;
    UINT32 remotes;
    UINT32 time_to_live;
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    CVECTOR *report_vec;
    LOG *des_log;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_uint32(param , 0, &hops);
    api_cmd_para_vec_get_uint32(param , 1, &remotes);
    api_cmd_para_vec_get_uint32(param , 2, &time_to_live);
    api_cmd_para_vec_get_tcid(param   , 3, &tcid);
    api_cmd_para_vec_get_cstring(param, 4, &where);


    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "sync taskcomm hops %ld remotes %ld ttl %ld on tcid %s at %s\n",
                        hops, remotes, time_to_live,
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_taskcomm_from_local beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_taskcomm_from_local end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_CVECTOR, LOC_API_0151);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        CVECTOR *mod_node_vec;

        mod_node_vec = cvector_new(0, MM_MOD_NODE, LOC_API_0152);
        cvector_push(report_vec, (void *)mod_node_vec);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_sync_taskcomm_from_local, ERR_MODULE_ID, hops, remotes, time_to_live, mod_node_vec);
    }
    task_wait(task_mgr, time_to_live, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        CVECTOR *mod_node_vec;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        mod_node_vec = (CVECTOR *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        cvector_print(des_log, mod_node_vec, (CVECTOR_DATA_PRINT)mod_node_print);

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cvector_clean(mod_node_vec, (CVECTOR_DATA_CLEANER)mod_node_free, LOC_API_0153);
        cvector_free(mod_node_vec, LOC_API_0154);
    }

    cvector_free(report_vec, LOC_API_0155);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_sync_rank_load(CMD_PARA_VEC * param)
{
    UINT32 fr_tcid;
    UINT32 fr_rank;
    UINT32 to_tcid;
    UINT32 to_rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param   , 0, &fr_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &fr_rank);
    api_cmd_para_vec_get_tcid(param   , 2, &to_tcid);
    api_cmd_para_vec_get_uint32(param , 3, &to_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "sync rank load from tcid %s rank %ld to tcid %s rank %ld\n",
                        c_word_to_ipv4(fr_tcid), fr_rank,
                        c_word_to_ipv4(to_tcid), to_rank
                        );

    mod_mgr = api_cmd_ui_gen_mod_mgr(to_tcid, to_rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_rank_load beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_rank_load end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_sync_rank_load, ERR_MODULE_ID, fr_tcid, fr_rank);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_sync_rank_load_to_all(CMD_PARA_VEC * param)
{
    UINT32 fr_tcid;
    UINT32 fr_rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param   , 0, &fr_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &fr_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "sync rank load from tcid %s rank %ld to all\n",
                        c_word_to_ipv4(fr_tcid), fr_rank
                        );

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_rank_load_to_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_sync_rank_load_to_all end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_sync_rank_load, ERR_MODULE_ID, fr_tcid, fr_rank);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_set_rank_load(CMD_PARA_VEC * param)
{
    UINT32 of_tcid;
    UINT32 of_rank;
    UINT32 on_tcid;
    UINT32 on_rank;

    UINT32 que_load;
    UINT32 obj_load;
    UINT32 cpu_load;
    UINT32 mem_load;
    UINT32 dsk_load;
    UINT32 net_load;

    CLOAD_STAT cload_stat;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param   , 0, &of_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &of_rank);
    api_cmd_para_vec_get_uint32(param , 2, &que_load);
    api_cmd_para_vec_get_uint32(param , 3, &obj_load);
    api_cmd_para_vec_get_uint32(param , 4, &cpu_load);
    api_cmd_para_vec_get_uint32(param , 5, &mem_load);
    api_cmd_para_vec_get_uint32(param , 6, &dsk_load);
    api_cmd_para_vec_get_uint32(param , 7, &net_load);
    api_cmd_para_vec_get_tcid(param   , 8, &on_tcid);
    api_cmd_para_vec_get_uint32(param , 9, &on_rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set rank load of tcid %s rank %ld as load que %ld obj %ld cpu %ld mem %ld dsk %ld net %ld on tcid %s rank %ld\n",
                        c_word_to_ipv4(of_tcid), of_rank,
                        que_load, obj_load, cpu_load, mem_load, dsk_load, net_load,
                        c_word_to_ipv4(on_tcid), on_rank
                        );

    CLOAD_STAT_QUE_LOAD(&cload_stat) = (UINT16)que_load;
    CLOAD_STAT_OBJ_LOAD(&cload_stat) = (UINT16)obj_load;
    CLOAD_STAT_CPU_LOAD(&cload_stat) = (UINT8 )cpu_load;
    CLOAD_STAT_MEM_LOAD(&cload_stat) = (UINT8 )mem_load;
    CLOAD_STAT_DSK_LOAD(&cload_stat) = (UINT8 )dsk_load;
    CLOAD_STAT_NET_LOAD(&cload_stat) = (UINT8 )net_load;

    mod_mgr = api_cmd_ui_gen_mod_mgr(on_tcid, on_rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_rank_load beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_rank_load end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_set_rank_load, ERR_MODULE_ID, of_tcid, of_rank, &cload_stat);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_set_rank_load_on_all(CMD_PARA_VEC * param)
{
    UINT32 of_tcid;
    UINT32 of_rank;

    UINT32 que_load;
    UINT32 obj_load;
    UINT32 cpu_load;
    UINT32 mem_load;
    UINT32 dsk_load;
    UINT32 net_load;

    CLOAD_STAT cload_stat;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param   , 0, &of_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &of_rank);
    api_cmd_para_vec_get_uint32(param , 2, &que_load);
    api_cmd_para_vec_get_uint32(param , 3, &obj_load);
    api_cmd_para_vec_get_uint32(param , 4, &cpu_load);
    api_cmd_para_vec_get_uint32(param , 5, &mem_load);
    api_cmd_para_vec_get_uint32(param , 6, &dsk_load);
    api_cmd_para_vec_get_uint32(param , 7, &net_load);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "set rank load of tcid %s rank %ld as load que %ld obj %ld cpu %ld mem %ld dsk %ld net %ld on all\n",
                        c_word_to_ipv4(of_tcid), of_rank,
                        que_load, obj_load, cpu_load, mem_load, dsk_load, net_load
                        );

    CLOAD_STAT_QUE_LOAD(&cload_stat) = (UINT16)que_load;
    CLOAD_STAT_OBJ_LOAD(&cload_stat) = (UINT16)obj_load;
    CLOAD_STAT_CPU_LOAD(&cload_stat) = (UINT8 )cpu_load;
    CLOAD_STAT_MEM_LOAD(&cload_stat) = (UINT8 )mem_load;
    CLOAD_STAT_DSK_LOAD(&cload_stat) = (UINT8 )dsk_load;
    CLOAD_STAT_NET_LOAD(&cload_stat) = (UINT8 )net_load;

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_rank_load_on_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_set_rank_load_on_all end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_set_rank_load, ERR_MODULE_ID, of_tcid, of_rank, &cload_stat);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

#if 1
EC_BOOL api_cmd_ui_cdfs_create_npp(CMD_PARA_VEC * param)
{
    CSTRING *cdfsnp_db_root_dir;
    CSTRING *cdfsnp_mode;
    CSTRING *where;
    UINT32   cdfsnp_disk_max_num;
    UINT32   cdfsnp_np_max_num;
    UINT32   cdfsnpp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &cdfsnp_db_root_dir);
    api_cmd_para_vec_get_cstring(param , 1, &cdfsnp_mode);
    api_cmd_para_vec_get_uint32(param  , 2, &cdfsnp_disk_max_num);
    api_cmd_para_vec_get_uint32(param  , 3, &cdfsnp_np_max_num);
    api_cmd_para_vec_get_tcid(param    , 4, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs create npp %s mode %s max %ld disk max %ld np on tcid %s at %s\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir),
                        (char *)cstring_get_str(cdfsnp_mode),
                        cdfsnp_disk_max_num,
                        cdfsnp_np_max_num,
                        c_word_to_ipv4(cdfsnpp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_create_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_create_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);

    /*NOTE: deploy default hash algorithms*/
    task_tcid_inc(task_mgr, cdfsnpp_tcid,
                  &ret, FI_cdfs_create_npp, ERR_MODULE_ID, api_cmd_ui_get_cdfsnp_mode(cdfsnp_mode), cdfsnp_disk_max_num, cdfsnp_np_max_num, CHASH_AP_ALGO_ID, CHASH_SDBM_ALGO_ID, cdfsnp_db_root_dir);
    task_wait(task_mgr, TASK_ALWAYS_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsdfs create npp %s mode %s max %ld disk max %ld np on tcid %s successfully\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir),
                        (char *)cstring_get_str(cdfsnp_mode),
                        cdfsnp_disk_max_num,
                        cdfsnp_np_max_num,
                        c_word_to_ipv4(cdfsnpp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsdfs create npp %s mode %s max %ld disk max %ld np on tcid %s failed\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir),
                        (char *)cstring_get_str(cdfsnp_mode),
                        cdfsnp_disk_max_num,
                        cdfsnp_np_max_num,
                        c_word_to_ipv4(cdfsnpp_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_open_npp(CMD_PARA_VEC * param)
{
    CSTRING *cdfsnp_db_root_dir;
    CSTRING *where;
    UINT32   cdfsnpp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &cdfsnp_db_root_dir);
    api_cmd_para_vec_get_tcid(param    , 1, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs open npp %s on tcid %s at %s\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir),
                        c_word_to_ipv4(cdfsnpp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_open_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_open_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid, &ret, FI_cdfs_open_npp, ERR_MODULE_ID, cdfsnp_db_root_dir);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] open npp %s on tcid %s successfully\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir), c_word_to_ipv4(cdfsnpp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] open npp %s on tcid %s failed\n",
                        (char *)cstring_get_str(cdfsnp_db_root_dir), c_word_to_ipv4(cdfsnpp_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_npp(CMD_PARA_VEC * param)
{
    CSTRING *where;
    UINT32   cdfsnpp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close npp on tcid %s at %s\n",
                        c_word_to_ipv4(cdfsnpp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid, &ret, FI_cdfs_close_npp, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] close npp on tcid %s successfully\n", c_word_to_ipv4(cdfsnpp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] close npp on tcid %s failed\n", c_word_to_ipv4(cdfsnpp_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_with_flush_npp(CMD_PARA_VEC * param)
{
    CSTRING *where;
    UINT32   cdfsnpp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close and flush npp on tcid %s at %s\n",
                        c_word_to_ipv4(cdfsnpp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid, &ret, FI_cdfs_close_with_flush_npp, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] close and flush npp on tcid %s successfully\n",
                        c_word_to_ipv4(cdfsnpp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] close and flush npp on tcid %s failed\n",
                        c_word_to_ipv4(cdfsnpp_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_npp_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close npp on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_npp_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_npp_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0156);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0157);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_close_npp, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0158);
    }

    cvector_free_no_lock(report_vec, LOC_API_0159);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_with_flush_npp_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close and flush npp on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_npp_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_npp_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0160);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0161);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_close_with_flush_npp, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0162);
    }

    cvector_free_no_lock(report_vec, LOC_API_0163);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_create_dn(CMD_PARA_VEC * param)
{
    CSTRING *root_dir;
    CSTRING *where;
    UINT32   cdfsdn_tcid;
    UINT32   max_gb_num_of_disk_space;
    UINT32   disk_num;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &root_dir);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_num);
    api_cmd_para_vec_get_uint32(param  , 2, &max_gb_num_of_disk_space);
    api_cmd_para_vec_get_tcid(param    , 3, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs create dn %s with %ld disk %ld GB on tcid %s at %s\n",
                        (char *)cstring_get_str(root_dir),
                        disk_num,
                        max_gb_num_of_disk_space,
                        c_word_to_ipv4(cdfsdn_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsdn_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_create_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_create_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);

    /*NOTE: deploy default hash algorithms*/
    task_tcid_inc(task_mgr, cdfsdn_tcid,
                  &ret, FI_cdfs_create_dn, ERR_MODULE_ID, root_dir, disk_num, max_gb_num_of_disk_space);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsdfs create dn %s with %ld disk %ld GB on tcid %s successfully\n",
                        (char *)cstring_get_str(root_dir),
                        disk_num,
                        max_gb_num_of_disk_space,
                        c_word_to_ipv4(cdfsdn_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] create dn %s with %ld disk %ld GB on tcid %s failed\n",
                        (char *)cstring_get_str(root_dir),
                        disk_num,
                        max_gb_num_of_disk_space,
                        c_word_to_ipv4(cdfsdn_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_open_dn(CMD_PARA_VEC * param)
{
    CSTRING *root_dir;
    CSTRING *where;
    UINT32   cdfsdn_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &root_dir);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs open dn %s on tcid %s at %s\n",
                        (char *)cstring_get_str(root_dir),
                        c_word_to_ipv4(cdfsdn_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsdn_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_open_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_open_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsdn_tcid, &ret, FI_cdfs_open_dn, ERR_MODULE_ID, root_dir);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] open dn %s on tcid %s successfully\n",
                        (char *)cstring_get_str(root_dir), c_word_to_ipv4(cdfsdn_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] open dn %s on tcid %s failed\n",
                         (char *)cstring_get_str(root_dir), c_word_to_ipv4(cdfsdn_tcid));
    }
    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_dn(CMD_PARA_VEC * param)
{
    CSTRING *where;
    UINT32   cdfsdn_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close dn on tcid %s at %s\n",
                        c_word_to_ipv4(cdfsdn_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsdn_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsdn_tcid, &ret, FI_cdfs_close_dn, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] close dn on tcid %s successfully\n",
                        c_word_to_ipv4(cdfsdn_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] close dn on tcid %s failed\n",
                        c_word_to_ipv4(cdfsdn_tcid));
    }

    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_with_flush_dn(CMD_PARA_VEC * param)
{
    CSTRING *where;
    UINT32   cdfsdn_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close and flush dn on tcid %s at %s\n",
                        c_word_to_ipv4(cdfsdn_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsdn_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsdn_tcid, &ret, FI_cdfs_close_with_flush_dn, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] close and flush dn on tcid %s successfully\n",
                        c_word_to_ipv4(cdfsdn_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] close and flush dn on tcid %s failed\n",
                        c_word_to_ipv4(cdfsdn_tcid));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_dn_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close dn on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_dn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_dn_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0164);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0165);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_close_dn, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0166);
    }

    cvector_free(report_vec, LOC_API_0167);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_close_with_flush_dn_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs close and flush dn on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_dn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_close_with_flush_dn_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0168);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0169);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_close_with_flush_dn, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0170);
    }

    cvector_free(report_vec, LOC_API_0171);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_read(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs read %s from npp %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_read beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_read end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    cbytes = cbytes_new(0);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_read, ERR_MODULE_ID, file_name, cbytes);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] read result: %.*s\n", cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cbytes_free(cbytes);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_write(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    CSTRING *where;
    UINT32   cdfsnp_tcid;
    UINT32   replica_num;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_uint32(param  , 2, &replica_num);
    api_cmd_para_vec_get_tcid(param  , 3, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs write %s with content %s and replicas %ld to npp %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        replica_num,
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_write beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_write end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_write, ERR_MODULE_ID, file_name, cbytes, replica_num);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]file name %s, replica num %ld\n", (char *)cstring_get_str(file_name), replica_num);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_trunc(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    UINT32   file_size;
    CSTRING *where;
    UINT32   cdfsnp_tcid;
    UINT32   replica_num;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_uint32(param  , 1, &file_size);
    api_cmd_para_vec_get_uint32(param  , 2, &replica_num);
    api_cmd_para_vec_get_tcid(param    , 3, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs trunc %s with size %ld and replicas %ld to npp %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        file_size,
                        replica_num,
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_trunc beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_trunc end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_truncate, ERR_MODULE_ID, file_name, file_size, replica_num);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]file name %s, file size %ld, replica num %ld\n",
                        (char *)cstring_get_str(file_name), file_size, replica_num);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_update(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_tcid(param    , 2, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs update %s with content %s to npp %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_update beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_update end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_update, ERR_MODULE_ID, file_name, cbytes);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_mkdir(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param    , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs mkdir %s on npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_mkdir beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_mkdir end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_mkdir, ERR_MODULE_ID, path);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] mkdir %s\n", (char *)cstring_get_str(path));
    }
    else
    {
        sys_log(des_log, "[FAIL] mkdir %s\n", (char *)cstring_get_str(path));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_mkdir_all(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs mkdir %s on all at %s\n",
                    (char *)cstring_get_str(path),
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_mkdir_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_mkdir_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0172);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0173);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_mkdir, ERR_MODULE_ID, path);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0174);
    }

    cvector_free(report_vec, LOC_API_0175);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qfile(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CDFSNP_ITEM *cdfsnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qfile %s from npp %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qfile beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qfile end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    cdfsnp_item = cdfsnp_item_new();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qfile, ERR_MODULE_ID, file_name, cdfsnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        cdfsnp_item_print(des_log, cdfsnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cdfsnp_item_free(cdfsnp_item);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qdir(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *cdfsnp_item_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qdir %s from npp %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qdir beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qdir end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    cdfsnp_item_vec = cvector_new(0, MM_CDFSNP_ITEM, LOC_API_0176);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qdir, ERR_MODULE_ID, dir_name, cdfsnp_item_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
        cvector_print(des_log, cdfsnp_item_vec, (CVECTOR_DATA_PRINT)cdfsnp_item_print);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
        cvector_print(des_log, cdfsnp_item_vec, (CVECTOR_DATA_PRINT)cdfsnp_item_print);
    }

    cvector_clean(cdfsnp_item_vec, (CVECTOR_DATA_CLEANER)cdfsnp_item_free, LOC_API_0177);
    cvector_free(cdfsnp_item_vec, LOC_API_0178);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qlist_path(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qlist %s full from npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qlist_path beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qlist_path end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0179);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qlist_path, ERR_MODULE_ID, path, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0180);
    cvector_free(path_cstr_vec, LOC_API_0181);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qlist_seg(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *seg_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qlist %s short from npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qlist_seg beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qlist_seg end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    seg_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0182);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qlist_seg, ERR_MODULE_ID, path, seg_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }

    cvector_clean(seg_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0183);
    cvector_free(seg_cstr_vec, LOC_API_0184);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qcount_files(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qcount %s files from npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qcount_files beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qcount_files end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0185);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qlist_path, ERR_MODULE_ID, path, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 total_file_num;
        UINT32 pos;

        total_file_num = 0;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;
            UINT32   file_num;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
            task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_file_num, ERR_MODULE_ID, path_cstr, &file_num);
            task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

            sys_log(des_log, "%ld # %s, file num %ld\n", pos, (char *)cstring_get_str(path_cstr), file_num);

            total_file_num += file_num;
        }
        sys_log(des_log, "total file num %ld\n", total_file_num);
    }
    else
    {
        UINT32 total_file_num;
        UINT32 pos;

        total_file_num = 0;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;
            UINT32   file_num;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
            task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_file_num, ERR_MODULE_ID, path_cstr, &file_num);
            task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

            sys_log(des_log, "%ld # %s, file num %ld\n", pos, (char *)cstring_get_str(path_cstr), file_num);

            total_file_num += file_num;
        }
        sys_log(des_log, "total file num %ld\n", total_file_num);
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0186);
    cvector_free(path_cstr_vec, LOC_API_0187);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qsize_files(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qsize %s files from npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qsize_files beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qsize_files end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0188);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_qlist_path, ERR_MODULE_ID, path, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 total_file_size;
        UINT32 pos;

        total_file_size = 0;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;
            UINT32   file_size;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
            task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_file_size, ERR_MODULE_ID, path_cstr, &file_size);
            task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

            sys_log(des_log, "%ld # %s, file size %ld\n", pos, (char *)cstring_get_str(path_cstr), file_size);

            total_file_size += file_size;
        }
        sys_log(des_log, "total file size %ld\n", total_file_size);
    }
    else
    {
        UINT32 total_file_size;
        UINT32 pos;

        total_file_size = 0;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;
            UINT32   file_size;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
            task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_file_size, ERR_MODULE_ID, path_cstr, &file_size);
            task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

            sys_log(des_log, "%ld # %s, file size %ld\n", pos, (char *)cstring_get_str(path_cstr), file_size);

            total_file_size += file_size;
        }
        sys_log(des_log, "total file size %ld\n", total_file_size);
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0189);
    cvector_free(path_cstr_vec, LOC_API_0190);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qsize_one_file(CMD_PARA_VEC * param)
{
    CSTRING *path;
    CSTRING *where;
    UINT32   cdfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;
    UINT32    file_size;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param  , 1, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qsize %s file from npp %s at %s\n",
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qsize_one_file beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qsize_one_file end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_file_size, ERR_MODULE_ID, path, &file_size);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file %s, get file size %ld\n", (char *)cstring_get_str(path), file_size);
    }
    else
    {
        sys_log(des_log, "[FAIL] file %s\n", (char *)cstring_get_str(path));
    }
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_qblock(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    UINT32   cdfsnp_tcid;
    UINT32   path_layout;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CVECTOR  *report_vec;
    LOG      *des_log;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    EC_BOOL ret;

    api_cmd_para_vec_get_tcid(param   , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_uint32(param , 1, &path_layout);
    api_cmd_para_vec_get_tcid(param   , 2, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs qblock tcid %s pathlayout %ld from npp %s at %s\n",
                        c_word_to_ipv4(cdfsdn_tcid), path_layout, c_word_to_ipv4(cdfsnp_tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(cdfsnp_tcid, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qblock beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_qblock end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0191);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_figure_out_block, ERR_MODULE_ID, cdfsdn_tcid, path_layout, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0192);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_flush_npp(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs flush npp tcid %s at %s\n", c_word_to_ipv4(cdfsnpp_tcid), (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid, &ret, FI_cdfs_flush_npp, ERR_MODULE_ID, cdfsnpp_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_flush_dn(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs flush dn tcid %s at %s\n", c_word_to_ipv4(cdfsdn_tcid), (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsdn_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsdn_tcid, &ret, FI_cdfs_flush_dn, ERR_MODULE_ID, cdfsdn_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_flush_npp_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs flush npp all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_npp_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_npp_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0193);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0194);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_flush_npp, ERR_MODULE_ID, MOD_NODE_TCID(mod_node));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0195);
    }

    cvector_free(report_vec, LOC_API_0196);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_flush_dn_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs flush dn all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_dn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_flush_dn_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0197);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0198);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_flush_dn, ERR_MODULE_ID, MOD_NODE_TCID(mod_node));
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0199);
    }

    cvector_free(report_vec, LOC_API_0200);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_add_npp(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid;
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_tcid(param    , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs add npp %s to tcid %s at %s\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_add_npp, ERR_MODULE_ID, cdfsnpp_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "api_cmd_ui_cdfs_add_npp: hsdfs add np %s to tcid %s successfully\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }
    else
    {
        sys_log(des_log, "api_cmd_ui_cdfs_add_npp: hsdfs add np %s to tcid %s failed\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_add_npp_to_all(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_tcid(param    , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs add npp %s to all at %s\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_npp_to_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_npp_to_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0201);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32  *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0202);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_add_npp, ERR_MODULE_ID, cdfsnpp_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0203);
    }

    cvector_free(report_vec, LOC_API_0204);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_add_dn(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_tcid(param  , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs add dn %s to tcid %s at %s\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_add_dn, ERR_MODULE_ID, cdfsdn_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsdfs add dn %s to tcid %s successfully\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }
    else
    {
        sys_log(des_log, "[FAIL] hsdfs add dn %s to tcid %s failed\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_add_dn_to_all(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs add dn %s to all at %s\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_dn_to_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_add_dn_to_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0205);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32  *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0206);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_add_dn, ERR_MODULE_ID, cdfsdn_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0207);
    }

    cvector_free(report_vec, LOC_API_0208);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_reg_npp(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid;
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param   , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_tcid(param   , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs reg npp %s to tcid %s at %s\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_reg_npp, ERR_MODULE_ID, cdfsnpp_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "api_cmd_ui_cdfs_reg_npp: hsdfs reg np %s to tcid %s successfully\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }
    else
    {
        sys_log(des_log, "api_cmd_ui_cdfs_reg_npp: hsdfs reg np %s to tcid %s failed\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_reg_npp_to_all(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsnpp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs reg npp %s to all at %s\n",
                    c_word_to_ipv4(cdfsnpp_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_npp_to_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_npp_to_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0209);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32  *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0210);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_reg_npp, ERR_MODULE_ID, cdfsnpp_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0211);
    }

    cvector_free(report_vec, LOC_API_0212);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_reg_dn(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_tcid(param  , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs reg dn %s to tcid %s at %s\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_reg_dn, ERR_MODULE_ID, cdfsdn_tcid);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsdfs reg dn %s to tcid %s successfully\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }
    else
    {
        sys_log(des_log, "[FAIL] hsdfs reg dn %s to tcid %s failed\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    c_word_to_ipv4(des_tcid)
                    );
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_reg_dn_to_all(CMD_PARA_VEC * param)
{
    UINT32   cdfsdn_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;

    api_cmd_para_vec_get_tcid(param  , 0, &cdfsdn_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs reg dn %s to all at %s\n",
                    c_word_to_ipv4(cdfsdn_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_dn_to_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_reg_dn_to_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0213);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32  *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0214);
        cvector_push(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_reg_dn, ERR_MODULE_ID, cdfsdn_tcid);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32  *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0215);
    }

    cvector_free(report_vec, LOC_API_0216);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_list_npp(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs list npp on tcid %s at %s\n",
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_list_npp, ERR_MODULE_ID, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_list_dn(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs list dn on tcid %s at %s\n",
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_list_dn, ERR_MODULE_ID, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_list_npp_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs list npp on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_npp_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_npp_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0217);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_list_npp, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        LOG *log;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        sys_print(des_log, "%s\n", (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0218);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_list_dn_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs list dn on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_dn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_list_dn_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0219);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_list_dn, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        LOG *log;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        sys_print(des_log, "%s\n", (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0220);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_delete_path_npp(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;
    CSTRING *path;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param    , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs del path %s from npp %s at %s\n",
                    (char *)cstring_get_str(path),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_path_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_path_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_delete, ERR_MODULE_ID, path, CDFSNP_ITEM_FILE_IS_ANY);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC] delete path %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL] delete path %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_delete_file_npp(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;
    CSTRING *path;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param    , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs del file %s from npp %s at %s\n",
                    (char *)cstring_get_str(path),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_file_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_file_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_delete, ERR_MODULE_ID, path, CDFSNP_ITEM_FILE_IS_REG);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC] delete file %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL] delete file %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_delete_dir_npp(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;
    CSTRING *path;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path);
    api_cmd_para_vec_get_tcid(param    , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs del dir %s from npp %s at %s\n",
                    (char *)cstring_get_str(path),
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_dir_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_delete_dir_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_delete, ERR_MODULE_ID, path, CDFSNP_ITEM_FILE_IS_DIR);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC] delete file %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL] delete file %s\n", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(path));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_transfer_npp(CMD_PARA_VEC * param)
{
    UINT32   transfer_max_gb;
    UINT32   src_dn_tcid;
    UINT32   des_dn_tcid;
    UINT32   on_np_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &transfer_max_gb);
    api_cmd_para_vec_get_tcid(param    , 1, &src_dn_tcid);
    api_cmd_para_vec_get_tcid(param    , 2, &des_dn_tcid);
    api_cmd_para_vec_get_tcid(param    , 3, &on_np_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs transfer %ld GB from dn %s to dn %s on npp %s at %s\n",
                    transfer_max_gb,
                    c_word_to_ipv4(src_dn_tcid),
                    c_word_to_ipv4(des_dn_tcid),
                    c_word_to_ipv4(on_np_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(on_np_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_transfer_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_transfer_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, on_np_tcid, &ret, FI_cdfs_transfer, ERR_MODULE_ID, src_dn_tcid, des_dn_tcid, transfer_max_gb);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC] transfer %ld GB from dn %s to dn %s\n",
                         c_word_to_ipv4(on_np_tcid), transfer_max_gb, c_word_to_ipv4(src_dn_tcid), c_word_to_ipv4(des_dn_tcid));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL] transfer %ld GB from dn %s to dn %s\n",
                         c_word_to_ipv4(on_np_tcid), transfer_max_gb, c_word_to_ipv4(src_dn_tcid), c_word_to_ipv4(des_dn_tcid));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_make_snapshot(CMD_PARA_VEC * param)
{
    UINT32   on_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &on_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs make snapshot on tcid %s at %s\n",
                    c_word_to_ipv4(on_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(on_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_make_snapshot beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_make_snapshot end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, on_tcid, &ret, FI_cdfs_snapshot, ERR_MODULE_ID);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n", c_word_to_ipv4(on_tcid));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n", c_word_to_ipv4(on_tcid));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_make_snapshot_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs make snapshot on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_make_snapshot_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_make_snapshot_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0221);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        EC_BOOL *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0222);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cdfs_snapshot, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        EC_BOOL *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (EC_BOOL  *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node), MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node), MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0223);
    }

    cvector_free_no_lock(report_vec, LOC_API_0224);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_cdfs_show_npp(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show npp on tcid %s at %s\n",
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_npp beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_npp end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_show_npp, ERR_MODULE_ID, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_dn(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show dn on tcid %s at %s\n",
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_dn beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_dn end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_show_dn, ERR_MODULE_ID, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_npp_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show npp on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_npp_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_npp_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0225);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_show_npp, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        LOG *log;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        sys_print(des_log, "%s\n", (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0226);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_dn_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show dn on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_dn_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_dn_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0227);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_show_dn, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        LOG *log;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        sys_print(des_log, "%s\n", (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0228);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_block_path(CMD_PARA_VEC * param)
{
    UINT32   disk_num;
    UINT32   path_layout;
    CSTRING *where;

    LOG *des_log;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_num);
    api_cmd_para_vec_get_uint32(param  , 1, &path_layout);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show disk %ld block path layout %ld at %s\n",
                    disk_num,
                    path_layout,
                    (char *)cstring_get_str(where));

    des_log = api_cmd_ui_get_log(where);

    cdfsdn_block_fname_print(des_log, disk_num, path_layout);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_cached_np(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param  , 0, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show cached np on tcid %s at %s\n",
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_cached_np beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_cached_np end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_show_cached_np, ERR_MODULE_ID, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_show_cached_np_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    UINT32    remote_mod_node_num;
    UINT32    remote_mod_node_idx;

    CVECTOR * report_vec;

    api_cmd_para_vec_get_cstring(param , 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs show dn on all at %s\n",
                    (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_CDFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_cached_np_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_show_cached_np_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0229);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cdfs_show_cached_np, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        LOG *log;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG  *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        sys_print(des_log, "%s\n", (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0230);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_showup_np(CMD_PARA_VEC * param)
{
    UINT32   des_tcid;
    UINT32   cdfsnp_path_layout;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &cdfsnp_path_layout);
    api_cmd_para_vec_get_tcid(param    , 1, &des_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs dbg show np %ld on tcid %s at %s\n",
                    cdfsnp_path_layout,
                    c_word_to_ipv4(des_tcid),
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(des_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_showup_np beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_showup_np end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    log = log_cstr_open();

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, des_tcid, &ret, FI_cdfs_show_specific_np, ERR_MODULE_ID, cdfsnp_path_layout, log);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n%s", c_word_to_ipv4(des_tcid),(char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_import_fnode_log(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid_src;
    UINT32   cdfsnpp_tcid_des;
    UINT32   timeout_mins;
    CSTRING *file_path;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_path);
    api_cmd_para_vec_get_tcid(param    , 1, &cdfsnpp_tcid_src);
    api_cmd_para_vec_get_tcid(param    , 2, &cdfsnpp_tcid_des);
    api_cmd_para_vec_get_uint32(param  , 3, &timeout_mins);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs dbg import fnode file %s on npp %s to npp %s timeout %ld minutes at %s\n",
                    (char *)cstring_get_str(file_path),
                    c_word_to_ipv4(cdfsnpp_tcid_src),
                    c_word_to_ipv4(cdfsnpp_tcid_des),
                    timeout_mins,
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid_src, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_import_fnode_log beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_import_fnode_log end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid_src, &ret, FI_cdfs_import_lost_fnode_from_file, ERR_MODULE_ID, file_path, cdfsnpp_tcid_des);
    task_wait(task_mgr, /*TASK_DEFAULT_LIVE*/timeout_mins * 60, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n", c_word_to_ipv4(cdfsnpp_tcid_src));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n", c_word_to_ipv4(cdfsnpp_tcid_src));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cdfs_import_replica_log(CMD_PARA_VEC * param)
{
    UINT32   cdfsnpp_tcid_src;
    UINT32   cdfsdn_tcid_des;
    UINT32   timeout_mins;
    CSTRING *file_path;
    CSTRING *where;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_path);
    api_cmd_para_vec_get_tcid(param    , 1, &cdfsnpp_tcid_src);
    api_cmd_para_vec_get_tcid(param    , 2, &cdfsdn_tcid_des);
    api_cmd_para_vec_get_uint32(param  , 3, &timeout_mins);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs dbg import replica file %s on npp %s to dn %s timeout %ld minutes at %s\n",
                    (char *)cstring_get_str(file_path),
                    c_word_to_ipv4(cdfsnpp_tcid_src),
                    c_word_to_ipv4(cdfsdn_tcid_des),
                    timeout_mins,
                    (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnpp_tcid_src, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_import_replica_log beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_import_replica_log end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, cdfsnpp_tcid_src, &ret, FI_cdfs_import_lost_replica_from_file, ERR_MODULE_ID, file_path, cdfsdn_tcid_des);
    task_wait(task_mgr, /*TASK_DEFAULT_LIVE*/timeout_mins * 60, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_1][SUCC]\n", c_word_to_ipv4(cdfsnpp_tcid_src));
    }
    else
    {
        sys_log(des_log, "[rank_%s_1][FAIL]\n", c_word_to_ipv4(cdfsnpp_tcid_src));
    }

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}
#endif
EC_BOOL api_cmd_ui_cbgt_show_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt show module on tcid %s rank %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_show_module beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_show_module end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0231);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_print_module_status, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0232);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_show_module_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt show module on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_show_module_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_show_module_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0233);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_print_module_status, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0234);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_flush(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt flush on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ANY_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(tcid, CMPI_ANY_COMM, rank, modi, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_flush beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_flush end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0235);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0236);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        cvector_push(report_vec, (void *)ret);
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cbgt_flush, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld_%ld] SUCC\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),MOD_NODE_MODI(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld_%ld] FAILED\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),MOD_NODE_MODI(mod_node));
        }

        free_static_mem(MM_UINT32, ret, LOC_API_0237);
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
    }

    cvector_free(report_vec, LOC_API_0238);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_flush_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt flush on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_flush_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_flush_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0239);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0240);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_cbgt_flush, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),MOD_NODE_MODI(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),MOD_NODE_MODI(mod_node));
        }

        free_static_mem(MM_UINT32, ret, LOC_API_0241);
        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
    }

    cvector_free_no_lock(report_vec, LOC_API_0242);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_traversal_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt traversal on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_module beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_module end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0243);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_traversal, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0244);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_traversal_module_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt traversal on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_module_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_module_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0245);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_traversal, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0246);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_traversal_depth_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt traversal depth on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_depth_module beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_depth_module end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0247);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_traversal_depth, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0248);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_traversal_depth_module_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt traversal depth on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_depth_module_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_traversal_depth_module_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0249);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_traversal_depth, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0250);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_debug_merge(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt debug merge on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_debug_merge beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_debug_merge end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        EC_BOOL ret;
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cbgt_merge, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);
    sys_log(des_log, "done\n");

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_debug_split(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt debug split on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_debug_split beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_debug_split end ----------------------------------\n");
    }
#endif

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        EC_BOOL ret;
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_cbgt_split, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);
    sys_log(des_log, "done\n");

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_runthrough_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt runthrough on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_module beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_module end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0251);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_runthrough, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0252);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_runthrough_module_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt runthrough on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_module_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_module_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0253);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_runthrough, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0254);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_runthrough_depth_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt runthrough depth on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_depth_module beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_depth_module end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0255);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_runthrough_depth, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0256);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_runthrough_depth_module_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt runthrough depth on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_depth_module_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_runthrough_depth_module_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0257);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_runthrough_depth, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0258);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_print_status(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt status on tcid %s rank %ld module %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ANY_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(tcid, CMPI_ANY_COMM, rank, modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_print_status beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_print_status end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0259);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_print_status, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0260);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_print_status_all(CMD_PARA_VEC * param)
{
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt status on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ANY_MODI);
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    //mod_mgr_excl(CMPI_ANY_TCID, CMPI_ANY_COMM, CMPI_FWD_RANK, CMPI_ANY_MODI, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_print_status_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_print_status_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0261);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_cbgt_print_status, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0262);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_create_root_table(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    CSTRING *root_table_name;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR  *src_mod_mgr;
    MOD_MGR  *des_mod_mgr;

    LOG     *des_log;

    CBYTES   root_table_name_bytes;
    MOD_NODE error_mod_node;

    api_cmd_para_vec_get_cstring(param, 0, &root_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt create root table %s with path %s on tcid %s rank %ld at %s\n",
                        (char *)cstring_get_str(root_table_name),
                        (char *)cstring_get_str(root_path),
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));

    src_mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ERROR_MODI);
#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_create_root_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_create_root_table end ----------------------------------\n");
#endif

    des_mod_mgr = NULL_PTR;
    cbytes_init(&root_table_name_bytes);
 
    cbytes_mount(&root_table_name_bytes, cstring_get_len(root_table_name), cstring_get_str(root_table_name));
    MOD_NODE_TCID(&error_mod_node) = CMPI_ERROR_TCID;
    MOD_NODE_COMM(&error_mod_node) = CMPI_ERROR_COMM;
    MOD_NODE_RANK(&error_mod_node) = CMPI_ERROR_RANK;
    MOD_NODE_MODI(&error_mod_node) = ERR_MODULE_ID;
    task_act(src_mod_mgr, &des_mod_mgr, TASK_DEFAULT_LIVE, 1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_cbgt_start, CBGT_TYPE_ROOT_SERVER, CBGT_ROOT_TABLE_ID, &root_table_name_bytes, &error_mod_node/*parent*/, root_path, CBGT_O_RDWR | CBGT_O_CREAT);

    des_log = api_cmd_ui_get_log(where);
    mod_mgr_print(des_log, des_mod_mgr);
    mod_mgr_free(des_mod_mgr);

    return (EC_TRUE);
}

static EC_BOOL __api_cmd_ui_cbgt_query_root_table(const CSTRING *root_path, UINT32 *root_table_id, MOD_NODE *root_mod_node)
{
    UINT32   cbgt_md_id;

    cbgt_md_id = cbgt_start(CBGT_TYPE_USER_CLIENT, CBGT_ERR_TABLE_ID, NULL_PTR, NULL_PTR, root_path, CBGT_O_UNDEF);
    if(ERR_MODULE_ID == cbgt_md_id)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:__api_cmd_ui_cbgt_query_root_table: start cbgt client failed\n");
        return (EC_FALSE);
    }

    if(EC_FALSE == __cbgt_load_root_record_file(cbgt_md_id, root_path, root_table_id, root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:__api_cmd_ui_cbgt_query_root_table: load root record failed\n");
        cbgt_end(cbgt_md_id);
        return (EC_FALSE);
    }

    cbgt_end(cbgt_md_id);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_create_user_table(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *user_table_colfs;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;
    CVECTOR *colfs_vec;

    char   *fields[32];
    UINT32  field_num;
    UINT32  field_pos;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &user_table_colfs);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt create user table %s column family %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(user_table_colfs),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_create_user_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_create_user_table: root table not loaded\n");
        return (EC_TRUE);
    }

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_create_user_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_create_user_table end ----------------------------------\n");
#endif
    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(user_table_colfs), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    colfs_vec = cvector_new(0, MM_CBYTES, LOC_API_0263);
    if(NULL_PTR == colfs_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_create_user_table: new cvector failed\n");
        return (EC_TRUE);
    }

    for(field_pos = 0; field_pos < field_num; field_pos ++)
    {
        CBYTES *colf_bytes;
        char   *colf_name;
    
        colf_bytes = cbytes_new(0);
        if(NULL_PTR == colf_bytes)
        {
            dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_create_user_table: new cbytes failed\n");
            cvector_loop_front(colfs_vec, (CVECTOR_DATA_HANDLER)cbytes_umount_only);
            cvector_clean(colfs_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0264);
            cvector_free(colfs_vec, 0);
            return (EC_TRUE);
        }

        colf_name = fields[ field_pos ];
        cbytes_mount(colf_bytes, strlen(colf_name), (UINT8 *)colf_name);
        cvector_push(colfs_vec, colf_bytes);
    }

    ret = EC_FALSE;
    task_super_mono(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_create_table_on_root, ERR_MODULE_ID, &user_table_name_bytes, colfs_vec);
    mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cvector_loop_front(colfs_vec, (CVECTOR_DATA_HANDLER)cbytes_umount_only);
    cvector_clean(colfs_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0265);
    cvector_free(colfs_vec, 0);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_delete_user_table(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt delete user table %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete_user_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete_user_table: root table not loaded\n");
        return (EC_TRUE);
    }

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete_user_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete_user_table end ----------------------------------\n");
#endif

    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    ret = EC_FALSE;
    task_super_mono(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_delete_user_table, ERR_MODULE_ID, &user_table_name_bytes);
    mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_cbgt_delete_colf_table(CMD_PARA_VEC * param)
{
    CSTRING *table_colf;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   colf_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &table_colf);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt delete colf table %s from path %s at %s\n",
                        (char *)cstring_get_str(table_colf),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    field_num = c_str_split((char *)cstring_get_str(table_colf), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete_colf_table: invalid command line\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete_colf_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete_colf_table: root table not loaded\n");
        return (EC_TRUE);
    }

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete_colf_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete_colf_table end ----------------------------------\n");
#endif

    cbytes_init(&user_table_name_bytes);
    cbytes_init(&colf_name_bytes);
 
    cbytes_mount(&user_table_name_bytes, strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_name_bytes      , strlen(fields[1]), (UINT8 *)fields[1]);

    ret = EC_FALSE;
    task_super_mono(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_delete_colf_table, ERR_MODULE_ID, &user_table_name_bytes, &colf_name_bytes);
    mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_add_colf_table(CMD_PARA_VEC * param)
{
    CSTRING *table_colf;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   colf_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &table_colf);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt add colf table %s from path %s at %s\n",
                        (char *)cstring_get_str(table_colf),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    field_num = c_str_split((char *)cstring_get_str(table_colf), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_add_colf_table: invalid command line\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_add_colf_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_add_colf_table: root table not loaded\n");
        return (EC_TRUE);
    }

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_add_colf_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_add_colf_table end ----------------------------------\n");
#endif

    cbytes_init(&user_table_name_bytes);
    cbytes_init(&colf_name_bytes);

    cbytes_mount(&user_table_name_bytes, strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_name_bytes      , strlen(fields[1]), (UINT8 *)fields[1]);

    ret = EC_FALSE;
    task_super_mono(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_add_colf_table, ERR_MODULE_ID, &user_table_name_bytes, &colf_name_bytes);
    mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}


/*I am sorry, we cannot open root table when have no idea regarding cdfs_md_id*/
/*thus this interface is only working for posix/network file system :-(*/
EC_BOOL api_cmd_ui_cbgt_open_root_table(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    CSTRING *root_table_name;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR  *src_mod_mgr;
    MOD_MGR  *des_mod_mgr;

    LOG     *des_log;

    CBYTES   root_table_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;
    MOD_NODE error_mod_node;

    api_cmd_para_vec_get_cstring(param, 0, &root_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    des_log = api_cmd_ui_get_log(where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt open root table %s from path %s on tcid %s rank %ld at %s\n",
                        (char *)cstring_get_str(root_table_name),
                        (char *)cstring_get_str(root_path),
                        c_word_to_ipv4(tcid),
                        rank,
                        (char *)cstring_get_str(where));


    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_open_root_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_TRUE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_open_root_table: root table was open on (tcid %s, comm %ld, rank %ld modi %ld)\n",
                            MOD_NODE_TCID_STR(&root_mod_node),
                            MOD_NODE_COMM(&root_mod_node),
                            MOD_NODE_RANK(&root_mod_node),
                            MOD_NODE_MODI(&root_mod_node)
                            );
        sys_log(des_log, "warn: root table was already open on (tcid %s, comm %ld, rank %ld modi %ld)\n",
                            MOD_NODE_TCID_STR(&root_mod_node),
                            MOD_NODE_COMM(&root_mod_node),
                            MOD_NODE_RANK(&root_mod_node),
                            MOD_NODE_MODI(&root_mod_node)
                            );
        return (EC_TRUE);
    }

    src_mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, CMPI_ERROR_MODI);
#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_open_root_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_open_root_table end ----------------------------------\n");
#endif

    des_mod_mgr = NULL_PTR;
    cbytes_init(&root_table_name_bytes);
 
    cbytes_mount(&root_table_name_bytes, cstring_get_len(root_table_name), cstring_get_str(root_table_name));
    MOD_NODE_TCID(&error_mod_node) = CMPI_ERROR_TCID;
    MOD_NODE_COMM(&error_mod_node) = CMPI_ERROR_COMM;
    MOD_NODE_RANK(&error_mod_node) = CMPI_ERROR_RANK;
    MOD_NODE_MODI(&error_mod_node) = ERR_MODULE_ID;
    task_act(src_mod_mgr, &des_mod_mgr, TASK_DEFAULT_LIVE, 1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_cbgt_start, CBGT_TYPE_ROOT_SERVER, CBGT_ROOT_TABLE_ID, &root_table_name_bytes, &error_mod_node/*parent*/, root_path, CBGT_O_RDWR);


    mod_mgr_print(des_log, des_mod_mgr);
    mod_mgr_free(des_mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_open_colf_table(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *user_colf_name;
    CSTRING *root_path;
    CSTRING *where;

    MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   user_colf_name_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    UINT32   colf_table_id;
    MOD_NODE colf_mod_node;

    EC_BOOL  ret;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &user_colf_name);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt open user table %s colf %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(user_colf_name),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_open_colf_table: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_open_colf_table: root table not loaded\n");
        return (EC_TRUE);
    }

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_open_colf_table beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_open_colf_table end ----------------------------------\n");
#endif

    cbytes_init(&user_table_name_bytes);
    cbytes_init(&user_colf_name_bytes);

    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));
    cbytes_mount(&user_colf_name_bytes , cstring_get_len(user_colf_name), cstring_get_str(user_colf_name));

    ret = EC_FALSE;
    task_super_mono(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_open_colf_table_from_root, ERR_MODULE_ID, &user_table_name_bytes, &user_colf_name_bytes, &colf_table_id, &colf_mod_node);
    mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] colf table id %ld on (tcid %s, comm %ld, rank %ld, modi %ld)\n",
                         colf_table_id,
                         MOD_NODE_TCID_STR(&colf_mod_node),
                         MOD_NODE_COMM(&colf_mod_node),
                         MOD_NODE_RANK(&colf_mod_node),
                         MOD_NODE_MODI(&colf_mod_node)
                         );
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_insert(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfqtv;
    CSTRING *root_path;
    CSTRING *where;

    //MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   row_bytes;
    CBYTES   colf_bytes;
    CBYTES   colq_bytes;
    CBYTES   val_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfqtv);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt insert user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfqtv),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_insert: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_insert: root table not loaded\n");
        return (EC_TRUE);
    }
#if 0
    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_insert beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_insert end ----------------------------------\n");
#endif
#endif
    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfqtv), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(4 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_insert: invalid data %s\n", data_rfqtv);
        return (EC_TRUE);
    }

    cbytes_init(&row_bytes);
    cbytes_init(&colf_bytes);
    cbytes_init(&colq_bytes);
    cbytes_init(&val_bytes);

    cbytes_mount(&row_bytes , strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_bytes, strlen(fields[1]), (UINT8 *)fields[1]);
    cbytes_mount(&colq_bytes, strlen(fields[2]), (UINT8 *)fields[2]);
    cbytes_mount(&val_bytes , strlen(fields[3]), (UINT8 *)fields[3]);

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_insert, ERR_MODULE_ID, &user_table_name_bytes, &row_bytes, &colf_bytes, &colq_bytes, &val_bytes);
    //mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_delete(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfqt;
    CSTRING *root_path;
    CSTRING *where;

    //MOD_MGR *src_mod_mgr;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   row_bytes;
    CBYTES   colf_bytes;
    CBYTES   colq_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfqt);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt delete user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfqt),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete: root table not loaded\n");
        return (EC_TRUE);
    }
#if 0
    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(MOD_NODE_TCID(&root_mod_node), MOD_NODE_COMM(&root_mod_node), MOD_NODE_RANK(&root_mod_node), MOD_NODE_MODI(&root_mod_node), src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_delete end ----------------------------------\n");
#endif
#endif
    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfqt), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(3 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_delete: invalid data %s\n", (char *)cstring_get_str(data_rfqt));
        return (EC_TRUE);
    }

    cbytes_init(&row_bytes);
    cbytes_init(&colf_bytes);
    cbytes_init(&colq_bytes);

    cbytes_mount(&row_bytes , strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_bytes, strlen(fields[1]), (UINT8 *)fields[1]);
    cbytes_mount(&colq_bytes, strlen(fields[2]), (UINT8 *)fields[2]);


    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_delete, ERR_MODULE_ID, &user_table_name_bytes, &row_bytes, &colf_bytes, &colq_bytes);
    //mod_mgr_free(src_mod_mgr);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_search(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfqt;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   row_bytes;
    CBYTES   colf_bytes;
    CBYTES   colq_bytes;
    CBYTES   val_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfqt);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt search user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfqt),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_search: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_search: root table not loaded\n");
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfqt), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(3 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_search: invalid data %s\n", (char *)cstring_get_str(data_rfqt));
        return (EC_TRUE);
    }

    cbytes_init(&row_bytes);
    cbytes_init(&colf_bytes);
    cbytes_init(&colq_bytes);
 
    cbytes_mount(&row_bytes , strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_bytes, strlen(fields[1]), (UINT8 *)fields[1]);
    cbytes_mount(&colq_bytes, strlen(fields[2]), (UINT8 *)fields[2]);
 
    cbytes_init(&val_bytes);

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_search, ERR_MODULE_ID, &user_table_name_bytes, &row_bytes, &colf_bytes, &colq_bytes, &val_bytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] val = %.*s\n", cbytes_len(&val_bytes), cbytes_buf(&val_bytes));
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_fetch(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfqt;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   row_bytes;
    CBYTES   colf_bytes;
    CBYTES   colq_bytes;
    CBYTES   val_bytes;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfqt);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt fetch user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfqt),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_fetch: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_fetch: root table not loaded\n");
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfqt), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(3 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_fetch: invalid data %s\n", (char *)cstring_get_str(data_rfqt));
        return (EC_TRUE);
    }

    cbytes_init(&row_bytes);
    cbytes_init(&colf_bytes);
    cbytes_init(&colq_bytes);

    cbytes_mount(&row_bytes , strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_bytes, strlen(fields[1]), (UINT8 *)fields[1]);
    cbytes_mount(&colq_bytes, strlen(fields[2]), (UINT8 *)fields[2]);
 
    cbytes_init(&val_bytes);

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_fetch, ERR_MODULE_ID, &user_table_name_bytes, &row_bytes, &colf_bytes, &colq_bytes, &val_bytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] val = %.*s\n", cbytes_len(&val_bytes), cbytes_buf(&val_bytes));
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    cbytes_clean(&val_bytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_select_in_cached_user(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;

    CSTRING   *row_regex_cstr;
    CSTRING   *colf_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfq_regex);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);
    /*example:hsbgt select in user table hansoul (row-10101021.*)(colf-1)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select in cached user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: root table not loaded\n");
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(3 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: invalid %s\n", (char *)cstring_get_str(data_rfq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_cached_user: row  pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_cached_user: colf pattern: %s\n", fields[1]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_cached_user: colq pattern: %s\n", fields[2]);

    row_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0266);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: new cstring for row_regex_cstr failed\n");
        return (EC_TRUE);
    }
    colf_regex_cstr = cstring_new((UINT8 *)fields[1], LOC_API_0267);
    if(NULL_PTR == colf_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: new cstring for colf_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }
    colq_regex_cstr = cstring_new((UINT8 *)fields[2], LOC_API_0268);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: new cstring for colq_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0269);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_user: new ret kv vec failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select_in_meta, ERR_MODULE_ID, CBGT_SELECT_FROM_CACHED_TABLE,
                    &user_table_name_bytes, row_regex_cstr, colf_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0270);
    cvector_free(ret_kv_vec, LOC_API_0271);

    cstring_free(row_regex_cstr);
    cstring_free(colf_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_select_in_all_user(CMD_PARA_VEC * param)
{
    CSTRING *user_table_name;
    CSTRING *data_rfq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;

    CSTRING   *row_regex_cstr;
    CSTRING   *colf_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &user_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rfq_regex);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);
    /*example:hsbgt select in user table hansoul (row-10101021.*)(colf-1)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select in all user table %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_table_name),
                        (char *)cstring_get_str(data_rfq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: root table not loaded\n");
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_mount(&user_table_name_bytes, cstring_get_len(user_table_name), cstring_get_str(user_table_name));

    field_num = c_str_split((char *)cstring_get_str(data_rfq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(3 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: invalid %s\n", (char *)cstring_get_str(data_rfq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_all_user: row  pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_all_user: colf pattern: %s\n", fields[1]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_all_user: colq pattern: %s\n", fields[2]);

    row_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0272);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: new cstring for row_regex_cstr failed\n");
        return (EC_TRUE);
    }
    colf_regex_cstr = cstring_new((UINT8 *)fields[1], LOC_API_0273);
    if(NULL_PTR == colf_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: new cstring for colf_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }
    colq_regex_cstr = cstring_new((UINT8 *)fields[2], LOC_API_0274);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: new cstring for colq_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0275);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_user: new ret kv vec failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select_in_meta, ERR_MODULE_ID, CBGT_SELECT_FROM_ALL_TABLE,
                    &user_table_name_bytes, row_regex_cstr, colf_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0276);
    cvector_free(ret_kv_vec, LOC_API_0277);

    cstring_free(row_regex_cstr);
    cstring_free(colf_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_select_in_cached_colf(CMD_PARA_VEC * param)
{
    CSTRING *user_colf_table_name;
    CSTRING *data_rq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   colf_table_name_bytes;

    CSTRING   *row_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &user_colf_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rq_regex);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);
    /*example:hsbgt select in cached colf hansoul:colf-1 (row-10101021.*)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select in cached colf %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_colf_table_name),
                        (char *)cstring_get_str(data_rq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: root table not loaded\n");
        return (EC_TRUE);
    }

    field_num = c_str_split((char *)cstring_get_str(user_colf_table_name), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: invalid table:colf %s\n",
                            (char *)cstring_get_str(user_colf_table_name));
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_init(&colf_table_name_bytes);

    cbytes_mount(&user_table_name_bytes, strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_table_name_bytes, strlen(fields[1]), (UINT8 *)fields[1]);

    field_num = c_str_split((char *)cstring_get_str(data_rq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: invalid %s\n",
                            (char *)cstring_get_str(data_rq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_cached_colf: row  pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_cached_colf: colq pattern: %s\n", fields[1]);

    row_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0278);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: new cstring for row_regex_cstr failed\n");
        return (EC_TRUE);
    }

    colq_regex_cstr = cstring_new((UINT8 *)fields[1], LOC_API_0279);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: new cstring for colq_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0280);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_cached_colf: new ret kv vec failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select_in_colf, ERR_MODULE_ID, CBGT_SELECT_FROM_CACHED_TABLE,
                    &user_table_name_bytes, &colf_table_name_bytes, row_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0281);
    cvector_free(ret_kv_vec, LOC_API_0282);

    cstring_free(row_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_select_in_all_colf(CMD_PARA_VEC * param)
{
    CSTRING *user_colf_table_name;
    CSTRING *data_rq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CBYTES   user_table_name_bytes;
    CBYTES   colf_table_name_bytes;

    CSTRING   *row_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &user_colf_table_name);
    api_cmd_para_vec_get_cstring(param, 1, &data_rq_regex);
    api_cmd_para_vec_get_cstring(param, 2, &root_path);
    api_cmd_para_vec_get_cstring(param, 3, &where);
    /*example:hsbgt select in all colf hansoul:colf-1 (row-10101021.*)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select in all colf %s %s from path %s at %s\n",
                        (char *)cstring_get_str(user_colf_table_name),
                        (char *)cstring_get_str(data_rq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: root table not loaded\n");
        return (EC_TRUE);
    }

    field_num = c_str_split((char *)cstring_get_str(user_colf_table_name), ":;,", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: invalid table:colf %s\n",
                            (char *)cstring_get_str(user_colf_table_name));
        return (EC_TRUE);
    }

    cbytes_init(&user_table_name_bytes);
    cbytes_init(&colf_table_name_bytes);
 
    cbytes_mount(&user_table_name_bytes, strlen(fields[0]), (UINT8 *)fields[0]);
    cbytes_mount(&colf_table_name_bytes, strlen(fields[1]), (UINT8 *)fields[1]);

    field_num = c_str_split((char *)cstring_get_str(data_rq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(2 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: invalid %s\n",
                            (char *)cstring_get_str(data_rq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_all_colf: row  pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_in_all_colf: colq pattern: %s\n", fields[1]);

    row_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0283);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: new cstring for row_regex_cstr failed\n");
        return (EC_TRUE);
    }

    colq_regex_cstr = cstring_new((UINT8 *)fields[1], LOC_API_0284);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: new cstring for colq_regex_cstr failed\n");
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0285);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_in_all_colf: new ret kv vec failed\n");
        cstring_free(row_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select_in_colf, ERR_MODULE_ID, CBGT_SELECT_FROM_ALL_TABLE,
                    &user_table_name_bytes, &colf_table_name_bytes, row_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0286);
    cvector_free(ret_kv_vec, LOC_API_0287);

    cstring_free(row_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_cbgt_select_cached(CMD_PARA_VEC * param)
{
    CSTRING *data_urfq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CSTRING   *table_regex_cstr;
    CSTRING   *row_regex_cstr;
    CSTRING   *colf_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &data_urfq_regex);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_cstring(param, 2, &where);
    /*example:hsbgt select user table (han.*)(row-10101021.*)(colf-1)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select of cached user table %s from path %s at %s\n",
                        (char *)cstring_get_str(data_urfq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: root table not loaded\n");
        return (EC_TRUE);
    }

    field_num = c_str_split((char *)cstring_get_str(data_urfq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(4 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: invalid %s\n", (char *)cstring_get_str(data_urfq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_cached: table pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_cached: row   pattern: %s\n", fields[1]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_cached: colf  pattern: %s\n", fields[2]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_cached: colq  pattern: %s\n", fields[3]);

    table_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0288);
    if(NULL_PTR == table_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: new cstring for table_regex_cstr failed\n");
        return (EC_TRUE);
    }

    row_regex_cstr  = cstring_new((UINT8 *)fields[1], LOC_API_0289);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: new cstring for row_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        return (EC_TRUE);
    }

    colf_regex_cstr = cstring_new((UINT8 *)fields[2], LOC_API_0290);
    if(NULL_PTR == colf_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: new cstring for colf_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }

    colq_regex_cstr = cstring_new((UINT8 *)fields[3], LOC_API_0291);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: new cstring for colq_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0292);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_cached: new ret kv vec failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select, ERR_MODULE_ID, CBGT_SELECT_FROM_CACHED_TABLE,
                    table_regex_cstr, row_regex_cstr, colf_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0293);
    cvector_free(ret_kv_vec, LOC_API_0294);

    cstring_free(table_regex_cstr);
    cstring_free(row_regex_cstr);
    cstring_free(colf_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_select_all(CMD_PARA_VEC * param)
{
    CSTRING *data_urfq_regex;
    CSTRING *root_path;
    CSTRING *where;

    LOG     *des_log;

    CSTRING   *table_regex_cstr;
    CSTRING   *row_regex_cstr;
    CSTRING   *colf_regex_cstr;
    CSTRING   *colq_regex_cstr;

    UINT32   root_table_id;
    MOD_NODE root_mod_node;

    EC_BOOL  ret;

    char   *fields[32];
    UINT32  field_num;

    CVECTOR *ret_kv_vec;
    UINT32   pos;

    api_cmd_para_vec_get_cstring(param, 0, &data_urfq_regex);
    api_cmd_para_vec_get_cstring(param, 1, &root_path);
    api_cmd_para_vec_get_cstring(param, 2, &where);
    /*example:hsbgt select user table (han.*)(row-10101021.*)(colf-1)(colq-00000003-.*-[0-9]+)(.*) from path /home/ezhocha/hsbgt at console*/
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt select of all user table %s from path %s at %s\n",
                        (char *)cstring_get_str(data_urfq_regex),
                        (char *)cstring_get_str(root_path),
                        (char *)cstring_get_str(where));

    if(EC_FALSE == __api_cmd_ui_cbgt_query_root_table(root_path, &root_table_id, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: load root record failed\n");
        return (EC_TRUE);
    }

    if(EC_FALSE == __cbgt_mod_node_is_valid(CMPI_ANY_MODI, &root_mod_node))
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: root table not loaded\n");
        return (EC_TRUE);
    }

    field_num = c_str_split((char *)cstring_get_str(data_urfq_regex), "()", fields, sizeof(fields)/sizeof(fields[0]));
    if(4 != field_num)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: invalid %s\n", (char *)cstring_get_str(data_urfq_regex));
        return (EC_TRUE);
    }

    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_all: table pattern: %s\n", fields[0]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_all: row   pattern: %s\n", fields[1]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_all: colf  pattern: %s\n", fields[2]);
    dbg_log(SEC_0010_API, 0)(LOGCONSOLE, "[DEBUG]api_cmd_ui_cbgt_select_all: colq  pattern: %s\n", fields[3]);

    table_regex_cstr  = cstring_new((UINT8 *)fields[0], LOC_API_0295);
    if(NULL_PTR == table_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: new cstring for table_regex_cstr failed\n");
        return (EC_TRUE);
    }

    row_regex_cstr  = cstring_new((UINT8 *)fields[1], LOC_API_0296);
    if(NULL_PTR == row_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: new cstring for row_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        return (EC_TRUE);
    }

    colf_regex_cstr = cstring_new((UINT8 *)fields[2], LOC_API_0297);
    if(NULL_PTR == colf_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: new cstring for colf_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        return (EC_TRUE);
    }

    colq_regex_cstr = cstring_new((UINT8 *)fields[3], LOC_API_0298);
    if(NULL_PTR == colq_regex_cstr)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: new cstring for colq_regex_cstr failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        return (EC_TRUE);
    }

    ret_kv_vec = cvector_new(0, MM_CBYTES, LOC_API_0299);
    if(NULL_PTR == ret_kv_vec)
    {
        dbg_log(SEC_0010_API, 0)(LOGSTDOUT, "error:api_cmd_ui_cbgt_select_all: new ret kv vec failed\n");
        cstring_free(table_regex_cstr);
        cstring_free(row_regex_cstr);
        cstring_free(colf_regex_cstr);
        cstring_free(colq_regex_cstr);
        return (EC_TRUE);
    }

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                    &root_mod_node,
                    &ret, FI_cbgt_select, ERR_MODULE_ID, CBGT_SELECT_FROM_ALL_TABLE,
                    table_regex_cstr, row_regex_cstr, colf_regex_cstr, colq_regex_cstr, NULL_PTR,
                    ret_kv_vec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    for(pos = 0; pos < cvector_size(ret_kv_vec); pos ++)
    {
        CBYTES *kv_bytes;
        kv_bytes = (CBYTES *)cvector_get_no_lock(ret_kv_vec, pos);
        if(NULL_PTR == kv_bytes)
        {
            continue;
        }

        sys_log(des_log, "%ld: ", pos);
        kvPrintHs(des_log, cbytes_buf(kv_bytes));
    }

    cvector_clean(ret_kv_vec, (CVECTOR_DATA_CLEANER)cbytes_free, LOC_API_0300);
    cvector_free(ret_kv_vec, LOC_API_0301);

    cstring_free(table_regex_cstr);
    cstring_free(row_regex_cstr);
    cstring_free(colf_regex_cstr);
    cstring_free(colq_regex_cstr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_cbgt_close_module(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *src_mod_mgr;

    LOG     *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsbgt close module on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    src_mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(tcid, CMPI_ANY_COMM, rank, modi, src_mod_mgr);

#if 1
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_close_module beg ----------------------------------\n");
    mod_mgr_print(LOGSTDOUT, src_mod_mgr);
    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "------------------------------------ api_cmd_ui_cbgt_close_module end ----------------------------------\n");
#endif

    task_dea(src_mod_mgr, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, FI_cbgt_end, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);
    sys_log(des_log, "[DONE]\n");

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_add(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;
    UINT32   session_expire_nsec;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_uint32(param , 1, &session_expire_nsec);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_uint32(param , 4, &modi);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session add name %s expire %ld on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        session_expire_nsec,
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_add beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_add end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0302);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0303);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_add, ERR_MODULE_ID, session_name, session_expire_nsec);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0304);
    }

    cvector_free_no_lock(report_vec, LOC_API_0305);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_rmv_by_name(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_uint32(param , 3, &modi);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session rmv name %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_name beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_name end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0306);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0307);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_rmv_by_name, ERR_MODULE_ID, session_name);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0308);
    }

    cvector_free_no_lock(report_vec, LOC_API_0309);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_rmv_by_id(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    UINT32   session_id;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &session_id);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_uint32(param , 3, &modi);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session rmv id %ld on tcid %s rank %ld modi %ld at %s\n",
                        session_id,
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_id beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_id end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0310);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0311);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_rmv_by_id, ERR_MODULE_ID, session_id);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0312);
    }

    cvector_free_no_lock(report_vec, LOC_API_0313);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_rmv_by_name_regex(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_uint32(param , 3, &modi);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session rmv nameregex %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_name_regex beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_name_regex end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0314);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0315);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_rmv_by_name_regex, ERR_MODULE_ID, session_name);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0316);
    }

    cvector_free_no_lock(report_vec, LOC_API_0317);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_rmv_by_id_regex(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_id;/*session id string*/

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_id);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_uint32(param , 2, &rank);
    api_cmd_para_vec_get_uint32(param , 3, &modi);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session rmv idregex %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_id),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_id_regex beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_rmv_by_id_regex end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0318);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0319);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_rmv_by_id_regex, ERR_MODULE_ID, session_id);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0320);
    }

    cvector_free_no_lock(report_vec, LOC_API_0321);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_set_by_name(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;
    CSTRING *key;
    CSTRING *val;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;
 
    CBYTES val_cbytes;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_cstring(param, 2, &val);
    api_cmd_para_vec_get_tcid(param   , 3, &tcid);
    api_cmd_para_vec_get_uint32(param , 4, &rank);
    api_cmd_para_vec_get_uint32(param , 5, &modi);
    api_cmd_para_vec_get_cstring(param, 6, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session set name %s key %s val %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        (char *)cstring_get_str(key),
                        (char *)cstring_get_str(val),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_set_by_name beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_set_by_name end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0322);
    cbytes_init(&val_cbytes);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
        alloc_static_mem(MM_UINT32, &ret, LOC_API_0323);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;
     
        cbytes_mount(&val_cbytes, cstring_get_len(val), cstring_get_str(val));
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_set_by_name, ERR_MODULE_ID, session_name, key, &val_cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0324);
    }

    cvector_free_no_lock(report_vec, LOC_API_0325);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_set_by_id(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    UINT32   session_id;
    CSTRING *key;
    CSTRING *val;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;
 
    CBYTES     val_cbytes;

    api_cmd_para_vec_get_uint32(param , 0, &session_id);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_cstring(param, 2, &val);
    api_cmd_para_vec_get_tcid(param   , 3, &tcid);
    api_cmd_para_vec_get_uint32(param , 4, &rank);
    api_cmd_para_vec_get_uint32(param , 5, &modi);
    api_cmd_para_vec_get_cstring(param, 6, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session set id %ld key %s val %s on tcid %s rank %ld modi %ld at %s\n",
                        session_id,
                        (char *)cstring_get_str(key),
                        (char *)cstring_get_str(val),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_set_by_id beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_set_by_id end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0326);
    cbytes_init(&val_cbytes);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
     
        alloc_static_mem(MM_UINT32, &ret, LOC_API_0327);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        cbytes_mount(&val_cbytes, cstring_get_len(val), cstring_get_str(val));
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_set_by_id, ERR_MODULE_ID, session_id, key, &val_cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0328);
    }

    cvector_free_no_lock(report_vec, LOC_API_0329);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_get_by_name(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;
    CSTRING *key;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *csession_item_list_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_uint32(param , 4, &modi);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session get name %s key %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        (char *)cstring_get_str(key),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_name beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_name end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0330);
    csession_item_list_vec = cvector_new(0, MM_CLIST, LOC_API_0331);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
        CLIST     *csession_item_list;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0332);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        csession_item_list = clist_new(MM_CSESSION_ITEM, LOC_API_0333);
        cvector_push_no_lock(csession_item_list_vec, (void *)csession_item_list);

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_get_by_name, ERR_MODULE_ID, session_name, key, csession_item_list);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;
        CLIST     *csession_item_list;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);
        csession_item_list = (CLIST *)cvector_get_no_lock(csession_item_list_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
            clist_print_level(des_log, csession_item_list, 0, (CLIST_DATA_LEVEL_PRINT)csession_item_print);
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0334);

        cvector_set_no_lock(csession_item_list_vec, remote_mod_node_idx, NULL_PTR);
        clist_free(csession_item_list, LOC_API_0335);
    }

    cvector_free_no_lock(report_vec, LOC_API_0336);
    cvector_free_no_lock(csession_item_list_vec, LOC_API_0337);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_get_by_id(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    UINT32   session_id;
    CSTRING *key;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *csession_item_list_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_uint32(param , 0, &session_id);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_uint32(param , 4, &modi);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session get id %ld key %s on tcid %s rank %ld modi %ld at %s\n",
                        session_id,
                        (char *)cstring_get_str(key),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_id beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_id end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0338);
    csession_item_list_vec = cvector_new(0, MM_CLIST, LOC_API_0339);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
        CLIST     *csession_item_list;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0340);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        csession_item_list = clist_new(MM_CSESSION_ITEM, LOC_API_0341);
        cvector_push_no_lock(csession_item_list_vec, (void *)csession_item_list);

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_get_by_id, ERR_MODULE_ID, session_id, key, csession_item_list);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;
        CLIST     *csession_item_list;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);
        csession_item_list = (CLIST *)cvector_get_no_lock(csession_item_list_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
            clist_print_level(des_log, csession_item_list, 0, (CLIST_DATA_LEVEL_PRINT)csession_item_print);
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0342);

        cvector_set_no_lock(csession_item_list_vec, remote_mod_node_idx, NULL_PTR);
        clist_free(csession_item_list, LOC_API_0343);
    }

    cvector_free_no_lock(report_vec, LOC_API_0344);
    cvector_free_no_lock(csession_item_list_vec, LOC_API_0345);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_csession_get_by_name_regex(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_name;
    CSTRING *key;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *csession_node_list_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_name);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_uint32(param , 4, &modi);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session get nameregex %s key %s on tcid %s rank %ld modi %ld at %s\n",
                        (char *)cstring_get_str(session_name),
                        (char *)cstring_get_str(key),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_name_regex beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_name_regex end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0346);
    csession_node_list_vec = cvector_new(0, MM_CLIST, LOC_API_0347);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
        CLIST     *csession_node_list;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0348);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        csession_node_list = clist_new(MM_CSESSION_NODE, LOC_API_0349);
        cvector_push_no_lock(csession_node_list_vec, (void *)csession_node_list);

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_get_by_name_regex, ERR_MODULE_ID, session_name, key, csession_node_list);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;
        CLIST     *csession_node_list;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);
        csession_node_list = (CLIST *)cvector_get_no_lock(csession_node_list_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
            clist_print_level(des_log, csession_node_list, 0, (CLIST_DATA_LEVEL_PRINT)csession_node_print);
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0350);

        cvector_set_no_lock(csession_node_list_vec, remote_mod_node_idx, NULL_PTR);
        clist_free(csession_node_list, LOC_API_0351);
    }

    cvector_free_no_lock(report_vec, LOC_API_0352);
    cvector_free_no_lock(csession_node_list_vec, LOC_API_0353);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csession_get_by_id_regex(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;
    CSTRING *session_id;/*regex string*/
    CSTRING *key;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *csession_node_list_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &session_id);
    api_cmd_para_vec_get_cstring(param, 1, &key);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_uint32(param , 3, &rank);
    api_cmd_para_vec_get_uint32(param , 4, &modi);
    api_cmd_para_vec_get_cstring(param, 5, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session get idregex %ld key %s on tcid %s rank %ld modi %ld at %s\n",
                        session_id,
                        (char *)cstring_get_str(key),
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_id_regex beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_get_by_id_regex end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0354);
    csession_node_list_vec = cvector_new(0, MM_CLIST, LOC_API_0355);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32    *ret;
        CLIST     *csession_node_list;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0356);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        csession_node_list = clist_new(MM_CSESSION_NODE, LOC_API_0357);
        cvector_push_no_lock(csession_node_list_vec, (void *)csession_node_list);

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_csession_get_by_id_regex, ERR_MODULE_ID, session_id, key, csession_node_list);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE  *mod_node;
        UINT32    *ret;
        CLIST     *csession_node_list;

        mod_node  = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);
        csession_node_list = (CLIST *)cvector_get_no_lock(csession_node_list_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
            clist_print_level(des_log, csession_node_list, 0, (CLIST_DATA_LEVEL_PRINT)csession_node_print);
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0358);

        cvector_set_no_lock(csession_node_list_vec, remote_mod_node_idx, NULL_PTR);
        clist_free(csession_node_list, LOC_API_0359);
    }

    cvector_free_no_lock(report_vec, LOC_API_0360);
    cvector_free_no_lock(csession_node_list_vec, LOC_API_0361);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_csession_show(CMD_PARA_VEC * param)
{
    UINT32   tcid;
    UINT32   rank;
    UINT32   modi;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param   , 0, &tcid);
    api_cmd_para_vec_get_uint32(param , 1, &rank);
    api_cmd_para_vec_get_uint32(param , 2, &modi);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "session show on tcid %s rank %ld modi %ld at %s\n",
                        c_word_to_ipv4(tcid),
                        rank,
                        modi,
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, modi);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_show beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csession_show end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0362);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_csession_show, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free_no_lock(report_vec, LOC_API_0363);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_enable_task_brd(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enable task brd on tcid %s rank %ld\n", c_word_to_ipv4((tcid)), rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_task_brd beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_task_brd end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_enable_task_brd, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_enable_all_task_brd(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "enable task brd on all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_all_task_brd beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_enable_all_task_brd end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_enable_task_brd, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_disable_task_brd(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    UINT32 rank;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_uint32(param, 1, &rank);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "disable task brd on tcid %s rank %ld\n", c_word_to_ipv4((tcid)), rank);

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, rank, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_task_brd beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_task_brd end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_disable_task_brd, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_disable_all_task_brd(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "disable task brd on all\n");

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
    mod_mgr_excl(CMPI_ANY_DBG_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);
    mod_mgr_excl(CMPI_ANY_MON_TCID, CMPI_ANY_COMM, CMPI_ANY_RANK, CMPI_ANY_MODI, mod_mgr);

#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_all_task_brd beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_disable_all_task_brd end ----------------------------------\n");
    }
#endif
#if 1
    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_disable_task_brd, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);
#endif
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

/*debug only*/
EC_BOOL api_cmd_ui_cdfs_write_files(CMD_PARA_VEC * param)
{
    UINT32   file_num;
    UINT32   replica_num;
    UINT32   cdfsnp_tcid;
    CSTRING *dir_name;
    CSTRING *where;

    MOD_MGR   *mod_mgr;
    TASK_MGR  *task_mgr;
    CBYTES    *cbytes;
    LOG       *des_log;

    UINT32    file_pos;

    char *file_content = (char *)"hello world!";

    api_cmd_para_vec_get_uint32(param , 0, &file_num);
    api_cmd_para_vec_get_uint32(param , 1, &replica_num);
    api_cmd_para_vec_get_cstring(param, 2, &dir_name);
    api_cmd_para_vec_get_tcid(param   , 3, &cdfsnp_tcid);
    api_cmd_para_vec_get_cstring(param, 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsdfs dbg write %ld files and replicas %ld in dir %s to npp %s at %s\n",
                        file_num,
                        replica_num,
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(cdfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(cdfsnp_tcid, CMPI_ANY_COMM, CMPI_CDFS_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_write_files beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_cdfs_write_files end ----------------------------------\n");
    }
#endif

    cbytes = cbytes_new(0);
    cbytes_mount(cbytes, strlen(file_content), (UINT8 *)file_content);

    des_log = api_cmd_ui_get_log(where);

    for(file_pos = 0; file_pos < file_num; file_pos ++)
    {
        CSTRING *file_name;
        EC_BOOL  ret;

        file_name = cstring_new(NULL_PTR, LOC_API_0364);
        cstring_format(file_name, "%s/%ld.dat", (char *)cstring_get_str(dir_name), file_pos);

        ret = EC_FALSE;


        task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
        task_tcid_inc(task_mgr, cdfsnp_tcid, &ret, FI_cdfs_write, ERR_MODULE_ID, file_name, cbytes, replica_num);
        task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

        cstring_free(file_name);

        if(EC_TRUE == ret)
        {
            sys_log(des_log, "%8ld# [SUCC]\n", file_pos);
        }
        else
        {
            sys_log(des_log, "%8ld# [FAIL]\n", file_pos);
        }
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_download(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *fname;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *fcontent_vec;

    LOG *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &fname);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec download %s on tcid %s at %s\n",
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_download beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_download end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0365);
    fcontent_vec = cvector_new(0, MM_CBYTES, LOC_API_0366);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CBYTES *cbytes;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0367);
        cbytes = cbytes_new(0);

        cvector_push(report_vec, (void *)ret);
        cvector_push(fcontent_vec, (void *)cbytes);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_download, ERR_MODULE_ID, fname, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret      = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cbytes   = (CBYTES *)cvector_get(fcontent_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n%.*s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                             cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));

        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cvector_set(fcontent_vec, remote_mod_node_idx, NULL_PTR);

        free_static_mem(MM_UINT32, ret, LOC_API_0368);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0369);
    cvector_free(fcontent_vec, LOC_API_0370);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_download_all(CMD_PARA_VEC * param)
{
    CSTRING *fname;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *fcontent_vec;

    LOG *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &fname);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec download %s on all at %s\n",
                        (char *)cstring_get_str(fname),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_ANY_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_download_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_download_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0371);
    fcontent_vec = cvector_new(0, MM_CBYTES, LOC_API_0372);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CBYTES *cbytes;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0373);
        cbytes = cbytes_new(0);

        cvector_push(report_vec, (void *)ret);
        cvector_push(fcontent_vec, (void *)cbytes);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_download, ERR_MODULE_ID, fname, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret      = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cbytes   = (CBYTES *)cvector_get(fcontent_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n%.*s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                             cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));

        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cvector_set(fcontent_vec, remote_mod_node_idx, NULL_PTR);

        free_static_mem(MM_UINT32, ret, LOC_API_0374);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0375);
    cvector_free(fcontent_vec, LOC_API_0376);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_upload(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *fname;
    CSTRING *fcontent;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *fcontent_vec;

    LOG *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &fname);
    api_cmd_para_vec_get_cstring(param, 1, &fcontent);
    api_cmd_para_vec_get_tcid(param   , 2, &tcid);
    api_cmd_para_vec_get_cstring(param, 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec upload %s with %s on tcid %s at %s\n",
                        (char *)cstring_get_str(fname),
                        (char *)cstring_get_str(fcontent),
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_upload beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_upload end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0377);
    fcontent_vec = cvector_new(0, MM_CBYTES, LOC_API_0378);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CBYTES *cbytes;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0379);
        cbytes = cbytes_new(0);
        cbytes_mount(cbytes, cstring_get_len(fcontent), cstring_get_str(fcontent));

        cvector_push(report_vec, (void *)ret);
        cvector_push(fcontent_vec, (void *)cbytes);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_upload, ERR_MODULE_ID, fname, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret      = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cbytes   = (CBYTES *)cvector_get(fcontent_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n%.*s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                             cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));

        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cvector_set(fcontent_vec, remote_mod_node_idx, NULL_PTR);

        free_static_mem(MM_UINT32, ret, LOC_API_0380);

        cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0381);
    cvector_free(fcontent_vec, LOC_API_0382);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_upload_all(CMD_PARA_VEC * param)
{
    CSTRING *fname;
    CSTRING *fcontent;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    CVECTOR *fcontent_vec;

    LOG *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &fname);
    api_cmd_para_vec_get_cstring(param, 1, &fcontent);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec upload %s with %s on all at %s\n",
                        (char *)cstring_get_str(fname),
                        (char *)cstring_get_str(fcontent),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_upload_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_upload_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0383);
    fcontent_vec = cvector_new(0, MM_CBYTES, LOC_API_0384);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;
        CBYTES *cbytes;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0385);
        cbytes = cbytes_new(0);
        cbytes_mount(cbytes, cstring_get_len(fcontent), cstring_get_str(fcontent));

        cvector_push(report_vec, (void *)ret);
        cvector_push(fcontent_vec, (void *)cbytes);

        (*ret) = EC_FALSE;
        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_upload, ERR_MODULE_ID, fname, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        UINT32 *ret;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret      = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);
        cbytes   = (CBYTES *)cvector_get(fcontent_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n%.*s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                             cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));

        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cvector_set(fcontent_vec, remote_mod_node_idx, NULL_PTR);

        free_static_mem(MM_UINT32, ret, LOC_API_0386);

        cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0387);
    cvector_free(fcontent_vec, LOC_API_0388);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_shell(CMD_PARA_VEC * param)
{
    UINT32 tcid;

    CSTRING *cmd_line;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG     *des_log;
    EC_BOOL  ret;

    api_cmd_para_vec_get_cstring(param, 0, &cmd_line);
    api_cmd_para_vec_get_tcid(param   , 1, &tcid);
    api_cmd_para_vec_get_cstring(param, 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec shell '%s' on tcid %s where %s\n",
                        (char *)cstring_get_str(cmd_line),
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_shell beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_shell end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_CBYTES, LOC_API_0389);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        CBYTES *cbytes;

        cbytes = cbytes_new(0);

        cvector_push(report_vec, (void *)cbytes);
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_super_exec_shell, ERR_MODULE_ID, cmd_line, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        cbytes = (CBYTES *)cvector_get(report_vec, remote_mod_node_idx);

        if(0 == cbytes_len(cbytes))
        {
            sys_log(des_log, "[rank_%s_%ld] %s\n(null)\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                            (char *)cstring_get_str(cmd_line));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] %s\n%.*s\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                            (char *)cstring_get_str(cmd_line),
                            cbytes_len(cbytes), cbytes_buf(cbytes));
        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0390);
    mod_mgr_free(mod_mgr);
    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_exec_shell_all(CMD_PARA_VEC * param)
{
    CSTRING *cmd_line;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;
    EC_BOOL ret;

    api_cmd_para_vec_get_cstring(param, 0, &cmd_line);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "exec shell '%s' on all at %s\n",
                        (char *)cstring_get_str(cmd_line),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_shell_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_exec_shell_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_CBYTES, LOC_API_0391);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        CBYTES *cbytes;

        cbytes = cbytes_new(0);

        cvector_push(report_vec, (void *)cbytes);
        task_pos_inc(task_mgr, remote_mod_node_idx, &ret, FI_super_exec_shell, ERR_MODULE_ID, cmd_line, cbytes);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        CBYTES *cbytes;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        cbytes = (CBYTES *)cvector_get(report_vec, remote_mod_node_idx);

        if(0 == cbytes_len(cbytes))
        {
            sys_log(des_log, "[rank_%s_%ld] %s\n(null)\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                            (char *)cstring_get_str(cmd_line));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld] %s\n%.*s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node),
                            (char *)cstring_get_str(cmd_line),
                            cbytes_len(cbytes), cbytes_buf(cbytes));
        }
        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        cbytes_free(cbytes);
    }

    cvector_free(report_vec, LOC_API_0392);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_version(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show version on tcid %s at %s\n", c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_version beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_version end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0393);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_version, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0394);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_version_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show version on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_version_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_version_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0395);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_version, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0396);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_vendor(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show vendor on tcid %s at %s\n", c_word_to_ipv4(tcid), (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_vendor beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_vendor end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0397);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();

        cvector_push(report_vec, (void *)log);
        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_vendor, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0398);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_show_vendor_all(CMD_PARA_VEC * param)
{
    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;
    CSTRING *where;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;
    LOG   *des_log;

    api_cmd_para_vec_get_cstring(param, 0, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "show vendor on all at %s\n", (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(CMPI_ANY_TCID, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_vendor_all beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_show_vendor_all end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_LOG, LOC_API_0399);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        LOG *log;

        log = log_cstr_open();
        cvector_push(report_vec, (void *)log);

        task_pos_inc(task_mgr, remote_mod_node_idx, NULL_PTR, FI_super_show_vendor, ERR_MODULE_ID, log);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
        LOG *log;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        log = (LOG *)cvector_get(report_vec, remote_mod_node_idx);

        sys_log(des_log, "[rank_%s_%ld]\n%s", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node), (char *)cstring_get_str(LOG_CSTR(log)));

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        log_cstr_close(log);
    }

    cvector_free(report_vec, LOC_API_0400);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_start_mcast_udp_server(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "start udp server on tcid %s at %s\n",
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_start_mcast_udp_server beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_start_mcast_udp_server end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0401);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0402);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_start_mcast_udp_server, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
         UINT32 *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);\

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0403);
    }

    cvector_free(report_vec, LOC_API_0404);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_stop_mcast_udp_server(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "stop udp server on tcid %s at %s\n",
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_stop_mcast_udp_server beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_stop_mcast_udp_server end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0405);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0406);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_stop_mcast_udp_server, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
         UINT32 *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get_no_lock(report_vec, remote_mod_node_idx);

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][SUCC]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][FAIL]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set_no_lock(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0407);
    }

    cvector_free(report_vec, LOC_API_0408);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_status_mcast_udp_server(CMD_PARA_VEC * param)
{
    UINT32 tcid;
    CSTRING *where;

    MOD_MGR  *mod_mgr;
    TASK_MGR *task_mgr;

    UINT32 remote_mod_node_num;
    UINT32 remote_mod_node_idx;

    CVECTOR *report_vec;

    LOG *des_log;

    api_cmd_para_vec_get_tcid(param, 0, &tcid);
    api_cmd_para_vec_get_cstring(param, 1, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "status udp server on tcid %s at %s\n",
                        c_word_to_ipv4(tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = api_cmd_ui_gen_mod_mgr(tcid, CMPI_FWD_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*super_md_id = 0*/
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_status_mcast_udp_server beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_status_mcast_udp_server end ----------------------------------\n");
    }
#endif

    report_vec = cvector_new(0, MM_UINT32, LOC_API_0409);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    remote_mod_node_num = MOD_MGR_REMOTE_NUM(mod_mgr);
    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        UINT32 *ret;

        alloc_static_mem(MM_UINT32, &ret, LOC_API_0410);
        cvector_push_no_lock(report_vec, (void *)ret);
        (*ret) = EC_FALSE;

        task_pos_inc(task_mgr, remote_mod_node_idx, ret, FI_super_status_mcast_udp_server, ERR_MODULE_ID);
    }
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    for(remote_mod_node_idx = 0; remote_mod_node_idx < remote_mod_node_num; remote_mod_node_idx ++)
    {
        MOD_NODE *mod_node;
         UINT32 *ret;

        mod_node = MOD_MGR_REMOTE_MOD(mod_mgr, remote_mod_node_idx);
        ret = (UINT32 *)cvector_get(report_vec, remote_mod_node_idx);\

        if(EC_TRUE == (*ret))
        {
            sys_log(des_log, "[rank_%s_%ld][ACTIVE]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }
        else
        {
            sys_log(des_log, "[rank_%s_%ld][INACTIVE]\n", MOD_NODE_TCID_STR(mod_node),MOD_NODE_RANK(mod_node));
        }

        cvector_set(report_vec, remote_mod_node_idx, NULL_PTR);
        free_static_mem(MM_UINT32, ret, LOC_API_0411);
    }

    cvector_free(report_vec, LOC_API_0412);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

#if 1
EC_BOOL api_cmd_ui_crfs_create_npp(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_model;
    UINT32   crfsnp_max_num;
    UINT32   crfsnp_2nd_chash_algo_id; 
    CSTRING *crfsnp_db_root_dir;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &crfsnp_model);
    api_cmd_para_vec_get_uint32(param  , 2, &crfsnp_max_num);
    api_cmd_para_vec_get_uint32(param  , 3, &crfsnp_2nd_chash_algo_id);
    api_cmd_para_vec_get_cstring(param , 4, &crfsnp_db_root_dir);
    api_cmd_para_vec_get_tcid(param    , 5, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 6, &where);

    /*hsrfs <id> create np model <model> max num <np mum> with hash algo <id> and root <root dir> on tcid <tcid> at <where>*/
    /*hsrfs %n create np model %n max num %n with hash algo %n and root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_npp: hsrfs %ld create np model %u max num %u with hash algo %u and root %s on tcid %s at %s\n",
                        crfs_modi,
                        crfsnp_model,
                        crfsnp_max_num,
                        crfsnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfsnp_db_root_dir),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_create_npp, ERR_MODULE_ID,
             crfsnp_model, crfsnp_max_num, crfsnp_2nd_chash_algo_id, crfsnp_db_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld create np model %u max num %u with hash algo %u and root %s on tcid %s successfully\n",
                        crfs_modi,
                        crfsnp_model,
                        crfsnp_max_num,
                        crfsnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfsnp_db_root_dir),
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld create np model %u max num %u with hash algo %u and root %s on tcid %s failed\n",
                        crfs_modi,
                        crfsnp_model,
                        crfsnp_max_num,
                        crfsnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfsnp_db_root_dir),
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_create_dn(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *crfsnp_dn_root_dir;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &crfsnp_dn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> create dn with root <root dir> on tcid <tcid> at <where>*/
    /*hsrfs %n create dn with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_dn: hsrfs %ld create dn with root %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfsnp_dn_root_dir),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_create_dn, ERR_MODULE_ID, crfsnp_dn_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld create dn with root %s on tcid %s successfully\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfsnp_dn_root_dir),
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld create dn with root %s on tcid %s failed\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfsnp_dn_root_dir),
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_add_disk(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   disk_no;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> add disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfs %n add disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_add_disk: hsrfs %ld add disk %u on tcid %s at %s\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_add_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld add disk %u on tcid %s successfully\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld add disk %u on tcid %s failed\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_del_disk(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   disk_no;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> del disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfs %n del disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_del_disk: hsrfs %ld del disk %u on tcid %s at %s\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_del_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld del disk %u on tcid %s successfully\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld del disk %u on tcid %s failed\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_mount_disk(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   disk_no;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> mount disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfs %n mount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_mount_disk: hsrfs %ld mount disk %u on tcid %s at %s\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_mount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld mount disk %u on tcid %s successfully\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld mount disk %u on tcid %s failed\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_umount_disk(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   disk_no;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> umount disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfs %n umount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_umount_disk: hsrfs %ld umount disk %u on tcid %s at %s\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_umount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfs %ld umount disk %u on tcid %s successfully\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfs %ld umount disk %u on tcid %s failed\n",
                        crfs_modi,
                        disk_no,
                        c_word_to_ipv4(crfsnp_tcid));
    }

    return (EC_TRUE);
}
EC_BOOL api_cmd_ui_crfs_open(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *crfs_root_dir;
    UINT32   crfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &crfs_root_dir);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);

    /*hsrfs <id> open from root <dir> on tcid <tcid>*/
    /*hsrfs %n open from root %s on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_open: hsrfs %ld open from root %s on tcid %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfs_root_dir),
                        c_word_to_ipv4(crfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(crfsnp_tcid, CMPI_CRFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, crfs_modi);

    task_act(mod_mgr_def, NULL_PTR, TASK_DEFAULT_LIVE, (UINT32)1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_crfs_start, crfs_root_dir);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_close(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);

    /*hsrfs <id> close on tcid <tcid>*/
    /*hsrfs %n close on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_close: hsrfs %ld close on tcid %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(crfsnp_tcid, CMPI_CRFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, crfs_modi);

    task_dea(mod_mgr_def, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, FI_crfs_end, ERR_MODULE_ID);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_read(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    UINT32     expires_timestamp;
    EC_BOOL    need_expired_content;
    LOG       *des_log;
    EC_BOOL    ret;
 
    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> read file <name> on tcid <tcid> at <where>*/
    /*hsrfs %n read file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_read: hsrfs %ld read file %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    cbytes = cbytes_new(0);

    need_expired_content = EC_TRUE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_read, ERR_MODULE_ID, file_name, cbytes, &expires_timestamp, need_expired_content);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_read: read %s result: \n%.*s\n",
                          (char *)cstring_get_str(file_name),
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_read: read %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_write(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *file_content;
//    UINT32   expire_nsec;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_cstring(param , 2, &file_content);
    //api_cmd_para_vec_get_uint32(param  , 3, &expire_nsec);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> write file <name> with content <string> on tcid <tcid> at <where>*/
    /*hsrfs %n write file %s with content %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_write: hsrfs %ld write file %s with content %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        /*expire_nsec,*/
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_write, ERR_MODULE_ID, file_name, cbytes, 0); // set expire_nsec = 0

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_create_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    uint64_t file_size;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_uint64(param  , 2, &file_size);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> create bigfile <name> with size <file size> on tcid <tcid> at <console|log>*/
    /*hsrfs %n create bigfile %s with size %N on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_b: hsrfs %ld create bigfile %s with size %lld on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        file_size,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_create_b, ERR_MODULE_ID, file_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_read: craete bigfile %s done\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_read: create bigfile %s failed\n", (char *)cstring_get_str(file_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_read_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    uint64_t offset;
    UINT32   max_len;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL    ret;

    UINT32    expires_timestamp;
    EC_BOOL   need_expired_content;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_uint64(param  , 2, &offset);
    api_cmd_para_vec_get_uint32(param  , 3, &max_len);
    api_cmd_para_vec_get_tcid(param    , 4, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    /*hsrfs <id> read bigfile <name> from offset <offset> max <max len> on tcid <tcid> at <console|log>*/
    /*hsrfs %n  read bigfile %s from offset %N max %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_read_b: hsrfs %ld read bigfile %s from offset %llu max %u on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        offset,
                        max_len,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    need_expired_content = EC_TRUE;
    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_read_b, ERR_MODULE_ID, file_name, &offset, max_len, cbytes, &expires_timestamp, need_expired_content);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_read: read bigfile %s to offset %llu result: %.*s\n",
                          (char *)cstring_get_str(file_name), offset,
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_read: read bigfile %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_write_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *file_content;
    uint64_t offset;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_cstring(param , 2, &file_content);
    api_cmd_para_vec_get_uint64(param  , 3, &offset);
    api_cmd_para_vec_get_tcid(param    , 4, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    /*hsrfs <id> write bigfile <name> with content <string> at offset <offset> on tcid <tcid> at <console|log>*/
    /*hsrfs %n write bigfile %s with content %s at offset %N on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_write_b: hsrfs %ld write bigfile %s with content %s at offset %llu on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        offset,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_write_b, ERR_MODULE_ID, file_name, &offset, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] bigfile name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] bigfile name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_delete_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> delete bigfile <name> on tcid <tcid> at <console|log>*/
    /*hsrfs %n delete bigfile %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_b: hsrfs %ld delete bigfile %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_file_b, ERR_MODULE_ID, file_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete bigfile name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete bigfile name %s\n", (char *)cstring_get_str(file_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_download_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *bigfile_name;
    uint64_t offset;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node; 
    LOG       *des_log;

    CBYTES    *cbytes;
    int        fd;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &bigfile_name);
    api_cmd_para_vec_get_cstring(param , 2, &file_name);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> download bigfile <name> to file <name> on tcid <tcid> at <console|log>*/
    /*hsrfs %n download bigfile %s to file <name> on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_download_b: hsrfs %ld download bigfile %s to file %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(bigfile_name),
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    fd = c_file_open((char *)cstring_get_str(file_name), O_RDWR | O_CREAT, 0666);
    if(ERR_FD == fd)
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfs_download_b: open file %s failed\n",
                            (char *)cstring_get_str(file_name));
        return (EC_TRUE);                     
    } 

    c_file_truncate(fd, 0);

    cbytes = cbytes_new(0);
    ASSERT(NULL_PTR != cbytes); 

    offset = 0;
    for(;;)
    {
        EC_BOOL    ret;
        UINT32     max_len;
        UINT32     offset_t;
        UINT32     expires_timestamp;
        EC_BOOL    need_expired_content;     

        max_len  = CPGB_CACHE_MAX_BYTE_SIZE;
        offset_t = (UINT32)offset;

        need_expired_content = EC_TRUE;
        ret = EC_FALSE;     
     
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                 &mod_node,
                 &ret,
                 FI_crfs_read_b, ERR_MODULE_ID, bigfile_name, &offset, max_len, cbytes, &expires_timestamp, need_expired_content);

        des_log = api_cmd_ui_get_log(where);

        if(EC_TRUE == ret)
        {
            sys_log(des_log, "[SUCC] api_cmd_ui_crfs_download_b: download bigfile %s to offset %llu\n",
                              (char *)cstring_get_str(bigfile_name), offset);
        }
        else
        {
            sys_log(des_log, "[FAIL] api_cmd_ui_crfs_download_b: download bigfile %s to offset %llu\n",
                              (char *)cstring_get_str(bigfile_name), offset);
        }  

        if(EC_FALSE == ret)
        {
            break;
        }

        if(0 == cbytes_len(cbytes))
        {
            break;
        }

        c_file_flush(fd, &offset_t, cbytes_len(cbytes), cbytes_buf(cbytes));
        cbytes_clean(cbytes);

        ASSERT(offset == offset_t);
    }

    cbytes_free(cbytes);
    c_file_close(fd);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_upload_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *bigfile_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node; 
    LOG       *des_log;
 
    uint64_t   file_size;
    uint64_t   offset;
    int        fd;
    CBYTES    *cbytes;
    uint8_t   *content;
    UINT32     content_max_len;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_cstring(param , 2, &bigfile_name);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> upload file <name> to bigfile <name> on tcid <tcid> at <console|log>*/
    /*hsrfs %n upload file %s to bigfile %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_upload_b: hsrfs %ld upload file %s to bigfile %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(bigfile_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    fd = c_file_open((char *)cstring_get_str(file_name), O_RDONLY, 0666);
    if(ERR_FD == fd)
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfs_upload_b: open file %s failed\n",
                            (char *)cstring_get_str(file_name));
        return (EC_TRUE);                     
    }

    if(EC_FALSE == c_file_size_b(fd, &file_size))
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfs_upload_b: get file %s size failed\n",
                            (char *)cstring_get_str(file_name));
 
        c_file_close(fd);
        return (EC_TRUE);
    }

    sys_log(LOGCONSOLE, "[DEBUG] api_cmd_ui_crfs_upload_b: file %s, size %ld\n", (char *)cstring_get_str(file_name), file_size);

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_create_b, ERR_MODULE_ID, bigfile_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_upload_b: create bigfile %s done\n", (char *)cstring_get_str(bigfile_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_upload_b: create bigfile %s failed\n", (char *)cstring_get_str(bigfile_name));
    }

    if(EC_FALSE == ret)
    {
        c_file_close(fd);
        return (EC_TRUE);/*terminate*/
    }

    //content_max_len = (CPGB_CACHE_MAX_BYTE_SIZE / 2) - 7;
    content_max_len = CPGB_CACHE_MAX_BYTE_SIZE;

    content = safe_malloc(content_max_len, LOC_API_0413);     
    ASSERT(NULL_PTR != content);
 
    cbytes = cbytes_new(0);
    ASSERT(NULL_PTR != cbytes); 

    offset = 0; 
    while(offset < file_size)
    {     
        UINT32     content_len;
     
        UINT32     offset_t;

        offset_t = (UINT32)offset;
        c_file_read(fd, &offset_t, content_max_len, content);
        content_len = (UINT32)(offset_t - offset);

        //sys_log(des_log, "[DEBUG] api_cmd_ui_crfs_upload_b: load data from %ld to %ld, len %ld\n", (UINT32)offset, offset_t, content_len);
     
        cbytes_mount(cbytes, content_len, content);

        ret = EC_FALSE;     
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                 &mod_node,
                 &ret,
                 FI_crfs_write_b, ERR_MODULE_ID, bigfile_name, &offset, cbytes);

        des_log = api_cmd_ui_get_log(where);

        if(EC_TRUE == ret)
        {
            sys_log(des_log, "[SUCC] import bigfile name %s to offset %lld\n", (char *)cstring_get_str(bigfile_name), offset);
        }
        else
        {
            sys_log(des_log, "[FAIL] import bigfile name %s to offset %lld\n", (char *)cstring_get_str(bigfile_name), offset);
        } 

        cbytes_umount(cbytes, NULL_PTR, NULL_PTR);     

        if(EC_FALSE == ret)
        {
            break;
        }
    }

    cbytes_free(cbytes);
    safe_free(content, LOC_API_0414);
    c_file_close(fd);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_mkdir(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> mkdir <path> on tcid <tcid> at <where>*/
    /*hsrfs %n mkdir %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_mkdir: hsrfs %ld mkdir %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_mkdir, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] mkdir %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] mkdir %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_search(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> search <path> on tcid <tcid> at <where>*/
    /*hsrfs %n search %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_search: hsrfs %ld search %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_search, ERR_MODULE_ID, path_name, (UINT32)CRFSNP_ITEM_FILE_IS_ANY);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] search %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] search %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_count_file_num(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    UINT32   file_num;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> count file num of <path> on tcid <tcid> at <where>*/
    /*hsrfs %n count file num of %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_count_file_num: hsrfs %ld count file num of path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_num, ERR_MODULE_ID, path_name, &file_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file num of path %s: %ld\n", (char *)cstring_get_str(path_name), file_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file num of path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_count_file_size(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    uint64_t  file_size;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> count file size of <path> on tcid <tcid> at <where>*/
    /*hsrfs %n count file size of %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_file_size: hsrfs %ld count file size of path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_size, ERR_MODULE_ID, path_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file size of path %s: %llu\n", (char *)cstring_get_str(path_name), file_size);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file size of path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qfile(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *where;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CRFSNP_ITEM *crfsnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qfile %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qfile beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qfile end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    crfsnp_item = crfsnp_item_new();
    ASSERT(NULL_PTR != crfsnp_item);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qfile, ERR_MODULE_ID, file_name, crfsnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        crfsnp_item_print(des_log, crfsnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    crfsnp_item_free(crfsnp_item);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qdir(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *dir_name;
    CSTRING *where;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CRFSNP_ITEM *crfsnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qdir %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qdir beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qdir end ----------------------------------\n");
    }
#endif

    crfsnp_item = crfsnp_item_new();
    ASSERT(NULL_PTR != crfsnp_item);

    ret = EC_FALSE;

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qdir, ERR_MODULE_ID, dir_name, crfsnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC]\n");
        crfsnp_item_print(des_log, crfsnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
        crfsnp_item_print(des_log, crfsnp_item);
    }

    crfsnp_item_free(crfsnp_item);
    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_path_of_np(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_id;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_uint32(param  , 2, &crfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s full of np %ld on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        crfsnp_id,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_path_of_np beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_path_of_np end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0415);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_path_of_np, ERR_MODULE_ID, path, crfsnp_id, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0416);
    cvector_free(path_cstr_vec, LOC_API_0417);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_seg_of_np(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_id;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *seg_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_uint32(param  , 2, &crfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s short of np %ld on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        crfsnp_id,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_seg_of_np beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_seg_of_np end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    seg_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0418);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_seg_of_np, ERR_MODULE_ID, path, crfsnp_id, seg_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }

    cvector_clean(seg_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0419);
    cvector_free(seg_cstr_vec, LOC_API_0420);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_path(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s full on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_path beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_path end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0421);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_path, ERR_MODULE_ID, path, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0422);
    cvector_free(path_cstr_vec, LOC_API_0423);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_seg(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *seg_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s short on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_seg beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_seg end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    seg_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0424);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_seg, ERR_MODULE_ID, path, seg_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(seg_cstr_vec); pos ++)
        {
            CSTRING *seg_cstr;

            seg_cstr = (CSTRING *)cvector_get(seg_cstr_vec, pos);
            if(NULL_PTR == seg_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(seg_cstr));
        }
    }

    cvector_clean(seg_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0425);
    cvector_free(seg_cstr_vec, LOC_API_0426);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_tree_of_np(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_id;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *tree_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_uint32(param  , 2, &crfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s tree of np %ld on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        crfsnp_id,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_tree_of_np beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_tree_of_np end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    tree_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0427);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_tree_of_np, ERR_MODULE_ID, crfsnp_id, path, tree_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(tree_cstr_vec); pos ++)
        {
            CSTRING *tree_cstr;

            tree_cstr = (CSTRING *)cvector_get(tree_cstr_vec, pos);
            if(NULL_PTR == tree_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(tree_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(tree_cstr_vec); pos ++)
        {
            CSTRING *tree_cstr;

            tree_cstr = (CSTRING *)cvector_get(tree_cstr_vec, pos);
            if(NULL_PTR == tree_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(tree_cstr));
        }
    }

    cvector_clean(tree_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0428);
    cvector_free(tree_cstr_vec, LOC_API_0429);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_qlist_tree(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path;
    CSTRING *where;
    UINT32   crfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CVECTOR     *path_cstr_vec;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfs %ld qlist %s tree on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfsnp_tcid, CMPI_ANY_COMM, CMPI_CRFS_RANK, crfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_tree beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfs_qlist_tree end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;

    path_cstr_vec = cvector_new(0, MM_CSTRING, LOC_API_0430);

    task_mgr = task_new(mod_mgr, TASK_PRIO_HIGH, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfsnp_tcid, &ret, FI_crfs_qlist_tree, ERR_MODULE_ID, path, path_cstr_vec);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        UINT32 pos;

        sys_log(des_log, "[SUCC]\n");
        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }
    else
    {
        UINT32 pos;
        sys_log(des_log, "[FAIL]\n");

        for(pos = 0; pos < cvector_size(path_cstr_vec); pos ++)
        {
            CSTRING *path_cstr;

            path_cstr = (CSTRING *)cvector_get(path_cstr_vec, pos);
            if(NULL_PTR == path_cstr)
            {
                continue;
            }

            sys_log(des_log, "%ld # %s\n", pos, (char *)cstring_get_str(path_cstr));
        }
    }

    cvector_clean(path_cstr_vec, (CVECTOR_DATA_CLEANER)cstring_free, LOC_API_0431);
    cvector_free(path_cstr_vec, LOC_API_0432);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_delete_file(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *fname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &fname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> delete file <name> on tcid <tcid> at <where>*/
    /*hsrfs %n delete file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_file: hsrfs %ld delete file %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_file, ERR_MODULE_ID, fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete file %s\n", (char *)cstring_get_str(fname));
        sys_log(LOGCONSOLE, "[SUCC] delete file %s\n", (char *)cstring_get_str(fname));
        if(des_log == LOGCONSOLE)
        {
            sys_log(LOGCONSOLE, "[SUCC] des_log is CONSOLE\n");
        }
        else
        {
            sys_log(LOGCONSOLE, "[SUCC] des_log is NOT CONSOLE\n");
        }
    }
    else
    {
        sys_log(des_log, "[FAIL] delete file %s\n", (char *)cstring_get_str(fname));
        sys_log(LOGCONSOLE, "[FAIL] delete file %s\n", (char *)cstring_get_str(fname));
        if(des_log == LOGCONSOLE)
        {
            sys_log(LOGCONSOLE, "[FAIL] des_log is CONSOLE\n");
        }
        else
        {
            sys_log(LOGCONSOLE, "[FAIL] des_log is NOT CONSOLE\n");
        }
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_delete_dir(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *dname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    EC_BOOL   is_root_dir;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &dname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> delete dir <name> on tcid <tcid> at <where>*/
    /*hsrfs %n delete dir %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_dir: hsrfs %ld delete dir %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(dname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;


    des_log = api_cmd_ui_get_log(where);
 
    ret = EC_FALSE;

    /* ensure the dir to delete is NOT root dir / */
    is_root_dir = cstring_is_str((const CSTRING *)dname, (const UINT8 *)"/");
    if(EC_FALSE == is_root_dir)
    {
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_dir, ERR_MODULE_ID, dname);
    }
    else
    {
        sys_log(des_log, "[ERRO] can NOT use this interface to delete root dir %s \n", (char *)cstring_get_str(dname));
        //sys_log(des_log, "[WARN] to delete root dir, use command: hsrfs <id> delete root dir / on tcid <tcid> at <console|log>\n");
    }

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete dir %s\n", (char *)cstring_get_str(dname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete dir %s\n", (char *)cstring_get_str(dname));
    }

    return (EC_TRUE);
}


EC_BOOL api_cmd_ui_crfs_delete_root_dir(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *dname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    EC_BOOL   is_root_dir;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &dname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_root_dir: hsrfs %ld delete root dir %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(dname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;


    des_log = api_cmd_ui_get_log(where);

    ret = EC_FALSE;

    /* ensure the dir to delete is root dir / */
    is_root_dir = cstring_is_str((const CSTRING *)dname, (const UINT8 *)"/");
    if(EC_TRUE == is_root_dir)
    {
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_dir, ERR_MODULE_ID, dname);
    }
    else
    {
        sys_log(des_log, "[ERRO] the dir %s to delete is not root dir / \n", (char *)cstring_get_str(dname));
    }
 
    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete root dir %s\n", (char *)cstring_get_str(dname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete dir %s\n", (char *)cstring_get_str(dname));
    }

    return (EC_TRUE);
}

/* check whether file is in memory cache */
EC_BOOL api_cmd_ui_crfs_check_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;

    LOG       *des_log;
    EC_BOOL    ret;
 
    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_check_memc: hsrfs %ld check whether file %s is in memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_check_memc, ERR_MODULE_ID, file_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_check_memc: file %s is in memcache\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_check_memc: file %s is NOT in memcache\n", (char *)cstring_get_str(file_name));
    }

    return (EC_TRUE);
}


/* only write data to memory cache but NOT rfs */
EC_BOOL api_cmd_ui_crfs_write_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *file_content;
    //UINT32   expire_nsec;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_cstring(param , 2, &file_content);
    //api_cmd_para_vec_get_uint32(param  , 3, &expire_nsec);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_write_memc: hsrfs %ld write file %s with content %s to memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        //expire_nsec,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_write_memc, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s has been written to memcache\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

/* read file from memory cache only */
EC_BOOL api_cmd_ui_crfs_read_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL    ret;
 
    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> read file <name> on tcid <tcid> at <where>*/
    /*hsrfs %n read file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_read_memc: hsrfs %ld read file %s from memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    cbytes = cbytes_new(0);

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_read_memc, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfs_read_memc: read %s from memcache result: %.*s\n",
                          (char *)cstring_get_str(file_name),
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfs_read_memc: read %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

/* update in memory cache only */
EC_BOOL api_cmd_ui_crfs_update_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *file_name;
    CSTRING *file_content;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_cstring(param , 2, &file_content);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_update_memc: hsrfs %ld update file %s with content %s to memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_update_memc, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s has been updated to memcache\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

/* delete dir from memory cache only */
EC_BOOL api_cmd_ui_crfs_delete_dir_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *dname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    EC_BOOL   is_root_dir;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &dname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_dir_memc: hsrfs %ld delete dir %s from memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(dname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;


    des_log = api_cmd_ui_get_log(where);
 
    ret = EC_FALSE;

    /* ensure the dir to delete is NOT root dir / */
    is_root_dir = cstring_is_str((const CSTRING *)dname, (const UINT8 *)"/");
    if(EC_FALSE == is_root_dir)
    {
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_dir_memc, ERR_MODULE_ID, dname);
    }
    else
    {
        sys_log(des_log, "[ERRO] can NOT use this interface to delete root dir %s \n", (char *)cstring_get_str(dname));
        //sys_log(des_log, "[WARN] to delete root dir, use command: hsrfs <id> delete root dir / on tcid <tcid> at <console|log>\n");
    }

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete dir %s from memcache\n", (char *)cstring_get_str(dname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete dir %s\n", (char *)cstring_get_str(dname));
    }

    return (EC_TRUE);
}


/* delete root dir from memory cache only */
EC_BOOL api_cmd_ui_crfs_delete_root_dir_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *dname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    EC_BOOL   is_root_dir;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &dname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_root_dir_memc: hsrfs %ld delete root dir %s from memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(dname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;


    des_log = api_cmd_ui_get_log(where);
 
    ret = EC_FALSE;

    /* ensure the dir to delete is root dir / */
    is_root_dir = cstring_is_str((const CSTRING *)dname, (const UINT8 *)"/");
    if(EC_TRUE == is_root_dir)
    {
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_dir_memc, ERR_MODULE_ID, dname);
    }
    else
    {
        sys_log(des_log, "[ERRO] the dir %s to delete is not root dir / \n", (char *)cstring_get_str(dname));
    }
 
    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete root dir %s from memcache\n", (char *)cstring_get_str(dname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete dir %s\n", (char *)cstring_get_str(dname));
    }

    return (EC_TRUE);
}

/* delete file from memory cache only */
EC_BOOL api_cmd_ui_crfs_delete_file_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *fname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &fname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> delete file <name> on tcid <tcid> at <where>*/
    /*hsrfs %n delete file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_file_memc: hsrfs %ld delete file %s from memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_file_memc, ERR_MODULE_ID, fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete file %s from memcache\n", (char *)cstring_get_str(fname));
        sys_log(LOGCONSOLE, "[SUCC] delete file %s from memcache\n", (char *)cstring_get_str(fname));
        if(des_log == LOGCONSOLE)
        {
            sys_log(LOGCONSOLE, "[SUCC] des_log is CONSOLE\n");
        }
        else
        {
            sys_log(LOGCONSOLE, "[SUCC] des_log is NOT CONSOLE\n");
        }
    }
    else
    {
        sys_log(des_log, "[FAIL] delete file %s\n", (char *)cstring_get_str(fname));
        sys_log(LOGCONSOLE, "[FAIL] delete file %s\n", (char *)cstring_get_str(fname));
        if(des_log == LOGCONSOLE)
        {
            sys_log(LOGCONSOLE, "[FAIL] des_log is CONSOLE\n");
        }
        else
        {
            sys_log(LOGCONSOLE, "[FAIL] des_log is NOT CONSOLE\n");
        }
    }

    return (EC_TRUE);
}

/* delete path from memory cache only */
EC_BOOL api_cmd_ui_crfs_delete_path_memc(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs delete path <name> on tcid <tcid> at <where>*/
    /*hsrfs delete path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_path_memc: hsrfs %ld delete path %s from memcache on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete_memc, ERR_MODULE_ID, path_name, (UINT32)CRFSNP_ITEM_FILE_IS_ANY);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete path %s from memcache\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_delete_path(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *path_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &path_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs delete path <name> on tcid <tcid> at <where>*/
    /*hsrfs delete path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_delete_path: hsrfs %ld delete path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_delete, ERR_MODULE_ID, path_name, (UINT32)CRFSNP_ITEM_FILE_IS_ANY);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete path %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_recycle(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;
    UINT32     complete_num;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> recycle on tcid <tcid> at <where>*/
    /*hsrfs %n recycle on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_recycle: hsrfs %ld recycle on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_recycle, ERR_MODULE_ID, CRFS_RECYCLE_MAX_NUM, &complete_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] recycle completion num %ld\n", complete_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] recycle\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_retire(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   max_retire_num;
    UINT32   nsec;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;
    UINT32     complete_retire_num;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &max_retire_num);
    api_cmd_para_vec_get_uint32(param  , 2, &nsec);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> retire on tcid <tcid> at <where>*/
    /*hsrfs %n retire on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_retire: hsrfs %ld retire max %ld files created before %ld seconds on tcid %s at %s\n",
                        crfs_modi,
                        max_retire_num,
                        nsec,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    complete_retire_num = 0;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_retire, ERR_MODULE_ID, nsec, max_retire_num, CRFS_RETIRE_MAX_NUM, &complete_retire_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] retire %ld files\n", complete_retire_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] retire %ld files\n", complete_retire_num);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_flush(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> flush on tcid <tcid> at <where>*/
    /*hsrfs %n flush on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_flush: hsrfs %ld flush on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_flush, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] flush\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] flush\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_flush_npp(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> flush npp on tcid <tcid> at <where>*/
    /*hsrfs %n flush on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_flush_npp: hsrfs %ld flush npp on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_flush_npp, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] flush npp\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] flush npp\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_flush_dn(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> flush dn on tcid <tcid> at <where>*/
    /*hsrfs %n flush on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_flush_dn: hsrfs %ld flush dn on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_flush_dn, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] flush dn\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] flush dn\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_npp(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> show npp on tcid <tcid> at <where>*/
    /*hsrfs %n show npp on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_npp: hsrfs %ld show npp on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_show_npp, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_dn(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> show dn on tcid <tcid> at <where>*/
    /*hsrfs %n show dn on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_dn: hsrfs %ld show dn on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_show_dn, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_cached_np(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> show cached np on tcid <tcid> at <where>*/
    /*hsrfs %n show cached np on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_cached_np: hsrfs %ld show cached np on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_show_cached_np, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_specific_np(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_id;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &crfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> show specific np <id> on tcid <tcid> at <where>*/
    /*hsrfs %n show specific np %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_specific_np: hsrfs %ld show specific np %ld on tcid %s at %s\n",
                        crfs_modi,
                        crfsnp_id,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_show_specific_np, ERR_MODULE_ID, crfsnp_id, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] np %ld\n%s",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,
                           crfsnp_id,
                           (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] np %ld\n%s",
                          c_word_to_ipv4(crfsnp_tcid),
                          CMPI_CRFS_RANK, crfsnp_id,
                          (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_locked_files(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> show locked files on tcid <tcid> at <console|log>*/
    /*hsrfs %n show locked files on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_locked_files: hsrfs %ld locked files on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             NULL_PTR,
             FI_crfs_locked_files_print, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    sys_log(des_log, "[rank_%s_%ld][SUCC] \n%s",
                       c_word_to_ipv4(crfsnp_tcid),
                       CMPI_CRFS_RANK,
                       (char *)cstring_get_str(LOG_CSTR(log)));
    log_cstr_close(log);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_md5sum(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *fname;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;
    CMD5_DIGEST md5sum;

    char md5str[34];

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &fname);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> md5sum file <name> on tcid <tcid> at <where>*/
    /*hsrfs %n md5sum file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_md5sum: hsrfs %ld md5sum file %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_md5sum, ERR_MODULE_ID, fname, &md5sum);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] file %s, md5sum %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,
                           (char *)cstring_get_str(fname),
                           c_md5_to_hex_str(CMD5_DIGEST_SUM(&md5sum), md5str, sizeof(md5str))
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] file %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,      
                           (char *)cstring_get_str(fname)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_md5sum_b(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    CSTRING *fname;
    UINT32   seg_no;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;
    CMD5_DIGEST md5sum;

    char md5str[34];

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &fname);
    api_cmd_para_vec_get_uint32(param  , 2, &seg_no);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> md5sum bigfile <name> seg <no> on tcid <tcid> at <where>*/
    /*hsrfs %n md5sum bigfile %s seg %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_md5sum_b: hsrfs %ld md5sum file %s seg %u on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(fname),
                        seg_no,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_file_md5sum_b, ERR_MODULE_ID, fname, seg_no, &md5sum);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] bigfile %s, seg %u, md5sum %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,      
                           (char *)cstring_get_str(fname),
                           seg_no,
                           c_md5_to_hex_str(CMD5_DIGEST_SUM(&md5sum), md5str, sizeof(md5str))
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] bigfile %s, seg %u\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,      
                           (char *)cstring_get_str(fname),
                           seg_no
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_np_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_id;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;
 
    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &crfsnp_id);
    api_cmd_para_vec_get_cstring(param , 2, &des_path);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> snapshot np <id> to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot np %n to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_np_snapshot: hsrfs %ld snapshot np %u to path %s on tcid %s at %s\n",
                        crfs_modi,
                        crfsnp_id,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_np_snapshot, ERR_MODULE_ID, crfsnp_id, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] np %u, path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           crfsnp_id,
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] np %u, path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           crfsnp_id,
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_npp_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &des_path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> snapshot npp to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot npp to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_npp_snapshot: hsrfs %ld snapshot npp to path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_npp_snapshot, ERR_MODULE_ID, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_disk_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   disk_no;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_uint32(param  , 1, &disk_no);
    api_cmd_para_vec_get_cstring(param , 2, &des_path);
    api_cmd_para_vec_get_tcid(param    , 3, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfs <id> snapshot disk <id> to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot disk %n to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_disk_snapshot: hsrfs %ld snapshot disk %u to path %s on tcid %s at %s\n",
                        crfs_modi,
                        disk_no,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_disk_snapshot, ERR_MODULE_ID, disk_no, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] disk %u, path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           disk_no,
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] disk %u, path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           disk_no,
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_dn_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &des_path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> snapshot dn to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot dn to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_dn_snapshot: hsrfs %ld snapshot dn to path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_dn_snapshot, ERR_MODULE_ID, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_vol_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &des_path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> snapshot vol to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot vol to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_vol_snapshot: hsrfs %ld snapshot vol to path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_vol_snapshot, ERR_MODULE_ID, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_all_snapshot(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *des_path;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &des_path);
    api_cmd_para_vec_get_tcid(param    , 2, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfs <id> snapshot all to path <dir> on tcid <tcid> at <where>*/
    /*hsrfs %n snapshot all to path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_all_snapshot: hsrfs %ld snapshot all to path %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(des_path),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_all_snapshot, ERR_MODULE_ID, des_path);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] path %s\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK,  
                           (char *)cstring_get_str(des_path)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_create_backup(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *crfsnp_root_dir_bk;
    CSTRING *crfsdn_root_dir_bk;
    CSTRING *crfs_op_fname;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &crfsnp_root_dir_bk);
    api_cmd_para_vec_get_cstring(param , 2, &crfsdn_root_dir_bk);
    api_cmd_para_vec_get_cstring(param , 3, &crfs_op_fname);
    api_cmd_para_vec_get_tcid(param    , 4, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    /*hsrfs <id> create backup np <dir> dn <dir> op <fname> on tcid <tcid> at <where>*/
    /*hsrfs %n create backup np %s dn %s op %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_backup: hsrfs %ld create backup np %s dn %s op %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfsnp_root_dir_bk),
                        (char *)cstring_get_str(crfsdn_root_dir_bk),
                        (char *)cstring_get_str(crfs_op_fname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_create_backup, ERR_MODULE_ID, crfsnp_root_dir_bk, crfsdn_root_dir_bk, crfs_op_fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_open_backup(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *crfsnp_root_dir_bk;
    CSTRING *crfsdn_root_dir_bk;
    CSTRING *crfs_op_fname;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_cstring(param , 1, &crfsnp_root_dir_bk);
    api_cmd_para_vec_get_cstring(param , 2, &crfsdn_root_dir_bk);
    api_cmd_para_vec_get_cstring(param , 3, &crfs_op_fname);
    api_cmd_para_vec_get_tcid(param    , 4, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    /*hsrfs <id> open backup np <dir> dn <dir> op <fname> on tcid <tcid> at <where>*/
    /*hsrfs %n open backup np %s dn %s op %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_backup: hsrfs %ld open backup np %s dn %s op %s on tcid %s at %s\n",
                        crfs_modi,
                        (char *)cstring_get_str(crfsnp_root_dir_bk),
                        (char *)cstring_get_str(crfsdn_root_dir_bk),
                        (char *)cstring_get_str(crfs_op_fname),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_open_backup, ERR_MODULE_ID, crfsnp_root_dir_bk, crfsdn_root_dir_bk, crfs_op_fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_close_backup(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> close backup on tcid <tcid> at <where>*/
    /*hsrfs %n closebackup on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_create_backup: hsrfs %ld close backup on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_close_backup, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_start_sync(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> start sync on tcid <tcid> at <where>*/
    /*hsrfs %n start sync on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_start_sync: hsrfs %ld start sync on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_start_sync, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_end_sync(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> end sync on tcid <tcid> at <where>*/
    /*hsrfs %n end sync on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_end_sync: hsrfs %ld end sync on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_end_sync, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_replay(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> replay on tcid <tcid> at <where>*/
    /*hsrfs %n replay on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_replay: hsrfs %ld replay on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_replay, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n",
                           c_word_to_ipv4(crfsnp_tcid),
                           CMPI_CRFS_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfs_show_backup(CMD_PARA_VEC * param)
{
    UINT32   crfs_modi;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfs_modi);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfs <id> show backup on tcid <tcid> at <where>*/
    /*hsrfs %n show backup on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfs_show_backup: hsrfs %ld show backup on tcid %s at %s\n",
                        crfs_modi,
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFS_RANK;
    MOD_NODE_MODI(&mod_node) = crfs_modi;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfs_show_backup, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}
#endif

#if 1

#if 0
EC_BOOL api_cmd_ui_crfsc_create_npp(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_model;
    UINT32   crfscnp_max_num;
    UINT32   crfscnp_2nd_chash_algo_id; 
    CSTRING *crfscnp_db_root_dir;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &crfscnp_model);
    api_cmd_para_vec_get_uint32(param  , 1, &crfscnp_max_num);
    api_cmd_para_vec_get_uint32(param  , 2, &crfscnp_2nd_chash_algo_id);
    api_cmd_para_vec_get_cstring(param , 3, &crfscnp_db_root_dir);
    api_cmd_para_vec_get_tcid(param    , 4, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 5, &where);

    /*hsrfsc create np model <model> max num <np mum> with hash algo <id> and root <root dir> on tcid <tcid> at <where>*/
    /*hsrfsc create np model %n max num %n with hash algo %n and root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_create_npp: hsrfsc create np model %u max num %u with hash algo %u and root %s on tcid %s at %s\n",
                        crfscnp_model,
                        crfscnp_max_num,
                        crfscnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfscnp_db_root_dir),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_create_npp, ERR_MODULE_ID,
             crfscnp_model, crfscnp_max_num, crfscnp_2nd_chash_algo_id, crfscnp_db_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc create np model %u max num %u with hash algo %u and root %s on tcid %s successfully\n",
                        crfscnp_model,
                        crfscnp_max_num,
                        crfscnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfscnp_db_root_dir),
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc create np model %u max num %u with hash algo %u and root %s on tcid %s failed\n",
                        crfscnp_model,
                        crfscnp_max_num,
                        crfscnp_2nd_chash_algo_id,
                        (char *)cstring_get_str(crfscnp_db_root_dir),
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_create_dn(CMD_PARA_VEC * param)
{
    CSTRING *crfscnp_dn_root_dir;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &crfscnp_dn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc create dn with root <root dir> on tcid <tcid> at <where>*/
    /*hsrfsc create dn with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_create_dn: hsrfsc create dn with root %s on tcid %s at %s\n",
                        (char *)cstring_get_str(crfscnp_dn_root_dir),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_create_dn, ERR_MODULE_ID, crfscnp_dn_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc create dn with root %s on tcid %s successfully\n",
                        (char *)cstring_get_str(crfscnp_dn_root_dir),
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc create dn with root %s on tcid %s failed\n",
                        (char *)cstring_get_str(crfscnp_dn_root_dir),
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_add_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc add disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfsc add disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_add_disk: hsrfsc add disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_add_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc add disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc add disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_del_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc del disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfsc del disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_del_disk: hsrfsc del disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_del_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc del disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc del disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_mount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc mount disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfsc mount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_mount_disk: hsrfsc mount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_mount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc mount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc mount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_umount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc umount disk <disk no> on tcid <tcid> at <where>*/
    /*hsrfsc umount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_umount_disk: hsrfsc umount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_umount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hsrfsc umount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hsrfsc umount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(crfscnp_tcid));
    }

    return (EC_TRUE);
}

#endif
EC_BOOL api_cmd_ui_crfsc_open(CMD_PARA_VEC * param)
{
    CSTRING *crfsc_root_dir;
    UINT32   crfscnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_cstring(param , 0, &crfsc_root_dir);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);

    /*hsrfsc open from root <dir> on tcid <tcid>*/
    /*hsrfsc open from root %s on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_open: hsrfsc open from root %s on tcid %s\n",
                        (char *)cstring_get_str(crfsc_root_dir),
                        c_word_to_ipv4(crfscnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(crfscnp_tcid, CMPI_CRFSC_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*crfsc_md_id = 0*/

    task_act(mod_mgr_def, NULL_PTR, TASK_DEFAULT_LIVE, (UINT32)1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_crfsc_start, ERR_MODULE_ID, crfsc_root_dir);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_close(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);

    /*hsrfsc close on tcid <tcid>*/
    /*hsrfsc close on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_close: hsrfsc close on tcid %s\n",
                        c_word_to_ipv4(crfscnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(crfscnp_tcid, CMPI_CRFSC_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*crfsc_md_id = 0*/

    task_dea(mod_mgr_def, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, FI_crfsc_end, ERR_MODULE_ID);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_read(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    UINT32     expires_timestamp;
    EC_BOOL    need_expired_content;
    LOG       *des_log;
    EC_BOOL    ret;
 

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc read file <name> on tcid <tcid> at <where>*/
    /*hsrfsc read file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_read: hsrfsc read file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    cbytes = cbytes_new(0);

    need_expired_content = EC_TRUE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_read, ERR_MODULE_ID, file_name, cbytes, &expires_timestamp, need_expired_content);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfsc_read: read %s result: %.*s\n",
                          (char *)cstring_get_str(file_name),
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfsc_read: read %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_write(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    UINT32   expire_nsec;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_uint32(param  , 2, &expire_nsec);
    api_cmd_para_vec_get_tcid(param    , 3, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfsc write file <name> with content <string> on tcid <tcid> at <where>*/
    /*hsrfsc write file %s with content %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_write: hsrfsc write file %s with content %s and expire %ld seconds on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        expire_nsec,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_write, ERR_MODULE_ID, file_name, cbytes, expire_nsec);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_create_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    uint64_t file_size;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_uint64(param  , 1, &file_size);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc create bigfile <name> with size <file size> on tcid <tcid> at <console|log>*/
    /*hsrfsc create bigfile %s with size %N on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_create_b: hsrfsc create bigfile %s with size %lld on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        file_size,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_create_b, ERR_MODULE_ID, file_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfsc_read: craete bigfile %s done\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfsc_read: create bigfile %s failed\n", (char *)cstring_get_str(file_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_read_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    uint64_t offset;
    UINT32   max_len;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL    ret;

    UINT32    expires_timestamp;
    EC_BOOL   need_expired_content;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_uint64(param  , 1, &offset);
    api_cmd_para_vec_get_uint32(param  , 2, &max_len);
    api_cmd_para_vec_get_tcid(param    , 3, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfsc read bigfile <name> from offset <offset> max <max len> on tcid <tcid> at <console|log>*/
    /*hsrfsc read bigfile %s from offset %N max %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_read_b: hsrfsc read bigfile %s from offset %llu max %u on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        offset,
                        max_len,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    need_expired_content = EC_TRUE;
    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_read_b, ERR_MODULE_ID, file_name, &offset, max_len, cbytes, &expires_timestamp, need_expired_content);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfsc_read: read bigfile %s to offset %llu result: %.*s\n",
                          (char *)cstring_get_str(file_name), offset,
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfsc_read: read bigfile %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_write_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    uint64_t offset;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_uint64(param  , 2, &offset);
    api_cmd_para_vec_get_tcid(param    , 3, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hsrfsc write bigfile <name> with content <string> at offset <offset> on tcid <tcid> at <console|log>*/
    /*hsrfsc write bigfile %s with content %s at offset %N on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_write_b: hsrfsc write bigfile %s with content %s at offset %llu on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        offset,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_write_b, ERR_MODULE_ID, file_name, &offset, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] bigfile name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] bigfile name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_delete_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc delete bigfile <name> on tcid <tcid> at <console|log>*/
    /*hsrfsc delete bigfile %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_delete_b: hsrfsc delete bigfile %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_delete_file_b, ERR_MODULE_ID, file_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete bigfile name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete bigfile name %s\n", (char *)cstring_get_str(file_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_download_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *bigfile_name;
    uint64_t offset;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node; 
    LOG       *des_log;

    CBYTES    *cbytes;
    int        fd;

    api_cmd_para_vec_get_cstring(param , 0, &bigfile_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc download bigfile <name> to file <name> on tcid <tcid> at <console|log>*/
    /*hsrfsc download bigfile %s to file <name> on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_download_b: hsrfsc download bigfile %s to file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(bigfile_name),
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    fd = c_file_open((char *)cstring_get_str(file_name), O_RDWR | O_CREAT, 0666);
    if(ERR_FD == fd)
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfsc_download_b: open file %s failed\n",
                            (char *)cstring_get_str(file_name));
        return (EC_TRUE);                     
    } 

    c_file_truncate(fd, 0);

    cbytes = cbytes_new(0);
    ASSERT(NULL_PTR != cbytes); 

    offset = 0;
    for(;;)
    {
        EC_BOOL    ret;
        UINT32     max_len;
        UINT32     offset_t;
        UINT32     expires_timestamp;
        EC_BOOL    need_expired_content;     

        max_len  = CPGB_CACHE_MAX_BYTE_SIZE;
        offset_t = (UINT32)offset;

        need_expired_content = EC_TRUE;
        ret = EC_FALSE;     
     
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                 &mod_node,
                 &ret,
                 FI_crfsc_read_b, ERR_MODULE_ID, bigfile_name, &offset, max_len, cbytes, &expires_timestamp, need_expired_content);

        des_log = api_cmd_ui_get_log(where);

        if(EC_TRUE == ret)
        {
            sys_log(des_log, "[SUCC] api_cmd_ui_crfsc_download_b: download bigfile %s to offset %llu\n",
                              (char *)cstring_get_str(bigfile_name), offset);
        }
        else
        {
            sys_log(des_log, "[FAIL] api_cmd_ui_crfsc_download_b: download bigfile %s to offset %llu\n",
                              (char *)cstring_get_str(bigfile_name), offset);
        }  

        if(EC_FALSE == ret)
        {
            break;
        }

        if(0 == cbytes_len(cbytes))
        {
            break;
        }

        c_file_flush(fd, &offset_t, cbytes_len(cbytes), cbytes_buf(cbytes));
        cbytes_clean(cbytes);

        ASSERT(offset == offset_t);
    }

    cbytes_free(cbytes);
    c_file_close(fd);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_upload_b(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *bigfile_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node; 
    LOG       *des_log;
 
    uint64_t   file_size;
    uint64_t   offset;
    int        fd;
    CBYTES    *cbytes;
    uint8_t   *content;
    UINT32     content_max_len;
    EC_BOOL    ret;
 
    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &bigfile_name);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc upload file <name> to bigfile <name> on tcid <tcid> at <console|log>*/
    /*hsrfsc upload file %s to bigfile %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_upload_b: hsrfsc upload file %s to bigfile %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(bigfile_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    fd = c_file_open((char *)cstring_get_str(file_name), O_RDONLY, 0666);
    if(ERR_FD == fd)
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfsc_upload_b: open file %s failed\n",
                            (char *)cstring_get_str(file_name));
        return (EC_TRUE);                     
    }

    if(EC_FALSE == c_file_size_b(fd, &file_size))
    {
        dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "error:api_cmd_ui_crfsc_upload_b: get file %s size failed\n",
                            (char *)cstring_get_str(file_name));
 
        c_file_close(fd);
        return (EC_TRUE);
    }

    sys_log(LOGCONSOLE, "[DEBUG] api_cmd_ui_crfsc_upload_b: file %s, size %ld\n", (char *)cstring_get_str(file_name), file_size);

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_create_b, ERR_MODULE_ID, bigfile_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_crfsc_upload_b: create bigfile %s done\n", (char *)cstring_get_str(bigfile_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_crfsc_upload_b: create bigfile %s failed\n", (char *)cstring_get_str(bigfile_name));
    }

    if(EC_FALSE == ret)
    {
        c_file_close(fd);
        return (EC_TRUE);/*terminate*/
    }

    //content_max_len = (CPGB_CACHE_MAX_BYTE_SIZE / 2) - 7;
    content_max_len = CPGB_CACHE_MAX_BYTE_SIZE;

    content = safe_malloc(content_max_len, LOC_API_0433);     
    ASSERT(NULL_PTR != content);
 
    cbytes = cbytes_new(0);
    ASSERT(NULL_PTR != cbytes); 

    offset = 0; 
    while(offset < file_size)
    {     
        UINT32     content_len;
     
        UINT32     offset_t;

        offset_t = (UINT32)offset;
        c_file_read(fd, &offset_t, content_max_len, content);
        content_len = (UINT32)(offset_t - offset);

        //sys_log(des_log, "[DEBUG] api_cmd_ui_crfsc_upload_b: load data from %ld to %ld, len %ld\n", (UINT32)offset, offset_t, content_len);
     
        cbytes_mount(cbytes, content_len, content);

        ret = EC_FALSE;     
        task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
                 &mod_node,
                 &ret,
                 FI_crfsc_write_b, ERR_MODULE_ID, bigfile_name, &offset, cbytes);

        des_log = api_cmd_ui_get_log(where);

        if(EC_TRUE == ret)
        {
            sys_log(des_log, "[SUCC] import bigfile name %s to offset %lld\n", (char *)cstring_get_str(bigfile_name), offset);
        }
        else
        {
            sys_log(des_log, "[FAIL] import bigfile name %s to offset %lld\n", (char *)cstring_get_str(bigfile_name), offset);
        } 

        cbytes_umount(cbytes, NULL_PTR, NULL_PTR);     

        if(EC_FALSE == ret)
        {
            break;
        }
    }

    cbytes_free(cbytes);
    safe_free(content, LOC_API_0434);
    c_file_close(fd);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_search_file(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc search file <path> on tcid <tcid> at <where>*/
    /*hsrfsc search file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_search_file: hsrfsc search file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_find_file, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] search %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] search %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_search_dir(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc search dir <path> on tcid <tcid> at <where>*/
    /*hsrfsc search dir %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_search_dir: hsrfsc search dir %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_find_dir, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] search %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] search %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_count_file_size(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    uint64_t  file_size;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc count file size of <path> on tcid <tcid> at <where>*/
    /*hsrfsc count file size of %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_file_size: hsrfsc count file size of path %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_file_size, ERR_MODULE_ID, path_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file size of path %s: %llu\n", (char *)cstring_get_str(path_name), file_size);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file size of path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_qfile(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *where;
    UINT32   crfscnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CRFSNP_ITEM *crfscnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hsrfsc qfile %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(crfscnp_tcid, CMPI_ANY_COMM, CMPI_CRFSC_RANK, 0, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfsc_qfile beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_crfsc_qfile end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    crfscnp_item = crfsnp_item_new();
    ASSERT(NULL_PTR != crfscnp_item);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, crfscnp_tcid, &ret, FI_crfsc_qfile, ERR_MODULE_ID, file_name, crfscnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        crfsnp_item_print(des_log, crfscnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    crfsnp_item_free(crfscnp_item);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_delete_file(CMD_PARA_VEC * param)
{
    CSTRING *fname;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &fname);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc delete file <name> on tcid <tcid> at <where>*/
    /*hsrfsc delete file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_delete_file: hsrfsc delete file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_delete_file, ERR_MODULE_ID, fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete file %s\n", (char *)cstring_get_str(fname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete file %s\n", (char *)cstring_get_str(fname));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_delete_dir(CMD_PARA_VEC * param)
{
    CSTRING *dname;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dname);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc delete dir <name> on tcid <tcid> at <where>*/
    /*hsrfsc delete dir %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_delete_dir: hsrfsc delete dir %s on tcid %s at %s\n",
                        (char *)cstring_get_str(dname),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_delete_dir, ERR_MODULE_ID, dname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete dir %s\n", (char *)cstring_get_str(dname));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete dir %s\n", (char *)cstring_get_str(dname));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_delete_path(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc delete path <name> on tcid <tcid> at <where>*/
    /*hsrfsc delete path %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_delete_path: hsrfsc delete path %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_delete, ERR_MODULE_ID, path_name, (UINT32)CRFSNP_ITEM_FILE_IS_ANY);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete path %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_recycle(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc recycle on tcid <tcid> at <where>*/
    /*hsrfsc recycle on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_recycle: hsrfsc recycle on tcid %s at %s\n",
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_recycle, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] recycle\n");
    }
    else
    {
        sys_log(des_log, "[FAIL] recycle\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_md5sum(CMD_PARA_VEC * param)
{
    CSTRING *fname;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;
    CMD5_DIGEST md5sum;

    char md5str[34];

    api_cmd_para_vec_get_cstring(param , 0, &fname);
    api_cmd_para_vec_get_tcid(param    , 1, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc md5sum file <name> on tcid <tcid> at <where>*/
    /*hsrfsc md5sum file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_md5sum: hsrfsc md5sum file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(fname),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_file_md5sum, ERR_MODULE_ID, fname, &md5sum);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] file %s, md5sum %s\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,
                           (char *)cstring_get_str(fname),
                           c_md5_to_hex_str(CMD5_DIGEST_SUM(&md5sum), md5str, sizeof(md5str))
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] file %s\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(fname)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_md5sum_b(CMD_PARA_VEC * param)
{
    CSTRING *fname;
    UINT32   seg_no;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;
    CMD5_DIGEST md5sum;

    char md5str[34];

    api_cmd_para_vec_get_cstring(param , 0, &fname);
    api_cmd_para_vec_get_uint32(param  , 1, &seg_no);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc md5sum bigfile <name> seg <no> on tcid <tcid> at <where>*/
    /*hsrfsc md5sum bigfile %s seg %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_md5sum_b: hsrfsc md5sum file %s seg %u on tcid %s at %s\n",
                        (char *)cstring_get_str(fname),
                        seg_no,
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_file_md5sum_b, ERR_MODULE_ID, fname, seg_no, &md5sum);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] bigfile %s, seg %u, md5sum %s\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(fname),
                           seg_no,
                           c_md5_to_hex_str(CMD5_DIGEST_SUM(&md5sum), md5str, sizeof(md5str))
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] bigfile %s, seg %u\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(fname),
                           seg_no
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_add_dir(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   dir_tcid;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &dir_tcid);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc add dir <name> <tcid> on tcid <tcid> at <where>*/
    /*hsrfsc add dir %s %t on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_add_dir: hsrfsc add dir %s %s on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(dir_tcid),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_add_dir, ERR_MODULE_ID, dir_tcid, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] add dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] add dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_del_dir(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   dir_tcid;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &dir_tcid);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc del dir <name> <tcid> on tcid <tcid> at <where>*/
    /*hsrfsc del dir %s %t on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_del_dir: hsrfsc del dir %s %s on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(dir_tcid),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_del_dir, ERR_MODULE_ID, dir_tcid, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] del dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] del dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_has_dir(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   dir_tcid;
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &dir_tcid);
    api_cmd_para_vec_get_tcid(param    , 2, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hsrfsc has dir <name> <tcid> on tcid <tcid> at <where>*/
    /*hsrfsc has dir %s %t on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_has_dir: hsrfsc has dir %s %s on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(dir_tcid),
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_has_dir, ERR_MODULE_ID, dir_tcid, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] has dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] has dir '%s'\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK,      
                           (char *)cstring_get_str(dir_name)
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_clone_dt(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc clone dt on tcid <tcid> at <where>*/
    /*hsrfsc clone dt on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_clone_dt: hsrfsc clone dt on tcid %s at %s\n",
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_clone_dt, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] clone dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] clone dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_rollback_dt(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc rollback dt on tcid <tcid> at <where>*/
    /*hsrfsc rollback dt on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_rollback_dt: hsrfsc rollback dt on tcid %s at %s\n",
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_rollback_dt, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] rollback dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] rollback dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_flush_dt(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc flush dt on tcid <tcid> at <where>*/
    /*hsrfsc flush dt on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_flush_dt: hsrfsc flush dt on tcid %s at %s\n",
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_flush_dt, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] flush dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] flush dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_load_dt(CMD_PARA_VEC * param)
{
    UINT32   crfscnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &crfscnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc load dt on tcid <tcid> at <where>*/
    /*hsrfsc load dt on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_load_dt: hsrfsc load dt on tcid %s at %s\n",
                        c_word_to_ipv4(crfscnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfscnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_load_dt, ERR_MODULE_ID);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] load dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] load dt\n",
                           c_word_to_ipv4(crfscnp_tcid),
                           CMPI_CRFSC_RANK
                           );
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_show_dt(CMD_PARA_VEC * param)
{
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;

    api_cmd_para_vec_get_tcid(param    , 0, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hsrfsc show dt on tcid <tcid> at <where>*/
    /*hsrfsc show dt on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_show_dt: hsrfsc show dt on tcid %s at %s\n",
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             NULL_PTR,
             FI_crfsc_print_dt, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    sys_log(des_log, "[rank_%s_%ld]\n%s", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
    log_cstr_close(log);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_trans_dir_whole(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc trans dir <name> whole on tcid <tcid> at <where>*/
    /*hsrfsc trans dir %s whole on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_trans_dir_whole: hsrfsc trans dir %s whole on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_trans_dir_whole, ERR_MODULE_ID, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_trans_dir_prepare(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc trans dir <name> prepare on tcid <tcid> at <where>*/
    /*hsrfsc trans dir %s prepare on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_trans_dir_prepare: hsrfsc trans dir %s prepare on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_trans_dir_pre, ERR_MODULE_ID, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_trans_dir_handle(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc trans dir <name> handle on tcid <tcid> at <where>*/
    /*hsrfsc trans dir %s handle on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_trans_dir_handle: hsrfsc trans dir %s handle on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_trans_dir_handle, ERR_MODULE_ID, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_trans_dir_post(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc trans dir <name> post on tcid <tcid> at <where>*/
    /*hsrfsc trans dir %s post on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_trans_dir_post: hsrfsc trans dir %s post on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_trans_dir_post, ERR_MODULE_ID, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_crfsc_trans_dir_recycle(CMD_PARA_VEC * param)
{
    CSTRING *dir_name;
    UINT32   crfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &dir_name);
    api_cmd_para_vec_get_tcid(param    , 1, &crfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hsrfsc trans dir <name> recycle on tcid <tcid> at <where>*/
    /*hsrfsc trans dir %s recycle on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_crfsc_trans_dir_recycle: hsrfsc trans dir %s recycle on tcid %s at %s\n",
                        (char *)cstring_get_str(dir_name),
                        c_word_to_ipv4(crfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = crfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CRFSC_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_crfsc_trans_dir_recycle, ERR_MODULE_ID, dir_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n", c_word_to_ipv4(crfsnp_tcid),CMPI_CRFSC_RANK);
    }

    return (EC_TRUE);
}

#endif


#if 1
EC_BOOL api_cmd_ui_chfs_create_npp(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_model;
    UINT32   chfsnp_max_num;
    CSTRING *chfsnp_db_root_dir;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &chfsnp_model);
    api_cmd_para_vec_get_uint32(param  , 1, &chfsnp_max_num);
    api_cmd_para_vec_get_cstring(param , 2, &chfsnp_db_root_dir); 
    api_cmd_para_vec_get_tcid(param    , 3, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hshfs create np model <model> max num <np mum> with root <root dir> on tcid <tcid> at <where>*/
    /*hshfs create np model %n max num %n with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_create_npp: hshfs create np model %u max num %u with root %s on tcid %s at %s\n",
                        chfsnp_model,
                        chfsnp_max_num,
                        (char *)cstring_get_str(chfsnp_db_root_dir),                     
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_create_npp, ERR_MODULE_ID,
             chfsnp_model, chfsnp_max_num, chfsnp_db_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs create np model %u max num %u with root %s on tcid %s successfully\n",
                        chfsnp_model,
                        chfsnp_max_num,
                        (char *)cstring_get_str(chfsnp_db_root_dir),
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs create np model %u max num %u with root %s on tcid %s failed\n",
                        chfsnp_model,
                        chfsnp_max_num,
                        (char *)cstring_get_str(chfsnp_db_root_dir),
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_create_dn(CMD_PARA_VEC * param)
{
    CSTRING *chfsnp_dn_root_dir;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &chfsnp_dn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs create dn with root <root dir> on tcid <tcid> at <where>*/
    /*hshfs create dn with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_create_dn: hshfs create dn with root %s on tcid %s at %s\n",
                        (char *)cstring_get_str(chfsnp_dn_root_dir),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_create_dn, ERR_MODULE_ID, chfsnp_dn_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs create dn with root %s on tcid %s successfully\n",
                        (char *)cstring_get_str(chfsnp_dn_root_dir),
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs create dn with root %s on tcid %s failed\n",
                        (char *)cstring_get_str(chfsnp_dn_root_dir),
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_add_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs add disk <disk no> on tcid <tcid> at <where>*/
    /*hshfs add disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_add_disk: hshfs add disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_add_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs add disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs add disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_del_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs del disk <disk no> on tcid <tcid> at <where>*/
    /*hshfs del disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_del_disk: hshfs del disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_del_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs del disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs del disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_mount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs mount disk <disk no> on tcid <tcid> at <where>*/
    /*hshfs mount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_mount_disk: hshfs mount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_mount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs mount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs mount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_umount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs umount disk <disk no> on tcid <tcid> at <where>*/
    /*hshfs umount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_umount_disk: hshfs umount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_umount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hshfs umount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hshfs umount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(chfsnp_tcid));
    }

    return (EC_TRUE);
}
EC_BOOL api_cmd_ui_chfs_open(CMD_PARA_VEC * param)
{
    CSTRING *chfsnp_root_dir;
    CSTRING *chfsdn_root_dir;
    UINT32   chfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_cstring(param , 0, &chfsnp_root_dir);
    api_cmd_para_vec_get_cstring(param , 1, &chfsdn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 2, &chfsnp_tcid);

    /*hshfs open from np root <dir> and dn root <dir> on tcid <tcid>*/
    /*hshfs open from np root %s and dn root %s on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_open: hshfs open from np root %s and dn root %s on tcid %s\n",
                        (char *)cstring_get_str(chfsnp_root_dir),
                        (char *)cstring_get_str(chfsdn_root_dir),
                        c_word_to_ipv4(chfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(chfsnp_tcid, CMPI_CHFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*chfs_md_id = 0*/

    task_act(mod_mgr_def, NULL_PTR, TASK_DEFAULT_LIVE, (UINT32)1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_chfs_start, ERR_MODULE_ID, chfsnp_root_dir, chfsdn_root_dir);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_close(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);

    /*hshfs close on tcid <tcid>*/
    /*hshfs close on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_close: hshfs close on tcid %s\n",
                        c_word_to_ipv4(chfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(chfsnp_tcid, CMPI_CHFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*chfs_md_id = 0*/

    task_dea(mod_mgr_def, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, FI_chfs_end, ERR_MODULE_ID);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_read(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs read file <file name> on tcid <tcid> at <where>*/
    /*hshfs read file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_read: hshfs read file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_read, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_chfs_read: read %s result: \n%.*s\n",
                          (char *)cstring_get_str(file_name),
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_chfs_read: read %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_write(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_tcid(param    , 2, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hshfs write file <file name> with content <string> on tcid <tcid> at <where>*/
    /*hshfs write file %s with content %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_write: hshfs write file %s with content %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_write, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_search(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs search <file name> on tcid <tcid> at <where>*/
    /*hshfs search %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_search: hshfs search %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_search, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] search %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] search %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_qfile(CMD_PARA_VEC * param)
{
    UINT32   chfs_modi;
    CSTRING *file_name;
    CSTRING *where;
    UINT32   chfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CHFSNP_ITEM *chfsnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hshfs qfile %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    chfs_modi = 0; /*xx*/

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(chfsnp_tcid, CMPI_ANY_COMM, CMPI_CHFS_RANK, chfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_chfs_qfile beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_chfs_qfile end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    chfsnp_item = chfsnp_item_new();
    ASSERT(NULL_PTR != chfsnp_item);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, chfsnp_tcid, &ret, FI_chfs_qfile, ERR_MODULE_ID, file_name, chfsnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        chfsnp_item_print(des_log, chfsnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    chfsnp_item_free(chfsnp_item);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_count_file_num(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_tcid;
    CSTRING *where;

    UINT32   file_num;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hshfs count file num on tcid <tcid> at <where>*/
    /*hshfs count file num on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_count_file_num: hshfs count file num on tcid %s at %s\n",
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_file_num, ERR_MODULE_ID, &file_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file num: %ld\n", file_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file num\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_count_file_size(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    uint64_t  file_size;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs count file size of <path> on tcid <tcid> at <where>*/
    /*hshfs count file size of %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_file_size: hshfs count file size of path %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_file_size, ERR_MODULE_ID, path_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file size of path %s: %llu\n", (char *)cstring_get_str(path_name), file_size);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file size of path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_delete(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs delete file <file name> on tcid <tcid> at <where>*/
    /*hshfs delete file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_delete: hshfs delete file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_delete, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_recycle(CMD_PARA_VEC * param)
{
    UINT32   chfs_modi;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;
    UINT32     complete_num;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hshfs recycle on tcid <tcid> at <where>*/
    /*hshfs recycle on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_recycle: hshfs recycle on tcid %s at %s\n",
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    chfs_modi = 0; /*xx*/

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = chfs_modi;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_recycle, ERR_MODULE_ID, CHFS_RECYCLE_MAX_NUM, &complete_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] recycle completion num %ld\n", complete_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] recycle\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_retire(CMD_PARA_VEC * param)
{
    UINT32   chfs_modi;
    UINT32   max_retire_num;
    UINT32   nsec;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;
    UINT32     complete_retire_num;

    api_cmd_para_vec_get_uint32(param  , 0, &max_retire_num);
    api_cmd_para_vec_get_uint32(param  , 1, &nsec);
    api_cmd_para_vec_get_tcid(param    , 2, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hshfs retire max <num> files created before <n> seconds on tcid <tcid> at <where>*/
    /*hshfs retire max %n files created before %n seconds on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_retire: hshfs retire max %ld files created before %ld seconds on tcid %s at %s\n",
                        max_retire_num,
                        nsec,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    chfs_modi = 0; /*xx*/

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = chfs_modi;

    ret = EC_FALSE;
    complete_retire_num = 0;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_retire, ERR_MODULE_ID, nsec, max_retire_num, CHFS_RETIRE_MAX_NUM, &complete_retire_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] retire %ld files\n", complete_retire_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] retire %ld files\n", complete_retire_num);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_show_npp(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hshfs show npp on tcid <tcid> at <where>*/
    /*hshfs show npp on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_show_npp: hshfs show npp on tcid %s at %s\n",
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_show_npp, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_show_dn(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hshfs show dn on tcid <tcid> at <where>*/
    /*hshfs show dn on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_show_dn: hshfs show dn on tcid %s at %s\n",
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_show_dn, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_show_cached_np(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hshfs show cached np on tcid <tcid> at <where>*/
    /*hshfs show cached np on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_show_cached_np: hshfs show cached np on tcid %s at %s\n",
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_show_cached_np, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(chfsnp_tcid),CMPI_CHFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_chfs_show_specific_np(CMD_PARA_VEC * param)
{
    UINT32   chfsnp_id;
    UINT32   chfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &chfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 1, &chfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hshfs show specific np <id> on tcid <tcid> at <where>*/
    /*hshfs show specific np %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_chfs_show_specific_np: hshfs show specific np %ld on tcid %s at %s\n",
                        chfsnp_id,
                        c_word_to_ipv4(chfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = chfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CHFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_chfs_show_specific_np, ERR_MODULE_ID, chfsnp_id, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] np %ld\n%s",
                           c_word_to_ipv4(chfsnp_tcid),
                           CMPI_CHFS_RANK,
                           (char *)cstring_get_str(LOG_CSTR(log)),
                           chfsnp_id);
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] np %ld\n%s",
                          c_word_to_ipv4(chfsnp_tcid),
                          CMPI_CHFS_RANK,
                          (char *)cstring_get_str(LOG_CSTR(log)),
                          chfsnp_id);
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

#endif

#if 1
EC_BOOL api_cmd_ui_csfs_create_npp(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_model;
    UINT32   csfsnp_max_num;
    CSTRING *csfsnp_db_root_dir;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &csfsnp_model);
    api_cmd_para_vec_get_uint32(param  , 1, &csfsnp_max_num);
    api_cmd_para_vec_get_cstring(param , 2, &csfsnp_db_root_dir); 
    api_cmd_para_vec_get_tcid(param    , 3, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 4, &where);

    /*hssfs create np model <model> max num <np mum> with root <root dir> on tcid <tcid> at <where>*/
    /*hssfs create np model %n max num %n with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_create_npp: hssfs create np model %u max num %u with root %s on tcid %s at %s\n",
                        csfsnp_model,
                        csfsnp_max_num,
                        (char *)cstring_get_str(csfsnp_db_root_dir),                     
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_ALWAYS_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_create_npp, ERR_MODULE_ID,
             csfsnp_model, csfsnp_max_num, csfsnp_db_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs create np model %u max num %u with root %s on tcid %s successfully\n",
                        csfsnp_model,
                        csfsnp_max_num,
                        (char *)cstring_get_str(csfsnp_db_root_dir),
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs create np model %u max num %u with root %s on tcid %s failed\n",
                        csfsnp_model,
                        csfsnp_max_num,
                        (char *)cstring_get_str(csfsnp_db_root_dir),
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_create_dn(CMD_PARA_VEC * param)
{
    CSTRING *csfsnp_dn_root_dir;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &csfsnp_dn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs create dn with root <root dir> on tcid <tcid> at <where>*/
    /*hssfs create dn with root %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_create_dn: hssfs create dn with root %s on tcid %s at %s\n",
                        (char *)cstring_get_str(csfsnp_dn_root_dir),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_create_dn, ERR_MODULE_ID, csfsnp_dn_root_dir);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs create dn with root %s on tcid %s successfully\n",
                        (char *)cstring_get_str(csfsnp_dn_root_dir),
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs create dn with root %s on tcid %s failed\n",
                        (char *)cstring_get_str(csfsnp_dn_root_dir),
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_add_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs add disk <disk no> on tcid <tcid> at <where>*/
    /*hssfs add disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_add_disk: hssfs add disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_add_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs add disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs add disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_del_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs del disk <disk no> on tcid <tcid> at <where>*/
    /*hssfs del disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_del_disk: hssfs del disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_del_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs del disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs del disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_mount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs mount disk <disk no> on tcid <tcid> at <where>*/
    /*hssfs mount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_mount_disk: hssfs mount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_mount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs mount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs mount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_umount_disk(CMD_PARA_VEC * param)
{
    UINT32   disk_no;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_uint32(param  , 0, &disk_no);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs umount disk <disk no> on tcid <tcid> at <where>*/
    /*hssfs umount disk %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_umount_disk: hssfs umount disk %u on tcid %s at %s\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_umount_disk, ERR_MODULE_ID, disk_no);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] hssfs umount disk %u on tcid %s successfully\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }
    else
    {
        sys_log(des_log, "[FAIL] hssfs umount disk %u on tcid %s failed\n",
                        disk_no,
                        c_word_to_ipv4(csfsnp_tcid));
    }

    return (EC_TRUE);
}
EC_BOOL api_cmd_ui_csfs_open(CMD_PARA_VEC * param)
{
    CSTRING *csfsnp_root_dir;
    CSTRING *csfsdn_root_dir;
    UINT32   csfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_cstring(param , 0, &csfsnp_root_dir);
    api_cmd_para_vec_get_cstring(param , 1, &csfsdn_root_dir);
    api_cmd_para_vec_get_tcid(param    , 2, &csfsnp_tcid);

    /*hssfs open from np root <dir> and dn root <dir> on tcid <tcid>*/
    /*hssfs open from np root %s and dn root %s on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_open: hssfs open from np root %s and dn root %s on tcid %s\n",
                        (char *)cstring_get_str(csfsnp_root_dir),
                        (char *)cstring_get_str(csfsdn_root_dir),
                        c_word_to_ipv4(csfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(csfsnp_tcid, CMPI_CSFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*csfs_md_id = 0*/

    task_act(mod_mgr_def, NULL_PTR, TASK_DEFAULT_LIVE, (UINT32)1, LOAD_BALANCING_LOOP, TASK_PRIO_NORMAL,
             FI_csfs_start, ERR_MODULE_ID, csfsnp_root_dir, csfsdn_root_dir);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_close(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_tcid;

    MOD_MGR   *mod_mgr_def;

    api_cmd_para_vec_get_tcid(param    , 0, &csfsnp_tcid);

    /*hssfs close on tcid <tcid>*/
    /*hssfs close on tcid %t*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_close: hssfs close on tcid %s\n",
                        c_word_to_ipv4(csfsnp_tcid));

    mod_mgr_def = api_cmd_ui_gen_mod_mgr(csfsnp_tcid, CMPI_CSFS_RANK, CMPI_ERROR_TCID, CMPI_ERROR_RANK, 0);/*csfs_md_id = 0*/

    task_dea(mod_mgr_def, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, FI_csfs_end, ERR_MODULE_ID);
    mod_mgr_free(mod_mgr_def);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_read(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs read file <file name> on tcid <tcid> at <where>*/
    /*hssfs read file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_read: hssfs read file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_read, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] api_cmd_ui_csfs_read: read %s result: \n%.*s\n",
                          (char *)cstring_get_str(file_name),
                          cbytes_len(cbytes), (char *)cbytes_buf(cbytes));
    }
    else
    {
        sys_log(des_log, "[FAIL] api_cmd_ui_csfs_read: read %s failed\n", (char *)cstring_get_str(file_name));
    }

    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_write(CMD_PARA_VEC * param)
{
    CSTRING *file_name;
    CSTRING *file_content;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    CBYTES    *cbytes;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_cstring(param , 1, &file_content);
    api_cmd_para_vec_get_tcid(param    , 2, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*hssfs write file <file name> with content <string> on tcid <tcid> at <where>*/
    /*hssfs write file %s with content %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_write: hssfs write file %s with content %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        (char *)cstring_get_str(file_content),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;
    cbytes = cbytes_new(0);
 
    cbytes_mount(cbytes,cstring_get_len(file_content), cstring_get_str(file_content));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_write, ERR_MODULE_ID, file_name, cbytes);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] file name %s\n", (char *)cstring_get_str(file_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] file name %s\n", (char *)cstring_get_str(file_name));
    }

    cbytes_umount(cbytes, NULL_PTR, NULL_PTR);
    cbytes_free(cbytes);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_search(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs search <file name> on tcid <tcid> at <where>*/
    /*hssfs search %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_search: hssfs search %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_search, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] search %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] search %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_qfile(CMD_PARA_VEC * param)
{
    UINT32   csfs_modi;
    CSTRING *file_name;
    CSTRING *where;
    UINT32   csfsnp_tcid;

    MOD_MGR     *mod_mgr;
    TASK_MGR    *task_mgr;
    CSFSNP_ITEM *csfsnp_item;
    LOG         *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &file_name);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    dbg_log(SEC_0010_API, 5)(LOGSTDOUT, "hssfs qfile %s on tcid %s at %s\n",
                        (char *)cstring_get_str(file_name),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    csfs_modi = 0; /*xx*/

    mod_mgr = mod_mgr_new(CMPI_ERROR_MODI, LOAD_BALANCING_LOOP);
    mod_mgr_incl(csfsnp_tcid, CMPI_ANY_COMM, CMPI_CSFS_RANK, csfs_modi, mod_mgr);
#if 1
    if(do_log(SEC_0010_API, 5))
    {
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csfs_qfile beg ----------------------------------\n");
        mod_mgr_print(LOGSTDOUT, mod_mgr);
        sys_log(LOGSTDOUT, "------------------------------------ api_cmd_ui_csfs_qfile end ----------------------------------\n");
    }
#endif

    ret = EC_FALSE;
    csfsnp_item = csfsnp_item_new();
    ASSERT(NULL_PTR != csfsnp_item);

    task_mgr = task_new(mod_mgr, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP);
    task_tcid_inc(task_mgr, csfsnp_tcid, &ret, FI_csfs_qfile, ERR_MODULE_ID, file_name, csfsnp_item);
    task_wait(task_mgr, TASK_DEFAULT_LIVE, TASK_NOT_NEED_RESCHEDULE_FLAG, NULL_PTR);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        csfsnp_item_print(des_log, csfsnp_item);
    }
    else
    {
        sys_log(des_log, "[FAIL]\n");
    }

    csfsnp_item_free(csfsnp_item);

    mod_mgr_free(mod_mgr);

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_count_file_num(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_tcid;
    CSTRING *where;

    UINT32   file_num;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_tcid(param    , 0, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hssfs count file num on tcid <tcid> at <where>*/
    /*hssfs count file num on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_count_file_num: hssfs count file num on tcid %s at %s\n",
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_file_num, ERR_MODULE_ID, &file_num);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file num: %ld\n", file_num);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file num\n");
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_count_file_size(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    uint64_t  file_size;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL    ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs count file size of <path> on tcid <tcid> at <where>*/
    /*hssfs count file size of %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_file_size: hssfs count file size of path %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    ret = EC_FALSE;

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_file_size, ERR_MODULE_ID, path_name, &file_size);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] count file size of path %s: %llu\n", (char *)cstring_get_str(path_name), file_size);
    }
    else
    {
        sys_log(des_log, "[FAIL] count file size of path %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_delete(CMD_PARA_VEC * param)
{
    CSTRING *path_name;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &path_name);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs delete file <file name> on tcid <tcid> at <where>*/
    /*hssfs delete file %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_delete: hssfs delete file %s on tcid %s at %s\n",
                        (char *)cstring_get_str(path_name),
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_delete, ERR_MODULE_ID, path_name);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] delete %s\n", (char *)cstring_get_str(path_name));
    }
    else
    {
        sys_log(des_log, "[FAIL] delete %s\n", (char *)cstring_get_str(path_name));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_show_npp(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hssfs show npp on tcid <tcid> at <where>*/
    /*hssfs show npp on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_show_npp: hssfs show npp on tcid %s at %s\n",
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_show_npp, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_show_dn(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hssfs show dn on tcid <tcid> at <where>*/
    /*hssfs show dn on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_show_dn: hssfs show dn on tcid %s at %s\n",
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_show_dn, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_show_cached_np(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_tcid(param    , 0, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 1, &where);

    /*hssfs show cached np on tcid <tcid> at <where>*/
    /*hssfs show cached np on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_show_cached_np: hssfs show cached np on tcid %s at %s\n",
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_show_cached_np, ERR_MODULE_ID, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL]\n%s", c_word_to_ipv4(csfsnp_tcid),CMPI_CSFS_RANK, (char *)cstring_get_str(LOG_CSTR(log)));
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_csfs_show_specific_np(CMD_PARA_VEC * param)
{
    UINT32   csfsnp_id;
    UINT32   csfsnp_tcid;
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    LOG *log;
    EC_BOOL   ret;

    api_cmd_para_vec_get_uint32(param  , 0, &csfsnp_id);
    api_cmd_para_vec_get_tcid(param    , 1, &csfsnp_tcid);
    api_cmd_para_vec_get_cstring(param , 2, &where);

    /*hssfs show specific np <id> on tcid <tcid> at <where>*/
    /*hssfs show specific np %n on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_csfs_show_specific_np: hssfs show specific np %ld on tcid %s at %s\n",
                        csfsnp_id,
                        c_word_to_ipv4(csfsnp_tcid),
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = csfsnp_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = CMPI_CSFS_RANK;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
    log = log_cstr_open();
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_csfs_show_specific_np, ERR_MODULE_ID, csfsnp_id, log);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[rank_%s_%ld][SUCC] np %ld\n%s",
                           c_word_to_ipv4(csfsnp_tcid),
                           CMPI_CSFS_RANK,
                           (char *)cstring_get_str(LOG_CSTR(log)),
                           csfsnp_id);
        log_cstr_close(log);
    }
    else
    {
        sys_log(des_log, "[rank_%s_%ld][FAIL] np %ld\n%s",
                          c_word_to_ipv4(csfsnp_tcid),
                          CMPI_CSFS_RANK,
                          (char *)cstring_get_str(LOG_CSTR(log)),
                          csfsnp_id);
        log_cstr_close(log);
    }

    return (EC_TRUE);
}

#endif



EC_BOOL api_cmd_ui_download_file(CMD_PARA_VEC * param)
{
    UINT32   super_tcid;
    CSTRING *src_fname;
    CSTRING *des_fname; 
    CSTRING *where;

    MOD_NODE   mod_node;
    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &src_fname);
    api_cmd_para_vec_get_cstring(param , 1, &des_fname); 
    api_cmd_para_vec_get_tcid(param    , 2, &super_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*download file <src fname> to <des fname> from tcid <tcid>  at <console|log>*/
    /*download file %s to %s from tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_download_file: download file %s to %s from tcid %s at %s\n",
                        (char *)cstring_get_str(src_fname),
                        (char *)cstring_get_str(des_fname),
                        c_word_to_ipv4(super_tcid),                     
                        (char *)cstring_get_str(where));

    MOD_NODE_TCID(&mod_node) = super_tcid;
    MOD_NODE_COMM(&mod_node) = CMPI_ANY_COMM;
    MOD_NODE_RANK(&mod_node) = 0;
    MOD_NODE_MODI(&mod_node) = 0;

    ret = EC_FALSE;
 
    task_p2p(CMPI_ANY_MODI, TASK_DEFAULT_LIVE, TASK_PRIO_NORMAL, TASK_NEED_RSP_FLAG, TASK_NEED_ALL_RSP,
             &mod_node,
             &ret,
             FI_super_transfer, ERR_MODULE_ID, src_fname, CMPI_LOCAL_TCID, des_fname);

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] %s:%s -> %s:%s\n",
                           c_word_to_ipv4(super_tcid),
                           (char *)cstring_get_str(src_fname),
                           c_word_to_ipv4(CMPI_LOCAL_TCID),
                           (char *)cstring_get_str(des_fname));
    }
    else
    {
        sys_log(des_log, "[FAIL] %s:%s -> %s:%s\n",
                           c_word_to_ipv4(super_tcid),
                           (char *)cstring_get_str(src_fname),
                           c_word_to_ipv4(CMPI_LOCAL_TCID),
                           (char *)cstring_get_str(des_fname));
    }

    return (EC_TRUE);
}

EC_BOOL api_cmd_ui_upload_file(CMD_PARA_VEC * param)
{
    UINT32   super_tcid;
    CSTRING *src_fname;
    CSTRING *des_fname; 
    CSTRING *where;

    LOG       *des_log;

    EC_BOOL   ret;

    api_cmd_para_vec_get_cstring(param , 0, &src_fname);
    api_cmd_para_vec_get_cstring(param , 1, &des_fname); 
    api_cmd_para_vec_get_tcid(param    , 2, &super_tcid);
    api_cmd_para_vec_get_cstring(param , 3, &where);

    /*upload file <src fname> to <des fname> on tcid <tcid>  at <console|log>*/
    /*upload file %s to %s on tcid %t at %s*/
    dbg_log(SEC_0010_API, 9)(LOGSTDOUT, "[DEBUG] api_cmd_ui_download_file: upload file %s to %s on tcid %s at %s\n",
                        (char *)cstring_get_str(src_fname),
                        (char *)cstring_get_str(des_fname),
                        c_word_to_ipv4(super_tcid),                     
                        (char *)cstring_get_str(where));

    ret = super_transfer(0, src_fname, super_tcid, des_fname);  

    des_log = api_cmd_ui_get_log(where);

    if(EC_TRUE == ret)
    {
        sys_log(des_log, "[SUCC] %s:%s -> %s:%s\n",
                           c_word_to_ipv4(CMPI_LOCAL_TCID),
                           (char *)cstring_get_str(src_fname),
                           c_word_to_ipv4(super_tcid),
                           (char *)cstring_get_str(des_fname));
    }
    else
    {
        sys_log(des_log, "[FAIL] %s:%s -> %s:%s\n",
                           c_word_to_ipv4(CMPI_LOCAL_TCID),
                           (char *)cstring_get_str(src_fname),
                           c_word_to_ipv4(super_tcid),
                           (char *)cstring_get_str(des_fname));
    }

    return (EC_TRUE);
}

#ifdef __cplusplus
}
#endif/*__cplusplus*/

