cmake_minimum_required(VERSION 3.21)

project(NocturneOS VERSION 0.3.5)
enable_language(C ASM)

# Options

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseOptimized" CACHE STRING "Available configurations" FORCE)

option(NOCTURNE_USE_CLANG "Use Clang compiler for building" OFF)

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "LANG=en_US.UTF-8 ${COMMAND}")

set(ISO_DIR "${CMAKE_SOURCE_DIR}/iso/")
set(KERNEL kernel.elf)

set(C_INCLUDES "-I${CMAKE_SOURCE_DIR}/kernel/include/")
set(CMAKE_ASM_FLAGS "-m32 -g")
set(CMAKE_C_FLAGS "-static -nostdlib -fno-stack-protector -fno-builtin -ffreestanding -Wall -Wno-div-by-zero -Wno-address-of-packed-member -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -mno-red-zone -MMD -MP -fdiagnostics-color -m32 -march=i386 -Wl,--build-id=none ${C_INCLUDES}")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/kernel/asm/link.ld -static")

if(NOCTURNE_USE_CLANG)
    message("Using Clang compiler!")
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_C_FLAGS "-target i386-pc-none ${CMAKE_C_FLAGS}")
else()
    message("Using GCC compiler!")
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_C_FLAGS "-fuse-ld=lld ${CMAKE_C_FLAGS}")
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING
            "No build configuration selected! Please specify one of the following:\n"
            "  -DCMAKE_BUILD_TYPE=Debug\n"
            "  -DCMAKE_BUILD_TYPE=Release\n"
            "  -DCMAKE_BUILD_TYPE=ReleaseOptimized\n"
            "Defaulting to: Debug"
    )
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_C_FLAGS_DEBUG "-ggdb3 -O0" CACHE STRING "Flags for Debug" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-DRELEASE -O0" CACHE STRING "Flags for Release" FORCE)
set(CMAKE_C_FLAGS_RELEASEOPTIMIZED "-DRELEASE -O2" CACHE STRING "Flags for ReleaseOptimized" FORCE)

set(ISO_FILE "NocturneOS_${CMAKE_BUILD_TYPE}.iso")


set(QEMU_MEMORY_SIZE 128M)

#set(QEMU "qemu-system-x86_64")
set(QEMU "qemu-system-i386")
set(QEMU_FLAGS -m ${QEMU_MEMORY_SIZE} -cdrom ${CMAKE_CURRENT_BINARY_DIR}/${ISO_FILE} -d int,cpu_reset,guest_errors -serial mon:stdio -rtc base=localtime -boot d -usb -net user -audiodev pa,id=audiopa -device ac97,audiodev=audiopa)

set(MKRESCUE grub-mkrescue)
set(MKRESCUE_FLAGS -o "${CMAKE_CURRENT_BINARY_DIR}/${ISO_FILE}" ${ISO_DIR} -V NocturneOS)

# Sources

set(ASM_SRC kernel/asm/init.s 
	kernel/asm/interrupt.s 
	kernel/asm/sys_calls.s 
	kernel/asm/usr.s 
	kernel/asm/sse.s 
	kernel/asm/gdt.s 
	kernel/asm/paging.s 
	kernel/asm/regs.s 
	kernel/src/lib/setjmp.s 
	kernel/asm/switch_task.s 
	kernel/asm/64bit_on_32bit.s)

