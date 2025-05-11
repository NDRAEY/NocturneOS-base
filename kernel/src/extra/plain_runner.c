#include <common.h>
#include <lib/stdio.h>
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <io/ports.h>

uint32_t plain_runner(const char* filename, size_t address)
{
    qemu_note("Address is: %x", address);

    FILE *file = fopen(filename, "rb");

    size_t filesize = fsize(file);

    qemu_note("File size is: %d", filesize);

    void *a = kmalloc_common(ALIGN(filesize, PAGE_SIZE), PAGE_SIZE);
    memset(a, 0, ALIGN(filesize, PAGE_SIZE));

    size_t a_phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t)a);

    map_pages(get_kernel_page_directory(), (physical_addr_t)a_phys, address, ALIGN(filesize, PAGE_SIZE), PAGE_WRITEABLE);

    fread(file, 1, filesize, (void *)a);

    int (*entry)(int, char **) = (int (*)(int, char **))address;

    qemu_log("RESULT IS: %d", entry(0, 0));

    unmap_pages_overlapping(get_kernel_page_directory(), address, filesize);

    kfree(a);
    fclose(file);

    return 0;
}
