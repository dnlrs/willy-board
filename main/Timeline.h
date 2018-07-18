/*
 	TIMELINE.h provides concurrent access to central clock for the board. central clock is keep updated by SNTP server.
HOW TO CONFIGURE SNTP SERVER ON WINDOWS 10:
- open regedit
- navigate to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\W32Time and locate "Start" registry key
- Double click on "Start" and edit the value

Change Startup type :
Automatic - 2
Manual - 3
Disabled - 4
Automatic (Delayed Start) - 2 

Note: When you change to Automatic (Delayed Start) a new key DelayedAutostart is created with value 1.
When you change to Automatic from Automatic (Delayed Start), DelayedAutostart change value to 0.

- change it to 2 (automatic)
- open cmd with privileges
- type commands:
net start w32time

reg add HKLM\system\CurrentControlSet\Services\W32Time\TimeProviders\NtpServer /v Enabled /t REG_DWORD /d 0x1 /f
w32tm /config /syncfromflags:manual /reliable:yes /update

net stop w32time

net start w32time

- now NTP server should answer requests at MACHINE_IP_ADDRESS at port 123
- in case you see through wireshark that the machine does not reply to requests then disable windows firewall and retry

*/

#pragma once

#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <iostream>
#include "lwip/err.h" 
#include "apps/sntp/sntp.h"
#include <mutex>

#define TIMESTAMP_VALIDATION 1514764801 //timestamp is valid if it's greater than 2018-01-01 

using namespace std;

class Timeline{
	clock_t start_offset; 
	//this is an offset that has to be added to the server's clock to generate a valid timestamp for the packet
	time_t central_clock;
    struct tm timeinfo;
    mutex m; //concurrent access to now variable

public:

	Timeline(){};
	
	void sync_time();
	time_t get_time(){ 
		lock_guard<mutex> lg(m);
		return central_clock + ((clock() - start_offset)/((time_t)CLOCKS_PER_SEC)); 
	}
	void initialize_sntp(char *ip_addr);
	void print_time();

};

extern Timeline *system_timeline;