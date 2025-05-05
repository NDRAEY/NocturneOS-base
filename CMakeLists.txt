cmake_minimum_required(VERSION 3.21)

project(NocturneOS VERSION 0.3.5)
enable_language(C ASM)

# Options

set(CMAKE_CONFIGURATION_TYPES "Debug;Dev;Release;ReleaseOptimized;ReleaseOptimizedSSE;OptSym" CACHE STRING "Available configurations" FORCE)

set(VALID_ARCHITECTURES "x86" "armv7")
set(SUPPORTED_ARMV7_BOARDS "opi-lite")

if(NOT NOCTURNE_ARCH)
  set(NOCTURNE_ARCH "x86")
endif()

list(FIND VALID_ARCHITECTURES "${NOCTURNE_ARCH}" __INDEX)
if(__INDEX EQUAL -1)
  message(FATAL_ERROR "Invalid architecture: ${NOCTURNE_ARCH}. Accepted values are: ${VALID_ARCHITECTURES}")
endif()

if(NOCTURNE_ARCH STREQUAL "armv7" AND NOT NOCTURNE_BOARD)
	message(WARNING "Board not selected. Select an ARM board you want compile for: ${SUPPORTED_ARMV7_BOARDS}")
	message(FATAL_ERROR "Read the warning above. When you're ready, add this to your CMake command: `-DNOCTURNE_BOARD=<myboard>`")
endif()

if(NOCTURNE_ARCH STREQUAL "armv7")
	list(FIND SUPPORTED_ARMV7_BOARDS "${NOCTURNE_BOARD}" __INDEX)
	if(__INDEX EQUAL -1)
		message(WARNING "Invalid board selected. Select an ARM board you want compile for: ${SUPPORTED_ARMV7_BOARDS}")
		message(FATAL_ERROR "Read the warning above. When you're ready, add this to your CMake command: `-DNOCTURNE_BOARD=<myboard>`")
	endif()
endif()

option(NOCTURNE_USE_CLANG "Use Clang compiler for building" OFF)

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "LANG=en_US.UTF-8 ${COMMAND}")

if(NOCTURNE_ARCH STREQUAL "x86")
	set(CLANG_ARCHITECTURE_TRIPLE "i386-pc-none")
	set(RUST_TARGET_TRIPLE "i686-unknown-none")
elseif(NOCTURNE_ARCH STREQUAL "armv7")
	set(CLANG_ARCHITECTURE_TRIPLE "armv7-unknown-none")
	set(RUST_TARGET_TRIPLE "armv7-unknown-none")
endif()

set(ISO_DIR "${CMAKE_SOURCE_DIR}/iso/")
set(KERNEL kernel.elf)

set(C_INCLUDES "-I${CMAKE_SOURCE_DIR}/kernel/include/ -I${CMAKE_SOURCE_DIR}/kernel/src/lib/libstring/include -I${CMAKE_SOURCE_DIR}/kernel/src/lib/libvector/include")
set(CMAKE_ASM_FLAGS "-m32 -g")
set(CMAKE_C_FLAGS "-static -nostdlib -fno-stack-protector -fno-builtin -ffreestanding -Wall -Wno-div-by-zero -Wno-address-of-packed-member -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -mno-red-zone -MMD -MP -fdiagnostics-color -m32 -march=i386 -Wl,--build-id=none ${C_INCLUDES} -std=c11 -Werror=return-type")
set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/kernel/asm/link.ld -static -z noexecstack")


if(NOCTURNE_USE_CLANG)
    message("Using Clang compiler!")
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_C_FLAGS "-target ${CLANG_ARCHITECTURE_TRIPLE} ${CMAKE_C_FLAGS}")
else()
    message("Using GCC compiler!")
    set(CMAKE_C_COMPILER "gcc")
	
	if(NOCTURNE_ARCH STREQUAL "armv7")
		set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
	endif()
	
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
set(CMAKE_C_FLAGS_DEV "-O0" CACHE STRING "Flags for Dev" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-DRELEASE -O0" CACHE STRING "Flags for Release" FORCE)
set(CMAKE_C_FLAGS_RELEASEOPTIMIZED "-DRELEASE -O2" CACHE STRING "Flags for ReleaseOptimized" FORCE)
set(CMAKE_C_FLAGS_RELEASEOPTIMIZEDSSE "-DRELEASE -O2 -msse2" CACHE STRING "Flags for ReleaseOptimizedSSE" FORCE)
set(CMAKE_C_FLAGS_OPTSYM "-DRELEASE -O2 -ggdb3" CACHE STRING "Flags for OptSym" FORCE)

set(ISO_FILE "NocturneOS_${CMAKE_BUILD_TYPE}.iso")


set(QEMU_MEMORY_SIZE 256M)

#set(QEMU "qemu-system-x86_64")
set(QEMU "qemu-system-i386")
set(QEMU_FLAGS -m ${QEMU_MEMORY_SIZE} -cdrom ${CMAKE_CURRENT_BINARY_DIR}/${ISO_FILE} -d int,cpu_reset,guest_errors -serial mon:stdio -rtc base=localtime -boot d -usb -audiodev pa,id=audiopa -device ac97,audiodev=audiopa -netdev user,id=net0,net=192.168.111.0,dhcpstart=192.168.111.128,hostfwd=tcp::9999-:9999 -device rtl8139,netdev=net0,id=mydev0 -object filter-dump,id=dump0,netdev=net0,file=netdump.pcap -trace "*ac97*")

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
	# kernel/asm/64bit_on_32bit.s
)

