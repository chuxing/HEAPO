#include <stdio.h>
#include <stdlib.h>
#include <pos-lib.h>
#include <string.h>
#include <time.h>
//#include "list/pos-list.h" 
//#include "hashtable/pos-hashtable.h"
//#include "hashtable/pos-hashtable_private.h" 
#include "btree/pos-btree.h" 
//#include "btree/@btree-type.h" 
//#include "btree/@btree-128.h" 

#include <pthread.h> 
#define RO	1
#define RW	0

#include "stm.h" 
#include "mod_ab.h" 
#define TM_START(tid , ro)      { stm_tx_attr_t a = {{.id = tid, .read_only = ro}}; sigjmp_buf * _e = stm_start(a); if(_e != NULL) sigsetjmp(*_e, 0)
#define TM_LOAD(addr)           stm_load((stm_word_t *)addr) 
#define TM_STORE(addr , value)  stm_store((stm_word_t *)addr , (stm_word_t)value) 

#define TM_COMMIT               stm_commit(); } 
#define TM_INIT			stm_init(); mod_mem_init(0); mod_ab_init(0,NULL);
#define TM_EXIT			stm_exit();

#define TM_INIT_THREAD          stm_init_thread() ;     
#define TM_EXIT_THREAD          stm_exit_thread() ;   

#define L1_CACHE_SHIFT (6)
#define L1_CACHE_BYTES (1 << L1_CACHE_SHIFT)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define NODESIZE MAX(L1_CACHE_BYTES , 128) 
static void merge(char * name , struct btree_head * head , struct btree_geo * geo ,
         int level ,unsigned long * left , int lfill ,
         unsigned long * right , int rfill ,
         unsigned long * parent , int lpos);
 int sbtree_remove_level( char * name , struct btree_head * head , struct btree_geo * geo , unsigned long *key , int level );
static void sbtree_shrink( char * name , struct btree_head * head , struct btree_geo * geo);
static void srebalance(char * name , struct  btree_head * head , struct btree_geo * geo , unsigned long * key , int level , unsigned long * child , int fill );
static void smerge(char * name , struct btree_head * head , struct btree_geo * geo ,
         int level ,unsigned long * left , int lfill ,
         unsigned long * right , int rfill ,
         unsigned long * parent , int lpos);

//dk s
extern char* Alloc_tree;
int garbage_count = 0;
int find_count = 0;
extern void * dk_buf[];
//dk e

// for hashtable
static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int sprime_table_length = sizeof(primes)/sizeof(primes[0]);       // 26
const float smax_load_factor = 0.65;

//for BTREE
#define ENOENT 2
#define ENOMEM	12
static int static_key[3000000] = {0} ;
static int debug_int = 0 ;  
struct btree_geo{
	int keylen ;
	int no_pairs;
	int no_longs;
};

//dk s
int insert_num = 0;
//dk e

void print_time(struct timeval T1 , struct timeval T2){
       long sec,usec;
        double time;
        double rate;

        time_t t;

        if(T1.tv_usec > T2.tv_usec)
        {
                sec = T2.tv_sec - T1.tv_sec -1;
                usec = 1000000 + T2.tv_usec - T1.tv_usec;
        }
        else
        {
                sec = T2.tv_sec - T1.tv_sec;
                usec = T2.tv_usec - T1.tv_usec;
        }

        time = (double)sec + (double)usec/1000000;

        printf("[TIME] :%8ld sec %06ldus.\n",sec,usec);




        //printf("[Time]:%8ld sec %06ld us\n" , sec , usec) ; 

}

pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
void t_lock(){ pthread_mutex_lock(&t_mutex);}
void t_unlock(){ pthread_mutex_unlock(&t_mutex);}

pthread_mutex_t b_mutex = PTHREAD_MUTEX_INITIALIZER;
void b_lock(){ pthread_mutex_lock(&b_mutex);}
void b_unlock(){ pthread_mutex_unlock(&b_mutex);}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void lock(){ pthread_mutex_lock(&mutex);}
void unlock(){ pthread_mutex_unlock(&mutex);}
pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
void m_lock(){ pthread_mutex_lock(&m_mutex);}
void m_unlock(){ pthread_mutex_unlock(&m_mutex);}
static char * obj_store ; //object storage name - argv[1] 


pthread_mutex_t r_mutex = PTHREAD_MUTEX_INITIALIZER;
void r_lock(){ pthread_mutex_lock(&r_mutex);}
void r_unlock(){ pthread_mutex_unlock(&r_mutex);}
/////////////////////////////////////////////////////////////////
/////////// 			////////////////////////////////
//////////   BTREE STM		///////////////////////////////
/////////			//////////////////////////////
/////////////////////////////////////////////////////////////

