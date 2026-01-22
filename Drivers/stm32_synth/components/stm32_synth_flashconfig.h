/*
 * stm32_synth_flashconfig.h
 *
 *  Created on: Oct 9, 2024
 *      Author: k-omura
 */

#ifndef STM32_SYNTH_COMPONENTS_STM32_SYNTH_FLASHCONFIG_H_
#define STM32_SYNTH_COMPONENTS_STM32_SYNTH_FLASHCONFIG_H_

#include "stm32_synth.h"

#ifdef SYNTH_USE_FLASH_CONFIG

// Choose one
#define SYNTH_ON_STM32F4
//#define SYNTH_ON_STM32G4

#ifdef SYNTH_ON_STM32F4
#include "stm32f4xx_hal.h"
#define SYNTH_SAVEVONFIG_FLASH_SECTOR FLASH_SECTOR_7 //STM32F411
#define SYNTH_SAVEVONFIG_FLASH_START_ADDR 0x8060000 //STM32F411
#define SYNTH_SAVEVONFIG_FLASH_SECTORSIZE 0x20000 //STM32F411
#endif

#define SYNTH_SAVEVONFIG_CONFIG_SAVESIZE 0x1000
#define SYNTH_SAVEVONFIG_CONFIGNUM_PER_BLOCK (SYNTH_SAVEVONFIG_FLASH_SECTORSIZE / SYNTH_SAVEVONFIG_CONFIG_SAVESIZE)

stm32synth_res_t stm32_synth_write_config_sector(stm32synth_config_t *_config);
stm32synth_res_t stm32_synth_read_config_sector(stm32synth_config_t *_config);
stm32synth_res_t stm32_synth_elase_config_sector();

#endif

#endif /* STM32_SYNTH_COMPONENTS_STM32_SYNTH_FLASHCONFIG_H_ */
