/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RK3399_SMARTCROSS_H
#define __RK3399_SMARTCROSS_H

#define ROCKCHIP_DEVICE_SETTINGS   \
	"stdin=serial\0"               \
	"stdout=serial\0"              \
	"stderr=serial\0"              \
    "upgrade_available=0\0"        \
    "active_slot=a\0"              \
    "extra_bootargs=btusb.enable_autosuspend=n rootwait\0"                                                               \
    "bsp_bootcmd="                                                                                              \
    "setenv mmc_num 1;\n"                                                                                       \
    "setenv part_name rootfs-${active_slot};\n"                                                                 \
    "part number mmc ${mmc_num} ${part_name} uboot_part;\n"                                                     \
    "echo \"Booting from slot ${active_slot} (part ${uboot_part}) attempt ${bootcount}/${bootlimit}...\";\n"    \
    "load mmc ${mmc_num}:${uboot_part} ${kernel_addr_r} /boot/Image.gz;\n"                                      \
    "load mmc ${mmc_num}:${uboot_part} ${fdt_addr_r} /boot/rk3399-smartcross.dtb;\n"                            \
    "smartcross_sysid;\n"                                                                                       \
    "setenv bootargs \"${machineid_arg} root=PARTLABEL=${part_name} ${extra_bootargs}\";\n"                     \
    "saveenv;\n"                                                                                                \
    "booti ${kernel_addr_r} - ${fdt_addr_r};\n"                                                                 \
    "if test ${upgrade_available} = 1; then reset; fi;\0"                                                       \
    "altbootcmd="                                                                                               \
    "echo \"Boot from slot ${active_slot} failed, reverting...\";\n"  \
    "if test \"x${active_slot}\" = \"xa\"; then\n"                    \
    "  setenv active_slot b;\n"                                       \
    "else\n"                                                          \
    "  setenv active_slot a;\n"                                       \
    "fi;\n"                                                           \
    "echo \"Reverted to slot ${active_slot}\";\n"                     \
    "setenv upgrade_available 0;\n"                                   \
    "saveenv;\n"                                                      \
    "reset;\0"     

#include <configs/rk3399_common.h>

#endif
