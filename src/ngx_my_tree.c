#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_my_tree.h"
#include "ngx_my_hash_table.h"

// comparator for my_data_t
int comparator(const void *p, const void *q)  
{ 
    int l = ((my_data_t *)p)->value; 
    int r = ((my_data_t *)q)->value;  
    return (l-r); 
} 
/*
 Traverses all the nodes in the rbtree and prints its hash table values in ascending order.
 In addition to that, it also prints the order of insertion -ind- for each hash table value.
*/
ngx_str_t print_tree(my_tree_t* root,ngx_http_request_t* r,ngx_http_total_shm_t* data){
	ngx_str_t 			result;
	my_node_t           *n;
	ngx_rbtree_node_t   *node, *sentinel;

	result.data = ngx_palloc(r->pool, sizeof(unsigned char)*1500);

    node 		= root->rbtree.root;
    sentinel 	= root->rbtree.sentinel;

    result.len = 0;

    my_queue_t    *f,*t;
	ngx_queue_t   values, *q;

	//Queue is used for traversing the red black tree.

	ngx_queue_init(&values);
	
	f = ngx_palloc(r->pool, sizeof(my_queue_t));

	f->value = node;

	ngx_queue_insert_tail(&values, &f->queue);

	for (q = ngx_queue_head(&values); q != ngx_queue_sentinel(&values); q = ngx_queue_next(q)) {
		ngx_str_t temp;

    	f 		= ngx_queue_data(q, my_queue_t, queue);
	    node 	= f->value;
	    n 		= (my_node_t*) node; 
		
		temp.data 	= ngx_palloc(r->pool, sizeof(unsigned char)*200);

	    sprintf((char*) temp.data, "%d", (int)n->val);
	    temp.len = strlen((char*) temp.data);

	    memcpy(result.data + result.len,temp.data,temp.len);
	    result.len += temp.len;
	    strcpy((char*)result.data+result.len,"\n"); result.len += strlen("\n");

	    ngx_str_t temp2 = print_hash_table(n,r);

	   	memcpy(result.data + result.len,temp2.data,temp2.len);
	   	result.len += temp2.len;

	    if(node->left != sentinel){
	    	t = ngx_palloc(r->pool, sizeof(my_queue_t));
	    	t->value = node->left;
			ngx_queue_insert_tail(&values, &t->queue);
	    }
	    if(node->right != sentinel){
	    	t = ngx_palloc(r->pool, sizeof(my_queue_t));
	    	t->value = node->right;
			ngx_queue_insert_tail(&values, &t->queue);
	    }
	}

	return result;
}

//Inserts the given node to the rbtree given that node is not found in the rbtree
void ngx_my_tree_insert_value(ngx_rbtree_node_t *temp,ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
	my_node_t     	*n, *t;
	ngx_rbtree_node_t  **p;
	
	while(1) {
		n = (my_node_t *) node;
		t = (my_node_t *) temp;


		if(n->key != t->key)
			p = (n->key < t->key) ? &temp->left : &temp->right;
		else 
			p = (n->val < t->val) ? &temp->left : &temp->right;

		if (*p == sentinel) {
			break;
		}
		temp = *p;
	}

	*p = node;

	node->parent	= temp;
	node->left		= sentinel;
	node->right		= sentinel;
	ngx_rbt_red(node);
}

//Finds the node in the rbtree and returns it.
my_node_t* find_node(my_tree_t* root, ngx_int_t value, ngx_int_t key, ngx_http_request_t *r){
	my_node_t 			*n;
	ngx_rbtree_node_t  	*node, *sentinel;

	node 		= root->rbtree.root;
	sentinel 	= root->rbtree.sentinel;

	while(node != sentinel){
		n = (my_node_t *) node;

		if(n->val == value)
			return n;

		if(n->key != key)
			node = (n->key > key) ? node->left : node->right;
		else if(n->val != value)
			node = (n->val > value) ? node->left : node->right;
		else
			return n;
	}

	return NULL;
}

//Specific key value for each rbtree node.
ngx_int_t create_key(ngx_int_t val){
	return (val*val)%10;
}