static void* sbval2(struct btree_geo * geo, unsigned long * node ,int n){
//	printf(" b %p\n" , (void*)node[geo->no_longs + n]) ; 	
//	printf(" a %p\n" , (void*)TM_LOAD(&node[geo->no_longs + n])) ;	
//	
//	void * a = node[geo->no_longs+n] ; 	
//	printf("a %p\n" , a) ; 	
//	void * b = (void *)TM_LOAD(&node[geo->no_longs+n]);
//	printf("b %p\n" , b) ;
//	printf("[%s]\n" , __func__ );  
//	void * addr = (void *)TM_LOAD(&node[geo->no_longs + n]) ; 	
//	printf(" addr : %p\n" , addr ) ; 	
//	return addr ; 
	
//	return (void *)TM_LOAD(&node[geo->no_longs + n]);
//	printf("[%s]\n" ,__func__) ; 	
//	printf("node = %p n = %d\n" ,node ,  n ) ; 	
//	void * addr = (void *)TM_LOAD((void *)&node[geo->no_longs+n]) ; 		
//	printf("ORI : %p\n" , (void*)node[geo->no_longs+n]) ; 	
//	printf("DDD : %p\n" , node[geo->no_longs+n]) ;
	
	void * a = TM_LOAD((void *)&node[geo->no_longs + n] ) ;
//	printf(" a = %p\n" ,a ) ; 	
	return a ; 	
		

//	printf("CHN : %p\n" , (void*)addr) ;
//	void * addr = 	(void *)TM_LOAD( &node[geo->no_longs+n] ) ; 	
//	unsigned long * addr = (unsigned long *)TM_LOAD(&node[geo->no_longs+n]) ; 	
//	printf("A\n") ;
//	return (void *)TM_LOAD(&node[geo->no_longs + n]) ;

	
	return (void*)node[geo->no_longs + n] ; 
}
unsigned long * lbkey( struct btree_geo * geo , unsigned long * node , int n){ 
	return &node[n * geo->keylen] ; 
} 
unsigned long * sbkey( struct btree_geo * geo , unsigned long * node , int n){
	unsigned long d = node[n * geo->keylen] ;  
	//printf("d = %d\n" ,d ) ;
	//unsigned long * a = (unsigned long *)TM_LOAD((void *)node[n * geo->keylen]) ; 	
//	unsigned long * q = &node[n * geo -> keylen] ; 	
//	unsigned long * s = ( unsigned long *)TM_LOAD(&q) ; 	
//	return s ;
	unsigned long * s = (unsigned long *)TM_LOAD(&node) ; 	
//	printf(" %p\n" , &s[n * geo->keylen]) ; 	
	return &s[ n * geo->keylen] ; 	
	

//	unsigned long * s = TM_LOAD((unsigned long *)&node[n*geo->keylen]) ; 	
//	printf("%p\n" , s) ; 	
//	printf("kk %p\n", &node[n * geo->keylen]) ;
// 	printf("kk %d\n", node[ n * geo->keylen]) ; 	


	
	//return &node[n * geo->keylen] ; 		
}
unsigned long* slongset(unsigned long * s, unsigned long c ,size_t n ){
	size_t i ; 
	unsigned long * _s = (unsigned long *)TM_LOAD(&s) ; 	
	unsigned long _c = (unsigned long)TM_LOAD(&c) ; 	
		
	for( i = 0 ; i < n ; i++)TM_STORE(&_s[i] , _c); 
	pos_clflush_cache_range(_s, n*sizeof(unsigned long)) ; 	
	return _s ;
}
void sclearpair( struct btree_geo * geo , unsigned long * node ,int n ){
	slongset( sbkey( geo , node , n) , 0, geo->keylen);
	//Modify to fit tinystm
	TM_STORE(&node[geo->no_longs+n] , 0) ; 	

//	node[geo->no_longs + n] = 0 ; 
	pos_clflush_cache_range(&node[geo->no_longs+n],sizeof(unsigned long));
}
int slongcmp(const unsigned long *l1,const unsigned long *l2, size_t n){
	size_t i ; 	
//	printf(" l1 = %lu , l2 = %lu\n" , l1, l2) ; 	

	//consider tinystm.
	unsigned long * _l1 = (unsigned long *)TM_LOAD(&l1) ; 	
	unsigned long * _l2 = (unsigned long *)TM_LOAD(&l2) ; 	
//	printf(" *l1 : %p\n *_l1 : %p\n" , l1 , _l1) ; 	
//	sleep(100) ;
	for( i = 0 ; i < n ; i++ ){
		//consider tinystm.
		if(_l1[i] < _l2[i])return -1;	
		if(_l1[i] > _l2[i])return  1;

//		if(l1[i] < l2[i] ) return -1; 
//		if(l1[i] > l2[i] ) return 1; 
	}
	return 0 ; 
}
int llongcmp(const unsigned long *l1,const unsigned long *l2, size_t n){
	size_t i ; 	

	for( i = 0 ; i < n ; i++ ){
		if(l1[i] < l2[i] ) return -1; 	
		if(l1[i] > l2[i] ) return 1; 	
	}
	return 0 ; 	
}
int lkeycmp(struct btree_geo * geo , unsigned long *node ,int pos ,
		unsigned long * key){ 
	return llongcmp(lbkey(geo,node,pos),key,geo->keylen);
}
int skeycmp(struct btree_geo * geo , unsigned long *node ,int pos ,
		unsigned long * key){ 
	return slongcmp( sbkey(geo,node,pos),key,geo->keylen);
} 
unsigned long* slongcpy( unsigned long * dest , const unsigned long * src, 
	size_t n ){
        size_t i;
	unsigned long * _src = (unsigned long *)TM_LOAD(&src) ; 	
	
	for( i = 0 ; i < n ; i++){ 
		//printf("%d\n" , i) ; 	
		//printf("dest[i] : %d\n" , dest[i]) ; 	
		TM_STORE(&dest[i] , _src[i]) ; 	
		//printf("after dest[i] : %d\n" , dest[i]) ; 	

	}
	//we have to delay flush
	pos_clflush_cache_range(dest, n*sizeof(unsigned long)) ;
	return dest ;

}
void ssetkey( struct btree_geo * geo , unsigned long * node , int n, 
		unsigned long * key ){
	//return (void *)node[geo->no_longs + n] ; 	
	slongcpy( sbkey(geo , node , n) , key , geo->keylen) ;
} 
void ssetval( struct btree_geo * geo , unsigned long * node , int n , void * val){
	TM_STORE(&node[geo->no_longs+n] , (unsigned long)val) ;
//	node[geo->no_longs + n] = (unsigned long)val ; 


//	printf(" node addr : %lu\n" , node) ;
//	printf("[%s] node[%d] = %lu\n" ,__func__, geo->no_longs+n , val) ; 	
    	pos_clflush_cache_range(&node[geo->no_longs + n] , sizeof(unsigned long)); 	
}
void* lbval( struct btree_geo * geo , unsigned long * node , int n ){ 
//	printf("[%s] %p\n" , __func__ , (void*)node[geo->no_longs+n]) ; 
	
	return (void *)node[geo->no_longs + n] ; 	
} 
void* sbval( struct btree_geo * geo , unsigned long * node , int n ){
//	printf("[%s]  %p\n" ,__func__,  (void*)node[geo->no_longs+n]) ; 	
//	printf("[%s]  %p\n" ,__func__, (void*)TM_LOAD(&node[geo->no_longs+n]));

//	return (void *)TM_LOAD(&node[geo->no_longs + n]) ;
	void * a = TM_LOAD((void *)&node[geo->no_longs + n]) ; 	
//	printf("return  : %p\n" , a) ; 	
//	printf("before  : %p\n" , (void *)node[geo->no_longs + n]) ;
	return a; 	
	
	return (void *)node[geo->no_longs + n] ; 	
} 
unsigned long* sbtree_node_alloc(char * name){ 
	unsigned long * node ; 	
	int i ; 	
	node = (unsigned long *)pos_malloc(name , NODESIZE) ; 	
	if(node){ 
		memset(node , 0 , NODESIZE) ; 	
	}
	pos_clflush_cache_range( &node , NODESIZE ) ; 	
	return node ; 	
} 
int sbtree_grow( char * name , struct btree_head * head , struct btree_geo * geo){ 
	unsigned long * node ; 	
	int fill ; 	

	//someday we will modified pos_malloc to thread-safe 
	t_lock() ; 		
	node = sbtree_node_alloc( name ) ; 	
	t_unlock(); 	

	if( ! node ){ 
		return -ENOMEM ; 	
	}
	unsigned long * head_node = (unsigned long *)TM_LOAD(&head->node) ; 	
	int head_height = (int)TM_LOAD(&head->height) ; 	
	if( head_node ){ 
//	if( head->node ){ 
		fill = sgetfill( geo , head->node , 0 ) ; 	
		ssetkey( geo , node , 0 , sbkey(geo , head->node , fill-1)) ; 	
		ssetval( geo , node , 0 , head->node ) ; 	
	}
//	pos_write_value(name , (unsigned long *)&head->node , (unsigned long)node) ; 	
// 	pos_write_value(name , (unsigned long *)&head->height , (unsigned long)( head->height+1)); 	

	


	TM_STORE(&head->node , node ) ; 	
	TM_STORE(&head->height , head_height+1 ) ; 	
//	head->node = node ; 	
//	head->height++ ; 	
	pos_clflush_cache_range( head , sizeof( struct btree_head )) ; 	

	return 0 ; 	
}

