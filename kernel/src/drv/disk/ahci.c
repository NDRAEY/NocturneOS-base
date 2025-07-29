#include "drv/disk/ahci.h"

#include <lib/math.h>

#include "lib/asprintf.h"
#include "generated/pci.h"
#include "io/ports.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "sys/isr.h"
#include "drv/disk/ata.h"
#include "drv/atapi.h"
#include "net/endianess.h"
#include "drv/disk/dpm.h"
#include "io/tty.h"

#define AHCI_CLASS 1
#define AHCI_SUBCLASS 6

void il_log(const char* message);

struct ahci_port_descriptor ports[32] = {0};

uint8_t ahci_busnum, ahci_slot, ahci_func;
uint16_t ahci_vendor = 0, ahci_devid = 0;
uint32_t ahci_irq;
bool ahci_initialized = false;

volatile AHCI_HBA_MEM* abar;

#define AHCI_PORT(num) ((volatile AHCI_HBA_PORT*)(abar->ports + (num)))

void ahci_irq_handler();

void ahci_init() {
	// Find controller
    il_log("Finding AHCI...");

	pci_find_device_by_class_and_subclass(AHCI_CLASS, AHCI_SUBCLASS, &ahci_vendor, &ahci_devid, &ahci_busnum, &ahci_slot, &ahci_func);


	if(ahci_vendor == 0 || ahci_devid == 0) {
		qemu_err("AHCI contoller not found!");

		return;
	}

	qemu_ok("Found VEN: %x DEV: %x", ahci_vendor, ahci_devid);

	// Enable Bus Mastering
    pci_enable_bus_mastering(ahci_busnum, ahci_slot, ahci_func);

//	qemu_ok("Enabled Bus Mastering");

	// Get ABAR

	abar = (volatile AHCI_HBA_MEM*)(pci_read32(ahci_busnum, ahci_slot, ahci_func, 0x24) & ~0b1111U);

	qemu_log("AHCI ABAR is: %p", abar);

	// Map memory
	map_pages(
            get_kernel_page_directory(),
            (physical_addr_t) abar,
            (virtual_addr_t) abar,
            PAGE_SIZE * 2,
            PAGE_WRITEABLE | PAGE_CACHE_DISABLE
    );

	qemu_log("Version: %x", abar->version);

	if(abar->host_capabilities_extended & 1U) {
		for(int i = 0; i < 5; i++) {
			qemu_warn("PERFORMING BIOS HANDOFF!!!");
		}

		abar->handoff_control_and_status = abar->handoff_control_and_status | (1 << 1);

		while(1) {
			size_t status = abar->handoff_control_and_status;

			if (~status & (1 << 0)) {
				break;
			}
		}
	} else {
		qemu_ok("No BIOS Handoff");
	}

	// Reset
	abar->global_host_control |= (1 << 31) | (1 << 0);

	while((abar->global_host_control & 1) == 1)
		;
	
	// Interrupts
	ahci_irq = pci_read32(ahci_busnum, ahci_slot, ahci_func, 0x3C) & 0xFF; // All 0xF PCI register
	qemu_log("AHCI IRQ: %x (%d)", ahci_irq, ahci_irq);

	register_interrupt_handler(32 + ahci_irq, ahci_irq_handler);

	abar->global_host_control |= (1U << 31) | (1U << 1);  // AHCI Enable and AHCI Interrupts

	qemu_ok("Enabled AHCI and INTERRUPTS");

	size_t caps = abar->capability;
	size_t slot_count = ((caps >> 8) & 0x1f) + 1;

	qemu_log("Slot count: %d", slot_count);

	char* a;
	asprintf(&a, "Slot count: %d", slot_count);
	il_log(a);
	kfree(a);

	size_t maxports = (caps & 0x1f) + 1;

	qemu_log("Max port count: %d", maxports);

	asprintf(&a, "Max port count: %d", maxports);
	il_log(a);
	kfree(a);

	// Scan bus

	uint32_t implemented_ports = abar->port_implemented;

	qemu_log("PI is: %x", implemented_ports);

	if(implemented_ports == 0) {
		implemented_ports = 0xffffffffu >> (32 - maxports);
		qemu_log("updated pi is: %x", implemented_ports);
	}

	for(uint32_t i = 0; i < 32; i++) {
		if (implemented_ports & (1 << i)) {
            volatile AHCI_HBA_PORT* port = AHCI_PORT(i);

            // Additional initialization here

            if((port->command_and_status & (1 << 2)) == 0) {
				port->command_and_status |= (1 << 2);

				//sleep_ms(20);  // Replace them with checks
				while((port->command_and_status & (1 << 2)) == (1 << 2)) {
					__asm__ volatile("nop");
				}
			}

			if((port->command_and_status & (1 << 1)) == 0) {
				port->sata_error = 0xFFFFFFFF;
                sleep_ms(10);

				port->sata_control = 0;
                sleep_ms(10);

				port->command_and_status |= (1 << 1); // Spin up.

				//sleep_ms(20);  // Replace them with checks
				while((port->command_and_status & (1 << 1)) == (1 << 1)) {
                    __asm__ volatile("nop");
                }
			}

			port->sata_error = 0xFFFFFFFF;
			
            port->command_and_status = (port->command_and_status & ~(0xf << 28)) | (1 << 28);
			port->command_and_status |= 1 << 4;

			if (!ahci_is_drive_attached(i)) {
				continue;
			}

			// Idk why we are clearing START bit.
			port->command_and_status = port->command_and_status & 0xfffffffe;

			while(port->command_and_status & (1 << 15))
                ;

            ahci_rebase_memory_for(i);
        }
	}

	for(register int i = 0; i < 32; i++) {
		if(abar->port_implemented & (1 << i)) {
			volatile AHCI_HBA_PORT* port = abar->ports + i;

			tty_printf("[%p: Port %d]\n", port, i);

			if(!ahci_is_drive_attached(i)) {
				qemu_log("\tNo drive attached to port!");
                continue;
            }

			if(port->signature == AHCI_SIGNATURE_SATAPI) { // SATAPI
				tty_printf("\tSATAPI drive\n");
                ahci_identify(i, true);
                //ahci_eject_cdrom(i);
			} else if(port->signature == AHCI_SIGNATURE_SATA) { // SATA
				tty_printf("\tSATA drive\n");
                ahci_identify(i, false);
			} else {
				qemu_log("Other device: %x", port->signature);
			}
		}
	}

	ahci_initialized = true;
}

