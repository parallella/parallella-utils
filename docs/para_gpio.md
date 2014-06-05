# Para_gpio

Implementation of general-purpose IO (GPIO) on the Parallella

## Implementation

* Works with the sysfs gpio implementation, rooted at /sys/class/gpio

* See the header files for the latest development and usage information.

## System requirements:

* Parallella board Gen 1.1 or later, with flash implementing CR10.  If
CR10 does not illuminate when your Parallella turns on, you need a flash
update to use that LED.  However, all other GPIOs (the ones on the PEC_FPGA)
 are available.

* Official Ubuntu environment

##  Available GPIO pins:

On the Parallella, the first user-accessible pin is GPIO7, the LED CR10.
Access to the LED requires an FSBL (flash image) created after 5/14/14,
which includes all 7010-based boards but not the kickstarter boards.
The easy way to tell if the LED is supported is to check if CR10
lights up when the board boots.  If not, the flash can be updated
through the UART but this may not be something everyone will be
comfortable doing.

Besides the FPGA-LED, the GPIO pins on the Parallella Expansion 
Connector (PEC) are accessible in the following ranges depending
on the FPGA and IO standard.  This assumes the 'standard' FPGA
configuration, different configurations may have different ranges
of pins available.

```
      FPGA   IOSTANDARD   RANGE
      7020   Single-ended 54-101
      7020   Differential 54-77
      7010   Single-ended 54-77
      7010   Differential 54-65
```

## Using the library

Download and unzip,

Add #include "para_gpio.h" to sources that require it.

Add para_gpio.c to the list of files to be compiled.

## C Functions Provided

    Unless otherwise stated, all functions return a result code, 0 always
      indicates success.  The codes are enumerated as e_para_gpiores.

    para_initgpio(para_gpio **ppGpio, int nID) - 
      initializes a single GPIO pin, returning either a pointer to a new
      gpio structure on success or NULL on failure.  The caller must 
      record this pointer for use with all other gpio functions.

    para_initgpio_ex(para_gpio **ppGpio, int nID, bool bMayExist) - 
      Same as para_initgpio but allow specifying whether the sysfs
      entries for gpio nID may already exist or not.  If the
      bMayExist argument is false and the pin has already been
      exported to the file system, the init call will fail.

    para_closegpio(para_gpio *pGpio) - Closes and de-allocates the GPIO,
      including deleting the para_gpio object.  The caller must not
      use the para_gpio pointer again after calling this function.

    para_closegpio_ex(para_gpio *pGpio, bool bForceUnexport) -
      Same as para_closegpio but allow forcibly un-exporting the
      pin even if we did not export it in the first place.

    para_setgpio(para_gpio *pGpio, int nValue) - Sets the pin level to
      nValue, looking only at the lsb.  Returns 0 on success or else
      an error code.

    para_dirgpio(para_gpio *pGpio, para_gpiodir eDir) - Sets the pin
      direction based on the enum eDir:
        para_dirin - input
        para_dirout - output
        para_dirwand - wired-and, a/k/a open-drain / open-collector
          (will either float or pull to 0)
        para_dirwor - wired-or (will either pull to 1 or float)

    para_getgpio(para_gpio *pGpio, int *pValue) - Gets the current
      pin level.  Returns 0 on success or else an error code.

    para_blinkgpio(para_gpio *pGpio, int nMSOn, nMSOff) - "Blinks"
      the gpio pin, turning it on for nMSOn milliseconds and then
      off for nMSOff before returning.

## Proposed C+ Class

Rather than keep a structure around that gets passed to every function, a
C++ class encapsulates that information and simplifies the user interface.
Here is a list of member functions for the CParaGpio class.  Except for
the constructors, all functions return 0 (success) or an error code.  The
error codes are the same as for the C functions.

    CParaGpio()  - Constructs an 'empty' GPIO object which may later have
       a pin or group of pins assigned to it.

    CParaGpio(int nStartID, int nNumIDs=1, bool bPorcOrder=false) - 
      Constructs a multi-pin GPIO object starting at pin nStartID and
      continuing for a total of nNumIDs pins.  If bPorcOrder is false,
      the pins are assigned in numerical order 0, 1, 2, 3...  If true,
      the pin assignments are made in the order of the Porcupine
      breakout board's single-ended assignments, i.e. 0 2 1 3 4 6 5 7, 
      so they come out 'nicely' on the headers.

    CParaGpio(int *pIDArray, int nNumIDs) - Constructs a multi-pin GPIO
      object using the pin numbers defined in the array pIDArray.  The
      first ID in the array corresponds to the lowest bit in any read
      or write transaction.

    AddPin(int nID) - Adds a new pin to the object, for multi-pin objects
      this will become the new most-significant bit.

    SetDirection(para_gpiodir eDir) - Sets the direction for all pins of the object
      based on the enum eDir:
        para_dirin - input
	para_dirout - output
	para_dirwand - wired-and, a/k/a open-drain / open-collector
	  (will either float or pull to 0)
        para_dirwor - wired-or (will either pull to 1 or float)

    GetDirection(para_gpiodir *pDir) - Gets the current direction setting as above.

    SetValue(int nValue) - Sets the values of all pins.  The effect of this
      function depends on the current Direction setting.

    GetValue(int *pValue) - Gets the current levels of all pins.  This
      function always reads the pin levels, it doesn't just return the values
      most recently Set, regardless of direction.

    WaitLevel(int nValue, int nTimeout) - Waits for the given value to be
      present on the input, meaning it will return immediately if the input
      is already at the requested value.  Times out after nTimeout seconds
      if request not satisfied.

    WaitEdge(int nValue, int nTimeout) - Waits for a rising (nValue = 1) or
      falling (nValue = 0) edge on the input.  Requires an edge, i.e. if
      the input is already at the requested level it must toggle before this
      function will return.  Times out after nTimeout seconds if no edge.

    Blink(int nMSOn, nMSOff) - "Blinks" the gpio pin(s) by turning them on
      for nMSOn milliseconds then then off for nMSOff before returning.

    Close() - Releases all pins from the object.  This happens automatically
      when the object is destroyed.

## Performance

As shown in the gpiotest application, the Parallella is capable of 
doing ~63k GPIO operations (reads or writes) per second.

## Notes

* The C++ class has not been released yet.

* There has been no attempt to make this thread-safe or to deal intelligently 
  with two or more objects that refer to the same pins.  

* No support yet for edge detection.  Next "to-do."

* Before things like the direction or value are set, they may be anything.  No
defaults are imposed when the gpio pins are opened.

## License

BSD 3-clause License

## Author

[Fred Huettig](mailto:Fred@Adapteva.com)

