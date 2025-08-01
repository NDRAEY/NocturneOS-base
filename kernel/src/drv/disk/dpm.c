/**
 * @file drv/disk/dpm.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Disk Partition Manager - Менеджер разметки дисков
 * @version 0.3.5
 * @date 2023-10-16
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <io/ports.h>
#include <drv/disk/dpm.h>
#include "mem/vmm.h"

bool dpm_debug = false;

DPM_Disk DPM_Disks[32] = {0};

int dpm_searchFreeIndex(int Index)
{
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	for (int i = Index; i < 32; i++)
	{
		if (DPM_Disks[i].Ready == 1)
			continue;

		return i;
	}

	for (int i = 4; i < 32; i++)
	{
		if (DPM_Disks[i].Ready == 1)
			continue;

		return i;
	}

	return -1;
}

void dpm_set_read_func(char Letter, dpm_disk_read_cmd Read)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return;

	DPM_Disks[Index].Read = Read;
}


void dpm_set_write_func(char Letter, dpm_disk_write_cmd Write)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return;

	DPM_Disks[Index].Write = Write;
}

void dpm_set_command_func(char Letter, dpm_disk_command_cmd Command)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return;

	DPM_Disks[Index].Command = Command;
}

void *dpm_metadata_read(char Letter)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return 0;

	return DPM_Disks[Index].Reserved;
}

void dpm_metadata_write(char Letter, uint32_t Addr)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	DPM_Disks[Index].Reserved = (void *)Addr;
}

/**
 * @brief [DPM] Считывание данных с диска
 *
 * @param Letter - Буква для считывания
 * @param Offset - Отступ для считывания
 * @param Size - Кол-во байт данных для считывания
 * @param Buffer - Буфер куда будет идти запись
 *
 * @return Кол-во прочитанных байт
 */
size_t dpm_read(char Letter, uint64_t high_offset, uint64_t low_offset, size_t Size, void *Buffer)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return DPM_ERROR_NOT_READY;

	if (DPM_Disks[Index].AddrMode == 2)
	{
		// Диск является частью ОЗУ, поэтому мы просто копируем данные оттуда
		if (dpm_debug)
		{
			qemu_log("[DPM] [2] An attempt to read data in 'Disk %c' from position %p to the number of %d bytes.", Index + 65, DPM_Disks[Index].Point + low_offset, Size);
		}
		memcpy(Buffer, (void *)(DPM_Disks[Index].Point + low_offset), Size);

		return Size;
	}
	else if (DPM_Disks[Index].AddrMode == 3)
	{
		// Режим 3, предполагает что вы указали функцию для чтения и записи с диска
		if (dpm_debug)
		{
			qemu_log("[DPM] [3] An attempt to read data in 'Disk %c' from position %p to the number of %d bytes.", Index + 65,
				DPM_Disks[Index].Point + low_offset,
				Size);
		}

		if (DPM_Disks[Index].Read == 0)
		{
			qemu_err("[DPM] [3] Function 404");
			return 0;
		}

		return DPM_Disks[Index].Read(Index, high_offset, low_offset, Size, Buffer);
	}
	else
	{
		if (dpm_debug) {
			qemu_log("[DPM] This functionality has not been implemented yet.");
		}
	}

	return DPM_ERROR_CANT_READ;
}

/**
 * @brief [DPM] Запись данных на диск
 *
 * @param Letter - Буква
 * @param size_t Offset - Отступ
 * @param size_t Size - Кол-во байт данных для записи
 * @param Buffer - Буфер откуда будет идти запись
 *
 * @return size_t - Кол-во записанных байт
 */
