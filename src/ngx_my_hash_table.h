#ifndef NGX_MY_HASH_TABLE_MODULE_H
#define NGX_MY_HASH_TABLE_MODULE_H
#define NGX_HASH_ELT_SIZE(name)                                               \
     (sizeof(void *) + ngx_align((name)->key.len + 2, sizeof(void *)))

typedef struct {
	ngx_int_t ind;
	ngx_int_t value;
} my_data_t;
     
ngx_int_t ngx_shm_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key, void *value, ngx_uint_t flags,ngx_http_request_t *r);
ngx_inline ngx_int_t ngx_shm_array_init(ngx_array_t *array, ngx_pool_t *pool, ngx_uint_t n, size_t size);
ngx_int_t ngx_shm_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_shm_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names, ngx_uint_t nelts,ngx_http_request_t *r);
void * ngx_shm_array_push(ngx_array_t *a);

#endif