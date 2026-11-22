# as5600-linux-driver

## Work In Progress
The implementation of this driver is not yet finished. Currently it only supports reading the raw value.

The driver uses the `IIO` subsystem for its userspace interaction.

The device userspace interface is available under:
```
/sys/bus/iio/<device_name>/
```

The available `IIO` attributes are:
- `magnet_too_strong`
- `magnet_too_weak`
- `magnet_detected`

## Build
This section describes the process of building and inserting the driver and its devicetree overlay.

**This driver was tested on an a `Raspberry Pi5`, so your devicetree overlay might need to be different.**

### Dependencies
Before compiling the driver we need to install its dependencies:
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
sudo insmod obj/as5600.ko
sudo dtoverlay as5600.dtbo
```

### Removal
Removing the driver and its overlay:
``` shell
sudo rmmod as5600
sudo dtoverlay -r as5600
```