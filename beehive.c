/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_beehive.h"

#define BEEHIVE_FIXED_HEADER_LEN 28
#if PHP_MAJOR_VERSION < 7
static inline int beehive_zend_hash_find(HashTable *ht, char *k, int len, void **v)
{
    zval **tmp = NULL;
    if (zend_hash_find(ht, k, len, (void **) &tmp) == SUCCESS)
    {
        *v = *tmp;
        return SUCCESS;
    }
    else
    {
        *v = NULL;
        return FAILURE;
    }
}
#else
static inline int beehive_zend_hash_find(HashTable *ht, char *k, int len, void **v)
{
    zval *value = zend_hash_str_find(ht, k, len - 1);
    if (value == NULL)
    {
        return FAILURE;
    }
    else
    {
        *v = (void *) value;
        return SUCCESS;
    }
}
#endif

#define php_beehive_array_get_value(ht, str, v) (beehive_zend_hash_find(ht, str, sizeof(str), (void **) &v) == SUCCESS && !ZVAL_IS_NULL(v))

/* If you declare any globals in php_beehive.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(beehive)
*/

/* True global resources - no need for thread safety here */
static int le_beehive;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("beehive.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_beehive_globals, beehive_globals)
    STD_PHP_INI_ENTRY("beehive.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_beehive_globals, beehive_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_beehive_compiled(string arg)
   Return a string to confirm that the module is compiled in */

typedef struct {
    uint32_t packet_len;   //定义包体长度,后续的包长度,不包括自身
    uint16_t header_len;   //定义包头长度后续到BODY的边界长度，不包括自身
    uint16_t flag; //协议标记
    uint32_t service;   //服务id
    uint32_t time;   //调用时间(s)
    uint32_t uniqid;   //单机下调用时间下的唯一值
    uint32_t askid;   //请求编号，反端响应原路返回
    uint16_t code;   //协议结果0表示成功，其它失败,一般用于返回
    uint8_t routers;     //路由总数，用于标记路由的线路
    uint8_t dst_mode;  //目标查找模式 0--默认 1--HASH查找 2--指定目标
} Header;

PHP_FUNCTION(beehive_packet_unpack)
{
    char *arg = NULL;
    int arg_len, len;
    zval *router_items;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
        return;
    }
    array_init(return_value);
    if (arg_len < 6) {
        return php_error_docref(NULL, E_WARNING, "packet lenght is to short!");
    }

    Header *header = (Header*) arg;

    uint32_t packet_len = ntohl(header->packet_len);
    if (arg_len < packet_len) {
        return php_error_docref(NULL, E_WARNING, "packet header lenght is to short!");
    }
    MAKE_STD_ZVAL(router_items);
    array_init(router_items);
    add_assoc_long(return_value, "packet_len", packet_len);
    add_assoc_long(return_value, "header_len", ntohs(header->header_len));
    add_assoc_long(return_value, "flag", ntohs(header->flag));
    add_assoc_long(return_value, "service", ntohl(header->service));
    add_assoc_long(return_value, "time", ntohl(header->time));
    add_assoc_long(return_value, "uniqid", ntohl(header->uniqid));
    add_assoc_long(return_value, "askid", ntohs(header->askid));
    add_assoc_long(return_value, "code", ntohs(header->code));
    add_assoc_long(return_value, "routers", header->routers);
    add_assoc_long(return_value, "dst_mode", header->dst_mode);
    //开始解 routers;
    uint64_t router_item = 0;
    int start = BEEHIVE_FIXED_HEADER_LEN;
    for (int i = 0; i < header->routers; i ++) {
        router_item = *(uint64_t*) (arg + start);
        router_item = ntohll(router_item);
        start = start + 8;
        php_printf("router_item %llu >>", router_item); 
        add_next_index_long(router_items, router_item);
    }
    uint64_t dst = 0;
    if (header->dst_mode > 0) {
        dst = *(uint64_t*) (arg + start);
        dst = ntohll(dst);
        start = start + 8;
    }
    php_printf("@start %d>> ", start); 
    add_assoc_long(return_value, "dst", dst);
    add_assoc_zval(return_value, "routers_list", router_items);
    add_assoc_stringl(return_value, "body", arg + start, packet_len - ntohs(header->header_len) - 2, 1);
}

PHP_FUNCTION(beehive_packet_pack)
{
    zval *zset = NULL;
    HashTable *vht;
    zval *v;
    zval **data;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zset) == FAILURE)
    {
        return;
    }
    vht = Z_ARRVAL_P(zset);

    Header header;
    memset(&header, 0, sizeof(header));
    if (php_beehive_array_get_value(vht, "flag", v)) {
        convert_to_long(v);
        header.flag = htons((uint16_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "service", v)) {
        convert_to_long(v);
        header.service = htonl((uint32_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "time", v)) {
        convert_to_long(v);
        header.time = htonl((uint32_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "uniqid", v)) {
        convert_to_long(v);
        header.uniqid = htonl((uint32_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "askid", v)) {
        convert_to_long(v);
        header.askid = htonl((uint32_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "code", v)) {
        convert_to_long(v);
        header.code = htons((uint16_t) Z_LVAL_P(v));
    }

    if (php_beehive_array_get_value(vht, "dst_mode", v)) {
        convert_to_long(v);
        header.dst_mode = (uint8_t) Z_LVAL_P(v);
        php_printf("The array passed contains %d elements ", header.dst_mode); 
    }

    //获取数由表
    HashTable *router_table = NULL;
    if (php_beehive_array_get_value(vht, "routers_list", v)) {
        router_table = Z_ARRVAL_P(v);
        header.routers = zend_hash_num_elements(router_table);
    }
    //计算包长
    int header_len = BEEHIVE_FIXED_HEADER_LEN - 6 + header.routers * 8;
    if (header.dst_mode > 0) {
        header_len = header_len + 8;
    }

    uint32_t body_len = 0;
    if (php_beehive_array_get_value(vht, "body", v)) {
        convert_to_string(v);
        body_len = strlen(Z_STRVAL_P(v));
    }

    header.header_len = htons(header_len);
    uint32_t packet_len_total = header_len + body_len + 6;
    header.packet_len = htonl(packet_len_total - 4);

    php_printf("Header %d header_len \n", header_len);
    php_printf("Header %d packet_len \n", packet_len_total);

    char *ret = (char*)emalloc(header_len + body_len + 6);
    memcpy(ret, &header, sizeof(header));
    int start = BEEHIVE_FIXED_HEADER_LEN;
    if (header.routers > 0) {
        HashPosition pointer;
        uint64_t router_item = 0;
        php_printf("The array passed contains %d elements ", header.routers);  
        for(zend_hash_internal_pointer_reset_ex(router_table, &pointer);
            zend_hash_get_current_data_ex(router_table, (void**) &data, &pointer) == SUCCESS;
            zend_hash_move_forward_ex(router_table, &pointer)) {
            convert_to_long_ex(data);
            router_item = htonll(Z_LVAL_PP(data));
            memcpy(ret + start, &router_item, 8);
            start = start + 8;
        }
    }
    if (header.dst_mode > 0) {
        uint64_t dst = 0;
        if (php_beehive_array_get_value(vht, "dst", v)) {
            convert_to_long(v);
            dst = htonll(Z_LVAL_P(v));
        }
        memcpy(ret + start, &dst, 8);
        start = start + 8;
    }
    if (php_beehive_array_get_value(vht, "body", v)) {
        //convert_to_string(v);
        memcpy(ret + start, Z_STRVAL_P(v), body_len);
    }

    RETURN_STRINGL(ret, packet_len_total, 0);
}

// array_init(return_value);
//         add_assoc_long(return_value, "port", ntohs(cli->remote_addr.addr.inet_v4.sin_port));
//         sw_add_assoc_string(return_value, "host", inet_ntoa(cli->remote_addr.addr.inet_v4.sin_addr), 1);

// PHP_FUNCTION(confirm_beehive_compiled)
// {
// 	char *arg = NULL;
// 	int arg_len, len;
// 	char *strg;

// 	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
// 		return;
// 	}

// 	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "beehive", arg);
// 	RETURN_STRINGL(strg, len, 0);
// }
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_beehive_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_beehive_init_globals(zend_beehive_globals *beehive_globals)
{
	beehive_globals->global_value = 0;
	beehive_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(beehive)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(beehive)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(beehive)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(beehive)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(beehive)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "beehive support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ beehive_functions[]
    * Every user visible function must have an entry in beehive_functions[].
 */
const zend_function_entry beehive_functions[] = {
	//PHP_FE(confirm_beehive_compiled,	NULL)		/* For testing, remove later. */
    PHP_FE(beehive_packet_pack, NULL)
    PHP_FE(beehive_packet_unpack, NULL)
	PHP_FE_END	/* Must be the last line in beehive_functions[] */
};
/* }}} */

/* {{{ beehive_module_entry
 */
zend_module_entry beehive_module_entry = {
	STANDARD_MODULE_HEADER,
	"beehive",
	beehive_functions,
	PHP_MINIT(beehive),
	PHP_MSHUTDOWN(beehive),
	PHP_RINIT(beehive),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(beehive),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(beehive),
	PHP_BEEHIVE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_BEEHIVE
ZEND_GET_MODULE(beehive)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
