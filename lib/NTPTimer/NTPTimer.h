#ifndef __NTPCLIENTEX_H__
#define __NTPCLIENTEX_H__
#include <Arduino.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>

class NTPTimer : public NTPClient {
public:
    typedef std::function<void(void)> callback_function_t;

    NTPTimer(UDP& udp, const char* poolServerName="id.pool.ntp.org", int timeOffset= 7*3600, int updateInterval=60000): 
        NTPClient(udp, poolServerName, timeOffset, updateInterval)
    {
        m_nTimeZone = timeOffset;
    }
    void begin(uint8 nMaxWaitNTP=10);
    void attach(callback_function_t callback) { m_callback_function = callback; }

    struct tm* now();
    String getFormattedDate();
    String getFormattedTime();
    String getFormattedDateTime();
protected:
    void OnTimer1Sec();
private:
    int m_nTimeZone;
    Ticker m_Ticker1Sec;
    callback_function_t m_callback_function = nullptr;
};
#endif