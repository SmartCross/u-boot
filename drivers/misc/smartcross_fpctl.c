
#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <misc.h>
#include <linux/time.h>
#include <asm/io.h>
#include <i2c.h>
#include <inttypes.h>
#include <crypto/x509_parser.h>
#include <linux/err.h>

struct smartcross_fpctl_regs {
	u32	id;		/* The system build id */
	u32	timestamp;	/* Timestamp */
};

struct smartcross_fpctl_plat {
	struct smartcross_fpctl_regs *regs;
};


#define FPCTL_CRYPTO_INDEX 56
#define FPCTL_CRYPTO_DATA_BUFFER 57
#define CRYTPO_BUFFER_CERTIFICATE 1
#define CERT_MAX_LEN 0x400

int do_smartcross_id(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *dev;
	int ret, i;
	u8 buf[CERT_MAX_LEN];
	ret = uclass_get_device_by_name(UCLASS_MISC, "fpctl@22", &dev);
	if (ret) {
		printf("Failed to obtain smartcross device: %d\n", ret);
		return ret;
	}
	ret = misc_read(dev, 0, buf, CERT_MAX_LEN);
	if (ret < 0) {
		printf("Failed to read device certificate: %d\n", ret);
		return ret;
	}
	int len = buf[2] * 256 + buf[3] + 4;
	if (len > CERT_MAX_LEN) {
		len = CERT_MAX_LEN;
	}
	char serial_buf[33];
	char machine_arg_buffer[110];
	memset(serial_buf, 0, sizeof(serial_buf));
	memset(machine_arg_buffer, 0, sizeof(machine_arg_buffer));
	struct x509_certificate *x509_cert = x509_cert_parse(buf, len);
	if (!IS_ERR(x509_cert)) {
		for (int i = 0; i < x509_cert->raw_serial_size - 1; i++) {
			if (i < sizeof(serial_buf) / 2) {
				sprintf(serial_buf + i * 2, "%02x", ((uint8_t*)x509_cert->raw_serial)[i + 1]);
			}
		}
		printf("Certificate serial number: %s\n", serial_buf);
		printf("Certificate subject: %s\n", x509_cert->subject);
		snprintf(machine_arg_buffer, sizeof(machine_arg_buffer), "systemd.hostname=%s systemd.machine_id=%s", x509_cert->subject, serial_buf);
		x509_free_certificate(x509_cert);
	} else {
		printf("Device certificate parse err = %d\n", PTR_ERR(x509_cert));
		snprintf(machine_arg_buffer, sizeof(machine_arg_buffer), "systemd.hostname=unknown.smartcross.net systemd.machine_id=123456789987654321");
	}
	env_set("machineid_arg", machine_arg_buffer);

	return 0;
}

U_BOOT_CMD(
	smartcross_sysid,	1,	1,	do_smartcross_id,
	"display SmartCross system id and save to env",
	""
);

int do_smartcross_led(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *dev;
	int ret;
	u8 rgb[3] = {0};
	
	ret = uclass_get_device_by_name(UCLASS_MISC, "fpctl@22", &dev);
	if (ret)
		return ret;
	if (argc == 1) {
		ret = dm_i2c_read(dev, 23, rgb, sizeof(rgb));
		if (ret)
			return ret;
	} else if (argc == 4) {
		rgb[0] = simple_strtoul(argv[1], NULL, 10);
		rgb[1] = simple_strtoul(argv[2], NULL, 10);
		rgb[2] = simple_strtoul(argv[3], NULL, 10);
	} else {
		return -EINVAL;
	}
	ret = dm_i2c_write(dev, 3, rgb, sizeof(rgb));
	if (ret)
		return ret;	

	return 0;
}

U_BOOT_CMD(
	smartcross_led,	4,	1,	do_smartcross_led,
	"Initialize Front Panel LED",
	"smartcross_led [r g b]"
);

static int smartcross_fpctl_read(struct udevice *dev,
			     int offset, void *buf, int size)
{
	int ret;
	ret = dm_i2c_reg_write(dev, FPCTL_CRYPTO_INDEX, CRYTPO_BUFFER_CERTIFICATE);
	if (ret) {
		printf("%s i2c write crypto index failed\n", __func__);
		return ret;
	}

	ret = dm_i2c_read(dev, FPCTL_CRYPTO_DATA_BUFFER, buf, size);
	if (ret) {
		printf("%s i2c read data buffer failed\n", __func__);
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
