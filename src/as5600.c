#include <linux/bitfield.h>
#include <linux/math.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

/* Configuration registers */
#define AS5600_REG_ZMCO 0x00
#define AS5600_REG_ZPOS 0x01
#define AS5600_REG_MPOS 0x03
#define AS5600_REG_MANG 0x05
#define AS5600_REG_CONF 0x07

/* Output registers */
#define AS5600_REG_RAW_ANGLE 0x0C
#define AS5600_REG_ANGLE 0x0E

/* Status registers */
#define AS5600_REG_STATUS 0x0B
#define AS5600_REG_AGC 0x1A
#define AS5600_REG_MAGNITUDE 0x1B

enum as5600_reg_fields {
	AS5600_FIELD_CONF_PM,
	AS5600_FIELD_CONF_HYST,
	AS5600_FIELD_CONF_OUTS,
	AS5600_FIELD_CONF_PWMF,
	AS5600_FIELD_CONF_SF,
	AS5600_FIELD_CONF_FTH,
	AS5600_FIELD_CONF_WD,

	AS5600_FIELD_STAT_MH,
	AS5600_FIELD_STAT_ML,
	AS5600_FIELD_STAT_MD,

	AS5600_FIELD_MAX,
};

#define AS5600_STATUS_MH BIT(3)
#define AS5600_STATUS_ML BIT(4)
#define AS5600_STATUS_MD BIT(5)

static const struct reg_field as5600_reg_fields[] = {
	[AS5600_FIELD_CONF_PM]          = REG_FIELD(AS5600_REG_CONF, 0, 1),
	[AS5600_FIELD_CONF_HYST]        = REG_FIELD(AS5600_REG_CONF, 2, 3),
	[AS5600_FIELD_CONF_OUTS]        = REG_FIELD(AS5600_REG_CONF, 4, 5),
	[AS5600_FIELD_CONF_PWMF]        = REG_FIELD(AS5600_REG_CONF, 6, 7),
	[AS5600_FIELD_CONF_SF]          = REG_FIELD(AS5600_REG_CONF, 8, 9),
	[AS5600_FIELD_CONF_FTH]         = REG_FIELD(AS5600_REG_CONF, 10, 12),
	[AS5600_FIELD_CONF_WD]          = REG_FIELD(AS5600_REG_CONF, 13, 13),

	[AS5600_FIELD_STAT_MH]          = REG_FIELD(AS5600_REG_STATUS, 3, 3),
	[AS5600_FIELD_STAT_ML]          = REG_FIELD(AS5600_REG_STATUS, 4, 4),
	[AS5600_FIELD_STAT_MD]          = REG_FIELD(AS5600_REG_STATUS, 5, 5),
};

/**
 * TODO
 *
 * implement burn command at address 0xFF
 * somehow secure the burn command from being issued by accident
 *
 * Maybe use a write only iio event attribute with super user permissions only (0200).
 */

struct as5600_data {
	struct regmap *regmap;
	struct regmap_field *regmap_fields[AS5600_FIELD_MAX];
};

static const struct iio_chan_spec as5600_channels[] = {
	{
		.type = IIO_ANGL,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_PROCESSED),
	},
};

static int as5600_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan, int *val,
			   int *val2, long mask)
{
	struct as5600_data *data = iio_priv(indio_dev);
	u8 buf[2];
	u32 angle_urad;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = regmap_bulk_read(data->regmap, AS5600_REG_RAW_ANGLE, buf,
				       sizeof(buf));
		if (ret)
			return ret;

		*val = ((buf[0] & 0x0F) << 8) | buf[1];
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_PROCESSED:
		ret = regmap_bulk_read(data->regmap, AS5600_REG_ANGLE, buf,
				       sizeof(buf));
		if (ret)
			return ret;

		angle_urad = DIV_ROUND_CLOSEST(
			(u64)(((buf[0] & 0x0F) << 8) | buf[1]) * 6283185, 4096);

		*val = angle_urad / 1000000;
		*val2 = angle_urad % 1000000;
		return IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}
}

static ssize_t as5600_reg8_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	unsigned int value;
	int ret;

	ret = regmap_read(data->regmap, iio_attr->address, &value);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%u\n", value);
}

static ssize_t as5600_reg8_write(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	u8 value;
	int ret;

	ret = kstrtou8(buf, 0, &value);
	if (ret)
		return ret;

	ret = regmap_write(data->regmap, iio_attr->address, value);
	return ret ? ret : len;
}

static ssize_t as5600_reg16_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	u8 temp[2];
	int ret;

	ret = regmap_bulk_read(data->regmap, iio_attr->address, temp,
			       sizeof(temp));
	if (ret)
		return ret;

	return sysfs_emit(buf, "%u\n", ((temp[0] & 0x0F) << 8) | temp[1]);
}

static ssize_t as5600_reg16_write(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	u16 value;
	u8 temp[2];
	int ret;

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	if (value > GENMASK(11, 0))
		return -EINVAL;

	temp[0] = (value >> 8) & 0x0F;
	temp[1] = value & 0xFF;

	ret = regmap_bulk_write(data->regmap, iio_attr->address, temp,
				sizeof(temp));

	return ret ? ret : len;
}

static ssize_t as5600_regmap_field_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	int ret, value;

	ret = regmap_field_read(data->regmap_fields[iio_attr->address], &value);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%u\n", value);
}

