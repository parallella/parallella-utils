# EVolt

Reads the voltage settings from the Parallella's Power-Management IC (PMIC),
optionally setting or disabling the Epiphany core supply.

## Implementation

* Uses the device file /dev/i2c-0 to access the boards I2C bus.

## Building

System requirements:

* Parallella board Gen 1.1 or later
* FPGA image / devicetree with I2C driver enabled
* Official Ubuntu environment
* libi2c-dev package
* (optional) i2c-tools

Download and unzip,

``% cd THE_PATH``

``% make``

## Usage

Must be run as root to access the device.

``% sudo ./evolt -h``

(Will show help including all options)

## License

BSD 3-clause License

## Author

[Fred Huettig](mailto:Fred@Adapteva.com)