if(NOCTURNE_ARCH STREQUAL "x86")
	set(C_SRC kernel/src/arch/x86/init.c)
elseif(NOCTURNE_ARCH STREQUAL "armv7")
	set(C_SRC kernel/src/arch/armv7/init.c)
endif()

set(C_SRC
	${C_SRC}
	kernel/src/sys/cpu_isr.c 
	kernel/src/sys/gdt.c 
	kernel/src/sys/isr.c 
	kernel/src/io/ports.c 
	kernel/src/io/serial_port.c 
	kernel/src/lib/string.c 
	kernel/src/sys/timer.c 
	kernel/src/sys/logo.c 
	kernel/src/lib/math/math.c 
	kernel/src/mem/pmm.c	
	kernel/src/mem/vmm.c	
	kernel/src/lib/stdio.c 
	kernel/src/lib/split.c 
	kernel/src/io/screen.c 
	kernel/src/io/tty.c 
	kernel/src/fs/fsm.c 
	kernel/src/lib/time_conversion.c 
	kernel/src/fs/nvfs.c 
	kernel/src/fs/tempfs.c 
	kernel/src/sys/scheduler.c
	kernel/src/lib/php/pathinfo.c 
	kernel/src/drv/psf.c 
	kernel/src/sys/unwind.c 
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
	kernel/src/drv/input/ps2_keyboard.c 
	kernel/src/drv/input/ps2_mouse.c 
	kernel/src/sys/syscalls.c 
	kernel/src/sys/testing.c 
	kernel/src/lib/rand.c 
	kernel/src/drv/cmos.c 
	kernel/src/drv/beeper.c 
	kernel/src/user/env.c
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
	kernel/src/drv/disk/memdisk.c 
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
	kernel/src/extra/plain_runner.c
	kernel/src/fs/fat32.c 
	kernel/src/sys/fxsave_region.c 
	kernel/src/toys/gfxbench.c 
	kernel/src/toys/miniplay.c 
	kernel/src/drv/network/rtl8139.c 
	# kernel/src/drv/network/virtio_network.c 
	kernel/src/fmt/tga.c 
	kernel/src/lib/asprintf.c
	kernel/src/lib/sprintf.c
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
	# kernel/src/toys/minesweeper.c 
	kernel/src/toys/diskctl.c 
	kernel/src/lib/utf_conversion.c 
	kernel/src/sys/file_descriptors.c 
	kernel/src/net/tcp.c 
	kernel/src/net/stack.c 
	# kernel/src/drv/audio/hda.c 
	kernel/src/sys/grub_modules.c 
	kernel/src/drv/disk/mbr.c 
	kernel/src/sys/lapic.c 
	kernel/src/drv/video/intel.c 
	kernel/src/extra/command_parser.c
    kernel/src/lib/libvector/src/vector.c
    kernel/src/lib/libstring/src/string.c
    # kernel/src/lib/intrin.c
	kernel/src/io/keyboard.c
	kernel/src/kernel.c 
)

set(SRC ${ASM_SRC} ${C_SRC})

add_executable(${KERNEL} ${SRC})


# Rust

set(RUST_COMPILER_TOOLCHAIN "nightly-2025-01-11")
set(RUST_TARGET "${RUST_TARGET_TRIPLE}.json")
set(RUST_FLAGS -Zbuild-std=core,compiler_builtins,alloc -Znext-lockfile-bump)
set(RUST_TOOLCHAIN_FULLNAME "${RUST_COMPILER_TOOLCHAIN}-x86_64-unknown-linux-gnu")
set(RUST_CARGO_PATH "~/.rustup/toolchains/${RUST_TOOLCHAIN_FULLNAME}/bin/src/cargo")

