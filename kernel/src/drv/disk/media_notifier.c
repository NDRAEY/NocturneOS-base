#include <common.h>
#include <io/ports.h>
#include <drv/disk/dpm.h>
#include <sys/scheduler.h>
#include <lib/asprintf.h>
#include <fs/fsm.h>
#include <mem/vmm.h>

void il_log(const char* message);

void notifier_loop(uint8_t* statuses) {
    for(char disk = 'A'; disk < 'Z'; disk++) {
        DPM_Disk d = dpm_info(disk);

        if(!d.Ready) continue;

        size_t status = dpm_ctl(disk, DPM_COMMAND_GET_MEDIUM_STATUS, NULL, NULL);

        if(status > 0xFFFF0000) {
            continue;
        }

        size_t real_status = status & ~DPM_MEDIA_STATUS_MASK;
        uint8_t st = real_status & 0xff;

        size_t index = disk - 'A';

        char* status_string;

        if(real_status == DPM_MEDIA_STATUS_OFFLINE) {
            status_string = "Offline";
        } else if(real_status == DPM_MEDIA_STATUS_LOADING) {
            status_string = "Loading";
        } else if(real_status == DPM_MEDIA_STATUS_ONLINE) {
            status_string = "Online";
        } else {
            status_string = "Unknown";
        }

        if(statuses[index] != st) {
            char* logstring;
            asprintf(&logstring, "Disk %c changed its status to %s", disk, status);
            il_log(logstring);
            kfree(logstring);

            if(real_status == DPM_MEDIA_STATUS_ONLINE) {
                fsm_dpm_update(disk);
            }
        
            statuses[index] = st;
        }
    }
}

void load_statuses(uint8_t* statuses) {
    for(char disk = 'A'; disk < 'Z'; disk++) {
        DPM_Disk d = dpm_info(disk);

        if(!d.Ready) continue;

        size_t status = dpm_ctl(disk, DPM_COMMAND_GET_MEDIUM_STATUS, NULL, NULL);

        if(status > 0xFFFF0000) {
            continue;
        }

        size_t real_status = status & ~DPM_MEDIA_STATUS_MASK;
        uint8_t st = real_status & 0xff;

        size_t index = disk - 'A';

        statuses[index] = st;
    }
}

void notifier_thread() {
    uint8_t old_statuses[32] = {0};

    load_statuses(old_statuses);

    while(true) {
        notifier_loop(old_statuses);

        sleep_ms(500);
    }
}

void launch_media_notifier() {
    thread_create(
        get_current_proc(),
        notifier_thread,
        0x10000,
        true
    );
}
