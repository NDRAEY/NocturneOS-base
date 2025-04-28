/**
 * @file drv/fs/fsm.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), Павленко Андрей (pikachu_andrey@vk.com)
 * @brief File System Manager (Менеджер файловых систем)
 * @version 0.3.5
 * @date 2023-10-16
 * @copyright Copyright SayoriOS & NocturneOS Team (c) 2022-2025
*/

#include <io/ports.h>
#include <fs/fsm.h>
#include <lib/php/pathinfo.h>
#include "lib/php/str_contains.h"
#include "lib/sprintf.h"
#include "mem/vmm.h"

#include "drv/disk/dpm.h"
#include <vector.h>

vector_t* fsm_entries = 0;

int C_FSM = 0;
bool fsm_debug = false;

void fsm_init() {
    fsm_entries = vector_new();
}

int fsm_getIDbyName(const char* Name){
	for (int i = 0; i < fsm_entries->size; i++) {
        vector_result_t res = vector_get(fsm_entries, i);
        FSM* fsm = (FSM*)res.element;

        qemu_note("`%s` =? `%s`", fsm->Name, Name);

		if (strcmp(fsm->Name, Name) != 0) {
            continue;
        }

		return i;
	}

	return -1;
}

void fsm_dump(FSM_FILE file){
	qemu_log("  |--- Ready  : %d",file.Ready);
	qemu_log("  |--- Name   : %s",file.Name);
	qemu_log("  |--- Path   : %s",file.Path);
	qemu_log("  |--- Mode   : %d",file.Mode);
	qemu_log("  |--- Size   : %d",file.Size);
	qemu_log("  |--- Type   : %d",file.Type);
	qemu_log("  |--- Date   : %d",file.LastTime.year);
}

size_t fsm_read(int FIndex, char DIndex, const char* Name, size_t Offset, size_t Count, void* Buffer){
    if (fsm_debug) {
        qemu_log("[FSM] [READ] F:%d | D:%d | N:`%s` | O:%d | C:%d",FIndex,DIndex,Name,Offset,Count);
    }
    
    vector_result_t res = vector_get(fsm_entries, FIndex);

	if (res.error) {
        return 0;
    }

    if (fsm_debug) {
        qemu_log("[FSM] [READ] GO TO DRIVER");
    }
    
    FSM* fsm = (FSM*)res.element;
	
    return fsm->Read(DIndex, Name, Offset, Count, Buffer);
}


int fsm_create(int FIndex, char DIndex, const char* Name, int Mode) {
    vector_result_t res = vector_get(fsm_entries, FIndex);

	if (res.error) {
        return 0;
    }

    FSM* fsm = (FSM*)res.element;

	return fsm->Create(DIndex,Name,Mode);
}

int fsm_delete(int FIndex, const char DIndex, const char* Name, int Mode) {
    vector_result_t res = vector_get(fsm_entries, FIndex);

	if (res.error) {
		return 0;
    }

    FSM* fsm = (FSM*)res.element;

	return fsm->Delete(DIndex, Name, Mode);
}

size_t fsm_write(int FIndex, const char DIndex, const char* Name, size_t Offset, size_t Count, const void* Buffer){
    vector_result_t res = vector_get(fsm_entries, FIndex);
	
    if (res.error) {
		return 0;
    }

    FSM* fsm = (FSM*)res.element;

	return fsm->Write(DIndex,Name,Offset, Count, Buffer);
}

FSM_FILE fsm_info(int FIndex,const char DIndex, const char* Name){
    if (fsm_debug) {
        qemu_log("[FSM] [INFO] F:%d | D:%d | N:%s",FIndex,DIndex,Name);
    }

    vector_result_t res = vector_get(fsm_entries, FIndex);

	if (res.error) {
        if (fsm_debug) {
            qemu_log("[FSM] [INFO] READY == 0");
        }
		return (FSM_FILE){};
	}
    
    if (fsm_debug) {
        qemu_log("[FSM] [INFO] GO TO GFSM");
    }
    
    FSM* fsm = (FSM*)res.element;
	
    return fsm->Info(DIndex,Name);
}

void fsm_dir(int FIndex, const char DIndex, const char* Name, FSM_DIR* out) {
    if (fsm_debug) {
        qemu_log("[FSM] [DIR] F:%d | D:%d | N:%s",FIndex,DIndex,Name);
    }

    vector_result_t res = vector_get(fsm_entries, FIndex);

	if (res.error) {
        if (fsm_debug) {
            qemu_log("[FSM] %d not ready", FIndex);
        }

		memset(out, 0, sizeof(FSM_DIR));
	}

    FSM* fsm = (FSM*)res.element;

    fsm->Dir(DIndex, Name, out);
}

void fsm_reg(const char* Name,fsm_cmd_read_t Read, fsm_cmd_write_t Write, fsm_cmd_info_t Info, fsm_cmd_create_t Create, fsm_cmd_delete_t Delete, fsm_cmd_dir_t Dir, fsm_cmd_label_t Label, fsm_cmd_detect_t Detect) {
    FSM* fsm = kcalloc(1, sizeof(FSM));
    fsm->Ready = 1;
	fsm->Read = Read;
	fsm->Write = Write;
	fsm->Info = Info;
	fsm->Create = Create;
	fsm->Delete = Delete;
	fsm->Dir = Dir;
	fsm->Label = Label;
	fsm->Detect = Detect;

	fsm->Name = strdynamize(Name);

    vector_push_back(fsm_entries, (size_t)fsm);

    qemu_ok("Registered filesystem: `%s`", Name);
}

void fsm_dpm_update(char Letter){
    if (Letter == -1) {
        // Global update
        for(int i = 0; i < 26; i++){
            int DISKID = i + 65;
            
            DPM_Disk dpm = dpm_info(DISKID);

            if (dpm.Ready != 1) {
                dpm_LabelUpdate(DISKID, NULL);
                dpm_FileSystemUpdate(DISKID, NULL);
        
            	continue;
            }

            for(int f = 0; f < fsm_entries->size; f++) {
                FSM* fsm = (FSM*)vector_get(fsm_entries, f).element;

                qemu_note("[FSM] [DPM] >>> Disk %c | Test %s", DISKID, fsm->Name);

                int detect = fsm->Detect(DISKID);

                if (detect != 1) {
                	continue;
                }

                char* lab_test = kcalloc(1, 64);

                fsm->Label(DISKID, lab_test);

                dpm_LabelUpdate(DISKID, lab_test);
                dpm_FileSystemUpdate(DISKID, fsm->Name);

                // qemu_note("                       | Label: %s", lab_test);

                dpm_dump(DISKID);

                kfree(lab_test);

                break;
            }
        }
    } else {
        // Personal update
        int DISKID  = Letter;

        dpm_LabelUpdate(DISKID, NULL);
        dpm_FileSystemUpdate(DISKID, NULL);

        for(int f = 0; f < fsm_entries->size; f++) {
            FSM* fsm = (FSM*)vector_get(fsm_entries, f).element;

            qemu_note("[FSM] [DPM] >>> Disk %c | Test %s", DISKID, fsm->Name);
            int detect = fsm->Detect(DISKID);

            if (detect != 1) {
                continue;
            }

            char* lab_test = kcalloc(1,129);

            fsm->Label(DISKID, lab_test);
            dpm_LabelUpdate(DISKID, lab_test);
            dpm_FileSystemUpdate(DISKID, fsm->Name);
            qemu_note("[FSM] [DPM] ^^^ Disk %c | Label: %s", DISKID, lab_test);

            kfree(lab_test);

            break;
        }
    }
}