unsigned long * imsi_find_level(char * name , struct btree_head * head , struct btree_geo * geo , unsigned long * key , int level){ 
	unsigned long * node = (unsigned long *)TM_LOAD(&head->node) ; 	
	int i , height ; 	
	int _height = (int)TM_LOAD(&head->height) ; 
//	printf("_height\n") ; 	
//	printf("%d\n" , _height );
	
	for( height = _height ; height > level ; height-- ){
//		printf("height : %d, level:%d\n" , height , level) ;
		for( i = 0 ; i < geo->no_pairs ; i++) 
			if(skeycmp(geo,node,i,key) <= 0) break;
		if(( i == geo->no_pairs) || !sbval2(geo,node,i)){ 
			//printf("This is necessary?\n") ;
			i-- ; 
			ssetkey( geo , node , i , key) ; 	
		}	
		//TM_STORE(&node , )
		node = (unsigned long *)sbval(geo,node,i); 
	}
	return node ; 	
} 
unsigned long * sfind_level(char * name , struct btree_head * head , struct btree_geo * geo , unsigned long * key , int level ){ 

       unsigned long *node = (unsigned long *)TM_LOAD(&head->node);


        int i, height;
        for (height = head->height; height > level; height--) {
                for (i = 0; i < geo->no_pairs; i++)
                        if (skeycmp(geo, node, i, key) <= 0)
                                break;
                //if ((i == geo->no_pairs) || !bval(geo, node, i)) {
                if ((i == geo->no_pairs) || !sbval2(geo, node, i)) {
                        i--;
                        ssetkey(geo, node, i, key);
                }
		//TM_STORE?
                node = sbval(geo, node, i);
		
        }
//	printf("[%s] %lu\n" ,__func__, node) ;
        return node;


} 
int sgetpos( struct btree_geo * geo , unsigned long * node , unsigned long * key)
{
       int i;

        for (i = 0; i < geo->no_pairs; i++) {
                if (skeycmp(geo, node, i, key) <= 0)
                        break;
        }
        return i;

}
int sgetfill( struct btree_geo * geo , unsigned long * node , int start){ 
        int i;
//	printf("[%s]\n" , __func__) ; 
//	printf(" node : %p , start : %d\n" , node , start) ; 	
        for (i = start; i < geo->no_pairs; i++){ 
                //if (!bval(geo, node, i))
//		printf(" i = %d\n" , i) ;	
                if (!sbval2(geo, node, i))
                        break;
	}
//	printf("%d\n" , i) ;
        return i;

}
int sbtree_remove_level( char * name , struct btree_head * head , struct btree_geo * geo , unsigned long * key , int level ){ 

//	struct btree_head * sahead = pos_get_prime_object(name) ; 	
	
	unsigned long * node ; 	
	int i , pos , fill ; 	
	int ret ; 	
//	sahead = pos_get_prime_object(name) ; 	
//	printf("[1] last head = %p\n" , sahead) ;
//	printf("[%s][%d]\n" , __func__ , *(unsigned long *)key);  	

	int head_height = (int)TM_LOAD(&head->height) ; 	
//	printf("[%s][%d] head_height : %d\n" , __func__, *(unsigned long *)key, head_height) ; 	
//	printf("[%s][%d] level %d\n" , __func__, *(unsigned long *)key, level) ; 	
	if( level > head_height ){ 
//		printf("Inside : [%d]\n" , *(unsigned long *)key) ; 
//	if( level > head->height ){ 
     	//      pos_write_value(name, (unsigned long *)&head->height, (unsigned long)0);
        //        pos_write_value(name, (unsigned long *)&head->node, (unsigned long)NULL);
       //         pos_clflush_cache_range(head, sizeof(struct btree_head)); // Delayed flush
		TM_STORE(&head->height , 0) ; 
		TM_STORE(&head->node , NULL) ; 	
		pos_clflush_cache_range( head , sizeof(struct btree_head)) ; 	

//	        head->height = 0;
//                head->node = NULL;
		return -1;	
	}

//	sahead = pos_get_prime_object(name) ; 	
//	printf("[2] last head = %p\n" , sahead) ;
	node = sfind_level(name, head, geo, key, level);
//	printf("node : %p\n" , node) ;
        pos = sgetpos(geo, node, key);
        fill = sgetfill(geo, node, pos);
        if ((level == 1) && (skeycmp(geo, node, pos, key) != 0)){
                //return NULL;
//		printf("return -1") ;
                return -1;
	}
        ret = sbval(geo, node, pos);
//	printf("ret  : %p\n" , ret) ; 
        // Free the allocated object
        //pos_free(name, bval(geo, node, pos));

        /* remove and shift */
        for (i = pos; i < fill - 1; i++) {
                ssetkey(geo, node, i, sbkey(geo, node, i + 1));
                ssetval(geo, node, i, sbval(geo, node, i + 1));
        }
        sclearpair(geo, node, fill - 1);
        pos_clflush_cache_range(node, NODESIZE); // Delayed flush in unit of NODESIZE

//	sahead = pos_get_prime_object(name) ; 	
//	printf("[3] last head = %p\n" , sahead) ;
        if (fill - 1 < geo->no_pairs / 2) {
//                if (level < head->height)
		if( level < head_height ){
//			printf("rebalance\n") ; 	
                        //rebalance(head, geo, key, level, node, fill - 1);
                        srebalance(name, head, geo, key, level, node, fill - 1);
		}
                else if (fill - 1 == 1){ 
                        //btree_shrink(head, geo);
//       		printf("shrink\n") ;
	                sbtree_shrink(name, head, geo);
		}
        }

//	printf("return -1\n") ; 
        //return ret;
//	sahead = pos_get_prime_object(name) ; 	
//	printf("[4] last head = %p\n" , sahead) ;
        return 0;
} 
static void smerge(char * name , struct btree_head * head , struct btree_geo * geo , 	
	int level ,unsigned long * left , int lfill , 
	unsigned long * right , int rfill , 
	unsigned long * parent , int lpos){ 
	
	        int i;

        for (i = 0; i < rfill; i++) {
                /* Move all keys to the left */
//#if CONSISTENCY == 1
//               setkey_log(name, geo, left, lfill + i, bkey(geo, right, i));
//                setval_log(name, geo, left, lfill + i, bval(geo, right, i));
//#else
                ssetkey(geo, left, lfill + i, sbkey(geo, right, i));
                ssetval(geo, left, lfill + i, sbval(geo, right, i));
//#endif
        }

//#if CONSISTENCY == 1
        pos_clflush_cache_range(left, NODESIZE); // Delayed flush in unit of NODESIZE
//#endif

        /* Exchange left and right child in parent */
//#if CONSISTENCY == 1
//        setval_log(name, geo, parent, lpos, right);
//        setval_log(name, geo, parent, lpos + 1, left);
//        pos_clflush_cache_range(parent, NODESIZE); // Delayed flush in unit of NODESIZE
//#else
        ssetval(geo, parent, lpos, right);
        ssetval(geo, parent, lpos + 1, left);
        pos_clflush_cache_range(parent, NODESIZE); // Delayed flush in unit of NODESIZE
//#endif
        /* Remove left (formerly right) child from parent */
        //btree_remove_level(head, geo, bkey(geo, parent, lpos), level + 1);
        sbtree_remove_level(name, head, geo, sbkey(geo, parent, lpos), level + 1);
        //mempool_free(right, head->mempool);

	// later pos free all open
        //pos_free(name, right);

}  
static void sbtree_shrink( char * name , struct btree_head * head , struct btree_geo * geo){ 
        unsigned long *node;
        int fill;

        if (head->height <= 1)
                return;

        node = head->node;
        fill = sgetfill(geo, node, 0);
        //BUG_ON(fill > 1);
//#if CONSISTENCY == 1
//        pos_write_value(name, (unsigned long *)&head->node, (unsigned long)bval(geo, node, 0));
//        pos_write_value(name, (unsigned long *)&head->height, (unsigned long)(head->height-1));
//        pos_clflush_cache_range(head, sizeof(struct btree_head)); // Delayed flush
//#else
	
        head->node = sbval(geo, node, 0);
        head->height--;
	pos_clflush_cache_range(head , sizeof(struct btree_head)) ; 	

//#endif
        //mempool_free(node, head->mempool);
 //       pos_free(name, node);

} 
static void srebalance(char * name , struct  btree_head * head , struct btree_geo * geo , 
	unsigned long * key , int level , unsigned long * child , int fill ){ 

	unsigned long * parent , * left = NULL , *right = NULL ; 	
	       int i, no_left, no_right;

        if (fill == 0) {
                /* Because we don't steal entries from a neighbour, this case
                 * can happen.  Parent node contains a single child, this
                 * node, so merging with a sibling never happens.
                 */
                //btree_remove_level(head, geo, key, level + 1);
                sbtree_remove_level(name, head, geo, key, level + 1);
                //mempool_free(child, head->mempool);
                return;
        }

        parent = sfind_level(name, head, geo, key, level + 1);
        i = sgetpos(geo, parent, key);
        //BUG_ON(bval(geo, parent, i) != child);

        if (i > 0) {
                left = sbval(geo, parent, i - 1);
                no_left = sgetfill(geo, left, 0);
                if (fill + no_left <= geo->no_pairs) {
                        //merge(head, geo, level,
                        smerge(name, head, geo, level,
                                        left, no_left,
                                        child, fill,
                                        parent, i - 1);
                        return;
                }
        }
        if (i + 1 < sgetfill(geo, parent, i)) {
                right = sbval(geo, parent, i + 1);
                no_right = sgetfill(geo, right, 0);
                if (fill + no_right <= geo->no_pairs) {
                        //merge(head, geo, level,
                        smerge(name, head, geo, level,
                                        child, fill,
                                        right, no_right,
                                        parent, i);
                        return;
                }
        }

} 
int spos_btree_remove( char * name , void * _key){ 
	struct btree_head * head ; 	
	unsigned long * key ; 	
	int rval ; 

//	printf("\n\n[%s]\n" , __func__) ;	
	if( name == NULL || _key == NULL ){ 
//		printf("D\n") ;
		return -1; 	
	}
//	TM_START(2,RW) ; 	

	key = (unsigned long *)_key ; 	
	head = (struct btree_head *)pos_get_prime_object(name) ;
	if(head->height == 0) return -1; 	

	TM_START(2,RW) ; 	

	rval = sbtree_remove_level(name , head , &btree_geo128, key, 1);
	
	TM_COMMIT ; 	
	return rval ; 	
} 
static int sbtree_insert_level( char * name , struct btree_head * head ,
	struct btree_geo * geo , unsigned long * key, 
	void * val , unsigned long val_size , int level){
	  unsigned long *node;
        int i, pos, fill, err;


	//printf("[%s] start k %d v %p \n" , __func__ , *(unsigned long *)key , val) ;
		

	//lock() ; 	
        //BUG_ON(!val);
	int imsi ;	
	int head_height ; 
	//int head->height = (int)TM_LOAD(&head->height);

	head_height = (int)TM_LOAD(&head->height) ; 	
//	imsi = head_height ; 	
//	printf("\n\n\n[%d]head_height : %d\n" , *(unsigned long *)key, head->height) ;

	if( head_height < level ){
//		printf("inside to head_height : %d level :%d\n" ,head_height , level) ;  
//        if (head->height < level) {
//         err = sbtree_grow(name, head, geo);
		unsigned long * nnode ; 	
		int nfill ; 	
		t_lock() ; 	
		nnode = sbtree_node_alloc(name) ; 	
		t_unlock();  
//		printf("nnode : %p key : %d\n" , nnode, *(unsigned long *)key) ; 

		unsigned long * head_node = (unsigned long *)TM_LOAD(&head->node) ; 	
//		printf("head ? key : %d\n" , *(unsigned long *)key) ; 	
		if(head_node){ 
//			printf("D key : %d\n" , *(unsigned long *)key) ; 	
			nfill = sgetfill(geo, head_node , 0) ; 	
			ssetkey( geo , nnode , 0 , sbkey(geo, head_node ,nfill-1)) ; 	
			ssetval( geo , nnode , 0 , head_node ) ; 	
//			printf("hark key : %d\n", *(unsigned long *)key) ;
		}
		//TM_START(1,RW) ; 
		//printf("head->node : %p\n" ,nnode) ; 	
		TM_STORE(&head->node , nnode) ; 	
		TM_STORE(&head->height , head->height+1) ; 	
		pos_clflush_cache_range(head,sizeof(struct btree_head)) ; 	

//		printf("CHAKCHAK key : %d\n" , *(unsigned long *)key) ;
	 }
retry:
//	printf("D\n") ; 	
        //node = sfind_level(name, head, geo, key, level);
	node = imsi_find_level(name , head , geo , key , level) ;
//	printf(" [%d] imsi_find_level\n" , *(unsigned long *)key) ;
//	printf(" [%d] node : %p\n" , *(unsigned long *)key , node ) ; 	

	// we must modified this code here..
        pos = sgetpos(geo, node, key);
//	printf(" [%d] pos  %d\n" , *(unsigned long *)key , pos) ;
        fill = sgetfill(geo, node, pos);
//	printf(" getfill\n") ;
//	printf(" [%d] %d %d\n" , *(unsigned long *)key , pos , fill ) ; 	


        if (fill == geo->no_pairs) {
//		printf("OHNO>...\n") ;
                unsigned long *new;

                //new = btree_node_alloc(head, gfp);
		t_lock() ; 	
                new = sbtree_node_alloc(name);	
		t_unlock() ;
	
                if (!new){ 
	//		unlock() ; 	
                        return -ENOMEM;
		}
                //err = btree_insert_level(head, geo,
                //             bkey(geo, node, fill / 2 - 1),
                //              new, level + 1, gfp);
		//b_lock();
                //err = re_btree_insert_level(name, head, geo,
                //             sbkey(geo, node, fill / 2 - 1),
                //                new, 0, level + 1);     // Val_size(0) indicates whether copy or not.
		//b_unlock();
        ////	printf("\t[%s] will called %p\n" , __func__, new );   
	      err = sbtree_insert_level(name, head, geo,
                             sbkey(geo, node, fill / 2 - 1),
                             new, 0, level + 1);     // Val_size(0) indicates whether copy or not.
                if (err) {
	//		unlock() ; 	
                        return err;
                }
                for (i = 0; i < fill / 2; i++) {
                        ssetkey(geo, new, i, sbkey(geo, node, i));
                        ssetval(geo, new, i, sbval(geo, node, i));
                        ssetkey(geo, node, i, sbkey(geo, node, i + fill / 2));
                        ssetval(geo, node, i, sbval(geo, node, i + fill / 2));
                        sclearpair(geo, node, i + fill / 2);
                }
                if (fill & 1) {
                        ssetkey(geo, node, i, sbkey(geo, node, fill - 1));
                        ssetval(geo, node, i, sbval(geo, node, fill - 1));
                        sclearpair(geo, node, fill - 1);
                }

                pos_clflush_cache_range(node, NODESIZE); // Delayed flush in unit of NODESIZE
	//	printf("retry\n") ;
                goto retry;
        }
        //BUG_ON(fill >= geo->no_pairs);

        for (i = fill; i > pos; i--) {
	//	printf("STOP HERE [%d] \n" , *(unsigned long *)key) ;
                ssetkey(geo, node, i, sbkey(geo, node, i - 1));
                ssetval(geo, node, i, sbval(geo, node, i - 1));
	//	printf("i = %d fill [%d] \n" , i, *(unsigned long *)key);
        }	
//	printf("[%d] for end\n" , *(unsigned long *)key) ; 	
//	printf("val_Size : %d [%d]\n" , val_size , *(unsigned long *)key);
        ssetkey( geo , node , pos , key ) ;
//	printf("This is not work\n") ;
        if (!val_size) {
  //      	printf("HTIS?\n") ; 	
	        ssetval(geo , node , pos , val) ;
        } else {
//		printf(" OAAAK!\n") ; 	
                void *new_val;
	
                // Allocate object and copy the content 
		t_lock() ; 	
                new_val = pos_malloc(name, val_size);	
		t_unlock() ; 	
//		printf("FUCK\n"); 
                memcpy(new_val, val, val_size);
                pos_clflush_cache_range(new_val, val_size);
                ssetval( geo , node , pos , new_val ) ;
        }
                pos_clflush_cache_range(node, NODESIZE); // Delayed flush in unit of NODESIZE
//	unlock() ; 
//	TM_COMMIT ;
//	printf("END %d\n" , *(unsigned long *)key) ;  
        return 0;
}

