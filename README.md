# CO2MiniMon

This Linux program reads temperature and relative CO2 concentration data from a
[CO2Mini][1] sensor and writes the values to temporary files. I use it to
display the data in [i3status][2].


### Setup

Put the udev rules file in `/etc/udev/rules.d/`, then reload the rules with:

```
# udevadm control --reload-rules && udevadm trigger
```

Then, when you plug in the device, `/dev/co2minix` will be created as a symlink
to the appropriate `/dev/hidrawx`.

Build and run:

```
$ ./build.sh
$ ./out/release/co2minimon
```

The 2 output files specified in the source code should be created and
continually updated with the latest values.


### Device Support

There are different versions of this device, frequently called the CO2Mini or
the RAD-0301. 

Some versions use a crappy encryption scheme as a detterent to getting the data
out. For some wonderful and informative reading on this subject, see
[Reverse-Engineering a low-cost USB CO₂ monitor][3].

For whatever reason, my device doesn't seem to engage in these encryption
shenanigans, so I can just get the data by decoding the very simple
[protocol][4].

My device has model number `ZGm053UKA`, and Linux reports the following data
for it:

```
# lsusb --verbose -d 04d9:a052

Bus 001 Device 020: ID 04d9:a052 Holtek Semiconductor, Inc. USB-zyTemp
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               1.10
  bDeviceClass            0 
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0         8
  idVendor           0x04d9 Holtek Semiconductor, Inc.
  idProduct          0xa052 USB-zyTemp
  bcdDevice            2.00
  iManufacturer           1 Holtek
  iProduct                2 USB-zyTemp
  iSerial                 3 2.00
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x0022
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              100mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           1
      bInterfaceClass         3 Human Interface Device
      bInterfaceSubClass      0 
      bInterfaceProtocol      0 
      iInterface              0 
        HID Device Descriptor:
          bLength                 9
          bDescriptorType        33
          bcdHID               1.10
          bCountryCode            0 Not supported
          bNumDescriptors         1
          bDescriptorType        34 Report
          wDescriptorLength      53
         Report Descriptors: 
           ** UNAVAILABLE **
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0008  1x 8 bytes
        bInterval              10
Device Status:     0x0000
  (Bus Powered)
```

[1]: https://www.co2meter.com/products/co2mini-co2-indoor-air-quality-monitor
[2]: https://i3wm.org/i3status/
[3]: https://hackaday.io/project/5301/logs?sort=oldest
[4]: http://co2meters.com/Documentation/Other/AN_RAD_0301_USB_Communications_Revised8.pdf
