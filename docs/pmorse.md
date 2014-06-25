# PMorse

Demonstration of the para_gpio and para_morse libraries.  Transmits arbitrary 
strings from the Parallella's LED using morse code.

## Implementation

* Works with the sysfs gpio implementation, rooted at /sys/class/gpio

* See the header files for the latest development and usage information.

## Building

System requirements:

* Parallella board Gen 1.1 or later, with flash implementing CR10.  If
CR10 does not illuminate when your Parallella turns on, you need a flash
update to use that LED.  However, all other GPIOs (the ones ont he PEC_FPGA)
 are available.

* Official Ubuntu environment

Download and unzip,

``% cd THE_PATH``

``% make pmorse``

## Usage

``% sudo ./pmorse [-w W] [-g G] [-r R]  "Hello World"``

Where:

* W is the code rate in Words Per Minute (default 5)
* G is the gpio ID to use (7 for LED CR10, others start at 54)
* R # of times to repeat the message

## License

BSD 3-clause License

## Author

[Fred Huettig](mailto:Fred@Adapteva.com)