int count = 0 ;	
#define lookup_debug 0  
void *sbtree_lookup( char * name ,unsigned long * key){ 
	// 3,000 entry lookup !
	struct btree_head * head = pos_get_prime_object(name) ; 	
	//printf("[%s] head = %lu\n" ,__func__,  head ) ; 	

	#if lookup_debug == 1 	
	//printf(" head = %lu\n", head ) ; 
	#endif 

	struct btree_geo * geo = &btree_geo128;
	#if lookup_debug == 1 
	//printf(" geo addr : %lu\n" , geo ) ; 	
	#endif

	int i, height = head->height;
        unsigned long *node = head->node;
	//printf("[%s] node : %lu\n",__func__ , node ) ; 	
	#if lookup_debug == 1 	
	//printf(" node : %lu\n" , node ) ; 	
	#endif 
        if (height == 0)
                return NULL;
        for ( ; height > 1; height--) {
                for (i = 0; i < geo->no_pairs; i++)
                        if (lkeycmp(geo, node, i, key) <= 0)
                                break;
                if (i == geo->no_pairs)
                        return NULL;
                node = lbval(geo, node, i);
		#if lookup_debug == 1
		//printf("next node : %lu\n" ) ; 	
		#endif 
                if (!node)
                        return NULL;
        }

        if (!node)
                return NULL;

        for (i = 0; i < geo->no_pairs; i++)
                if (lkeycmp(geo, node, i, key) == 0){
			//dk s
                        //return lbval(geo, node, i);
                        return node+i;
			//dk e
		}
        return NULL;

} 
static int index_counter = 0 ; 	
int spos_btree_insert( char * name , void *_key , void *_val , unsigned long val_size){ 
	struct btree_head * head ; 	
	unsigned long * key , * val ; 	
	int rval ; 	
	if( name == NULL || _key == NULL || _val == NULL || val_size < 0){ 
		return -1;	
	}
	key = (unsigned long*)_key ; 
	val = (unsigned long*)_val ; 
//	printf("check \n");
	head = (struct btree_head *)pos_get_prime_object(name);//get head;
/*
	printf("[%s] key : %d  val : %d head : %p\n" ,__func__,  *(unsigned long *)key , *val , head) ; 			
	printf("[%s] head : %lu\n" , __func__ , head ) ; 		
*/
	
//	lock() ; 
//	lock() ; 		
	TM_START(1,RW) ; 	
//	unsigned long * _key = (unsigned long *)TM_LOAD(&key) ; 	
//	void * val = (void *)TM_LOAD(&val) ; 	
//	printf("check 1\n");
	rval = sbtree_insert_level(name , head , &btree_geo128, _key, _val , val_size,1);
//	printf("check 2\n");
	//rval = sbtree_insert_level( name , head, &btree_geo128, key, val, val_size,1);
	TM_COMMIT;
/*	lock() ;	
	printf("INDEX : %d\n", index_counter++)  ;	
	void * addr = sbtree_lookup( name , (unsigned long *)key ) ; 
	if(addr == NULL) exit(1) ; 	
	printf("addr found : %p\n" , addr) ; 	
	unlock() ;*/ 
//	unlock() ;
//	lock() ; 	
//	TM_START(2,RW) ; 	
//	if( rval == -1){ 
//		printf("head->node : %p\n", head->node ) ; 	
//		rval = sbtree_insert_level( name , head , &btree_geo128 , key ,val ,val_size , 1);
//	}
//	TM_COMMIT;
//	unlock();
	return rval ; 
 
}

