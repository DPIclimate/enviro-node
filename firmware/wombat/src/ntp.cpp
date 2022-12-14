#include <Arduino.h>
#include <time.h> // Note: this is the standard c time library, not Time.h
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"

#define TAG "ntp"

// Get the time from NTP
// This code is based heavily on:
// https://docs.arduino.cc/tutorials/mkr-nb-1500/mkr-nb-library-examples#mkr-nb-gprs-udp-ntp-client
// Many thanks to: Michael Margolis, Tom Igoe, Arturo Guadalupi, et al

// NTP Server
const char* ntpServer = "au.pool.ntp.org";               // The Network Time Protocol Server
//const char* ntpServer = "africa.pool.ntp.org";        // The Network Time Protocol Server
//const char* ntpServer = "asia.pool.ntp.org";          // The Network Time Protocol Server
//const char* ntpServer = "europe.pool.ntp.org";        // The Network Time Protocol Server
//const char* ntpServer = "north-america.pool.ntp.org"; // The Network Time Protocol Server
//const char* ntpServer = "oceania.pool.ntp.org";       // The Network Time Protocol Server
//const char* ntpServer = "south-america.pool.ntp.org"; // The Network Time Protocol Server

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

bool getNTPTime(SARA_R5 &r5, uint8_t *y, uint8_t *mo, uint8_t *d, uint8_t *h, uint8_t *min, uint8_t *s) {
    int serverPort = 123; //NTP requests are to port 123

    // Set up the packetBuffer for the NTP request
    const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
    byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    //Allocate a UDP socket to talk to the NTP server

    int socketNum = r5.socketOpen(SARA_R5_UDP);
    if (socketNum == -1) {
        ESP_LOGE(TAG, "socketOpen failed");
        return false;
    }

    ESP_LOGI(TAG, "using socket %u", socketNum);

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // Send the request - NTP uses UDP

    if (r5.socketWriteUDP(socketNum, ntpServer, serverPort, (const char *)&packetBuffer, NTP_PACKET_SIZE) != SARA_R5_SUCCESS) {
        ESP_LOGE(TAG, "socketWrite failed");
        r5.socketClose(socketNum); // Be nice. Close the socket
        return false;
    }

    // Wait up to 10 seconds for the response
    unsigned long requestTime = millis();

    while (millis() < (requestTime + 10000)) {
        // We could use the Socket Read Callback to get the data, but, just for giggles,
        // and to prove it works, let's poll the arrival of the data manually...
        int avail = 0;
        if (r5.socketReadAvailableUDP(socketNum, &avail) != SARA_R5_SUCCESS) {
            ESP_LOGE(TAG, "socketReadAvailable failed");
            r5.socketClose(socketNum); // Be nice. Close the socket
            return (false);
        }

        if (avail >= NTP_PACKET_SIZE) {
            if (avail > NTP_PACKET_SIZE) {
                ESP_LOGW(TAG, "Too much data received: %d. Expected: %d", avail, NTP_PACKET_SIZE);
            }

            if (r5.socketReadUDP(socketNum, NTP_PACKET_SIZE, (char *)&packetBuffer) != SARA_R5_SUCCESS) {
                ESP_LOGE(TAG, "socketRead failed");
                r5.socketClose(socketNum); // Be nice. Close the socket
                return (false);
            }

            // Extract the time from the reply

            //the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, esxtract the two words:
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;

            ESP_LOGI(TAG, "seconds since Jan 1 1900 = %lu", secsSince1900);

            // now convert NTP time into everyday time:
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;

            // subtract seventy years:
            unsigned long epoch = secsSince1900 - seventyYears;

            ESP_LOGI(TAG, "Unix epoch = %lu", epoch);

            // Instead of calculating the year, month, day, etc. manually, let's use time_t and tm to do it for us!
            time_t dateTime = epoch;
            tm *theTime = gmtime(&dateTime);

            setenv("TZ", "UTC", 1);
            tzset();

            struct timeval tv;

            tv.tv_sec = dateTime;
            tv.tv_usec = 0;

            settimeofday(&tv, nullptr);

            // Load the time into y, mo, d, h, min, s
            *y = theTime->tm_year - 100; // tm_year is years since 1900. Convert to years since 2000.
            *mo = theTime->tm_mon + 1; //tm_mon starts at zero. Add 1 for January.
            *d = theTime->tm_mday;
            *h = theTime->tm_hour;
            *min = theTime->tm_min;
            *s = theTime->tm_sec;

            //Set the SARA's RTC. Set the time zone to zero so the clock uses UTC
            if (r5.setClock(*y, *mo, *d, *h, *min, *s, 0) != SARA_R5_SUCCESS) {
                ESP_LOGE(TAG, "SARA R5 setClock failed");
            }

            // Finish off by printing the time
            ESP_LOGI(TAG, "YY/MM/DD HH:MM:SS : %02u/%02u/%02u %02u:%02u:%02u", *y, *mo, *d, *h, *min, *s);

            r5.socketClose(socketNum); // Be nice. Close the socket
            return (true); // We are done!
        }

        delay(100); // Wait before trying again
    }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ESP_LOGW(TAG, "no NTP data received");
    r5.socketClose(socketNum); // Be nice. Close the socket
    return false;
}
