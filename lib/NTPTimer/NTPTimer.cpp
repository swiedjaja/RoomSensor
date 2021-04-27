/*
   NTPTimer : add several function to NTPClient
   How to use:
   #include "NTPTimer.h"

   WiFiUDP udp;
   NTPTimer ntp(udp);

   void setup()
   {
      ntp.begin():
   }

   void loop()
   {
      ntp.loop();
      Serial.println(ntp.getFormattedDateTime());
      delay(1000);
   }

   or using interrupt mode, generate every 1 sec

   void OnTimer1Sec();
   void setup()
   {
      ntp.attach(OnTimer1Sec);
      ntp.begin();
   }

   void loop()
   {
      yield();
   }
*/

#include "NTPTimer.h"

void NTPTimer::begin(uint8 nMaxWaitNTP)
{
  NTPClient::begin();
  setTimeOffset(m_nTimeZone);
  for (uint8 i=0; i<nMaxWaitNTP; i++)
  {
      Serial.printf("Waiting for NTP...\n");
      if (forceUpdate())
         break;
  }
  m_Ticker1Sec.attach_ms_scheduled(1000, std::bind(&NTPTimer::OnTimer1Sec, this));
}

void NTPTimer::OnTimer1Sec()
{
   update();
   if (m_callback_function)
      m_callback_function();
}

struct tm* NTPTimer::now()
{
  time_t rawtime = getEpochTime();
  return localtime (&rawtime);
}

String NTPTimer::getFormattedTime() {
   time_t rawtime = getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return hoursStr +":"+ minuteStr +":"+ secondStr;
}

String NTPTimer::getFormattedDate() {
   time_t rawtime = getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   return yearStr + "-" + monthStr + "-" + dayStr;
}

String NTPTimer::getFormattedDateTime() {
   return getFormattedDate() + " " + getFormattedTime();
}
