# Wifi Watchdog Borard

Esp32 software.

## SNTP Server Configuration on Windows

Timeline.h provides concurrent access to central clock for the board.
central clock is keep updated by SNTP server. HOW TO CONFIGURE SNTP SERVER ON
WINDOWS 10:

1. open regedit
2. navigate to `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\W32Time` and 
locate "*Start*" registry key
3. Double click on "*Start*" and edit the value

    > Possible values (startup type):
    >
    > - *Automatic* - `2`
    > - *Automatic (Delayed Start)* - `2`
    > - *Manual* - `3`
    > - *Disabled* - `4`
    >
    > When you change to Automatic (Delayed Start), a new key `DelayedAutostart` is
    > created with value `1`. When you change to Automatic from Automatic (Delayed
    > Start), `DelayedAutostart` change value to `0`.

4. change it to `2` (Automatic)
5. open cmd with privileges
6. type commands:

    ```shell
    net start w32time

    reg add HKLM\system\CurrentControlSet\Services\W32Time\TimeProviders\NtpServer /v Enabled /t REG_DWORD /d 0x1 /f w32tm /config /syncfromflags:manual /reliable:yes /update

    net stop w32time
    net start w32time
    ```

Now NTP server should answer requests at *MACHINE_IP_ADDRESS* at port `123`.  
In case you see through wireshark that the machine does not reply to requests then disable windows firewall or add a rule allowing sntp connection on that port and retry.