// Initializes a rbtree node.
ngx_rbtree_node_t* init_node(ngx_int_t id, ngx_http_request_t *r,ngx_http_shm_data_structures_loc_conf_t *lccf){
    my_node_t          	*my_node;
    ngx_rbtree_node_t 	*node;
    ngx_shm_zone_t 		*shm_zone;

    shm_zone = lccf->shm_zone;

    my_node = ngx_slab_alloc((ngx_slab_pool_t *) shm_zone->shm.addr, sizeof(my_node_t));

    my_node->val 					= id;
    my_node->nelts					= 0;
    my_node->key					= create_key(id);
	my_node->hash.hash 				= &my_node->foo_hash;
	my_node->hash.key 				= ngx_hash_key;
	my_node->hash.max_size 			= 512;
	my_node->hash.bucket_size 		= ngx_align(64, ngx_cacheline_size);
	my_node->hash.name 				= (char *) r->uri.data;
	my_node->hash.pool 				= (ngx_pool_t *) shm_zone->shm.addr;
	my_node->hash.temp_pool 		= NULL;

	my_node->hash_keys.pool 		= (ngx_pool_t *) shm_zone->shm.addr;
	my_node->hash_keys.temp_pool	= NULL;

	ngx_shm_hash_keys_array_init(&my_node->hash_keys, NGX_HASH_SMALL);

    node 		= &my_node->rbnode;
    node->key 	= create_key(my_node->val);

    return node;
}

ngx_str_t print_hash_table(my_node_t *n, ngx_http_request_t *r)
{
    ngx_shm_hash_init(&n->hash, n->hash_keys.keys.elts, n->hash_keys.keys.nelts,r);

    ngx_uint_t        	i;
    ngx_hash_t        	*hash = n->hash.hash;
    ngx_uint_t        	last = hash->size;
    ngx_hash_elt_t    	*hash_elt;
    ngx_array_t 		array;
    ngx_str_t 			result;
	my_data_t 			*array_elt;

    result.data = ngx_palloc(r->pool, sizeof(unsigned char)*100);
    strcpy((char*)result.data,"\n"); result.len = strlen("\n");
    
    ngx_array_init(&array, r->pool, 10, sizeof(my_data_t));

    //Traversing all elements in the hash table
    for(i=0 ; i < last ; i++){

        hash_elt = hash->buckets[i];

        if(hash_elt == NULL)
            continue;
        
        while(hash_elt->value){

        	array_elt = ngx_array_push(&array);
        	*array_elt = *((my_data_t *)(hash_elt->value));

            hash_elt = (ngx_hash_elt_t *) ngx_align_ptr(&hash_elt->name[0] + hash_elt->len,sizeof(void *));
        }
    }

    ngx_qsort(array.elts, array.nelts, array.size, comparator);

    //Traversing the array, and then putting it in a result which is ngx_str_t typed

    for(i=0, last=array.nelts ; i < last ; i++){
    		array_elt = (my_data_t*)((u_char *) array.elts + array.size * i);
            int ind 	= array_elt->ind;
            int value 	= array_elt->value;

            ngx_str_t       temp,temp2;            

            temp.data   = ngx_palloc(r->pool, sizeof(unsigned char)*10);
            sprintf((char*) temp.data, "%d", ind);
            temp.len = strlen((char*) temp.data);
            
            strcpy((char*)result.data+result.len,"\t"); result.len += strlen("\t");

            memcpy(result.data + result.len,temp.data,temp.len);
            result.len += temp.len;
  
            strcpy((char*)result.data+result.len," "); result.len += strlen(" ");

            temp2.data   = ngx_palloc(r->pool, sizeof(unsigned char)*10);
            sprintf((char*) temp2.data, "%d", value);
            temp2.len = strlen((char*) temp2.data);

            memcpy(result.data + result.len,temp2.data,temp2.len);
            result.len += temp2.len;

            strcpy((char*)result.data+result.len,"\n"); result.len += strlen("\n");
	}

    return result;
}

