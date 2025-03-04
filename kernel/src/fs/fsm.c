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
#include "lib/php/explode.h"
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

int fsm_isPathToFile(const char* Path, const char* Name){
	char* zpath = pathinfo(Name, PATHINFO_DIRNAME);					/// Получаем родительскую папку элемента
	char* bpath = pathinfo(Name, PATHINFO_BASENAME);				/// Получаем имя файла (или пустоту если папка)
	bool   isCheck1 = strcmpn(zpath,Path);				/// Проверяем совпадение путей
	bool   isCheck2 = strlen(bpath) == 0;				/// Проверяем, что путе нет ничего лишнего (будет 0, если просто папка)
	bool   isCheck3 = str_contains(Name, Path);	/// Проверяем наличие, вхождения путя
	size_t c1 = str_cdsp2(Path,'\\');
	size_t c2 = str_cdsp2(Name,'\\');
	size_t c3 = str_cdsp2(Path,'/');
	size_t c4 = str_cdsp2(Name,'/');
	
	bool   isCheck4 = ((c2 - c1) == 1) && (c4 == c3);
	bool   isCheck5 = ((c4 - c3) == 1) && (c2 == c1);

    bool isPassed = ((isCheck1 && !isCheck2 && isCheck3) || (!isCheck1 && isCheck2 && isCheck3 && (isCheck4 || isCheck5)));

	kfree(zpath);
	kfree(bpath);

	return isPassed;
}

int fsm_getIDbyName(const char* Name){
	for (int i = 0; i < fsm_entries->size; i++) {
        vector_result_t res = vector_get(fsm_entries, i);
        FSM* fsm = (FSM*)res.element;

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
	memcpy(fsm->Name, Name, strlen(Name));

    vector_push_back(fsm_entries, (size_t)fsm);

    qemu_ok("Registered filesystem: `%s`", Name);
}

void fsm_dpm_update(char Letter){
    const char* BLANK = "Unknown";

    if (Letter == -1) {
        // Global update
        for(int i = 0; i < 26; i++){
            int DISKID = i + 65;
            
            dpm_LabelUpdate(DISKID, BLANK);
            dpm_FileSystemUpdate(DISKID, BLANK);

            DPM_Disk dpm = dpm_info(DISKID);

            if (dpm.Ready != 1) {
            	continue;
            }

            for(int f = 0; f < fsm_entries->size; f++) {
                FSM* fsm = (FSM*)vector_get(fsm_entries, f).element;

                qemu_note("[FSM] [DPM] >>> Disk %c | Test %s", DISKID, fsm);

                int detect = fsm->Detect(DISKID);

                if (detect != 1) {
                	continue;
                }

                char* lab_test = kcalloc(1, 129);

                fsm->Label(DISKID, lab_test);

                dpm_LabelUpdate(DISKID, lab_test);
                dpm_FileSystemUpdate(DISKID, fsm->Name);

                qemu_note("                       | Label: %s", lab_test);

                kfree(lab_test);

                break;
            }
        }
    } else {
        // Personal update
        int DISKID  = Letter;
        dpm_LabelUpdate(DISKID, BLANK);
        dpm_FileSystemUpdate(DISKID, BLANK);

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