void ahci_rebase_memory_for(size_t port_num) {
	if(port_num > 31)
		return;

	ahci_stop_cmd(port_num);

	// Memory rebase
	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

    void* virt = kmalloc_common(MEMORY_PER_AHCI_PORT, PAGE_SIZE);
    memset(virt, 0, MEMORY_PER_AHCI_PORT);

    // If gets laggy, comment it.
    phys_set_flags(get_kernel_page_directory(), (virtual_addr_t) virt, PAGE_WRITEABLE | PAGE_CACHE_DISABLE);

    size_t phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) virt);

    ports[port_num].command_list_addr_virt = virt;
    ports[port_num].command_list_addr_phys = phys;

    ports[port_num].fis_virt = AHCI_FIS(virt, 0);
    ports[port_num].fis_phys = AHCI_FIS(phys, 0);

//    qemu_log("Virtual addresses: Command list %x, FIS %x", ports[port_num].command_list_addr_virt, ports[port_num].fis_virt);

	port->command_list_base_address_low = phys;
	port->command_list_base_address_high = 0;

	port->fis_base_address_low = AHCI_FIS(phys, 0);
	port->fis_base_address_high = 0;

	AHCI_HBA_CMD_HEADER *cmdheader = (AHCI_HBA_CMD_HEADER*)virt;

	for(int i = 0; i < 32; i++) {
		cmdheader[i].prdtl = COMMAND_TABLE_PRDT_ENTRY_COUNT;
		cmdheader[i].ctba = AHCI_COMMAND_TABLE_ENTRY(phys, 0, i);
		cmdheader[i].ctbau = 0;
	}

