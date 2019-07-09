#ifndef NGX_HTTP_SHM_DATA_STRUCTURES_MODULE_H
#define NGX_HTTP_SHM_DATA_STRUCTURES_MODULE_H

static char* ngx_http_insert_value(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char* ngx_http_print_tree(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void* ngx_http_shm_data_structures_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_shm_data_structures_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

#endif