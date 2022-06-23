
#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <misc.h>
#include <linux/time.h>
#include <asm/io.h>
#include <i2c.h>

struct smartcross_fpctl_regs {
	u32	id;		/* The system build id */
	u32	timestamp;	/* Timestamp */
};

struct smartcross_fpctl_plat {
	struct smartcross_fpctl_regs *regs;
};

#define ID_BYTE_LEN 16
int read_smartcross_id(u8* id)
{
	struct udevice *dev;
	int ret, i;

	/* the first misc device will be used */
	ret = uclass_get_device_by_name(UCLASS_MISC, "fpctl@22", &dev);
	if (ret)
		return ret;
	ret = misc_read(dev, 0, id, ID_BYTE_LEN);
	if (ret < 0)
		return ret;

	return 0;
}

int do_smartcross_id(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret, i;
	u8 id[ID_BYTE_LEN];
	char id_str[ID_BYTE_LEN * 2 + 1];
	char cmd_str[ID_BYTE_LEN * 2 + 30];
	ret = read_smartcross_id(&id);
	if (ret) {
		printf("Failed to read device ID\n");
	} else {
		for (i = 0; i < ID_BYTE_LEN; i++) {
			sprintf(id_str + i * 2, "%02x", id[i]);
		}
		printf("Device ID: %s\n", id_str);
		snprintf(cmd_str, sizeof(cmd_str), "systemd.machine_id=%s", id_str);
		env_set("machineid_arg", cmd_str);
	}
	return 0;
}

U_BOOT_CMD(
	smartcross_sysid,	1,	1,	do_smartcross_id,
	"display SmartCross system id and save to env",
	""
);

#define CHIP_ID_MAX_SIZE 40
static int smartcross_fpctl_read(struct udevice *dev,
			     int offset, void *buf, int size)
{
	int ret;
	if (size > CHIP_ID_MAX_SIZE) {
		return -EINVAL;
	}

	ret = dm_i2c_read(dev, 56, buf, size);
	if (ret) {
		printf("%s i2c read id failed\n", __func__);
		return ret;
	}

	return 0;
}

static int smartcross_fpctl_probe(struct udevice  *dev)
{
	u8 buf;
	int ret;

	ret = dm_i2c_read(dev, 32, &buf, 1);
	if (ret) {
		printf("%s i2c read failed\n", __func__);
		return ret;
	}

	if (buf == 0xAB) {
		printf("%s id ok\n", __func__);
	} else {
		printf("%s invalid id: %02X\n", __func__, buf);
		return -EINVAL;
	}

	return 0;
}


static const struct misc_ops smartcross_fpctl_ops = {
	.read = smartcross_fpctl_read,
};

static const struct udevice_id smartcross_fpctl_ids[] = {
	{ .compatible = "smartcross,fpctl-v1" },
	{}
};

U_BOOT_DRIVER(smartcross_fpctl) = {
	.name	= "smartcross-fpctl",
	.id	= UCLASS_MISC,
	.of_match = smartcross_fpctl_ids,
	.ops	= &smartcross_fpctl_ops,
	.probe  = &smartcross_fpctl_probe,
};