//	qemu_log("Port %d", port_num);
//	qemu_log("\t|- CMD LIST BASE: %x (%s)", port->command_list_base_address_low, IS_ALIGNED(port->command_list_base_address_low, 1024) ? "aligned" : "not aligned");
//	qemu_log("\t|- FIS BASE: %x (%s)", port->fis_base_address_low, IS_ALIGNED(port->fis_base_address_low, 256) ? "aligned" : "not aligned");
//	qemu_log("\t|- TABLE ENTRIES: %x - %x", cmdheader[0].ctba, cmdheader[31].ctba + 256);

	ahci_start_cmd(port_num);

	qemu_ok("Rebasing memory for: %d is OK.", port_num);
}

bool ahci_is_drive_attached(size_t port_num) {
	if(port_num > 31){
		return false;
	}

	uint32_t implemented_ports = abar->port_implemented;

	if(implemented_ports & (1 << port_num)) {
		volatile AHCI_HBA_PORT* port = abar->ports + port_num;

		uint32_t status = port->sata_status;

		uint8_t ipm = (status >> 8) & 0xF;
		uint8_t det = status & 0xF;

		if(ipm == 1 && det == 3) {
			return true;
		}
	}

	return false;
}

int ahci_free_cmd_slot(size_t port_num) {
	if(port_num > 31)
		return -1;

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	uint32_t slots = port->sata_active | port->command_issue;

	for(int i = 0; i < 32; i++) {
		if((slots & (1 << i)) == 0)
			return i;
	}

	return -1;
}

void ahci_start_cmd(size_t port_num) {
	if(port_num > 31)
		return;

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	while (port->command_and_status & AHCI_HBA_CR)
        ;

	port->command_and_status |= AHCI_HBA_FRE;
	port->command_and_status |= AHCI_HBA_ST;
}

void ahci_stop_cmd(size_t port_num) {
	if(port_num > 31)
		return;

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	port->command_and_status &= ~AHCI_HBA_ST;
	port->command_and_status &= ~AHCI_HBA_FRE;

	while(1) {
		if (port->command_and_status & AHCI_HBA_FR)
			continue;
		if (port->command_and_status & AHCI_HBA_CR)
			continue;
		break;
	}
}

void ahci_irq_handler() {
    qemu_warn("AHCI interrupt!");
    volatile uint32_t status = abar->interrupt_status;

    abar->interrupt_status = status;

    for(int i = 0; i < 32; i++) {
        if(status & (1 << i)) {
            volatile AHCI_HBA_PORT* port = AHCI_PORT(i);

            volatile uint32_t port_interrupt_status = port->interrupt_status;

            port->interrupt_status = port_interrupt_status;
        }
    }
}

bool ahci_wait_spin(volatile AHCI_HBA_PORT* port) {
    int spin = 0;
    while ((port->task_file_data & (ATA_SR_BSY | ATA_SR_DRQ)) && spin < 1000000) {
        spin++;
    }

    if (spin == 1000000) {
        qemu_err("Port is hung");
        return false;
    }

    return true;
}

bool ahci_send_cmd(volatile AHCI_HBA_PORT *port, size_t slot) {
    if(!ahci_wait_spin(port)) {
        return false;
    }

    port->interrupt_status = 0xFFFFFFFF;

    port->command_issue |= 1u << slot;

    //tty_printf("Command issued!\n");

    // qemu_warn("COMMAND IS ISSUED");
    //
    int spin = 0;

    while(true) {
        if(spin > 10000) {
            //tty_printf("Spin timeout!\n");
            return false;
        }

        if ((port->command_issue & (1u << slot)) == 0) { // Command is not running? Break 
            break;
        }

        if (port->interrupt_status & AHCI_HBA_TFES)	{  // Task file error? Tell about error and exit
            qemu_err("Read disk error (Task file error); IS: %x", port->interrupt_status);
            //tty_printf("TF error!\n");

            return false;
        }

        spin++;

		__asm__ volatile("hlt");
    }

	qemu_warn("OK");
    //tty_printf("Ok!\n");

	return true;
}