add_custom_command(
	OUTPUT "${CMAKE_SOURCE_DIR}/utils/chen/target/release/chen" ".fake"
	COMMAND cargo build --release 
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/utils/chen
	COMMENT "Building Chen utility..."
	DEPENDS "${CMAKE_SOURCE_DIR}/utils/chen/src"
	DEPENDS "${CMAKE_SOURCE_DIR}/utils/chen/Cargo.toml"
	USES_TERMINAL
	VERBATIM
)

add_custom_target(chen ALL DEPENDS "${CMAKE_SOURCE_DIR}/utils/chen/target/release/chen")
add_dependencies(${KERNEL} chen)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_command(
		OUTPUT "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/debug/libnocturne.a" "fake.a"
		COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} rustc --target ${RUST_TARGET} ${RUST_FLAGS}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
		COMMENT "Building Rust files..."
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/src"
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/Cargo.toml"
		USES_TERMINAL
		VERBATIM
	)

	add_custom_target(rs ALL DEPENDS "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/debug/libnocturne.a")
	add_dependencies(${KERNEL} rs)

	target_link_libraries(${KERNEL} "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/debug/libnocturne.a")

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")

	add_custom_command(
		OUTPUT "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release_no_opt/libnocturne.a" "fake.a"
		COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} rustc --target ${RUST_TARGET} ${RUST_FLAGS} --profile release_no_opt
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
		COMMENT "Building Rust files..."
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/src"
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/Cargo.toml"
		USES_TERMINAL
		VERBATIM
	)

	add_custom_target(rs ALL DEPENDS "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release_no_opt/libnocturne.a")
	add_dependencies(${KERNEL} rs)

	target_link_libraries(${KERNEL} "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release_no_opt/libnocturne.a")

else()

	add_custom_command(
		OUTPUT "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release/libnocturne.a" "fake.a"
		COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} rustc --target ${RUST_TARGET} ${RUST_FLAGS} --release
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
		COMMENT "Building Rust files..."
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/src"
		DEPENDS "${CMAKE_SOURCE_DIR}/rust/Cargo.toml"
		USES_TERMINAL
		VERBATIM
	)

	add_custom_target(rs DEPENDS "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release/libnocturne.a")
	add_dependencies(${KERNEL} rs)

	target_link_libraries(${KERNEL} "${CMAKE_SOURCE_DIR}/rust/target/${RUST_TARGET_TRIPLE}/release/libnocturne.a")
endif()

# Targets

add_custom_command(
	TARGET ${KERNEL}
	COMMAND ${CMAKE_SOURCE_DIR}/utils/chen/target/release/chen ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}
	COMMENT "Adding symbols..."
	POST_BUILD
)

add_custom_target(
	copy_symbols
	COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}.map ${ISO_DIR}/${KERNEL}.map
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS ${KERNEL}
	COMMENT "Copying files..."
)

if(
	(NOT (${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
	AND
	(NOT (${CMAKE_BUILD_TYPE} STREQUAL "Dev"))
    AND
	(NOT (${CMAKE_BUILD_TYPE} STREQUAL "OptSym"))
	)
    
	add_custom_target(
			strip_down
            COMMAND strip -s ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}
            COMMENT "Copying files..."
			DEPENDS copy_symbols
    )

    add_custom_target(
            copy_kernel
            COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} ${ISO_DIR}/${KERNEL}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS strip_down
            COMMENT "Copying files..."
    )
else()
    add_custom_target(
            copy_kernel
            COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} ${ISO_DIR}/${KERNEL}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS copy_symbols
            COMMENT "Copying files..."
    )
endif()

add_custom_target(
	clippy
    COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} clippy --target ${RUST_TARGET} ${RUST_FLAGS}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
	USES_TERMINAL
	VERBATIM
)

add_custom_target(
	fmt
	COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} fmt
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
	USES_TERMINAL
	VERBATIM
)

add_custom_target(
  check
	COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} check
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
	USES_TERMINAL
	VERBATIM
)


add_custom_target(
	doc
	COMMAND cargo +${RUST_COMPILER_TOOLCHAIN} doc
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
	USES_TERMINAL
	VERBATIM
)


add_custom_target(
    ramdisk
    COMMAND tar cvf ${ISO_DIR}/ramdisk -C ramdisk/ .
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

add_custom_target(
        run_with_disk
        COMMAND ${QEMU} -accel kvm ${QEMU_FLAGS} -hda ${CMAKE_SOURCE_DIR}/test_disk.img
        USES_TERMINAL
        DEPENDS iso
)
