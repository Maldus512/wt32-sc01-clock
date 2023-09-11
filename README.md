# Desktop Clock for WT32SC01

Simple ESP32 application that displays the time and allows to set simple alarms.
It requires an internet connection to sync the current time.

Made for [WT32-SC01](https://www.amazon.it/AMIUHOUN-WT32-SC01-Multi-Touch-Capacitivo-Integrato/dp/B0BS3NZFC3).

## Project Structure

The project is organized in the following folders

 - `assets`: static assets (e.g. images).
 - `components`: software dependencies.
 - `docs`: technical documentation.
 - `main`: application firmware.
 - `simulator`: software related to host simulation of the application.
 - `tools`: miscellaneous tooling: custom build tools and 3d model generator.

## Application Structure

The application follows a flavor of the MVC design pattern, thus it is mostly split into three components:

 - `main/model`: the model contains the business logic (data logic under `model` and application logic under `updater`) of the application. It has no meaningful dependencies.
 - `main/view`: the view handles the UI logic: visualization of the state and reception of user input (via touch screen). It depends on the model for reading the state and sending transforming signals.
 - `main/controller`: the controller includes everything else; it is the glue that holds logic and hardware together. It depends on every other component. 

Other notable modules should be considered part of the controller, but are coherent enought to deserve their independence:
 
 - `main/config`: configuration headers.
 - `main/peripherals`: hardware-related functions.
 - `main/services`: non-dependencies.

### Components

 - `components/c-page-manager/`: a page stack for the UI.
 - `components/c-watcher`: data structure for observer patterns.
 - `lvgl`: an excellent low level virtual graphic libarry.
 - `lvgl_esp32_drivers`: repository for the touch screen driver.

## Building from Source

The project is based on ESP-IDF. Instructions on how to install the development environment can be found [here](https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/get-started/index.html#get-started-get-prerequisites).

The version required is ESP-IDF v4.4.0 due to constraints on the display driver component.

## Simulator

The application can be simulated and run under a Linux environment instead of the actual target.
This is achieved by plugging in stubs for every hardware-related function (found under `main/peripherals` and `main/services`) and compiling the purely logical components of the application together with a Linux-based FreeRTOS port.

The compilation for the simulated application is handled by [SCons](https://scons.org/); it requires the tool itself, a C compiler and the SDL2 library.

It can be run with the command `scons run`.

## Updating the Device

Starting from version 0.1.1 the local webserver exposes a simple webpage that includes an updating interface.
For previous versions the device can be still updated via an HTTP API. 

Assuming that the binary `wt32sc01-clock.bin` has been downloaded on the host machine, it is sufficient to run the following command on Linux:

```
curl -X PUT <ip>/firmware_update --data-binary @./wt32sc01-clock.bin
```

Where `<ip>` is the ip address of the device.
