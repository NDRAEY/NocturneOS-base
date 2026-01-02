/**
 * @file drv/fs/nvfs.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief New Virtual File System - Новая виртуальная файловая система
 * @version 0.4.3
 * @date 2023-10-14
 * @copyright Copyright SayoriOS Team (c) 2022-2026
*/

#include <io/ports.h>
#include <fs/nvfs.h>

#include "mem/vmm.h"
#include "fs/fsm.h"

#include "generated/nvfs_helper.h"
#include <io/logging.h>

bool nvfs_debug = false;

size_t nvfs_read(const char* Name, size_t Offset, size_t Count, void* Buffer){
	if(nvfs_debug) {
		qemu_log("Name=%s", Name);
	}

	NVFS_DECINFO* vinfo = nvfs_decode(Name);
	size_t res = 0;

	if (vinfo->Ready == 0) {
		goto end;
	}
	
	res = fsm_read(vinfo->DriverFS, vinfo->disk_id, vinfo->Path, Offset, Count, Buffer);

end:
	nvfs_decinfo_free(vinfo);

	return res;
}

int nvfs_create(const char* Name, int Mode){
	NVFS_DECINFO* vinfo = nvfs_decode(Name);
	size_t res = 0;

	if (vinfo->Ready == 0) {
		goto end;
	}

	res = fsm_create(vinfo->DriverFS, vinfo->disk_id, vinfo->Path, Mode);

end:
	nvfs_decinfo_free(vinfo);
	return res;
}

int nvfs_delete(const char* Name, int Mode){
	NVFS_DECINFO* vinfo = nvfs_decode(Name);
	size_t res = 0;

	if (vinfo->Ready == 0) {
		goto end;
	}
	
	res = fsm_delete(vinfo->DriverFS, vinfo->disk_id, vinfo->Path, Mode);

	end:

	nvfs_decinfo_free(vinfo);

	return res;
}

size_t nvfs_write(const char* Name, size_t Offset, size_t Count, const void *Buffer){
	NVFS_DECINFO* vinfo = nvfs_decode(Name);
	size_t res = 0;

	if (vinfo->Ready == 0) {
		goto end;
	}

	res = fsm_write(vinfo->DriverFS, vinfo->disk_id, vinfo->Path, Offset, Count, Buffer);

	end:

	nvfs_decinfo_free(vinfo);

	return res;
}

FSM_FILE nvfs_info(const char* Name) {
	NVFS_DECINFO* vinfo = nvfs_decode(Name);  // no memleak
    // if (nvfs_debug) {
	//     qemu_log("NVFS INFO:\n"
	// 		"Ready: %d\n"
	// 		"Disk: `%s`\n"
	// 		"Path: `%s`\n"
	// 		"Disk Online: %d\n"
	// 		"Disk file system: `%s`\n"
	// 		"Filesystem ID: %d",
	// 		vinfo->Ready,
	// 		vinfo->disk_id,
	// 		vinfo->Path,
	// 		vinfo->Online,
	// 		vinfo->FileSystem,
	// 		vinfo->DriverFS
	//    );
    // }

	FSM_FILE file = { .Ready = 0 };

	if (vinfo->Ready != 1){
		goto end;
	}

	file = fsm_info(vinfo->DriverFS, vinfo->disk_id, vinfo->Path);

// 	qemu_log("NVFS FILE INFO:\n"
// 		"Ready: %d\n"
// 		"Name: `%s`\n"
// 		"Path: `%s`\n"
// 		"Mode: %d\n"
// 		"Type: `%s`\n"
// 		"Size: %d",
// 		file.Ready,
// 		file.Name,
// 		file.Path,
// 		file.Mode,
// 		file.Type == TYPE_FILE ? "File" : "Dir",
// 		file.Size
//    );
end:

	nvfs_decinfo_free(vinfo);

	return file;
}

void nvfs_dir_v2(const char* Name, FSM_DIR* dir) {
	NVFS_DECINFO* vinfo = nvfs_decode(Name);
	
	if (!vinfo->Ready) {
		dir->Ready = 0;
		goto end;
	}

	fsm_dir(vinfo->DriverFS, vinfo->disk_id, vinfo->Path, dir);

	// qemu_printf("[%d] Files: %p (%d + %d + %d)\n", dir->Ready, dir->Files, dir->CountFiles, dir->CountDir, dir->CountOther);
	
	end:
	nvfs_decinfo_free(vinfo);
}

void nvfs_close_dir_v2(FSM_DIR* dir) {
	if(dir == NULL) {
		return;
	}

	size_t count = dir->CountDir + dir->CountFiles + dir->CountOther;

	for(size_t i = 0; i < count; i++) {
		fsm_file_close(dir->Files + i);
	}

	kfree(dir->Files);
}

void nvfs_decinfo_free(NVFS_DECINFO* decinfo) {
	kfree(decinfo->Path);
	kfree(decinfo);
}