/*int spos_btree_remove( char * name , void *_key){ 
	struct btree_head * head; 	
	unsigned long * key;  	
        int rval;

        if (name==NULL || key==NULL)
                return -1;

        key = (unsigned long *)_key;
        head = (struct btree_head *)pos_get_prime_object(name);
        if (head->height == 0)
                return -1;

        //return btree_remove_level(head, geo, key, 1);
        //return btree_remove_level(name, head, &btree_geo128, key, 1);
        rval = sbtree_remove_level(name, head, &btree_geo128, key, 1);
        return rval;

}*/
 
static int btree_sum = 0 ; 	 
void* normal_btree_insert(void * data){ 
	int dat = *(int *)data ; 	
	int ret_dat= dat*30000 ; 	
	int i = 0 ; 	
	for( i = 0 + ret_dat ; i < 30000 + ret_dat ; i++){ 
		pos_btree_insert(obj_store,&static_key[i],&static_key[i],0);
	}
}
void* normal_btree_remove(void * data){ 
	int dat = *(int *)data ; 	
	int ret_dat = dat * 30000 ; 	
	int i = 0 ; 	
	for( i = 0 + ret_dat ; i < 30000 + ret_dat ; i++){ 
//		printf("%d\n" , i) ;
		pos_btree_remove(obj_store , &static_key[i]) ; 	
	}	
}  
void* lock_btree_insert(void * data){ 
	int dat = *(int *)data ; 	
	int ret_dat= dat*10 ; 	
	int i = 0 ; 	
	printf("[%s]\n" , __func__ ) ; 	
	printf("data : %d\n" , dat) ; 
	//for( i = 0 + ret_dat ; i < 30000 + ret_dat ; i++){ 
	for( i = 0 + ret_dat ; i < 10 + ret_dat ; i++){ 
		m_lock() ; 
		printf("====== iteratiron = %d\n", i);
		pos_btree_insert(obj_store,&static_key[i],&static_key[i],0);
		m_unlock() ; 	
	}
} 
void* lock_btree_remove(void * data){ 
	int dat = *(int *)data ; 	
	int ret_dat = dat * 30000 ; 	
	int i = 0 ; 	
	for( i = 0 + ret_dat ; i < 30000 + ret_dat ; i++){ 
		m_lock() ; 	
		pos_btree_remove(obj_store , &static_key[i]) ; 	
		m_unlock() ;
	}	
} 
void* stm_btree_insert(void * data){ 
	printf("[%s]\n" , __func__ ) ; 	
	int dat = *(int *)data ; 	
	printf("data : %d\n" , dat) ; 
	int ret_dat = dat*insert_num; 	
	
	int i = 0 ; 	
	TM_INIT_THREAD ; 
	for( i = 0 + ret_dat ; i < insert_num + ret_dat ; i++){ 
		spos_btree_insert(obj_store , &static_key[i] , &static_key[i] , 0) ; 	
	}
	TM_EXIT_THREAD; 

}
void* stm_btree_remove(void * data){ 
	//printf("[%s]\n" , __func__ ) ;
	int i = 0 ; 	
	int dat = *(int *)data ; 	
//	t_lock() ; 	
	int ret_dat = dat*30000 ; 	
	TM_INIT_THREAD ; 	
	for( i = 0+ret_dat ; i < 30000+ret_dat ; i++){ 
		spos_btree_remove( obj_store , &static_key[i]) ; 		
	}	
	TM_EXIT_THREAD ;
//	t_unlock(); 	 
}
void* stm_btree_lookup_nil(void * data){ 
	int dat = *(int *)data ; 	
	int i = 0 ; 	
	for( i = 0 ; i < 3000000 ; i++){ 
		void * addr = sbtree_lookup(obj_store , &static_key[i]) ; 
		printf("%d's [%p] is not nil remove is not thread-safe\n" , i , addr) ;
		if(addr != NULL ) sleep(2); 
	}
} 
void* stm_btree_lookup(void * data){ 
	//printf("[%s]\n" , __func__ ) ; 	
	int i = 0 ; 	
//	int data = *(int*)data; 	
//	for( i = 0 + (data*3000) ; i < 3000+(data*3000) ; i++ );
// 	sequential count
	int dat = *(int *)data ; 	

	for( i = 0  ; i < 3000000 ; i++){ 
//		void * addr = sbtree_lookup( obj_store , &i) ; 	
		void * addr = sbtree_lookup( obj_store , &static_key[i]) ;
		if( addr == NULL ) sleep(2);
		printf("[%d] found addr : [%p]\n" , i , addr) ;
	}
}  
 