size_t dpm_write(char Letter, uint64_t high_offset, uint64_t low_offset, size_t Size, const void *Buffer)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		return DPM_ERROR_NOT_READY;

	if (DPM_Disks[Index].AddrMode == 2)
	{
		// Диск является частью ОЗУ, поэтому мы просто копируем данные туда
		// Опастна! Если не знать, что делать!
		if (dpm_debug) {
			qemu_log("[DPM] [2] An attempt to write data in 'Disk %c' from position %p to the number of %u bytes.",
				Index + 65,
				DPM_Disks[Index].Point + low_offset,
				Size);
		}
		memcpy((void *)(DPM_Disks[Index].Point + low_offset), Buffer, Size);

		return Size;
	}
	else if (DPM_Disks[Index].AddrMode == 3)
	{
		// Режим 3, предполагает что вы указали функцию для чтения и записи с диска
		if (dpm_debug) {
			qemu_log("[DPM] [3] An attempt to write data in 'Disk %c' from position %p to the number of %d bytes.", Index + 65, DPM_Disks[Index].Point + low_offset, Size);
		}

		if (DPM_Disks[Index].Write == 0)
		{
			qemu_err("[DPM] [3] No function");
			return 0;
		}
		return DPM_Disks[Index].Write(Index, high_offset, low_offset, Size, Buffer);
	}
	else
	{
		if (dpm_debug) {
			qemu_log("[DPM] This functionality has not been implemented yet.");
		}
	}

	return DPM_ERROR_CANT_READ;
}

/**
 * @brief [DPM] Запись данных на диск
 *
 * @param Letter - Буква
 * @param size_t Offset - Отступ
 * @param size_t Size - Кол-во байт данных для записи
 * @param Buffer - Буфер откуда будет идти запись
 *
 * @return size_t - Кол-во записанных байт
 */
size_t dpm_ctl(char Letter, size_t command, void* data, size_t length) {
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0) {
		return DPM_ERROR_NOT_READY;
	}

	if(DPM_Disks[Index].Command == NULL) {
		return DPM_ERROR_NOT_IMPLEMENTED;
	}

	if(command == DPM_COMMAND_EJECT) {
		// Detach filesystem on eject.
		dpm_FileSystemUpdate(Letter, NULL);
	}

	return DPM_Disks[Index].Command(Index, command, data, length);
}

int dpm_unmount(char Letter, bool FreeReserved)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 0)
		return 0;

	DPM_Disks[Index].Ready = 0;

	kfree(DPM_Disks[Index].Name);
	kfree(DPM_Disks[Index].FileSystem);

	DPM_Disks[Index].Status = 0;
	DPM_Disks[Index].Size = 0;
	DPM_Disks[Index].Sectors = 0;
	DPM_Disks[Index].SectorSize = 0;
	DPM_Disks[Index].AddrMode = 0;
	DPM_Disks[Index].Point = 0;

	if (FreeReserved && DPM_Disks[Index].Reserved != 0)
	{
		kfree(DPM_Disks[Index].Reserved);
	}
	return 1;
}

/**
 * @brief [DPM] Регистрация дискового раздела
 *
 * @param Letter - Буква для регистрации
 *
 * @return int - Результат регистрации
 */
int dpm_reg(char Letter, char *Name, char *FS, int Status, size_t Size, size_t Sectors, size_t SectorSize, int AddrMode, char *Serial, void *Point)
{
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	if (DPM_Disks[Index].Ready == 1)
	{
		qemu_warn("[DPM] Warning! This letter is already occupied, and an attempt will be made to search for a free letter.");
		Index = dpm_searchFreeIndex(Index);
		if (Index == DPM_ERROR_CANT_MOUNT)
		{
			qemu_warn("[DPM] Sorry, but the disk could not be registered because there is no free letter. Delete the extra devices and try again.");
			return DPM_ERROR_CANT_MOUNT;
		}
		qemu_log("[DPM] The drive was assigned the letter '%c'", Index + 65);
	}

	DPM_Disks[Index].Name = strdynamize(Name);
	
	DPM_Disks[Index].FileSystem = strdynamize(FS);

	memcpy(DPM_Disks[Index].Serial, Serial, strlen(Serial));
	DPM_Disks[Index].Status = Status;
	DPM_Disks[Index].Size = Size;
	DPM_Disks[Index].Sectors = Sectors;
	DPM_Disks[Index].SectorSize = SectorSize;
	DPM_Disks[Index].AddrMode = AddrMode;
	DPM_Disks[Index].Point = Point;

	DPM_Disks[Index].Ready = 1;

	qemu_log("[DPM] Disk '%c' is registered!", Index + 65);
	qemu_log("  |-- Name: %s", DPM_Disks[Index].Name);
	qemu_log("  |-- Serial: %s", DPM_Disks[Index].Serial);
	qemu_log("  |-- FileSystem: %s", DPM_Disks[Index].FileSystem ?: "(null)");
	qemu_log("  |-- Status: %d", DPM_Disks[Index].Status);
	// qemu_log("  |-- Size: %d", DPM_Disks[Index].Size);  // Most disks have capacity is greater than 4GB (32-bit space), so every disk with capacity greater than 4GB will give a bug. (We need to impelement BigInt?)
	qemu_log("  |-- Sectors: %d", DPM_Disks[Index].Sectors);
	qemu_log("  |-- SectorSize: %d", DPM_Disks[Index].SectorSize);
	qemu_log("  |-- AddrMode: %d", DPM_Disks[Index].AddrMode);
	qemu_log("  |-- Point: %p", DPM_Disks[Index].Point);

	return Index;
}

