#include <stddef.h>


#ifndef HMALLOC_H
#define HMALLOC_H

// Husky Malloc Interface
// cs3650 Starter Code

//stats struct 
typedef struct hm_stats {
    long pages_mapped;
    long pages_unmapped;
    long chunks_allocated;
    long chunks_freed;
    long free_length;
} hm_stats;

typedef struct free_list {
	void* address;
	struct free_list* tail;
} free_list;
//stats methods
hm_stats* hgetstats();
void hprintstats();

//free_list methods
void insert(free_list** head, void* addr);
void remov(free_list** head, void* addr);
void coalesce(free_list**);
free_list* find_first_fit(free_list** head, size_t size);
size_t get_block_size(free_list* head);
void set_block_size(free_list* block, size_t size);


//allocator methods
void* hmalloc(size_t size);
void hfree(void* item);
void* hmalloc_big(size_t size);
void* hmalloc_small(size_t size);
void free_leftover(void* item);






#endif