void help(){ 
	printf("[1] normal btree insert \n") ; 	
	printf("[2] normal btree remove \n") ; 	
	printf("[3] lock   btree insert \n") ; 	
	printf("[4] lock   btree remove \n") ; 	
	printf("[5] stm    btree insert \n") ; 	
	printf("[6] stm    btree remove \n") ; 	
	printf("[7] lookup addr         \n") ; 	
	printf("[8] lookup nil		\n") ; 
}
int main(int argc , char ** argv){ 
	struct timeval T1,T2 ;
	struct timeval T_GC1,T_GC2 ;
	pthread_t * threads; 
	if( argc != 5 ){
		help();	
		exit(1) ; 	
	}
	// input obj , thread_num ;
	obj_store = argv[1] ; 	
	int thread_num = atoi( argv[2] ) ; 	
	int num = atoi( argv[3] ) ; 	
	insert_num = atoi(argv[4]);

	
	//dk s
	pos_map(Alloc_tree);
	/*
	int ret = 0;
	printf("main ck point 1\n");
	if(pos_create(obj_store) == 1)
	{
		printf("main ck point 2\n");
		ret = pos_btree_init(obj_store) ; 	
		printf("main ck point 3\n");
		ret = pos_btree_open( obj_store ) ;
		printf("open ret = %d\n", ret);
	}
	else
	{
		printf("main ck point 4\n");
		pos_map(obj_store);
		printf("main ck point 5\n");
		ret = pos_btree_init(obj_store) ; 
		printf("main ck point 6\n");
		ret = pos_btree_open( obj_store ) ; 	
		printf("open ret = %d\n", ret);
	}
	*/
	//dk e
	
	int a[100] = {0} ; 	
	int q = 0 ; 	
	for( q = 0 ; q < 100; q++){ 
		a[q] = q; 
	}
//	printf("\n[BTREE operation]\n") ; 	
//	printf("[5] INSERT 30,000 ops\n") ; 	
//	printf("[6] REMOVE 30,000 ops\n") ; 	
//	printf("[7] LOOKUP-addr 30,000 ops\n") ; 	
//	printf("[8] LOOKUP-nul 30,000 ops\n") ; 	
//	scanf("%d", &num) ; 	
	

	if(( threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t)))==NULL){
		perror("malloc");
		exit(1); 
	}
	// This Program is only btree performance program.
	
	//printf("bt ck point 0\n");
	int ret = pos_btree_init(obj_store) ; 	
	//printf("init ret = %d\n", ret);
	ret = pos_btree_open( obj_store ) ; 	
	//printf("open ret = %d\n", ret);
	//printf("bt ck point 2\n");
	
	// global key initialization
	int j = 0 ; 	
	for( j = 0 ; j < 3000000 ; j++ ){
		static_key[j] = j ; 	
	}
	TM_INIT; //this is coded in pos_list_init(?) or pos_list_close(?)
	if( num == 1){// normal btree insert
		if( thread_num > 1 ){
			printf("[normal mode must be one thread]\n") ; 
			exit(1) ; 	
		} 
		gettimeofday(&T1,NULL) ; 	
		// btree 30,000 insert
 		int i = 0 ; 	
		for( i = 0 ; i < thread_num ; i++){
			if(pthread_create(&threads[i] , NULL , normal_btree_insert , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		} 
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2) ; 
		//spos_btree_count( obj_store ) ; 	
		pos_btree_close( obj_store) ; 	
	}else if( num == 2){ // normal btree remove
		if( thread_num > 1 ){
			printf("[normal mode must be one thread]\n") ; 
			exit(1) ; 	
		} 
		gettimeofday(&T1,NULL) ; 	
		// btree 30,000 insert
 		int i = 0 ; 	
		for( i = 0 ; i < thread_num ; i++){
			if(pthread_create(&threads[i] , NULL , normal_btree_remove , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		}
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2) ; 
		pos_btree_close(obj_store) ; 	
	}else if( num == 3 ){ // lock btree insert 
		gettimeofday(&T1,NULL) ; 	
		// btree 30,000 insert
 		int i = 0 ; 	
		for( i = 0 ; i < thread_num ; i++){
			if(pthread_create(&threads[i] , NULL , lock_btree_insert , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		}
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2) ;
		pos_btree_close(obj_store) ; 	 
	}else if( num == 4 ){ // lock btree remove 
		gettimeofday(&T1,NULL) ; 	
		// btree 30,000 insert
 		int i = 0 ; 	
		for( i = 0 ; i < thread_num ; i++){
			if(pthread_create(&threads[i] , NULL , lock_btree_remove , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		}
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2) ;
		pos_btree_close(obj_store) ; 	 
	}
	else if( num == 5 ){

		gettimeofday(&T1,NULL) ; 	
 
		// btree 30,000 insert
 		int i = 0 ; 	
		printf("bt ck point 1111\n");
		for( i = 0 ; i < thread_num ; i++){
			printf("bt ck point 2222\n");
			if(pthread_create(&threads[i] , NULL , stm_btree_insert , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		} 
		
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2) ;
		//spos_btree_count( obj_store ) ; 	
		pos_btree_close( obj_store) ; 	
	}else if( num == 6 ){ 
		// btree 30,000 remove
 		int i = 0 ; 	
		gettimeofday(&T1,NULL) ; 	
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_create(&threads[i] , NULL , stm_btree_remove , (void *)&a[i])!=0){ 
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1);
			}
		}
		for( i = 0 ; i < thread_num ; i++){ 
			if(pthread_join(threads[i] , NULL)!=0){ 
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1) ; 	
			}
		} 
		gettimeofday(&T2,NULL) ; 	
		print_time(T1,T2);
		//spos_btree_count( obj_store ) ; 	
		pos_btree_close( obj_store) ; 	
	}else if( num == 7){ 
		//btree 30,000 lookup
		int i = 0 ; 	
		for( i = 0 ; i < 1; i++){ 
			if(pthread_create(&threads[i] , NULL , stm_btree_lookup, (void *)&a[i])!=0){
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1) ; 	
			}
		}
		for( i = 0 ; i < 1 ;i++){ 
			if(pthread_join(threads[i] , NULL)!=0){
				fprintf(stderr, "Error waiting for thread completion\n") ; 	
				exit(1); 	
			}
		}
		pos_btree_close(obj_store) ; 	
	}else if( num == 8 ){ 
		int i = 0 ; 
		for( i = 0 ; i < 1 ; i++){ 
			if(pthread_create(&threads[i] , NULL , stm_btree_lookup_nil , (void *)&a[i])!=0){
				fprintf(stderr, "Error creating threads\n") ; 	
				exit(1) ; 	
			}
		}
		for( i = 0 ; i < 1 ; i++){ 
			if (pthread_join( threads[i] , NULL)!=0){ 
				fprintf(stderr , "Error waiting for thread completion\n") ; 	
				exit(1); 	
			}
		}
	}

	TM_EXIT; //this is coded in pos_list_delete(?) or pos_list_close(?) 

	struct btree_head *root_node_ptr;
	unsigned long *node_ptr = NULL;

	pos_map(obj_store);

	root_node_ptr=pos_get_prime_object(obj_store);
	//printf("root_node_ptr = %p\n", root_node_ptr);
	node_ptr = root_node_ptr->node;
	//printf("node_ptr = %p\n", root_node_ptr->node);
	//pos_btree_traverse(node_ptr);

	root_node_ptr=pos_get_prime_object(Alloc_tree);
	//printf("Alloc tree ptr = %p\n", root_node_ptr);
	node_ptr = root_node_ptr->node;
	//printf("Alloc_tree head = %p\n", root_node_ptr->node);
	//pos_btree_traverse(node_ptr);
	
	printf("GC start\n");
	gettimeofday(&T_GC1, NULL) ;
	pos_btree_gc(node_ptr, obj_store);
	int k;
	void * address;
	//printf("find count = %d\n", find_count);
	printf("garbage count = %d\n", garbage_count);

	
	for(k=0 ; k<garbage_count ; k++)
	{
		//printf("dk_buf[%d] = %p\n", k, dk_buf[k]);
		address = dk_buf[k];
		printf("address = %p\n", address);
		pos_free(obj_store, address);
	}
	
	gettimeofday(&T_GC2, NULL) ;
	printf("GC end\n");
	print_time(T_GC1,T_GC2) ;


} 
