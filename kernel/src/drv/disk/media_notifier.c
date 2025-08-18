#include <common.h>
#include <io/ports.h>
#include <sys/scheduler.h>
#include <lib/asprintf.h>
#include <lib/math.h>

#include <fs/fsm.h>
#include <mem/vmm.h>

#include <drv/disk/dpm.h>

#include <generated/diskman.h>
#include <generated/diskman_commands.h>

void il_log(const char *message);

void notifier_loop(uint8_t *statuses)
{
    size_t disk_count = diskman_get_registered_disk_count();

    for (size_t i = 0, real_count = MIN(disk_count, 1024U); i < real_count; i++)
    {
        uint32_t status = 0;
        char *name = diskman_get_disk_id_by_index(i);

        long long resp = diskman_control(
            name,
            DISKMAN_COMMAND_GET_MEDIUM_STATUS,
            NULL,
            0,
            (uint8_t*)&status,
            sizeof(uint32_t)
        );

        if(resp == -1) {
            statuses[i] = 0;
            
            kfree(name);

            continue;
        }

        char* status_string;

        if(status == DPM_MEDIA_STATUS_OFFLINE) {
            status_string = "Offline";
        } else if(status == DPM_MEDIA_STATUS_LOADING) {
            status_string = "Loading";
        } else if(status == DPM_MEDIA_STATUS_ONLINE) {
            status_string = "Online";
        } else {
            status_string = "Unknown";
        }

        if(statuses[i] != status) {
            char* logstring;
            asprintf(&logstring, "Disk `%s` changed its status to `%s`", name, status_string);
            il_log(logstring);
            kfree(logstring);

            if(status == DPM_MEDIA_STATUS_ONLINE) {
                // Run filesystem detection on this disk
                fsm_dpm_update(name);
            } else if(status == DPM_MEDIA_STATUS_OFFLINE) {
                // Detach filesystem on eject
                fsm_detach_fs(name);
            }

            statuses[i] = status;
        }

        kfree(name);
    }
}

void load_statuses(uint8_t *statuses)
{
    size_t disk_count = diskman_get_registered_disk_count();

    for (size_t i = 0, real_count = MIN(disk_count, 1024U); i < real_count; i++)
    {
        uint32_t status = 0;
        char *name = diskman_get_disk_id_by_index(i);

        long long resp = diskman_control(
            name,
            DISKMAN_COMMAND_GET_MEDIUM_STATUS,
            NULL,
            0,
            (uint8_t*)&status,
            sizeof(uint32_t)
        );

        if(resp == -1) {
            statuses[i] = 0;
            
            kfree(name);

            continue;
        }

        statuses[i] = (uint8_t)status;

        kfree(name);
    }
}

void notifier_thread()
{
    uint8_t old_statuses[1024] = {0};

    load_statuses(old_statuses);

    while (true)
    {
        notifier_loop(old_statuses);

        sleep_ms(500);
    }
}

void launch_media_notifier()
{
    thread_create(
        get_current_proc(),
        notifier_thread,
        0x10000,
        true);
}
