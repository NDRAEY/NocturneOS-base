#include <common.h>
#include <io/ports.h>
#include <drv/disk/dpm.h>
#include <sys/scheduler.h>

void notifier_loop() {
    for(char disk = 'A'; disk < 'Z'; disk++) {
        DPM_Disk d = dpm_info(disk);

        if(!d.Ready) continue;

        size_t status = dpm_ctl(disk, DPM_COMMAND_GET_MEDIUM_STATUS, NULL, NULL);

        if(status > 0xFFFF0000) {
            continue;
        }

        size_t real_status = status & ~DPM_MEDIA_STATUS_MASK;

        qemu_printf("[%c:] %d\n", disk, real_status);
    }
}

void notifier_thread() {
    while(true) {
        notifier_loop();

        sleep_ms(2500);
    }
}

void launch_media_notifier() {
    thread_create(
        get_current_proc(),
        notifier_thread,
        0x1000,
        true
    );
}