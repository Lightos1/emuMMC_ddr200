/*
 * Copyright (c) 2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "nx_sd.h"
#include "sdmmc.h"
#include "sdmmc_driver.h"
#include "../soc/gpio.h"
#include "../libs/fatfs/ff.h"
#include "../utils/log.h"

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;

#ifndef EMUMMC_SDMMC_UHS_DDR200_SUPPORT
#define SD_DEFAULT_SPEED SD_UHS_SDR104
#else
#define SD_DEFAULT_SPEED SD_UHS_DDR208
#endif

static u32  sd_mode = SD_DEFAULT_SPEED;

u32 nx_sd_mode_get()
{
	return sd_mode;
}

bool nx_sd_is_ddr200(void)
{
	return sd_storage.csd.busspeed == 200;
}

int nx_sd_init_retry(bool power_cycle)
{
	u32 bus_width = SDMMC_BUS_WIDTH_4;
#ifndef EMUMMC_SDMMC_UHS_DDR200_SUPPORT
	u32 type = SDHCI_TIMING_UHS_SDR104;
#else
	u32 type = SDHCI_TIMING_UHS_DDR200;
#endif

	// Power cycle SD card.
	if (power_cycle)
	{
		sd_mode--;
		sdmmc_storage_end(&sd_storage);
	}

	// Get init parameters.
	switch (sd_mode)
	{
	case SD_INIT_FAIL: // Reset to max.
		return 0;
	case SD_1BIT_HS25:
		bus_width = SDMMC_BUS_WIDTH_1;
		type = SDHCI_TIMING_SD_HS25;
		break;
	case SD_4BIT_HS25:
		type = SDHCI_TIMING_SD_HS25;
		break;
	case SD_UHS_SDR82:
		type = SDHCI_TIMING_UHS_SDR82;
		break;
	case SD_UHS_SDR104:
		type = SDHCI_TIMING_UHS_SDR104;
		break;
#ifdef EMUMMC_SDMMC_UHS_DDR200_SUPPORT
	case SD_UHS_DDR208:
		type = SDHCI_TIMING_UHS_DDR200;
		break;
#endif
	default:
		sd_mode = SD_DEFAULT_SPEED;
	}

	return sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, bus_width, type);
}

bool nx_sd_initialize(bool power_cycle)
{
	if (power_cycle)
		sdmmc_storage_end(&sd_storage);

	int res = !nx_sd_init_retry(false);

	while (true)
	{
		if (!res)
			return true;
		else
		{
			if (sd_mode == SD_INIT_FAIL)
				break;

			res = !nx_sd_init_retry(true);
		}
	}

	return false;
}