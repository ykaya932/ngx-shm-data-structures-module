#ifndef NGX_MY_TREE_H
#define NGX_MY_TREE_H

typedef struct {
    ngx_rbtree_t       rbtree;
    ngx_rbtree_node_t  sentinel;
} my_tree_t;

typedef struct {
    ngx_rbtree_node_t 		   rbnode;
    ngx_int_t       	   	   val;
    ngx_hash_keys_arrays_t 	 hash_keys;
    ngx_hash_t      	     	 foo_hash;
    ngx_hash_init_t 		     hash;
    ngx_int_t                nelts;
    ngx_int_t                key;
} my_node_t;

typedef struct {
    ngx_rbtree_node_t	*value;
    ngx_queue_t			  queue;
} my_queue_t;

typedef struct{
    my_tree_t           *root;
    ngx_hash_t          foo_hash;
    ngx_hash_init_t     hash;
} ngx_http_total_shm_t;

typedef struct{
  ngx_shm_zone_t    *shm_zone;
} ngx_http_shm_data_structures_loc_conf_t;

void ngx_my_tree_insert_value(ngx_rbtree_node_t *temp,ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel);
ngx_rbtree_node_t* init_node(ngx_int_t id, ngx_http_request_t *r,ngx_http_shm_data_structures_loc_conf_t *lccf);
my_node_t* find_node(my_tree_t* root, ngx_int_t value, ngx_int_t key, ngx_http_request_t *r);
ngx_str_t print_tree(my_tree_t* root,ngx_http_request_t* r,ngx_http_total_shm_t* data);
ngx_str_t print_hash_table(my_node_t *n, ngx_http_request_t *r);
ngx_int_t create_key(ngx_int_t val);


#endif