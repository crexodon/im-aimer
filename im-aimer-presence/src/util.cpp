#include "util.h"

#include <Arduino.h>
#include <time.h>
#include <chrono>


const char* ntpServer = "de.pool.ntp.org";
const char* TZ_INFO =   "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Berlin, for others, see: https://remotemonitoringsystems.ca/time-zone-abbreviations.php
tm timeinfo;
time_t now;

bool getNTPtime(int sec) {
    {
        uint32_t start = millis();
        do {
            time(&now);
            localtime_r(&now, &timeinfo);
            delay(10);
        } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
        if (timeinfo.tm_year <= (2016 - 1900))
            return false; // the NTP call was not successful
    }
    return true;
}

char curISOStr[32] = {0};
const char *getISOTimeStr() {
    getNTPtime(0);
    memset(curISOStr, 0, sizeof(curISOStr));
    strftime(curISOStr, sizeof(curISOStr), "%FT%TZ", localtime(&now));  // Formats: http://www.cplusplus.com/reference/ctime/strftime/
    return curISOStr;
}

// unix timestamp in ms
uint64_t getTimestamp() {
    getNTPtime(0);
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void initNTP() {
    configTime(0, 0, ntpServer);
    setenv("TZ", TZ_INFO, 1);
    if (getNTPtime(10)) {
    } // wait up to 10sec to sync
    else {
        // DEBUG_println("Time not set");
    }
}