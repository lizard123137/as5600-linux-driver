# as5600-linux-driver

**This driver is not finished! It implements some functionality, but it is still a work in progress.** 

This project is a **Linux Driver** for a **Magnetic Contactless Potentiometer** called the **AS5600**.

## Userspace Interface

The driver uses the `IIO` subsystem for its userspace interaction.

The device userspace interface is available under:
```
/sys/bus/iio/<device_name>/
```

Its properties can be viewed using the provided `test.py` script.
The source code of that script also serves as a good example of how to read the device data.

## Build
This section describes the process of building and inserting the driver and its devicetree overlay.

**This driver was tested on a `Raspberry Pi5`, so your devicetree overlay might need to be different.**

### Dependencies
Before compiling the driver, we need to install its dependencies:
``` shell
sudo apt install linux-headers-$(uname -r)
```

### Compilation
Compiling the driver and overlay:
``` shell
make
dtc -I dts -o as5600.dtbo as5600-overlay.dts
```

### Insertion
Inserting the driver and its overlay:
``` shell
sudo modprobe regmap-i2c
sudo modprobe iio_hwmon

sudo insmod obj/as5600.ko
sudo dtoverlay as5600.dtbo
```

### Removal
Removing the driver and its overlay:
``` shell
sudo rmmod as5600
sudo dtoverlay -r as5600
```