void ahci_fill_prdt(AHCI_HBA_CMD_HEADER* hdr, HBA_CMD_TBL* table, char* buffer_mem, size_t bytes) {
	int index = 0;
	size_t i;
	for(i = 0; i < bytes; i += (4 * MB) - 1) {
		size_t buffer_phys = virt2phys(get_kernel_page_directory(), (size_t)buffer_mem + i);

		table->prdt_entry[index].dba = buffer_phys;
		table->prdt_entry[index].dbau = 0;
		table->prdt_entry[index].rsv0 = 0;
		table->prdt_entry[index].dbc = MIN((4U * MB), (bytes - i) % (4U * MB)) - 1;  // Size in bytes 4M max
		table->prdt_entry[index].rsv1 = 0;
		table->prdt_entry[index].i = 0;

		qemu_log("PRDT[%d]: Address: %x (V%x); Size: %d bytes; Last: %d",
			i,
			table->prdt_entry[index].dba,
			(size_t)buffer_mem + i,
			table->prdt_entry[index].dbc + 1,
			table->prdt_entry[index].i);

		index++;
	}

	table->prdt_entry[index - 1].i = 1;

    hdr->prdtl = index;
}

/**
 * @brief Чтение `size` секторов с AHCI диска
 * @param port_num - номер порта
 * @param location - номер начального сектора
 * @param sector_count - колчество секторов
 * @param buffer - буфер куда сохранять данные
 */
size_t ahci_read_sectors(size_t port_num, uint64_t location, size_t sector_count, void* buffer) {
	if(!ahci_initialized) {
		qemu_err("AHCI not present!");
		return 0;
	}

	// Get our port.
	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

    ahci_wait_spin(port);
	    
    //tty_printf("Read sectors\n");

	// Get the descriptor of our AHCI port.
    struct ahci_port_descriptor desc = ports[port_num];

	if(desc.is_atapi) {
		//tty_printf("ATAPI check media\n");

		bool status = ahci_atapi_check_media_presence(port_num);

		//tty_printf("ATAPI media is in: %d\n", status);

		// Don't allow reading empty drive
		if(!status) {
            //tty_printf("Refused.\n");
			return 0;
		}
	}

    //tty_printf("After check\n");

	// Hard disks have always 512 bytes/sector; Optical discs have 2048 bytes/sector.
    size_t block_size = desc.is_atapi ? 2048 : 512;

	qemu_warn("\033[7mAHCI READ STARTED\033[0m");

    // Clear out interrupt status.
	port->interrupt_status = (uint32_t)-1;
	
	// Get command list (we will use only first entry in it).
	AHCI_HBA_CMD_HEADER* hdr = ports[port_num].command_list_addr_virt;
	
	hdr->cfl = sizeof(AHCI_FIS_REG_HOST_TO_DEVICE) / sizeof(uint32_t);
	hdr->a = desc.is_atapi ? 1 : 0;  // ATAPI / Not ATAPI
	hdr->w = 0;  // Read
	hdr->p = 0;  // No prefetch
	
	// Get command table and clear it out.
	HBA_CMD_TBL* table = (HBA_CMD_TBL*)AHCI_COMMAND_TABLE(ports[port_num].command_list_addr_virt, 0);
	memset(table, 0, sizeof(HBA_CMD_TBL));
	
	// Calculate total size
    size_t bytes = sector_count * block_size;

	// Allocate memory for buffer.
	char* buffer_mem = kmalloc_common(ALIGN(bytes, PAGE_SIZE), PAGE_SIZE);
	// char* buffer_mem = kmalloc_common_contiguous(get_kernel_page_directory(), ALIGN(bytes, PAGE_SIZE) / PAGE_SIZE);
	memset(buffer_mem, 0, bytes);

	// Use this data to fill out PRDT table.
	ahci_fill_prdt(hdr, table, buffer_mem, bytes);

	// Get FIS.
	AHCI_FIS_REG_HOST_TO_DEVICE *cmdfis = (AHCI_FIS_REG_HOST_TO_DEVICE*)&(table->cfis);

	qemu_log("CMDFIS at: %p", cmdfis);

	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;  // OS -> Drive
	cmdfis->c = 1;	// Command
	cmdfis->command = desc.is_atapi ? ATA_CMD_PACKET : ATA_CMD_READ_DMA_EXT;  // Choose command based on drive type.

    if(desc.is_atapi) {
        qemu_log("ATAPI DEVICE");

		// If ATAPI, fill out SCSI command.
        char command[16] = {
                ATAPI_CMD_READ,  // Command
                0, // ?
                (location >> 24) & 0xFF,  // LBA
                (location >> 16) & 0xFF,
                (location >> 8) & 0xFF,
                (location >> 0) & 0xFF,
                (sector_count >> 24) & 0xFF,  // Sector count
                (sector_count >> 16) & 0xFF,
                (sector_count >> 8) & 0xFF,
                (sector_count >> 0) & 0xFF,
                0, // ?
                0  // ?
        };

        memcpy(table->acmd, command, 16);

        // size_t bytecount = sector_count * 2048;

        // cmdfis->lba0 = bytecount & 0xff;
        cmdfis->lba1 = 2048 & 0xff;
        cmdfis->lba2 = (2048 >> 8) & 0xff;
    } else {
        qemu_log("JUST A DISK DEVICE");

        cmdfis->lba0 = location & 0xFF;
        cmdfis->lba1 = (location >> 8) & 0xFF;
        cmdfis->lba2 = (location >> 16) & 0xFF;
        cmdfis->lba3 = (location >> 24) & 0xFF;
        cmdfis->lba4 = (location >> 32) & 0xFF;
        cmdfis->lba5 = (location >> 40) & 0xFF;

        cmdfis->countl = sector_count & 0xffU;
        cmdfis->counth = (sector_count >> 8) & 0xffU;

        cmdfis->device = 1U << 6;	// LBA mode
    }

	bool status = ahci_send_cmd(port, 0);

	if(!status) {
		kfree(buffer_mem);

		return 0;
	}

	qemu_log("COPYING");
	memcpy(buffer, buffer_mem, bytes);
	qemu_log("COPIED");

	kfree(buffer_mem);

	return bytes;
}

