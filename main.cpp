#include <Arduino.h>
#include <GNSS.h>     //GNSS library
#include <RTC.h>      //RTC library
#include <LowPower.h> //LowPower library
#include <SDHCI.h>    //SD card library

/*Parameters*/
#define DEFAULT_INTERVAL_SEC 1
#define STRING_BUFFER_SIZE 128

SpGnss Gnss; // GNSS object
SDClass SD;  // SD object
File myFile; // File object

/*DEBUG*/
#define DEBUG_MODE true        /**< Debug mode */
#define SERIAL_BAUDRATE 115200 /**< Serial baud rate */

static void save_position_data(SpNavData *NavData)
{
  /*データの保存*/

  char StringBuffer[STRING_BUFFER_SIZE];

  /* print time */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%04d/%02d/%02d ", pNavData->time.year, pNavData->time.month, pNavData->time.day);
  Serial.print(StringBuffer);
  myFile.print(StringBuffer);

  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%02d:%02d:%02d.%06ld, ", pNavData->time.hour, pNavData->time.minute, pNavData->time.sec, pNavData->time.usec);
  Serial.print(StringBuffer);
  myFile.print(StringBuffer);

  /* print satellites count */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "numSat:%2d, ", pNavData->numSatellites);
  Serial.print(StringBuffer);
  myFile.print(StringBuffer);

  /* print position data */
  if (pNavData->posFixMode == FixInvalid)
  {
    Serial.print("No-Fix, ");
    // myFile.print("No-Fix, ");
  }
  else
  {
    Serial.print("Fix, ");
    // myFile.print("Fix, ");
  }
  if (pNavData->posDataExist == 0)
  {
    Serial.print("No Position");
    // myFile.print("No Position");
  }
  else
  {
    Serial.print("Lat=");
    myFile.print(", ");

    Serial.print(pNavData->latitude, 6);
    myFile.print(pNavData->latitude, 6);

    Serial.print(", Lon=");
    myFile.print(", ");

    Serial.print(pNavData->longitude, 6);
    myFile.print(pNavData->longitude, 6);
  }

  Serial.println("");
  myFile.println("");
}

void setup()
{
  int error_flag = 0;

  /*DEBUG メッセージのシリアル出力 */
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("Serial begin");

  /*LED の初期化*/
  ledOn(PIN_LED0);
  ledOn(PIN_LED1);
  ledOn(PIN_LED2);
  ledOn(PIN_LED3);
  delay(1000);
  ledOff(PIN_LED0);
  ledOff(PIN_LED1);
  ledOff(PIN_LED2);
  ledOff(PIN_LED3);

  /*ストレージの初期化*/
  SD.begin();
  Serial.println("SD begin");
  SD.mkdir("dir/");
  //*作業ファイルの作成*/
  myFile = SD.open("dir/test_data.txt", FILE_WRITE);

  /*GNSS の初期化*/
  sleep(3);
  Gnss.begin();
  Serial.println("GNSS begin");
  Gnss.useGps(true);
  Gnss.useQzss(true);
  Gnss.useGlonass(true);

  Gnss.setInterval(DEFAULT_INTERVAL_SEC);
  Gnss.start();

  /*低電力モードの有効化*/
}

void loop()
{

  /*DEFAULT_INTERVAL_SEC秒ごとに動作*/
  if (Gnss.waitUpdate(-1))
  {
    Serial.println("loop");

    /*GNSSのデータ取得*/
    Serial.println("Get NaviData");
    SpNavData NavData;
    Gnss.getNavData(&NavData);

    /*データの保存*/
    save_position_data(&NavData);

    /*    Serial.print(NavData.latitude);
    ledOn(PIN_LED2);
    Serial.print(NavData.longitude);
    ledOn(PIN_LED3);
    */
  }
}
