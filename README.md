S130_LBS_client_server_central_peripheral
======================
This example shows how a S130 softdevice can be set up with the same structure as the SDK examples. The main goal of thisi project is to demonstrate that S130 softdevice for both central and peripheral configuration.

------------
* nRF51 series SDK 8.1.1
* S130 1.0.0
* nRF51-DK * 2
  
The project may need modifications to work with other versions or other boards. 
 
To compile it, clone the repository in the following folder: ..\nRF51_SDK_8.1.0_b6ed55f\examples\ble_central_and_peripheral

NOTE:
There was a bug in nrf_drv_gpiote drv at the time of writing this.
in app_button.c->app_button_init : you need to change GPIOTE_CONFIG_IN_SENSE_TOGGLE(false) -> GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
If you do not enable hi_accuracy, then enabling and disabling buttons does not work.

About this project
------------------
This application is one of several applications that has been built by the support team at Nordic Semiconductor, as a demo of some particular feature or use case. It has not necessarily been thoroughly tested, so there might be unknown issues. It is hence provided as-is, without any warranty.
  
However, in the hope that it still may be useful also for others than the ones we initially wrote it for, we've chosen to distribute it here on GitHub. 
  
The application is built to be used with the official nRF51 SDK, that can be downloaded from https://www.nordicsemi.no, provided you have a product key for one of our kits.
 
Please post any questions about this project on https://devzone.nordicsemi.com.

How does it work
-------------------
The application can be either configured as peripheral (setting the PERIPHERAL flag) or the central (setting the CENTRAL flag). both peripheral and central are not supported yet, even though it is
not very difficult to achieve it taking this project as starting point.

This example is made to work with PCA100028 and if you want to test it on other boards, then configurations for LEDS and buttons needs to be changed and include the correct header files.
Below is how it looks on board PCA10028 and how they are assigned in the application
LEDS:  

scanning/advertising  -->  LED1  
Connected             -->  LED2  
notification_from_peer-->  LED3  
write from peer       -->  LED4  

Buttons:  
Wakeup from IDLE/Sleep --> Button1  
notification from peer --> Button2  
Write to peer          --> Button3  
not used               --> Button4  

 
The buttons and leds operations are completely symetric on both central and peripheral except for advertising/scanning LED.
Central looks for an advertiser with the device name "LedButtonDemo" and connects to it. As soon as this is done, both peripheral and central discovers service on the other and enables all the services.
Two specific service UUID has been used for both central and peripheral. You can find those in the device_config.h file. There are two characteristic in this service, one for writing to LED and the other for button press notification.
System will go to sleep if no connection is established before timeout. Pressing Button1 after sleep will wake the system up and it will again continue to advertise/scan

Pressing Button2 will write the button value to button charatecteris which has notification properties to it. Hence will send an HVX event to peer. Peer when gets this event writes to LED3. This is to demonstrate how notifications can be used
from one device to another. The device which sends HVX notification is hence a server (central and peripheral both could be servers)

Prssing Button3 will send a write command to peer LED characteristic (without response) and the characteristic on the peer will be updated with the requested value. The value is also written to LED4.
Here the device that sends the write request is a client for the server which changes the LED value. So both central and peripheral here can act as clients.

Effort has been taken to keep the code common to both central and peripheral. The differences should standout evidently.
Projects
----------
Adding projects for both Keil and GCC.
Tested on Keil 5.15 and GNUTools\4.9 2015q1.

Bugs
------
06.07.2015 . Sometimes the device hangs after sometime. we will fix them as time permits. Main intention was to demonstrate how things are done.
