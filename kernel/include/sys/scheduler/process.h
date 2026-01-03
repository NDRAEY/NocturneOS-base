#pragma once

#include "arch/x86/mem/paging_common.h"
#include "elf/elf.h"
#include "lib/list.h"

typedef	struct {
    // 0
	list_item_t		list_item;		/* List item */
	// 12
    physical_addr_t	page_dir;		/* Page directory */
	// 16
    size_t			threads_count;	/* Count of threads */
    // 20
	uint32_t		pid;		/* Process ID (PID) */
    // 24
    virtual_addr_t  page_dir_virt;	/* Virtual address of page directory */
    // 28
	char*			name;		/* Process name */
	// 32
	size_t          page_tables_virts[1024];    /* Page table addresses */
    // Every process should have a path that process operates
    char*           cwd;
    // If process is a program, it should contain elf header.
    elf_t*          program;
} process_t;

/* Get current process */
process_t* get_current_proc(void);