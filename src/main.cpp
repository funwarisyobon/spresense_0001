

#include <Arduino.h>
#include <GNSS.h>     //GNSS library
#include <LowPower.h> //LowPower library

#include <SDHCI.h> //SD card library
#include <File.h>  //File library

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
char headerString[120] = "time,latitude,longitude,altitude,velocity,direction,positionDOP,HorizontalDOP,VerticalDOP,TimeDOP,satellites";

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
    if (!SD.begin())
    {
        Serial.println("SD begin error");
        error_flag = 1;
        while (1)
        {
            ;
        }
    }
    Serial.println("SD begin");
    SD.mkdir("dir/");

    /*作業ファイルの作成*/
    bool sw_continueWriting = true;
    File dir = SD.open("dir/");
    int filenum = 0;
    char filename[18];

    snprintf(filename, sizeof(filename), "dir/%04d.csv", filenum);
    // 0000.csvが存在しないとき
    if (!SD.exists(filename))
    {
        // 0000.csvを作成し、へッダーを書き込む
        Serial.print("Create ");
        Serial.println(filename);
        myFile = SD.open(filename, FILE_WRITE);
        myFile.println(headerString);
        Serial.println(headerString);
    }
    else
    {

        while (1)
        {
            filenum++;
            snprintf(filename, sizeof(filename), "dir/%04d.csv", filenum);

            // sw_continueWritingがtrueのとき、前のファイルに続けて書き込む
            if (sw_continueWriting)
            {

                if (!SD.exists(filename))
                {

                    snprintf(filename, sizeof(filename), "dir/%04d.csv", filenum - 1);
                    myFile = SD.open(filename, FILE_WRITE);

                    Serial.println("sw_continueWriting is on. ");
                    Serial.print("Write to ");
                    Serial.println(filename);

                    break;
                }
            }
            else
            // sw_continueWritingがfalseのとき、新しいファイルを作成する
            {
                if (!SD.exists(filename))
                {
                    myFile = SD.open(filename, FILE_WRITE);
                    myFile.println(headerString);

                    Serial.print("sw_continueWriting is off. ");
                    Serial.print("Create ");
                    Serial.println(filename);
                    break;
                }
            }
        }
    }

    if (myFile)
    {
        Serial.print(filename);
        Serial.println(" is open");
        myFile.println("test");
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
    Serial.println("GNSS start");

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
        if (NavData.posFixMode == FixInvalid || NavData.posDataExist == 0)
        {
            return;
        }

        /* データの保存 */

        if (myFile)
        {
            char writeBuffer[STRING_BUFFER_SIZE];
            snprintf(writeBuffer, sizeof(writeBuffer), "%04d/%02d/%02dT%02d:%02d:%02dZ, %.10f, %.10f, %.10f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %2d",
                     NavData.time.year, NavData.time.month, NavData.time.day,
                     NavData.time.hour, NavData.time.minute, NavData.time.sec,
                     NavData.latitude, NavData.longitude, NavData.altitude,
                     NavData.velocity, NavData.direction,
                     NavData.pdop, NavData.hdop, NavData.vdop, NavData.tdop,
                     NavData.numSatellites);
            myFile.println(writeBuffer);
            Serial.println(writeBuffer);
        }
    }
}