
#include <Arduino.h>
#include <GNSS.h>     //GNSS library
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

/*Header Strings*/
String headerString = "gpstime,latitude,longitude,altitude,velocity,direction,positionDOP,HorizontalDOP,VerticalDOP,TimeDOP,satellites";

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

    /*作業ファイルの作成*/
    int maxNumber = -1;
    File dir = SD.open("dir/");
    File entry;
    while ((entry = dir.openNextFile()))
    {
        if (!entry.isDirectory())
        {
            String fileName = entry.name();
            if (fileName.endsWith(".csv"))
            {
                int fileNumber = fileName.substring(0, 4).toInt();
                if (fileNumber > maxNumber)
                {
                    maxNumber = fileNumber;
                }
            }
        }
        entry.close();
    }
    dir.close();

    if (maxNumber == -1)
    {
        myFile = SD.open("dir/0000.csv", FILE_WRITE);
        if (myFile)
        {
            myFile.println(headerString);
        }
    }
    else
    {
        if (false)
        { // スイッチがOFFの時
            // digitalRead(PIN_SW) == LOW
            myFile = SD.open("dir/" + String(maxNumber) + ".csv", FILE_WRITE);
        }
        else
        { // スイッチがONの時
            char fileName[16];
            snprintf(fileName, sizeof(fileName), "dir/%04d.csv", maxNumber + 1);
            myFile = SD.open(fileName, FILE_WRITE);
            if (myFile)
            {
                myFile.println(headerString);
            }
        }
    }

    /*GNSS の初期化*/
    sleep(3);
    Gnss.begin();
    Serial.println("GNSS begin");
    Gnss.select(GPS);
    Gnss.select(QZ_L1CA);
    Gnss.select(QZ_L1S);

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
        ledOff(PIN_LED0);
        ledOff(PIN_LED1);
        ledOff(PIN_LED2);
        ledOff(PIN_LED3);

        /*GNSSのデータ取得*/
        Serial.println("Get NaviData");
        SpNavData NavData;
        Gnss.getNavData(&NavData);
        ledOn(PIN_LED0);
        if (NavData.posFixMode == FixInvalid || NavData.posDataExist == 0)
        {
            ledOn(PIN_LED3);
            return;
        }

        /* データの保存 */

        if (myFile)
        {
            char writeBuffer[STRING_BUFFER_SIZE];
            snprintf(writeBuffer, sizeof(writeBuffer), "%04d/%02d/%02dT%02d:%02d:%02d,%.10f,%.10f,%.10f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%2d\n",
                     NavData.time.year, NavData.time.month, NavData.time.day,
                     NavData.time.hour, NavData.time.minute, NavData.time.sec,
                     NavData.latitude, NavData.longitude, NavData.altitude,
                     NavData.velocity, NavData.direction,
                     NavData.pdop, NavData.hdop, NavData.vdop, NavData.tdop,
                     NavData.numSatellites);
            ledOn(PIN_LED1);
            myFile.print(writeBuffer);
            ledOn(PIN_LED2);
            Serial.print(writeBuffer);
        }
    }
}