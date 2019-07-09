#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_shm_data_structures_module.h"
#include "ngx_my_tree.h"
#include "ngx_my_hash_table.h"

static ngx_command_t ngx_http_shm_data_structures_commands[] = {
    { ngx_string("insert"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_insert_value, 
      0,
      0,
      NULL},
    { ngx_string("print-tree"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_print_tree, 
      0,
      0,
      NULL},
      ngx_null_command
};

static ngx_http_module_t ngx_http_shm_data_structures_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

	ngx_http_shm_data_structures_create_loc_conf, 
    ngx_http_shm_data_structures_merge_loc_conf
};

ngx_module_t ngx_http_shm_data_structures_module = {
    NGX_MODULE_V1,
    &ngx_http_shm_data_structures_module_ctx, /* module context */
    ngx_http_shm_data_structures_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_insert_value_handler(ngx_http_request_t *r)
{    
	u_char  *result;
    int     len = 0;

    ngx_http_shm_data_structures_loc_conf_t     *lccf;
    ngx_shm_zone_t 					            *shm_zone;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_shm_data_structures_module);

    result = ngx_pnalloc(r->pool, sizeof(unsigned char)*100);

    if(lccf->shm_zone == NULL){
        return NGX_DECLINED;
    }

    ngx_int_t id = ngx_atoi(r->uri.data+1,1);
    ngx_int_t to_inserted = ngx_atoi(r->uri.data+3,r->uri.len-3);

    ngx_str_t *to_inserted_str;

    shm_zone = lccf->shm_zone;
    my_tree_t *shm_root;
    shm_root = ((ngx_http_total_shm_t*)shm_zone->data)->root;


	my_node_t* found = find_node(shm_root,id,create_key(id),r);

	if(found == NULL){

	    ngx_rbtree_node_t  *node;
	    node = init_node(id, r, lccf);

	    ngx_rbtree_insert(&(shm_root->rbtree), node);

	    found = find_node(shm_root,id,create_key(id),r);

	    memcpy(result+len,r->uri.data+1,1); len += 1;
	    strcpy((char *)result+len," is inserted.");  len += strlen(" is inserted.");
	}
	else{
	    memcpy(result+len,r->uri.data+1,1); len += 1;
	    strcpy((char *)result+len," is already inserted.");  len += strlen(" is already inserted.");
	}

	if(found == NULL)
		return NGX_ERROR;

	my_data_t *my_data;

	my_data = ngx_slab_alloc((ngx_slab_pool_t *)shm_zone->shm.addr,sizeof(my_data_t));
	to_inserted_str 		= ngx_slab_alloc((ngx_slab_pool_t *)shm_zone->shm.addr,sizeof(ngx_str_t));
	to_inserted_str->data 	= ngx_slab_alloc((ngx_slab_pool_t *)shm_zone->shm.addr,sizeof(char)*20);

	memcpy(to_inserted_str->data,r->uri.data,r->uri.len);
	to_inserted_str->len = r->uri.len;

	my_data->value 	= to_inserted;
	my_data->ind 	= (++found->nelts);
   	

	if(ngx_shm_hash_add_key(&found->hash_keys, to_inserted_str, my_data, NGX_HASH_READONLY_KEY,r) == NGX_ERROR)
		found->nelts--;

   	print_tree(shm_root,r,((ngx_http_total_shm_t*)shm_zone->data));

//Response stuff starts here

    ngx_buf_t *b;
    ngx_chain_t out;

    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *) "text/plain";

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out.buf = b;

    out.next = NULL;
   
    b->pos = result; 
    b->last = result + len;
    b->memory = 1; 
    b->last_buf = 1; 

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = len;
    ngx_http_send_header(r);

    return ngx_http_output_filter(r, &out);

}
static ngx_int_t ngx_http_print_tree_handler(ngx_http_request_t *r)
{

	ngx_http_shm_data_structures_loc_conf_t *lccf;
    ngx_shm_zone_t *shm_zone;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_shm_data_structures_module);

    if(lccf->shm_zone == NULL){
        return NGX_DECLINED;
    }
    shm_zone = lccf->shm_zone;

    my_tree_t* shm_root = ((ngx_http_total_shm_t*)shm_zone->data)->root;


//Response stuff starts here

    ngx_str_t result = print_tree(shm_root,r,((ngx_http_total_shm_t*)shm_zone->data));

    ngx_buf_t *b;
    ngx_chain_t out;

    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *) "text/plain";

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out.buf = b;

    out.next = NULL;

    b->pos = result.data; 
    b->last = result.data + result.len;
    b->memory = 1; 
    b->last_buf = 1; 

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = result.len;
    ngx_http_send_header(r);

    return ngx_http_output_filter(r, &out);
}
static char* ngx_http_insert_value(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_insert_value_handler;

    return NGX_CONF_OK;
}
static char* ngx_http_print_tree(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_print_tree_handler;

    return NGX_CONF_OK;
}
static ngx_int_t ngx_http_shm_data_structures_init_shm_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    if(data){
        shm_zone->data = data;
        return NGX_OK;
    }

    ngx_http_total_shm_t* shm;
    shm = ngx_slab_alloc((ngx_slab_pool_t *)shm_zone->shm.addr, sizeof(ngx_http_total_shm_t));

  	shm->root = ngx_slab_alloc((ngx_slab_pool_t *) shm_zone->shm.addr, sizeof(my_tree_t));
	ngx_rbtree_init(&(shm->root->rbtree), &(shm->root->sentinel), ngx_my_tree_insert_value);


    shm_zone->data = shm;

    return NGX_OK;
}


static void* ngx_http_shm_data_structures_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_total_shm_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_total_shm_t));

    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    return conf;
}

static char* ngx_http_shm_data_structures_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_shm_data_structures_loc_conf_t *prev = parent;
    ngx_http_shm_data_structures_loc_conf_t *conf = child;
	
	ngx_shm_zone_t  	*shm_zone;
    ngx_str_t 			*shm_name;

    shm_name 	= ngx_palloc(cf->pool, sizeof *shm_name);

    shm_name->len   = sizeof("shm_data_structure") - 1;
    shm_name->data  = (unsigned char *) "shm_data_structure";

    shm_zone 	    = ngx_shared_memory_add(cf, shm_name, 1024 * ngx_pagesize, &ngx_http_shm_data_structures_module);

    if(shm_zone == NULL){
        return NGX_CONF_ERROR;
    }

    shm_zone->init 	= ngx_http_shm_data_structures_init_shm_zone;
    conf->shm_zone 	= shm_zone;

    ngx_conf_merge_ptr_value(conf->shm_zone, prev->shm_zone, NULL);

	return NGX_CONF_OK;
}