/**
 * @brief Запись `size` секторов с AHCI диска
 * @param port_num - номер порта
 * @param location - номер начального сектора
 * @param sector_count - колчество секторов
 * @param buffer - буфер куда сохранять данные
 */
void ahci_write_sectors(size_t port_num, size_t location, size_t sector_count, void* buffer) {
	if(!ahci_initialized) {
		qemu_err("AHCI not present!");
		return;
	}

	qemu_warn("\033[7mAHCI WRITE STARTED\033[0m");

    struct ahci_port_descriptor desc = ports[port_num];

    size_t block_size = desc.is_atapi ? 2048 : 512;

	char* buffer_mem = kmalloc_common(sector_count * block_size, PAGE_SIZE);
	memset(buffer_mem, 0, sector_count * block_size);
    memcpy(buffer_mem, buffer, sector_count * block_size);

	// size_t buffer_phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) buffer_mem);

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	port->interrupt_status = (uint32_t)-1;

	AHCI_HBA_CMD_HEADER* hdr = ports[port_num].command_list_addr_virt;

	hdr->cfl = sizeof(AHCI_FIS_REG_DEVICE_TO_HOST) / sizeof(uint32_t);  // Should be 5
	hdr->a = 0;  // Not ATAPI
	hdr->w = 1;  // Write
	hdr->p = 0;  // No prefetch

	qemu_log("FIS IS %d DWORDs long", hdr->cfl);

	HBA_CMD_TBL* table = (HBA_CMD_TBL*)AHCI_COMMAND_TABLE(ports[port_num].command_list_addr_virt, 0);

	memset(table, 0, sizeof(HBA_CMD_TBL));

    size_t bytes = sector_count * block_size;

	ahci_fill_prdt(hdr, table, buffer_mem, bytes);

	AHCI_FIS_REG_HOST_TO_DEVICE *cmdfis = (AHCI_FIS_REG_HOST_TO_DEVICE*)&(table->cfis);

	qemu_log("CMDFIS at: %p", cmdfis);

	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_WRITE_DMA_EXT;

	cmdfis->lba0 = location & 0xFF;
	cmdfis->lba1 = (location >> 8) & 0xFF;
	cmdfis->lba2 = (location >> 16) & 0xFF;
	cmdfis->device = 1 << 6;	// LBA mode

	cmdfis->lba3 = (location >> 24) & 0xFF;
	cmdfis->countl = sector_count & 0xff;
	cmdfis->counth = (sector_count >> 8) & 0xff;

	ahci_send_cmd(port, 0);

	kfree(buffer_mem);

	qemu_warn("\033[7mOK?\033[0m");
}

