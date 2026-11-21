#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

/* Configuration registers */
#define AS5600_REG_ZMCO		0x00
#define AS5600_REG_ZPOS		0x01
#define AS5600_REG_MPOS		0x03
#define AS5600_REG_MANG		0x05
#define AS5600_REG_CONF		0x07

/* Output registers */
#define AS5600_REG_RAW_ANGLE	0x0C
#define AS5600_REG_ANGLE	0x0E

/* Status registers */
#define AS5600_REG_STATUS	0x0B
#define AS5600_REG_AGC		0x1A
#define AS5600_REG_MAGNITUDE	0x1B

#define AS5600_STATUS_MH	BIT(3)
#define AS5600_STATUS_ML	BIT(4)
#define AS5600_STATUS_MD	BIT(5)

/* TODO implement burn command at adress 0xFF */

/* TODO figure out how to read the 12 bit registers and differenciate between the 8 bit ones */

struct as5600_data {
	struct regmap *regmap;
	struct mutex lock;
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

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		uint16_t angle;

		mutex_lock(&data->lock);
		ret = regmap_bulk_read(data->regmap, AS5600_REG_RAW_ANGLE, &angle, sizeof(angle));
		mutex_unlock(&data->lock);
		if (ret)
			return ret;

		*val = angle;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static ssize_t as5600_status_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	int ret, status;

	ret = regmap_read(data->regmap, AS5600_REG_STATUS, &status);
	if (ret)
		return ret;

	/* TODO look through kernel sources to double check this is ok */
	return sysfs_emit(buf, "%u\n", !!(status & iio_attr->address));
}

static IIO_DEVICE_ATTR(magnet_too_strong,
		       0444,
		       as5600_status_show,
		       NULL,
		       AS5600_STATUS_MH);

static IIO_DEVICE_ATTR(magnet_too_weak,
		       0444,
		       as5600_status_show,
		       NULL,
		       AS5600_STATUS_ML);

static IIO_DEVICE_ATTR(magnet_detected,
		       0444,
		       as5600_status_show,
		       NULL,
		       AS5600_STATUS_MD);

static struct attribute *as5600_attributes[] = {
	&iio_dev_attr_magnet_too_strong.dev_attr.attr,
	&iio_dev_attr_magnet_too_weak.dev_attr.attr,
	&iio_dev_attr_magnet_detected.dev_attr.attr,
	NULL,
};

static const struct attribute_group as5600_attribute_group = {
	.attrs = as5600_attributes,
};

static const struct iio_info as5600_info = {
	.attrs 			= &as5600_attribute_group,
	.read_raw		= as5600_read_raw,
	//.write_raw		=
	//.read_avail		=
	//.write_raw_get_fmt	=
};

static const struct regmap_config as5600_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xFF,
};

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
	mutex_init(&data->lock);

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
