
#include "hmalloc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

const size_t PAGE_SIZE = 4096;
static hm_stats stats;  // This initializes the stats to 0.

free_list* ff;

void* hmalloc_small(size_t size);
void* hmalloc_big(size_t size);
long len(free_list* head);
void set_block_size(free_list* block, size_t size);

long free_list_length() { return len(ff); }

hm_stats* hgetstats() {
  stats.free_length = free_list_length();
  return &stats;
}

void hprintstats() {
  stats.free_length = free_list_length();
  fprintf(stderr, "\n== husky malloc stats ==\n");
  fprintf(stderr, "Mapped:   %ld\n", stats.pages_mapped);
  fprintf(stderr, "Unmapped: %ld\n", stats.pages_unmapped);
  fprintf(stderr, "Allocs:   %ld\n", stats.chunks_allocated);
  fprintf(stderr, "Frees:    %ld\n", stats.chunks_freed);
  fprintf(stderr, "Freelen:  %ld\n", stats.free_length);
}

static size_t div_up(size_t xx, size_t yy) {
  // This is useful to calculate # of pages
  // for large allocations.
  size_t zz = xx / yy;

  if (zz * yy == xx) {
    return zz;
  } else {
    return zz + 1;
  }
}

void* hmalloc(size_t size) {
  stats.chunks_allocated += 1;
  size += sizeof(size_t);

  //  if(len(ff) == 0) {
  //	ff = mmap(NULL, sizeof(free_list*), PROT_READ|PROT_WRITE,
  //MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

  // }

  if (size <= PAGE_SIZE) {
    return hmalloc_small(size);
  } else {
    return hmalloc_big(size);
  }
}

void* hmalloc_big(size_t size) {
  int num_pages = div_up(size, PAGE_SIZE);
  stats.pages_mapped += num_pages;
  void* addr = mmap(NULL, num_pages * PAGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  size_t* addr_size = (size_t*)addr;
  *addr_size = size - sizeof(size_t);

  return (void*)(addr_size + 1);
}

void* hmalloc_small(size_t size) {
  free_list* first_fit = find_first_fit(&ff, size - sizeof(size_t));
  // void* addr;
  //  size_t block_size;
  size_t* size_addr;
  // exit(1);
  if (first_fit) {
    //	  exit(1);
    // addr = first_fit->address;
    size_addr = (size_t*)first_fit->address;
    remov(&ff, first_fit->address);
    coalesce(&ff);
    //  block_size = get_block_size(first_fit);
    set_block_size(first_fit, size - sizeof(size_t));
    // exit(1);
  } else {
    size_addr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // size_addr = (size_t*)addr;
    *size_addr = size - sizeof(size_t);
    //  block_size = PAGE_SIZE - sizeof(size_t);
    stats.pages_mapped++;
  }
  /*  size_t remainder = block_size - size;

    if (remainder > 16) {
      size_t* leftover_address = size_addr + (size / 4);
      *leftover_address = remainder - sizeof(size_t);
      hfree(leftover_address + 1);
    }
  */
  size_addr++;
  return (void*)size_addr;
}

void hfree(void* item) {
  stats.chunks_freed += 1;

  size_t* item_cast = (size_t*)item;
  item_cast--;
  size_t size = *(item_cast);

  // printf("%ld\n", size);
  if (size <= PAGE_SIZE) {
    insert(&ff, item);
    coalesce(&ff);
  } else {
    stats.pages_unmapped += (div_up(size, PAGE_SIZE));
    munmap(item, size);
  }
}

size_t get_block_size(free_list* block) {
  // FIXME 2
  return *(size_t*)(block->address - sizeof(size_t));
}

void set_block_size(free_list* block, size_t size) {
  size_t* size_addr = (size_t*)(block->address - sizeof(size_t));
  *size_addr = size;
}

free_list* find_first_fit(free_list** head, size_t size) {
  free_list* curr = *head;
  //	printf("entering while\n");
  //	puts("entering while\n");
  // printf("add:%ld\n", *(uint64_t*)curr->address);
  while (curr) {
    //  puts("while entered");
    //  printf("block size: %ld, desired size: %ld\n", get_block_size(curr),
    //  size);

    if (get_block_size(curr) <= size) {
      return curr;
    }
    curr = curr->tail;
  }

  return NULL;
}

void remov(free_list** head, void* addr) {
  uint64_t address_to_remove = *(uint64_t*)addr;
  uint64_t curr_address;

  // TODO handle case where remove first element

  free_list* prev = *head;
  free_list* curr = prev->tail;

  while (curr) {
    curr_address = *(uint64_t*)curr->address;
    if (curr_address - address_to_remove == 0) {
      prev->tail = curr->tail;
      break;
    }
    prev = curr;
    curr = curr->tail;
  }
}

void coalesce(free_list** head) {
  free_list* prev = *head;
  free_list* curr = prev->tail;

  if (len(*head) > 1) {
    uint64_t prev_address = *(uint64_t*)prev->address;
    uint64_t curr_address = *(uint64_t*)curr->address;

    if (prev_address + *(uint64_t*)get_block_size(prev) == curr_address) {
      // update prev size
      // includes memory that was storing size of curr and
      // the entire curr memory block
      set_block_size(
          prev, get_block_size(prev) + get_block_size(curr) + sizeof(size_t));
      prev->tail = curr->tail;
    }

    while (curr) {
      prev_address = *(uint64_t*)prev->address;
      curr_address = *(uint64_t*)curr->address;
      if (prev_address + *(uint64_t*)get_block_size(prev) == curr_address) {
        // update sizei
        //
        //	set_block_size(prev, get_block_size(prev) + get_block_size(curr)
        //+ sizeof(size_t));
        prev->tail = curr->tail;
      }

      prev = curr;
      curr = curr->tail;
    }
  }
}

void insert(free_list** head, void* addr) {
  free_list* new_block = mmap(NULL, sizeof(free_list*), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  new_block->address = (uint64_t*)addr;

  if (len(*head) == 0) {
    *head = new_block;

  } else {
    free_list* prev = *head;
    free_list* curr = prev->tail;
    uint64_t new_address = *(uint64_t*)new_block->address;
    uint64_t curr_address = *(uint64_t*)prev->address;

    if (curr_address > new_address) {
      new_block->tail = *head;
    }

    while (curr) {
      curr_address = *(uint64_t*)curr->address;
      if (curr_address > new_address) {
        prev->tail = new_block;
        new_block->tail = curr;
        break;
      }
      prev = curr;
      curr = curr->tail;
    }
  }
}

long len(free_list* head) {
  long ll = 0;

  while (head) {
    ll++;
    head = head->tail;
  }
  return ll;
}
