#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_my_tree.h"
#include "ngx_my_hash_table.h"

//Editted version of ngx_hash_init for memory allocation in shared memory zone
ngx_int_t ngx_shm_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names, ngx_uint_t nelts,ngx_http_request_t *r)
{
    u_char          *elts;
    size_t           len;
    u_short         *test;
    ngx_uint_t       i, n, key, size, start, bucket_size;
    ngx_hash_elt_t  *elt, **buckets;
    if (hinit->max_size == 0) {
        ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0,
                      "could not build %s, you should "
                      "increase %s_max_size: %i",
                      hinit->name, hinit->name, hinit->max_size);
        return NGX_ERROR;
    }

    for (n = 0; n < nelts; n++) {
        if (hinit->bucket_size < NGX_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
        {
            ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0,
                          "could not build %s, you should "
                          "increase %s_bucket_size: %i",
                          hinit->name, hinit->name, hinit->bucket_size);
            return NGX_ERROR;
        }
    }

    test = ngx_slab_alloc((ngx_slab_pool_t *) hinit->pool, hinit->max_size * sizeof(u_short));

    if (test == NULL) {
        return NGX_ERROR;
    }

    bucket_size = hinit->bucket_size - sizeof(void *);

    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;

    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

    for (size = start; size <= hinit->max_size; size++) {

        ngx_memzero(test, size * sizeof(u_short));

        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            key = names[n].key_hash % size;
            test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));

            if (test[key] > (u_short) bucket_size) {
                goto next;
            }
        }

        goto found;

    next:

        continue;
    }

    size = hinit->max_size;

    ngx_log_error(NGX_LOG_WARN, hinit->pool->log, 0,
                  "could not build optimal %s, you should increase "
                  "either %s_max_size: %i or %s_bucket_size: %i; "
                  "ignoring %s_bucket_size",
                  hinit->name, hinit->name, hinit->max_size,
                  hinit->name, hinit->bucket_size, hinit->name);

found:

    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *);
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }

    len = 0;

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        test[i] = (u_short) (ngx_align(test[i], ngx_cacheline_size));

        len += test[i];
    }

    if (hinit->hash == NULL) {
        hinit->hash = ngx_slab_alloc((ngx_slab_pool_t *)hinit->pool, sizeof(ngx_hash_wildcard_t)
                                             + size * sizeof(ngx_hash_elt_t *));
        if (hinit->hash == NULL) {
            ngx_slab_free((ngx_slab_pool_t *) hinit->pool,test);
            return NGX_ERROR;
        }

        buckets = (ngx_hash_elt_t **)
                      ((u_char *) hinit->hash + sizeof(ngx_hash_wildcard_t));

    } else {
        buckets = ngx_slab_alloc((ngx_slab_pool_t *)hinit->pool, size * sizeof(ngx_hash_elt_t *));
        if (buckets == NULL) {
            ngx_slab_free((ngx_slab_pool_t *) hinit->pool,test);
            return NGX_ERROR;
        }
    }

    elts = ngx_slab_alloc((ngx_slab_pool_t *)hinit->pool, len + ngx_cacheline_size);
    if (elts == NULL) {
        ngx_slab_free((ngx_slab_pool_t *) hinit->pool,test);
        return NGX_ERROR;
    }

    elts = ngx_align_ptr(elts, ngx_cacheline_size);

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        buckets[i] = (ngx_hash_elt_t *) elts;
        elts += test[i];
    }

    for (i = 0; i < size; i++) {
        test[i] = 0;
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        elt = (ngx_hash_elt_t *) ((u_char *) buckets[key] + test[key]);

        elt->value = names[n].value;
        elt->len = (u_short) names[n].key.len;

        ngx_strlow(elt->name, names[n].key.data, names[n].key.len);

        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }

    for (i = 0; i < size; i++) {
        if (buckets[i] == NULL) {
            continue;
        }

        elt = (ngx_hash_elt_t *) ((u_char *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    ngx_slab_free((ngx_slab_pool_t *) hinit->pool,test);

    hinit->hash->buckets = buckets;
    hinit->hash->size = size;

    return NGX_OK;
}

//Editted version of ngx_hash_keys_array_init for memory allocation in shared memory zone
 ngx_int_t ngx_shm_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type)
 {
    ngx_uint_t  asize;

    if (type == NGX_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

   } else {
        asize = NGX_HASH_LARGE_ASIZE;
        ha->hsize = NGX_HASH_LARGE_HSIZE;
    }
 
     if (ngx_shm_array_init(&ha->keys, ha->pool, asize, sizeof(ngx_hash_key_t))
         != NGX_OK)
     {
         return NGX_ERROR;
     } 
     ha->keys_hash = ngx_slab_alloc((ngx_slab_pool_t *)ha->pool, sizeof(ngx_array_t) * ha->hsize);
     if (ha->keys_hash == NULL) {
         return NGX_ERROR;
     } 
     return NGX_OK;
 }

//Editted version of ngx_array_push for memory allocation in shared memory zone
 void * ngx_shm_array_push(ngx_array_t *a)
 {
     void        *elt, *new;
     size_t       size;
     ngx_pool_t  *p;
 
     if (a->nelts == a->nalloc) {
  
         size = a->size * a->nalloc;
 
         p = a->pool;
 
         if ((u_char *) a->elts + size == p->d.last
             && p->d.last + a->size <= p->d.end)
         {
             
              
 
             p->d.last += a->size;
             a->nalloc++;
 
         } else {

 
             new = ngx_slab_alloc((ngx_slab_pool_t *)p, 2 * size);
             if (new == NULL) {
                 return NULL;
             }
 
             ngx_memcpy(new, a->elts, size);
             a->elts = new;
             a->nalloc *= 2;
         }
     }
 
     elt = (u_char *) a->elts + a->size * a->nelts;
     a->nelts++;
 
     return elt;
 }

//Editted version of ngx_array_init for memory allocation in shared memory zone
ngx_inline ngx_int_t ngx_shm_array_init(ngx_array_t *array, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{ 
     array->nelts = 0;
     array->size = size;
     array->nalloc = n;
     array->pool = pool;
     array->elts = ngx_slab_alloc((ngx_slab_pool_t *)pool, n * size);
     if (array->elts == NULL) {
         return NGX_ERROR;
     }
 
     return NGX_OK;
 }
//Editted version of ngx_hash_add_key for memory allocation in shared memory zone. Also wildcard match option are removed

ngx_int_t ngx_shm_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key, void *value, ngx_uint_t flags,ngx_http_request_t *r)
{
    ngx_str_t       *name;
    ngx_uint_t       i, k, last;
    ngx_hash_key_t  *hk;

    last = key->len;

    k = 0;

    for (i = 0; i < last; i++) {
        k = ngx_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            
            if (last != name[i].len) {
                continue;
            }

            if (ngx_strncmp(key->data, name[i].data, last) == 0) {
                return NGX_ERROR;
            }
        }

    } else {

        if (ngx_shm_array_init(&ha->keys_hash[k], ha->pool, 4,
                           sizeof(ngx_str_t))
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    name = ngx_shm_array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return NGX_ERROR;
    }

    *name = *key;

    hk = ngx_shm_array_push(&ha->keys);
    if (hk == NULL) {
        return NGX_ERROR;
    }

    hk->key = *key;
    hk->key_hash = ngx_hash_key(key->data, last);

    hk->value = (my_data_t*) value;

    return NGX_OK;
}





