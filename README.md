# ESP32 Probe Request Sniffer

Esp32 board software.

## SNTP Server Configuration on Windows

Timeline.h provides concurrent access to central clock for the board.
central clock is keep updated by SNTP server. HOW TO CONFIGURE SNTP SERVER ON
WINDOWS 10:

1. open regedit
2. navigate to `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\W32Time`
and locate "*Start*" registry key
3. Double click on "*Start*" and edit the value

    > Possible values (startup type):
    >
    > - *Automatic* - `2`
    > - *Automatic (Delayed Start)* - `2`
    > - *Manual* - `3`
    > - *Disabled* - `4`
    >
    > When changing to Automatic (Delayed Start), a new key `DelayedAutostart`
    > is created with value `1`. When you change to Automatic from Automatic
    > (Delayed Start), `DelayedAutostart` change value to `0`.

4. change it to `2` (Automatic)
5. open cmd with privileges
6. type commands:

    ```powershell
    net start w32time

    reg add HKLM\system\CurrentControlSet\Services\W32Time\TimeProviders\NtpServer /v Enabled /t REG_DWORD /d 0x1 /f w32tm /config /syncfromflags:manual /reliable:yes /update

    net stop w32time
    net start w32time
    ```

Now NTP server should answer requests at *MACHINE_IP_ADDRESS* at port `123`.  
In case you see through wireshark that the machine does not reply to requests
then disable windows firewall or add a rule allowing sntp connection on that
port and retry.

## Configuration files

### `sdconfig.defaults`

When updating `esp-idf` version, it is not uncommon to find that new Kconfig
options are introduced. In some cases, such as when `sdkconfig` file is under
revision control, the fact that `sdkconfig` file gets changed by the build
system may be inconvenient. The build system offers a way to avoid this,
in the form of `sdkconfig.defaults` file. This file is never touched by the
build system.

Project build targets will automatically create `sdkconfig` file, populated
with the settings from `sdkconfig.defaults` file, and the rest of the settings
will be set to their default values. Note that when `make defconfig` is used,
settings in `sdkconfig` will be overriden by the ones in `sdkconfig.defaults`.

### `Kconfig.projbuild`

This is an equivalent to `Makefile.projbuild` for component configuration
KConfig files. If you want to include configuration options at the top-level
of `menuconfig`, rather than inside the “Component Configuration” sub-menu,
then these can be defined in the `KConfig.projbuild` file alongside the
`component.mk` file.

> `Makefile.projbuild`  
> For components that have build requirements that must be evaluated in the
> top-level project make pass, you can create the file `Makefile.projbuild`
> in the component directory. This makefile is included when `project.mk` is
> evaluated.

Take care when adding configuration values in this file, as they will be
included across the entire project configuration. Where possible, it’s
generally better to create a KConfig file for component configuration.