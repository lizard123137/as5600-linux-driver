#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include <linux/iio/iio.h>

/* Configuration registers */
#define AS5600_REG_ZMCO		0x00
#define AS5600_REG_ZPOS1	0x01
#define AS5600_REG_ZPOS2	0x02
#define AS5600_REG_MPOS1	0x03
#define AS5600_REG_MPOS2	0x04
#define AS5600_REG_MANG1	0x05
#define AS5600_REG_MANG2	0x06
#define AS5600_REG_CONF1	0x07
#define AS5600_REG_CONF2	0x08

/* Output registers */
#define AS5600_REG_RAW_ANGLE1	0x0C
#define AS5600_REG_RAW_ANGLE2	0x0D
#define AS5600_REG_ANGLE1	0x0E
#define AS5600_REG_ANGLE2	0x0F

/* Status registers */
#define AS5600_REG_STATUS	0x0B
#define AS5600_REG_AGC		0x1A
#define AS5600_REG_MAGNITUDE1	0x1B
#define AS5600_REG_MAGNITUDE2	0x1C

/* TODO implement burn command at adress 0xFF */

/* TODO figure out how to read the 12 bit registers and differenciate between the 8 bit ones */

struct as5600_data {
	struct regmap *regmap;
};

static const struct iio_chan_spec as5600_channels[] = {
	{
		.type = IIO_ANGL, /* TODO check if this type is correct */
		.output = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
};

static int as5600_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val,
			   int *val2,
			   long mask)
{
	struct as5600_data *data = iio_priv(indio_dev);
	int ret;
	u8 buf[2];

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = regmap_bulk_read(data->regmap, AS5600_REG_RAW_ANGLE1, buf, sizeof(buf));
		if (ret < 0)
			return ret;

		*val = buf[0] << 8 | buf[1];
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

/* TODO use iio for interacting with userspace */
static const struct iio_info as5600_info = {
	.read_raw		= as5600_read_raw,
	//.write_raw		=
	//.read_avail		=
	//.write_raw_get_fmt	=
};

static const struct regmap_config as5600_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	/* TODO */
};

/* Use regmap_bulk_read for reading the 12 bit registers */

static int as5600_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct as5600_data *data;
	struct regmap *map;
	struct iio_dev *iio;
	int ret;

	iio = devm_iio_device_alloc(dev, sizeof(*data));
	if (!iio)
		return -ENOMEM;

	map = devm_regmap_init_i2c(client, &as5600_regmap_config);
	if (IS_ERR(map))
		return dev_err_probe(dev, PTR_ERR(map),
				     "Failed to initialize regmap\n");

	data = iio_priv(iio);
	data->regmap = map;

	iio->name = "as5600";
	iio->info = &as5600_info;
	iio->channels = as5600_channels;
	iio->num_channels = ARRAY_SIZE(as5600_channels);
	iio->modes = INDIO_DIRECT_MODE;
	
	/* TODO add power down action */

	return devm_iio_device_register(dev, iio);
}

static const struct of_device_id as5600_match[] = {
	{ .compatible = "infineon,as5600" },
	{ }
};
MODULE_DEVICE_TABLE(of, as5600_match);

static struct i2c_driver as5600_driver = {
	.driver = {
		.name = "as5600",
		.of_match_table = as5600_match,
	},
	.probe = as5600_probe,
};
module_i2c_driver(as5600_driver);

MODULE_DESCRIPTION("AS5600 Contactless Potentiometer Driver");
MODULE_AUTHOR("Michal Gagos <michal.gagos.dev@gmail.com>");
MODULE_LICENSE("GPL");
