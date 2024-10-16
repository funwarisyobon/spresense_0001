

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
String headerString = "time,latitude,longitude,altitude,velocity,direction,positionDOP,HorizontalDOP,VerticalDOP,TimeDOP,satellites";

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
    int maxNumber = -1;
    File dir = SD.open("dir/");
    File entry;
    // ここがバグっている。ファイルの数をカウントできていない
    while ((entry = dir.openNextFile()))
    {
        Serial.print("checked File: ");
        Serial.println(entry.name());
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

    // 下はmaxnumberの数でまとめられる。
    bool sw_continueWriting = false;
    // ファイルがないとき
    int fileNumber = 0;
    if (maxNumber == -1)
    {
        fileNumber = 0;
        Serial.println("No file found. Make new file.");
    }
    else
    // ファイルがあるとき
    {
        // スイッチがONの時、maxNumberのファイルを開いて続きに書き込む。

        if (sw_continueWriting)
        {
            fileNumber = maxNumber;
            Serial.println("File found. Continue writing.");
        }
        else
        {
            fileNumber = maxNumber + 1;
            Serial.print("maxNumber: ");
            Serial.println(maxNumber);
            Serial.println("File found. Make new file.");
        }
    }
    // ファイル名を指定
    char filePath[16];
    snprintf(filePath, sizeof(filePath), "dir/%04d.csv", fileNumber);
    Serial.println(filePath);
    myFile = SD.open(filePath, FILE_WRITE);
    if (maxNumber == -1 || !sw_continueWriting)
    {
        myFile.println(headerString);
        Serial.println(headerString);
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