#pragma once

#include "common.h"
#include "drv/disk/ata.h"
#include "io/ports.h"
#include <mem/vmm.h>

#define ATAPI_CMD_READY      0x00
#define ATAPI_CMD_RQ_SENSE   0x03
#define ATAPI_CMD_START_STOP 0x1B
#define ATAPI_READ_CAPACITY  0x25
#define ATAPI_CMD_READ       0xA8

#define SCSI_SENSEKEY_NO_SENSE 0x00
#define SCSI_SENSEKEY_RECOVERED_ERROR 0x01
#define SCSI_SENSEKEY_NOT_READY 0x02
#define SCSI_SENSEKEY_MEDIUM_ERROR 0x03
#define SCSI_SENSEKEY_HW_ERROR 0x04
#define SCSI_SENSEKEY_UNIT_ATTENTION 0x06

#define SCSI_ASC_NO_INFO 0x00
#define SCSI_ASC_NO_INDEX 0x01
#define SCSI_ASC_NO_SEEK_COMPLETE 0x02
#define SCSI_ASC_FAULT 0x03

#define SCSI_ASC_NOT_READY 0x04
#define SCSI_ASCQ_NR_NOT_REPORTABLE 0x00
#define SCSI_ASCQ_NR_BECOMING_READY 0x01
#define SCSI_ASCQ_NR_SMART_UNIT_REQUIRED 0x02
#define SCSI_ASCQ_NR_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_ASCQ_NR_FORMAT_IN_PROGRESS 0x04
#define SCSI_ASCQ_SELF_TEST_IN_PROGRESS 0x09

#define SCSI_ASC_UNIT_ATTENTION 0x06

typedef struct {
    bool valid;
    
    uint8_t sense_key;
    uint8_t sense_code;
    uint8_t sense_code_qualifier;
} atapi_error_code;

bool ata_scsi_status_wait(uint8_t bus);
bool ata_scsi_send(uint16_t bus, bool slave, uint16_t lba_mid_hi, uint8_t command[12]);

size_t ata_scsi_receive_size_of_transfer(uint16_t bus);
void ata_scsi_read_result(uint16_t bus, size_t size, uint16_t* buffer);
size_t atapi_read_size(uint16_t bus, bool slave);
size_t atapi_read_block_size(uint16_t bus, bool slave);

bool atapi_read_sectors(uint16_t drive, uint8_t *buf, uint32_t lba, size_t sector_count);
bool atapi_eject(uint8_t bus, bool slave);

/// Returns true if present, false otherwise
bool atapi_check_media_presence(uint8_t bus, bool slave);
atapi_error_code atapi_request_sense(uint8_t bus, bool slave, uint8_t out[18]);