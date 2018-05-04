#include <vitasdk.h>
#include "taipool.h"

#define POOL_PADDING 0x100 // Difference between stack pointer and mempool start in bytes

static int dummy_thread(SceSize args, void *argp){return 0;}
static void* mempool_addr = NULL;
static SceUID mempool_id = 0;
static size_t mempool_size = 0;
static size_t mempool_free = 0;
static SceUID mempool_type = 0;
static SceUID taipool_sema = 0;

// memblock header struct
typedef struct mempool_block_hdr{
	uint8_t used;
	size_t size;
} mempool_block_hdr;

// Locks taipool mempool access (blocking)
static void _taipool_lock_access(){
	sceKernelWaitSema(taipool_sema, 1, NULL);
}

// Unlocks taipool mempool access
static void _taipool_unlock_access(){
	sceKernelSignalSema(taipool_sema, 1);
}

// Allocks a new block on mempool
static void* _taipool_alloc_block(size_t size){
	size_t i = 0;
	mempool_block_hdr* hdr = (mempool_block_hdr*)mempool_addr;
	
	// Checking for a big enough free memblock
	while (i < mempool_size){
		if (!hdr->used){
			if (hdr->size >= size){
				
				// Reserving memory
				hdr->used = 1;
				size_t old_size = hdr->size;
				hdr->size = size;
				
				// Splitting blocks
				mempool_block_hdr* new_hdr = (mempool_block_hdr*)(mempool_addr + i + sizeof(mempool_block_hdr) + size);
				new_hdr->used = 0;
				new_hdr->size = old_size - (size + sizeof(mempool_block_hdr));
				
				mempool_free -= (sizeof(mempool_block_hdr) + size);
				return (void*)(mempool_addr + i + sizeof(mempool_block_hdr));
			}
		}
		
		// Jumping to next block
		i += (hdr->size + sizeof(mempool_block_hdr));
		hdr = (mempool_block_hdr*)(mempool_addr + i);
		
	}
	return NULL;
}

// Frees a block on mempool
static void _taipool_free_block(void* ptr){
	mempool_block_hdr* hdr = (mempool_block_hdr*)(ptr - sizeof(mempool_block_hdr));
	hdr->used = 0;
	mempool_free += hdr->size;
}

// Merge contiguous free blocks in a bigger one
static void _taipool_merge_blocks(){
	size_t i = 0;
	mempool_block_hdr* hdr = (mempool_block_hdr*)mempool_addr;
	mempool_block_hdr* previousBlock = NULL;
	
	while (i < mempool_size){
		if (!hdr->used){
			if (previousBlock != NULL){
				previousBlock->size += (hdr->size + sizeof(mempool_block_hdr));
				mempool_free += sizeof(mempool_block_hdr); 
			}else{
				previousBlock = hdr;
			}
		}else{
			previousBlock = NULL;
		}
		
		// Jumping to next block
		i += hdr->size + sizeof(mempool_block_hdr);
		hdr = (mempool_block_hdr*)(mempool_addr + i);
		
	}
}

// Extend an allocated block by a given size
static void* _taipool_extend_block(void* ptr, size_t size){
	mempool_block_hdr* hdr = (mempool_block_hdr*)(ptr - sizeof(mempool_block_hdr));
	mempool_block_hdr* next_block = (mempool_block_hdr*)(ptr + hdr->size);
	size_t extra_size = size - hdr->size;
	
	// Checking if enough contiguous blocks are available
	while (extra_size > 0){
		if (next_block->used) return NULL;
		extra_size -= (next_block->size + sizeof(mempool_block_hdr));
		next_block = (mempool_block_hdr*)(next_block + sizeof(mempool_block_hdr) + next_block->size);
	}
	
	// Extending current block
	hdr->size = size;
	mempool_free -= extra_size;
	
	return ptr;
}

// Compact an allocated block to a given size
static void _taipool_compact_block(void* ptr, size_t size){
	mempool_block_hdr* hdr = (mempool_block_hdr*)(ptr - sizeof(mempool_block_hdr));
	size_t old_size = hdr->size;
	hdr->size = size;
	mempool_block_hdr* new_block = (mempool_block_hdr*)(ptr + hdr->size);
	new_block->used = 0;
	new_block->size = old_size - (size + sizeof(mempool_block_hdr));
	mempool_free += new_block->size;
}