void dpm_FileSystemUpdate(char Letter, char *FileSystem)
{
	Letter -= 65;

	size_t index = (Letter < 0 || Letter > 25 ? 0 : Letter);

	if(DPM_Disks[index].FileSystem != NULL) {
		kfree(DPM_Disks[index].FileSystem);
	}

	if(FileSystem != NULL) {
		DPM_Disks[index].FileSystem = strdynamize(FileSystem);
	} else {
		DPM_Disks[index].FileSystem = NULL;
	}

	// qemu_warn("[%d] Updated filesystem to: `%s` (%x)", Letter, DPM_Disks[index].FileSystem ?: "(null)", DPM_Disks[index].FileSystem);
}

void dpm_LabelUpdate(char Letter, const char *Label)
{
	Letter -= 65;

	size_t index; // = (Letter > 32 ? Letter - 32 : Letter);
	index = (Letter < 0 || Letter > 25 ? 0 : Letter);

	if(DPM_Disks[index].Name != NULL) {
		kfree(DPM_Disks[index].Name);
	}

	if(Label != NULL) {
		DPM_Disks[index].Name = strdynamize(Label);
	} else {
		DPM_Disks[index].Name = 0;
	}

	// qemu_warn("[%d] Updated label to: `%s` (%x)", Letter, DPM_Disks[index].FileSystem ?: "(null)", DPM_Disks[index].FileSystem);
}

size_t dpm_disk_size(char Letter)
{
	Letter -= 65;

	size_t index = (Letter > 32 ? Letter - 32 : Letter);
	index = (Letter < 0 || Letter > 25 ? 0 : Letter);

	return (DPM_Disks[index].Size > 0 ? DPM_Disks[index].Size : 0);
}

DPM_Disk dpm_info(char Letter)
{
	Letter -= 65;

	size_t index; // = (Letter > 32 ? Letter - 32 : Letter);
	index = (Letter < 0 || Letter > 25 ? 0 : Letter);

	return DPM_Disks[index];
}

void dpm_dump(char Letter) {
	DPM_Disk info = dpm_info(Letter);
  (void)info;

	qemu_log("  |-- Name: %s (%p)", info.Name ?: "(null)", info.Name);
	qemu_log("  |-- Serial: %s", info.Serial);
	qemu_log("  |-- FileSystem: %s (%p)", info.FileSystem ?: "(null)", info.FileSystem);
	qemu_log("  |-- Status: %d", info.Status);
	// qemu_log("  |-- Size: %d", info.Size);  // Most disks have capacity is greater than 4GB (32-bit space), so every disk with capacity greater than 4GB will give a bug. (We need to impelement BigInt?)
	qemu_log("  |-- Sectors: %d", info.Sectors);
	qemu_log("  |-- SectorSize: %d", info.SectorSize);
	qemu_log("  |-- AddrMode: %d", info.AddrMode);
	qemu_log("  |-- Point: %p", info.Point);
}