bool ahci_send_atapi_nomem(size_t port_num, uint8_t command[16]) {
	qemu_log("ATAPI command on port %d (CMD: %x)", port_num, command[0]);

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	port->interrupt_status = (uint32_t)-1;

	AHCI_HBA_CMD_HEADER* hdr = ports[port_num].command_list_addr_virt;

	hdr->cfl = sizeof(AHCI_FIS_REG_HOST_TO_DEVICE) / sizeof(uint32_t);
	hdr->a = 1;  // ATAPI
	hdr->w = 0;  // Read
	hdr->p = 1;  // Prefetch
	hdr->prdtl = 0;  // No entries

	HBA_CMD_TBL* table = (HBA_CMD_TBL*)AHCI_COMMAND_TABLE(ports[port_num].command_list_addr_virt, 0);
	memset(table, 0, sizeof(HBA_CMD_TBL));

    memcpy(table->acmd, command, 16);

	volatile AHCI_FIS_REG_HOST_TO_DEVICE *cmdfis = (volatile AHCI_FIS_REG_HOST_TO_DEVICE*)&(table->cfis);
    memset((void*)cmdfis, 0, sizeof(AHCI_FIS_REG_HOST_TO_DEVICE));

	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_PACKET;

    return ahci_send_cmd(port, 0);
}

void ahci_send_atapi(size_t port_num, uint8_t command[16], uint8_t* output, size_t size) {
	// tty_printf("ATAPI command on port %d (CMD: %x)\n", port_num, command[0]);

	volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

	port->interrupt_status = (uint32_t)-1;

	AHCI_HBA_CMD_HEADER* hdr = ports[port_num].command_list_addr_virt;

	hdr->cfl = sizeof(AHCI_FIS_REG_DEVICE_TO_HOST) / sizeof(uint32_t);  // Should be 5
	hdr->a = 1;  // ATAPI
	hdr->w = 0;  // Read
	hdr->p = 0;  // No prefetch

	HBA_CMD_TBL* table = (HBA_CMD_TBL*)AHCI_COMMAND_TABLE(ports[port_num].command_list_addr_virt, 0);
	memset(table, 0, sizeof(HBA_CMD_TBL));

    memcpy(table->acmd, command, 16);

	size_t buffer_size = ALIGN(size, PAGE_SIZE);

	// Allocate memory for buffer.
	char* buffer_mem = kmalloc_common(buffer_size, PAGE_SIZE);
	memset(buffer_mem, 0, buffer_size);

	// Get its physical address
	// size_t buffer_phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) buffer_mem);

	// Use this data to fill out PRDT table.
	ahci_fill_prdt(hdr, table, buffer_mem, buffer_size);

	volatile AHCI_FIS_REG_HOST_TO_DEVICE *cmdfis = (volatile AHCI_FIS_REG_HOST_TO_DEVICE*)&(table->cfis);
    memset((void*)cmdfis, 0, sizeof(AHCI_FIS_REG_HOST_TO_DEVICE));

	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_PACKET;

	cmdfis->lba1 = (size & 0xff);
	cmdfis->lba2 = ((size >> 8) & 0xff);
	// cmdfis->lba3 = ((size >> 16) & 0xff);
	// cmdfis->lba4 = ((size >> 24) & 0xff);
	// cmdfis->lba5 = ((size >> 32) & 0xff);

	// tty_printf("Sending command...\n");
    ahci_send_cmd(port, 0);

	memcpy(output, buffer_mem, size);

	kfree(buffer_mem);
}

