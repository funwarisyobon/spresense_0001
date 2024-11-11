#include <Arduino.h>

//GNSS
#include <GNSS.h>     //GNSS library
SpGnss Gnss; // GNSS object

#include <LowPower.h> //LowPower library

//SD
#include <SDHCI.h> //SD card library
SDClass SD; //SD object
#include <File.h>  //File library
File myFile; //File object

//RTC
#include <RTC.h>   //RTC library

//Parameters
#define DEFAULT_INTERVAL_SEC 1
#define STRING_BUFFER_SIZE 128



//DEBUG
#define DEBUG_MODE true        /**< Debug mode */
#define SERIAL_BAUDRATE 115200 /**< Serial baud rate */

/*Header Strings*/
char headerString[120] = "time,latitude,longitude,altitude,velocity,direction,positionDOP,HorizontalDOP,VerticalDOP,TimeDOP,satellites";

/*filename*/
char filename[18];

// sleep
void sleep()
{

    // GPSのデータを保存して終了
    Gnss.saveEphemeris();
    Gnss.stop();
    Gnss.end();
    // 電源オフ
    LowPower.coldSleep();
}

void setup()
{
    // int error_flag = 0;

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

    /*RTC*/
    RTC.begin();

    /*ストレージの初期化*/
    SD.begin();
    if (!SD.begin())
    {
        Serial.println("SD begin error");
        // error_flag = 1;
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

    snprintf(filename, sizeof(filename), "dir/%04d.csv", filenum);
    // 0000.csvが存在しないとき
    if (!SD.exists(filename))
    {
        // 0000.csvを作成し、へッダーを書き込む
        Serial.print("Create ");
        Serial.println(filename);
        myFile = SD.open(filename, FILE_WRITE);
        myFile.println(headerString);
        myFile.flush();
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
    int count_receive_signal = 0;

    /*DEFAULT_INTERVAL_SEC秒ごとに動作*/
    if (!Gnss.waitUpdate(-1))
        return;

    Serial.println("loop");

    /*GNSSのデータ取得*/
    Serial.println("Get NaviData");
    SpNavData NavData;
    Gnss.getNavData(&NavData);

    /*もし、受信できていたら、ファイルに保存*/
    if (NavData.posFixMode == FixInvalid || NavData.posDataExist == 0)
    {
        return;
    }
    else
    {
        char writeBuffer[STRING_BUFFER_SIZE];
        snprintf(writeBuffer, sizeof(writeBuffer), "%04d/%02d/%02dT%02d:%02d:%02dZ, %.10f, %.10f, %.10f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %2d",
                 NavData.time.year, NavData.time.month, NavData.time.day,
                 NavData.time.hour, NavData.time.minute, NavData.time.sec,
                 NavData.latitude, NavData.longitude, NavData.altitude,
                 NavData.velocity, NavData.direction,
                 NavData.pdop, NavData.hdop, NavData.vdop, NavData.tdop,
                 NavData.numSatellites);
        myFile = SD.open(filename, FILE_WRITE);
        /*ファイル書き込み部分。書き込み中はLED0を点灯*/
        if (myFile)
        {
            ledOn(PIN_LED0);
            myFile.println(writeBuffer);
            myFile.close();
            ledOff(PIN_LED0);
        }
        Serial.println(writeBuffer);
        count_receive_signal++;
    }
    // 100回に一回の受診ごとに、RTCの時刻をGPSの時刻に合わせる。
    if (count_receive_signal % 100 == 0)
    {

        RtcTime gps_time(NavData.time.year, NavData.time.month, NavData.time.day, NavData.time.hour, NavData.time.minute, NavData.time.sec, NavData.time.usec * 1000 /* RtcTime requires nsec */);

        if (abs(RTC.getTime() - (double)gps_time) > 10)
        {
            RTC.setTime(gps_time);
            Serial.println("RTC time is updated");
        }
    }

    // sleepに入る。なにかのスイッチを想定

    // if (false)
    // {
    //     attachInterrupt(digitalPinToInterrupt(PIN_SW), sleep, FALLING);
    // }
}
