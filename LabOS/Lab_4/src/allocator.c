#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16

typedef struct BlockHeader {
    size_t size;               
    struct BlockHeader *next;  
    bool is_free;  
} BlockHeader;


typedef struct Allocator {
    BlockHeader *free_list;  
    void *memory_start;      
    size_t total_size;  
    void *base_addr; 
} Allocator;


Allocator *allocator_create(void *memory, size_t size) {
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;
    allocator->base_addr = memory;
    allocator->memory_start = (char *)memory + sizeof(Allocator);
    allocator->total_size = size - sizeof(Allocator);
    allocator->free_list = (BlockHeader *)allocator->memory_start;

    allocator->free_list->size = allocator->total_size - sizeof(BlockHeader);
    allocator->free_list->next = NULL;
    allocator->free_list->is_free = true;

    return allocator;
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE *
           MIN_BLOCK_SIZE;  // Округление вверх

    BlockHeader *best_fit = NULL;
    BlockHeader *prev_best = NULL;
    BlockHeader *current = allocator->free_list;
    BlockHeader *prev = NULL;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (best_fit == NULL || current->size < best_fit->size) {
                best_fit = current;
                prev_best = prev;
            }
        }
        prev = current;
        current = current->next;
    }

    if (best_fit) {
        size_t remaining_size = best_fit->size - size;
        if (remaining_size >= sizeof(BlockHeader) + MIN_BLOCK_SIZE) {
            BlockHeader *new_block =
                (BlockHeader *)((char *)best_fit + sizeof(BlockHeader) + size);
            new_block->size = remaining_size - sizeof(BlockHeader);
            new_block->is_free = true;
            new_block->next = best_fit->next;

            best_fit->next = new_block;
            best_fit->size = size;
        }

        best_fit->is_free = false;
        if (prev_best == NULL) {
            allocator->free_list = best_fit->next;
        } else {
            prev_best->next = best_fit->next;
        }

        return (void *)((char *)best_fit + sizeof(BlockHeader));
    }

    return NULL;
}

void allocator_free(Allocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return;
    }

    BlockHeader *header = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    if (!header) return;
    header->is_free = true;


    header->next = allocator->free_list;
    allocator->free_list = header;

    BlockHeader *current = allocator->free_list;
    BlockHeader *prev = NULL;


    while (current && current->next) {
        BlockHeader *next = current->next;
        
        if (((char *)current + sizeof(BlockHeader) + current->size) ==
            (char *)next) {
            current->size += next->size + sizeof(BlockHeader);
            current->next = next->next;
            continue;  
        }
        
        if (prev && ((char *)prev + sizeof(BlockHeader) + prev->size) ==
                        (char *)current) {
            prev->size += current->size + sizeof(BlockHeader);
            prev->next = current->next;
            current = prev;
            if (allocator->free_list == current) allocator->free_list = prev;
            continue;
        }
        prev = current;
        current = current->next;
    }
}


void allocator_destroy(Allocator *allocator) {
    if (allocator) {
        munmap(allocator->base_addr, allocator->total_size + sizeof(Allocator));
    }
}