// Resets taipool mempool
void taipool_reset(void){
	if (mempool_addr != NULL){
		_taipool_lock_access();
		mempool_block_hdr* master_block = (mempool_block_hdr*)mempool_addr;
		master_block->used = 0;
		master_block->size = mempool_size - sizeof(mempool_block_hdr);
		mempool_free = master_block->size;
		_taipool_unlock_access();
	}
}

// Terminate taipool mempool
void taipool_term(void){
	if (mempool_addr == NULL) return;
	_taipool_lock_access();
	switch(mempool_type) {
	case 1:
		sceKernelDeleteThread(mempool_id);
		break;
	case 2:
		sceKernelFreeMemBlock(mempool_id);
		break;
	}
	sceKernelDeleteSema(taipool_sema);
	mempool_id = 0;
	mempool_addr = NULL;
	mempool_size = 0;
	mempool_free = 0;
	mempool_type = 0;
}

// Initialize taipool mempool
int taipool_init(size_t size, int type){
	
	if (mempool_addr != NULL) taipool_term();

	switch(type){
	case 1:{	
		// Creating a thread in order to reserve requested memory
		SceUID pool_id = sceKernelCreateThread("mempool_thread", dummy_thread, 0x40, size, 0, 0, NULL);
		int ret;
		if (pool_id < 0) return pool_id;
		SceKernelThreadInfo mempool_info;
		mempool_info.size = sizeof(SceKernelThreadInfo);
		ret = sceKernelGetThreadInfo(pool_id, &mempool_info);
		if(ret < 0) return -1;
		mempool_addr = mempool_info.stack + POOL_PADDING;
		mempool_id = pool_id;
		mempool_size = mempool_info.stackSize - POOL_PADDING;
		mempool_type = type;
		break;
	}
	case 2:{
		SceUID pool_id = sceKernelAllocMemBlock("mempool_phycont", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, size, NULL);
		int ret;
		if(pool_id < 0) return pool_id;
		ret = sceKernelGetMemBlockBase(pool_id, &mempool_addr);
		if(ret < 0){
			sceKernelFreeMemBlock(pool_id);
			return -1;
		}
		mempool_id = pool_id;
		mempool_size = size;
		mempool_type = type;
		break;
	}
	}
	// Initializing mempool as a single block
	taipool_reset();
	
	// Setting up a semaphore for mutual exclusion
	taipool_sema = sceKernelCreateSema("taipool_sema", 0, 1, 1, 0);
	return 0;
}

// Frees a block on taipool mempool
void taipool_free(void* ptr){
	_taipool_lock_access();
	_taipool_free_block(ptr);
	_taipool_merge_blocks();
	_taipool_unlock_access();
}

// Allocates a new block on taipool mempool
void* taipool_alloc(size_t size){
	_taipool_lock_access();
	void* res = NULL;
	if (size <= mempool_free) res = _taipool_alloc_block(size);
	_taipool_unlock_access();
	return res;
}

// Allocates a new block on taipool mempool and zero-initialize it
void* taipool_calloc(size_t num, size_t size){
	_taipool_lock_access();
	void* res = taipool_alloc(num * size);
	if (res != NULL) memset(res, 0, num * size);
	_taipool_unlock_access();
	return res;
}

// Reallocates a currently allocated block on taipool mempool
void* taipool_realloc(void* ptr, size_t size){
    _taipool_lock_access();
	mempool_block_hdr* hdr = (mempool_block_hdr*)(ptr - sizeof(mempool_block_hdr));
	void* res = NULL;
	if (hdr->size < size){ // Increasing size
	
		// Trying to extend the block with successive contiguous blocks
		void* res = _taipool_extend_block(ptr, size);
		if (res == NULL){
		
			// Trying to extend the block by fully relocating it
			res = taipool_alloc(size);
			if (res != NULL){
				memcpy(res, ptr, hdr->size);
				taipool_free(ptr);
			}else{
			
				// Trying to extend the block with contiguous blocks
				size_t orig_size = hdr->size;
				taipool_free(ptr);
				res = taipool_alloc(size);
				if (res == NULL){
					hdr->used = 1;
					hdr->size = orig_size;
				}else if (res != ptr){
					memmove(res, ptr, orig_size);
				}
				
			}
		}
	}else{ // Reducing size
		_taipool_compact_block(ptr, size);
		_taipool_merge_blocks();
		res = ptr;
	}
	_taipool_unlock_access();
	return res;
}

// Returns currently free space on mempool
size_t taipool_get_free_space(void){
	return mempool_free;
}