// Call SCSI START_STOP command to eject a disc
void ahci_eject_cdrom(size_t port_num) {
    uint8_t command[16] = {
        ATAPI_CMD_START_STOP,  // Command
        0, 0, 0,  // Reserved
        1 << 1, // Eject the disc
        0, 0, 0, 0, 0,   // Reserved
		0, 0, 0, 0, 0, 0
    };

	ahci_send_atapi_nomem(port_num, command);
}

atapi_error_code ahci_atapi_request_sense(size_t port_num, uint8_t* output) {
	uint8_t command[16] = {
        ATAPI_CMD_RQ_SENSE, 0, 0, 0,
		18, // Allocation Length: We need only 18 bytes (mininal response length)
		0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0
    };

	ahci_send_atapi(port_num, command, output, 18);

	// hexview_advanced(output, 18, 16, false, new_qemu_printf);
	
	return (atapi_error_code){(output[0] >> 7) & 1, output[2] & 0b00001111, output[12], output[13]};
}

bool ahci_atapi_check_media_presence(size_t port_num) {
	uint8_t command[16] = {
        ATAPI_CMD_READY, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
    };

	uint8_t errorcode[18] = {0};
    
	// tty_printf("Sending READY command\n");
	bool is_okay = ahci_send_atapi_nomem(port_num, command);

    if(!is_okay) {
        return false;
    }

	// tty_printf("Fetching sense\n");
	atapi_error_code error_code = ahci_atapi_request_sense(port_num, errorcode);

	// hexview_advanced(errorcode, 24, 16, false, new_qemu_printf);
	
	return error_code.valid && !(error_code.sense_key == SCSI_SENSEKEY_NOT_READY && error_code.sense_code == 0x3A);
}

void ahci_read(size_t port_num, uint8_t* buf, uint64_t location, uint32_t length) {
	ON_NULLPTR(buf, {
		qemu_log("Buffer is nullptr!");
		return;
	});

    //tty_printf("Read port %d, location: %x; len: %d\n", port_num, (uint32_t)location, length);

	// TODO: Get sector size somewhere (Now we hardcode it into 512).

    size_t block_size = ports[port_num].is_atapi ? 2048 : 512;

	uint64_t start_sector = location / block_size;
	uint64_t end_sector = (location + length - 1) / block_size;
	uint64_t sector_count = end_sector - start_sector + 1;

	uint64_t real_length = sector_count * block_size;

	qemu_log("Reading %d sectors...", (uint32_t)sector_count);

	uint8_t* real_buf = kmalloc(real_length);

	// BUG: Reading big amount of sectors in one call can cause memory corruptions.
    int sectors_per_transfer = 64 + 32;
	for(int i = 0; i < sector_count; i += sectors_per_transfer) {
		ahci_read_sectors(
			port_num,
			start_sector + i,
			MIN(sector_count - i, sectors_per_transfer), 
			real_buf + (i * block_size)
		);
	}

	//ahci_read_sectors(port_num, start_sector, sector_count, real_buf);
	
	memcpy(buf, real_buf + (location % block_size), length);

	kfree(real_buf);
}

size_t ahci_dpm_read(size_t Disk, uint64_t high_offset, uint64_t low_offset, size_t Size, void* Buffer) {
//    qemu_err("TODO: SATA DPM READ");
	(void)high_offset;

	DPM_Disk dpm = dpm_info(Disk + 65);

    ahci_read((size_t) dpm.Point, Buffer, low_offset, Size);


    return Size;
}

size_t ahci_dpm_write(size_t Disk, uint64_t high_offset, uint64_t low_offset, size_t size, const void* buffer) {
	(void)Disk; (void)high_offset; (void)low_offset; (void)size; (void)buffer;
    
	qemu_err("TODO: SATA DPM WRITE");

//    DPM_Disk dpm = dpm_info(Disk + 65);

    return 0;
}