set(C_SRC 	kernel/src/sys/cpuinfo.c 
	kernel/src/sys/cpu_isr.c 
	kernel/src/sys/gdt.c 
	kernel/src/sys/isr.c 
	kernel/src/io/ports.c 
	kernel/src/io/serial_port.c 
	kernel/src/lib/string.c 
	kernel/src/drv/fpu.c 
	kernel/src/sys/timer.c 
	kernel/src/sys/logo.c 
	kernel/src/lib/math/math.c 
	kernel/src/mem/pmm.c	
	kernel/src/mem/vmm.c	
	kernel/src/lib/stdio.c 
	kernel/src/lib/split.c 
	kernel/src/io/screen.c 
	kernel/src/io/tty.c 
	kernel/src/fs/tarfs.c 
	kernel/src/fs/fsm.c 
	kernel/src/fs/nvfs.c 
	kernel/src/fs/natfs.c 
	kernel/src/fs/tempfs.c 
	kernel/src/lib/php/str_replace.c 
	kernel/src/sys/scheduler.c 
	kernel/src/lib/php/explode.c 
	kernel/src/lib/php/pathinfo.c 
	kernel/src/drv/psf.c 
	kernel/src/sys/unwind.c 
	kernel/src/fs/NatSuki.c 
	kernel/src/drv/disk/initrd.c 
	kernel/src/drv/disk/dpm.c 
	kernel/src/lib/list.c 
	kernel/src/lib/fileio.c 
	kernel/src/sys/sync.c 
	kernel/src/gui/basics.c 
	kernel/src/lib/pixel.c 
	kernel/src/sys/bootscreen.c 
	kernel/src/debug/hexview.c 
	kernel/src/drv/video/vbe.c 
	kernel/src/drv/input/keyboard.c 
	kernel/src/drv/input/mouse.c 
	kernel/src/sys/syscalls.c 
	kernel/src/sys/testing.c 
	kernel/src/sys/trigger.c 
	kernel/src/lib/rand.c 
	kernel/src/drv/cmos.c 
	kernel/src/drv/beeper.c 
	kernel/src/user/env.c 
	kernel/src/drv/pci.c 
	kernel/src/gui/pointutils.c 
	kernel/src/gui/line.c 
	kernel/src/gui/circle.c 
	kernel/src/lib/math/exp.c 
	kernel/src/lib/math/log.c 
	kernel/src/lib/math/pow.c 
	kernel/src/lib/math/acos.c 
	kernel/src/lib/math/asin.c 
	kernel/src/lib/math/atan.c 
	kernel/src/lib/math/modf.c 
	kernel/src/lib/math/integral.c 
	kernel/src/lib/math/sin.c 
	kernel/src/lib/math/cos.c 
	kernel/src/lib/math/tan.c 
	kernel/src/lib/math/sqrt.c 
	kernel/src/io/rgb_image.c 
	kernel/src/lib/math/cbrt.c 
	kernel/src/sys/cpuid.c	
	kernel/src/drv/disk/ata.c 
	kernel/src/drv/disk/atapi.c 
	kernel/src/sys/cputemp.c	
	kernel/src/net/endianess.c 
	kernel/src/net/cards.c 
	kernel/src/net/ethernet.c 
	kernel/src/net/arp.c 
	kernel/src/net/ipv4.c 
	kernel/src/net/udp.c 
	kernel/src/net/dhcp.c 
	kernel/src/net/icmp.c 
	kernel/src/sys/system.c 
	kernel/src/io/status_sounds.c 
	kernel/src/io/status_loggers.c 
	kernel/src/extra/cli.c 
	kernel/src/sys/variable.c	
	kernel/src/fs/fat32.c 
	kernel/src/sys/fxsave_region.c 
	kernel/src/toys/gfxbench.c 
	kernel/src/toys/miniplay.c 
	kernel/src/drv/network/rtl8139.c 
	kernel/src/drv/network/virtio_network.c 
	kernel/src/fmt/tga.c 
	kernel/src/lib/sprintf.c 
	kernel/src/debug/ubsan.c 
	kernel/src/drv/disk/floppy.c 
	kernel/src/drv/disk/ata_dma.c 
	kernel/src/drv/audio/ac97.c 
	kernel/src/sys/elf.c 
	kernel/src/sys/acpi.c 
	kernel/src/sys/pixfmt.c 
	kernel/src/desktop/render.c 
	kernel/src/desktop/window.c 
	kernel/src/desktop/widget.c 
	kernel/src/desktop/widget_button.c 
	kernel/src/desktop/widget_progress.c 
	kernel/src/desktop/widget_image.c 
	kernel/src/desktop/widget_label.c 
	kernel/src/desktop/eki.c 
	kernel/src/desktop/parallel_desktop.c 
	kernel/src/sys/mtrr.c 
	kernel/src/net/net_info_cli.c 
	kernel/src/toys/mala.c 
	kernel/src/debug/memmeter.c 
	kernel/src/drv/disk/ahci.c 
	kernel/src/drv/disk/ata_pio.c 
	kernel/src/toys/minesweeper.c 
	kernel/src/toys/calendar.c 
	kernel/src/toys/diskctl.c 
	kernel/src/lib/utf_conversion.c 
	kernel/src/lib/base64.c 
	kernel/src/sys/file_descriptors.c 
	kernel/src/net/tcp.c 
	kernel/src/net/stack.c 
	kernel/src/toys/pavi.c 
	kernel/src/drv/audio/hda.c 
	kernel/src/sys/grub_modules.c 
	kernel/src/drv/disk/mbr.c 
	kernel/src/sys/lapic.c 
	kernel/src/drv/ps2.c 
	kernel/src/drv/video/intel.c 
	kernel/src/extra/command_parser.c
    kernel/src/lib/libvector/src/vector.c
    kernel/src/lib/libstring/src/string.c
	kernel/src/kernel.c 
)

set(SRC ${ASM_SRC} ${C_SRC})

add_executable(${KERNEL} ${SRC})


# Targets

if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    add_custom_command(
            TARGET ${KERNEL}
            COMMAND strip -s ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}
            COMMENT "Copying files..."
    )

    add_custom_target(
            copy_kernel
            COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} ${ISO_DIR}/${KERNEL}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${KERNEL}
            COMMENT "Copying files..."
    )
else()
    add_custom_command(
            TARGET ${KERNEL}
            COMMAND bash ${CMAKE_SOURCE_DIR}/tools/geninsert.sh ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}
            COMMENT "Adding debug symbols..."
    )

    add_custom_target(
            copy_kernel
            COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} ${ISO_DIR}/${KERNEL}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${KERNEL}
            COMMENT "Copying files..."
    )
endif()

add_custom_target(
    ramdisk
    COMMAND tar cvf ${ISO_DIR}/ramdisk ramdisk/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS copy_kernel
)

add_custom_target(
        iso
        COMMAND ${MKRESCUE} ${MKRESCUE_FLAGS}
        DEPENDS ramdisk
)

add_custom_target(
        run
        COMMAND ${QEMU} -accel kvm ${QEMU_FLAGS}
        USES_TERMINAL
        DEPENDS iso
)
