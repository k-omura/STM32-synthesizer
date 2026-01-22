/*
 * stm32_synth_flashconfig.c
 *
 *  Created on: Oct 9, 2024
 *      Author: k-omura
 */

#include "components/stm32_synth_flashconfig.h"

#ifdef SYNTH_USE_FLASH_CONFIG

// Private function prototype
void readConfig(stm32synth_config_t *_config, uint8_t _recordPoint);
void writeConfig(uint8_t _recordPoint, uint8_t *_data, uint32_t _size);
uint8_t checkFreeRecordPoint();
uint32_t elaseConfigSector(uint32_t _sector);

// Public
stm32synth_res_t stm32_synth_write_config_sector(stm32synth_config_t *_config)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	uint8_t freeRecordPoint = checkFreeRecordPoint();
	if (freeRecordPoint == SYNTH_SAVEVONFIG_CONFIGNUM_PER_BLOCK)
	{
		elaseConfigSector(SYNTH_SAVEVONFIG_FLASH_SECTOR);
		freeRecordPoint = 0;
	}
	writeConfig(freeRecordPoint, (uint8_t *)(_config), sizeof(stm32synth_config_t));

	return res;
}

stm32synth_res_t stm32_synth_read_config_sector(stm32synth_config_t *_config)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	uint8_t freeRecordPoint = checkFreeRecordPoint();
	if (freeRecordPoint == 0)
	{
		return STM32SYNTH_RES_NG;
	}
	readConfig(_config, freeRecordPoint - 1);

	return res;
}

stm32synth_res_t stm32_synth_elase_config_sector()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	elaseConfigSector(SYNTH_SAVEVONFIG_FLASH_SECTOR);

	return res;
}

// Private
uint8_t checkFreeRecordPoint()
{
	uint8_t point;
	uint32_t check32;

	for (point = 0; point < SYNTH_SAVEVONFIG_CONFIGNUM_PER_BLOCK; point++)
	{
		memcpy(&check32, (uint32_t *)(SYNTH_SAVEVONFIG_FLASH_START_ADDR + (point * SYNTH_SAVEVONFIG_CONFIG_SAVESIZE)), sizeof(uint32_t)); // copy data
		if (check32 == 0xFFFFFFFF)
		{
			break;
		}
	}

	return point;
}

uint32_t elaseConfigSector(uint32_t _sector)
{
	FLASH_EraseInitTypeDef erase;
	erase.TypeErase = FLASH_TYPEERASE_SECTORS;	// select sector
	erase.Sector = _sector;						// set selector11
	erase.NbSectors = 1;						// set to erase one sector
	erase.VoltageRange = FLASH_VOLTAGE_RANGE_3; // set voltage range (2.7 to 3.6V)

	uint32_t pageError = 0;

	HAL_FLASHEx_Erase(&erase, &pageError); // erase sector

	return pageError;
}

void writeConfig(uint8_t _recordPoint, uint8_t *_data, uint32_t _size)
{
	uint32_t address = SYNTH_SAVEVONFIG_FLASH_START_ADDR + (_recordPoint * SYNTH_SAVEVONFIG_CONFIG_SAVESIZE);
	uint32_t endOfAddr = address + _size;

	HAL_FLASH_Unlock(); // unlock flash

	for (uint32_t add = address; add < endOfAddr; add++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, add, *_data); // write byte
		_data++;												// add data pointer
	}

	HAL_FLASH_Lock(); // lock flash
}

void readConfig(stm32synth_config_t *_config, uint8_t _recordPoint)
{
	memcpy(_config, (stm32synth_config_t *)(SYNTH_SAVEVONFIG_FLASH_START_ADDR + (_recordPoint * SYNTH_SAVEVONFIG_CONFIG_SAVESIZE)), sizeof(stm32synth_config_t)); // copy data
}

#endif