size_t ahci_dpm_ctl(size_t Disk, size_t command, const void* data, size_t length) {
	DPM_Disk dpm = dpm_info(Disk + 65);
	size_t port_nr = (size_t)dpm.Point;

	if(command == DPM_COMMAND_EJECT) {
		ahci_eject_cdrom(port_nr);

		return 0;
	} else if(command == DPM_COMMAND_GET_MEDIUM_STATUS) {
		bool status = ahci_atapi_check_media_presence(port_nr);

		return DPM_MEDIA_STATUS_MASK | (status ? DPM_MEDIA_STATUS_ONLINE : DPM_MEDIA_STATUS_OFFLINE);
	}

	return DPM_ERROR_NOT_IMPLEMENTED;
}

void ahci_identify(size_t port_num, bool is_atapi) {
    qemu_log("Identifying %d", port_num);

    volatile AHCI_HBA_PORT* port = AHCI_PORT(port_num);

    port->interrupt_status = (uint32_t)-1;

	uint32_t block_size = 512;

    AHCI_HBA_CMD_HEADER* hdr = ports[port_num].command_list_addr_virt;

    hdr->cfl = sizeof(AHCI_FIS_REG_HOST_TO_DEVICE) / sizeof(uint32_t);
    hdr->a = 0;  // IDENTIFY COMMANDS DOES NOT NEED TO SET ATAPI FLAG
    hdr->w = 0;  // Read
    hdr->p = 1;  // Prefetch
    hdr->prdbc = 512;
    hdr->prdtl = 1;  // One entry only

    void* memory = kmalloc_common(512, PAGE_SIZE);
    size_t buffer_phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) memory);
    memset(memory, 0, 512);

    HBA_CMD_TBL* table = (HBA_CMD_TBL*)AHCI_COMMAND_TABLE(ports[port_num].command_list_addr_virt, 0);
    memset(table, 0, sizeof(HBA_CMD_TBL));

    // Set only first PRDT for testing
    table->prdt_entry[0].dba = buffer_phys;
    table->prdt_entry[0].dbc = 0x1ff;  // 512 bytes - 1
    table->prdt_entry[0].i = 0;

    volatile AHCI_FIS_REG_HOST_TO_DEVICE *cmdfis = (volatile AHCI_FIS_REG_HOST_TO_DEVICE*)&(table->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
    cmdfis->c = 1;	// Command
    if(is_atapi) {
        cmdfis->command = ATA_CMD_IDENTIFY_PACKET;
        block_size = 2048;
    } else {
        cmdfis->command = ATA_CMD_IDENTIFY;
    }

    cmdfis->lba1 = 0;

    ahci_send_cmd(port, 0);

    uint16_t* memory16 = (uint16_t*)memory;

	size_t capacity = (memory16[101] << 16) | memory16[100];

    uint16_t* model = kcalloc(20, 2);

    for(int i = 0; i < 20; i++) {
        model[i] = bit_flip_short(memory16[0x1b + i]);
    }

    *(((uint8_t*)model) + 39) = 0;

    //tty_printf("[SATA] MODEL: '%s'; CAPACITY: %d sectors\n", model, capacity);
	
	kfree(model);

	//if(!is_atapi) {
		int disk_inx = dpm_reg(
	           (char)dpm_searchFreeIndex(0),
	           "SATA Disk",
	           "Unknown",
	           1,
	           capacity * block_size,
	           capacity,
	           block_size,
	           3, // Ставим 3ку, так как будем юзать функции для чтения и записи
	           "DISK1234567890",
	           (void*)port_num // Оставим тут индекс диска
	   	);
	
		if (disk_inx < 0){
		    qemu_err("[SATA/DPM] [ERROR] An error occurred during disk registration, error code: %d", disk_inx);
		} else {
		    qemu_ok("[SATA/DPM] [Successful] Registering OK");

			dpm_set_read_func(disk_inx + 65, &ahci_dpm_read);
			dpm_set_write_func(disk_inx + 65, &ahci_dpm_write);
			dpm_set_command_func(disk_inx + 65, &ahci_dpm_ctl);
		}
	//}

    ports[port_num].is_atapi = is_atapi;

    kfree(memory);
}