static ssize_t as5600_regmap_field_write(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct as5600_data *data = iio_priv(indio_dev);
	struct iio_dev_attr *iio_attr = to_iio_dev_attr(attr);
	uint8_t value;
	int ret;

	ret = kstrtou8(buf, 0, &value);
	if (ret)
		return ret;

	ret = regmap_field_write(data->regmap_fields[iio_attr->address], value);
	return ret ? ret : len;
}

/* Configuration attributes */
static IIO_DEVICE_ATTR(start_position, 0644, as5600_reg16_show,
		       as5600_reg16_write, AS5600_REG_ZPOS);
static IIO_DEVICE_ATTR(stop_position, 0644, as5600_reg16_show,
		       as5600_reg16_write, AS5600_REG_MPOS);
static IIO_DEVICE_ATTR(max_angle, 0644, as5600_reg16_show,
		       as5600_reg16_write, AS5600_REG_MANG);

static IIO_DEVICE_ATTR(conf_power_mode, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_PM);
static IIO_DEVICE_ATTR(conf_hysteresis, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_HYST);
static IIO_DEVICE_ATTR(conf_output_stage, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_OUTS);
static IIO_DEVICE_ATTR(conf_pwm_freq, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_PWMF);
static IIO_DEVICE_ATTR(conf_slow_filter, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_SF);
static IIO_DEVICE_ATTR(conf_fast_filter, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_FTH);
static IIO_DEVICE_ATTR(conf_watchdog, 0644, as5600_regmap_field_show,
		       as5600_regmap_field_write, AS5600_FIELD_CONF_WD);

/* Status attributes */
static IIO_DEVICE_ATTR(magnet_too_strong, 0444, as5600_regmap_field_show, NULL,
		       AS5600_FIELD_STAT_MH);
static IIO_DEVICE_ATTR(magnet_too_weak, 0444, as5600_regmap_field_show, NULL,
		       AS5600_FIELD_STAT_ML);
static IIO_DEVICE_ATTR(magnet_detected, 0444, as5600_regmap_field_show, NULL,
		       AS5600_FIELD_STAT_MD);

static IIO_DEVICE_ATTR(automatic_gain_control, 0444, as5600_reg8_show, NULL,
		       AS5600_REG_AGC);
static IIO_DEVICE_ATTR(magnitude, 0444, as5600_reg16_show, NULL,
		       AS5600_REG_MAGNITUDE);
static IIO_DEVICE_ATTR(burn_counter, 0444, as5600_reg8_show, NULL,
		       AS5600_REG_ZMCO);

/* TODO add support for DIR gpio */
static struct attribute *as5600_attributes[] = {
	&iio_dev_attr_start_position.dev_attr.attr,
	&iio_dev_attr_stop_position.dev_attr.attr,
	&iio_dev_attr_max_angle.dev_attr.attr,

	&iio_dev_attr_conf_power_mode.dev_attr.attr,
	&iio_dev_attr_conf_hysteresis.dev_attr.attr,
	&iio_dev_attr_conf_output_stage.dev_attr.attr,
	&iio_dev_attr_conf_pwm_freq.dev_attr.attr,
	&iio_dev_attr_conf_slow_filter.dev_attr.attr,
	&iio_dev_attr_conf_fast_filter.dev_attr.attr,
	&iio_dev_attr_conf_watchdog.dev_attr.attr,

	&iio_dev_attr_magnet_too_strong.dev_attr.attr,
	&iio_dev_attr_magnet_too_weak.dev_attr.attr,
	&iio_dev_attr_magnet_detected.dev_attr.attr,

	&iio_dev_attr_automatic_gain_control.dev_attr.attr,
	&iio_dev_attr_magnitude.dev_attr.attr,
	&iio_dev_attr_burn_counter.dev_attr.attr,
	NULL,
};

static const struct attribute_group as5600_attribute_group = {
	.attrs = as5600_attributes,
};

static const struct iio_info as5600_info = {
	.attrs		= &as5600_attribute_group,
	.read_raw	= as5600_read_raw,
};

static const struct regmap_config as5600_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= 0xFF,
};

static int as5600_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct as5600_data *data;
	struct regmap *map;
	struct regmap_field *field;
	struct iio_dev *iio;
	int i;

	iio = devm_iio_device_alloc(dev, sizeof(*data));
	if (!iio)
		return -ENOMEM;

	map = devm_regmap_init_i2c(client, &as5600_regmap_config);
	if (IS_ERR(map))
		return dev_err_probe(dev, PTR_ERR(map),
				     "Failed to initialize regmap\n");

	data = iio_priv(iio);
	data->regmap = map;

	for (i = 0; i < AS5600_FIELD_MAX; i++) {
		field = devm_regmap_field_alloc(dev, data->regmap,
						as5600_reg_fields[i]);
		if (IS_ERR(field))
			return dev_err_probe(
				dev, PTR_ERR(field),
				"Failed to initialize regmap_field\n");
		data->regmap_fields[i] = field;
	}

	iio->name = "as5600";
	iio->info = &as5600_info;
	iio->channels = as5600_channels;
	iio->num_channels = ARRAY_SIZE(as5600_channels);
	iio->modes = INDIO_DIRECT_MODE;

	/* TODO add power down action */
	/* TODO maybe implement PM power saving */

	return devm_iio_device_register(dev, iio);
}

static const struct of_device_id as5600_match[] = {
	{ .compatible = "infineon,as5600" },
	{}
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
