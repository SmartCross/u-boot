// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <linux/delay.h>
#include <i2c.h>

void smartcross_rk808_sdmmc_reset(void)
{
	struct udevice *chip;
	int i2c_bus_no = 8;
	uint rk808_addr = 0x1b;
	uint ldo4_voltage_reg = 0x41;
	unsigned char ldo4_3v0 = 0xc, ldo4_old;
	int ret;

	ret = i2c_get_chip_for_busnum(i2c_bus_no, rk808_addr, 1, &chip);
	if (ret) {
		printf("Failed to find RK808 on i2c bus (%d)\n", ret);
		return;
	}

	ret = dm_i2c_read(chip, ldo4_voltage_reg, &ldo4_old, 1);
	if (ret) {
		printf("Failed to read RK808 LDO4 voltage (%d)\n", ret);
		return;
	}

	if (ldo4_old != ldo4_3v0) {
		printf("RK808 LDO4 reg val = %umV, setting to 3000mV\n",
		       ldo4_old * 100 + 1800);
		ret = dm_i2c_write(chip, ldo4_voltage_reg, &ldo4_3v0, 1);
		if (ret) {
			printf("Failed to set RK808 LDO4 voltage (%d)\n", ret);
			return;
		}
		mdelay(10);
		ret = dm_i2c_read(chip, ldo4_voltage_reg, &ldo4_old, 1);
		if (ret) {
			printf("Failed to read RK808 LDO4 voltage (%d)\n", ret);
			return;
		}
	}
	printf("RK808 LDO4 reg val = %umV OK\n", ldo4_old * 100 + 1800);
}
