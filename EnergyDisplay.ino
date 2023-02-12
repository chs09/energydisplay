#include <WiFi.h>
#include <SPI.h>

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <esp_system.h>

#include "config.h"
#include "bgimage.h"
#include "arrowimage.h"
#include "arrowrightimage.h"

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define EPD_BUSY 3  // BUSY
#define EPD_CS 5    // CS
#define EPD_RST 16  // RST
#define EPD_DC 17   // DC
#define EPD_SCK 18  // CLK
#define EPD_MISO 19 // Master-In Slave-Out not used, as no data from display
#define EPD_MOSI 23 // DIN

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RST);
GxEPD_Class display(io, EPD_RST, EPD_BUSY);

const bool test = DATA_TEST;

hw_timer_t *timer = NULL;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const char *host = DATA_HOST;
const int port = DATA_PORT;

const int MAX_PV_POWER = INSTALLED_WATT_PEAK;

typedef struct
{
    String battery;
    String pv;
    String use;
    String grid;
    String battuse;
    String curtime;
    String curdate;
} Status;

Status state = {
    "0", "0", "0", "0", "0", "11:23", "01.01.2000"};

const int HISTOGRAM_WIDTH = 130;
const int HISTOGRAM_HEIGHT = 60;

typedef struct
{
    uint16_t data[HISTOGRAM_WIDTH];
} Histogram;

Histogram lastUse;
Histogram lastPv;

void IRAM_ATTR resetModule()
{
    ets_printf("reboot\n");
    esp_restart();
}

void setup()
{
    Serial.begin(115200);
    delay(10);

    SPI.end();
    SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);

    btStop();

    display.init(115200);
    Serial.println("display initialized");

    display.fillScreen(GxEPD_WHITE);
    bigText(90, 105, "Solar Monitor");
    display.update();

    sleep(2);

    // Watchdog timer
    timer = timerBegin(0, 80, true);                 // timer 0, div 80
    timerAttachInterrupt(timer, &resetModule, true); // reset ESP if timer runs out
    timerAlarmWrite(timer, 60000000 * 10, false);    // timeout after 10 minutes
    timerAlarmEnable(timer);                         // enable interrupt

    display.fillScreen(GxEPD_WHITE);
    display.update();

    sleep(3);
}

void wifiReconnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        // We start by connecting to a WiFi network
        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);
        int i = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("try to connect WIFI");
            i = i + 1;
            if (i > 20)
            {
                esp_restart();
            }
        }
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

int gridArrows(float fgrid)
{
    fgrid = (fgrid > 0) ? fgrid : -fgrid;
    if (fgrid < 0.1)
    {
        return 0;
    }

    if (fgrid <= 0.7)
    {
        return 1;
    }

    if (fgrid <= 2.0)
    {
        return 2;
    }

    if (fgrid <= 6.0)
    {
        return 3;
    }

    return 4;
}

int battArrows(float fbattuse)
{
    fbattuse = (fbattuse > 0) ? fbattuse : -fbattuse;
    if (fbattuse < 0.1)
    {
        return 0;
    }

    if (fbattuse <= 0.7)
    {
        return 1;
    }

    if (fbattuse <= 1.4)
    {
        return 2;
    }
    return 3;
}

void drawFrame(int x, int y, int width, int height)
{
    for (int w = 0; w <= width; w += 2)
    {
        display.drawPixel(x + w, y, GxEPD_BLACK);
        display.drawPixel(x + w, y + height, GxEPD_BLACK);
    }
    for (int h = 0; h <= height; h += 2)
    {
        display.drawPixel(x, y + h, GxEPD_BLACK);
        display.drawPixel(x + width, y + h, GxEPD_BLACK);
    }
}

void loop()
{
    Serial.println("loop");

    timerWrite(timer, 0); // reset watchdog timer

    display.fillScreen(GxEPD_WHITE);
    // display.drawBitmap(0,0, gImage_IMG_0001, 400, 300, GxEPD_WHITE);
    display.drawExampleBitmap(gImage_IMG_0001, 0, 0, 400, 300, GxEPD_WHITE);
    display.update();

    Serial.println("update done ");

    wifiReconnect();

    Serial.print("connecting to ");
    Serial.println(host);

    readState();

    // Read all the lines of the reply from server and print them to Serial
    smallTextBlack(90, 115, state.battuse + " kW");
    smallTextBlack(245, 115, state.grid + " kW");
    bigText(245, 25, state.pv + " kW");
    bigText(245, 260, state.use + " kW");
    tinyText(350, 8, state.curtime);

    int battgraphmax = state.battery.toInt(); // Just so happens to be 100 px high, so batt/100*100
    for (int x = 16; x < 69; x++)
    {
        for (int y = 0; y < battgraphmax; y++)
        {
            display.drawPixel(x, 204 - y, GxEPD_BLACK);
        }
    }

    if (battgraphmax == 100)
    {
        smallTextWhite(20, 140, "99%");
    }
    else if (battgraphmax > 70)
    {
        smallTextWhite(20, 140, state.battery + "%");
    }
    else if (battgraphmax > 30)
    {
        smallTextWhite(20, 175, state.battery + "%");
    }
    else
    {
        smallTextBlack(20, 140, state.battery + "%");
    }

    float fbattuse = state.battuse.toFloat();
    int numBattArrows = battArrows(fbattuse);
    if (fbattuse < -0.1)
    {
        drawArrows(gImage_arrowright, numBattArrows, 100, 160);
    }
    else if (fbattuse > 0.1)
    {
        drawArrows(gImage_arrowleft, numBattArrows, 100, 160);
    }

    float fgrid = state.grid.toFloat();
    int numGridArrows = gridArrows(fgrid);
    if (fgrid < -0.1)
    {
        drawArrows(gImage_arrowright, numGridArrows, 248, 160);
    }
    else if (fgrid > 0.1)
    {
        drawArrows(gImage_arrowleft, numGridArrows, 248, 160);
    }

    Serial.println("Updating PV Histogram");
    readLastPv();

    // draw dotted outline
    drawFrame(10, 10, HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);
    for (int x = 0; x < HISTOGRAM_WIDTH; x++)
    {
        float curpvf = (lastPv.data[x] > MAX_PV_POWER) ? MAX_PV_POWER : lastPv.data[x];
        int maxy = (int)((curpvf / MAX_PV_POWER) * 60);
        for (int y = 0; y <= maxy; y++)
        {
            display.drawPixel(10 + x, 70 - y, GxEPD_BLACK);
        }
    }
    tinyText(HISTOGRAM_WIDTH + 16, 8, String((float)MAX_PV_POWER / 1000, 1));
    tinyText(HISTOGRAM_WIDTH + 16, 64, "0.0");
    Serial.println("PV Histogram completed!");

    Serial.println("Getting Usage Histogram");
    readLastUse();

    drawFrame(10, 231, HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);

    int useGraphX1 = 10;
    int useGraphY1 = 231;
    int useGraphX2 = useGraphX1 + 130;
    int useGraphY2 = useGraphY1 + 60;
    int maxUse = 0;

    for (int x = 0; x < HISTOGRAM_WIDTH; x++)
    {
        if (maxUse < lastUse.data[x])
        {
            maxUse = lastUse.data[x];
        }
    }

    if (maxUse < 2000)
    {
        // scale, but not less than 2kW
        maxUse = 2000;
    }
    else
    {
        // scale in 0.5kW steps
        maxUse = ((maxUse + 499) / 500) * 500;
    }

    for (int x = 0; x < HISTOGRAM_WIDTH; x++)
    {
        float curpvf = (float) lastUse.data[x];
        float maxy = (int) (curpvf / maxUse * HISTOGRAM_HEIGHT);
        for (int y = 0; y <= maxy; y++)
        {
            display.drawPixel(useGraphX1 + x, useGraphY2 - y, GxEPD_BLACK);
        }
    }

    tinyText(useGraphX2 + 6, useGraphY1 - 2, String((float)maxUse / 1000, 1));
    tinyText(useGraphX2 + 6, useGraphY2 - 6, "0.0");
    Serial.println("Usage Histogram completed!");

    display.update();
    Serial.println("done, sleeping 30 seconds");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(30000);
    // delay(1000*60*5);
}

void readLastPv()
{
    Serial.println("Getting PV Histogram");
    if (test)
    {
        for (int i = 0; i < HISTOGRAM_WIDTH; i++)
        {
            lastPv.data[i] = (esp_random() % 9900);
        }
        return;
    }

    WiFiClient client;
    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return;
    }

    doGetRequest(&client, "/energydisplay/lastpv");

    int x = 0;
    while (x < HISTOGRAM_WIDTH)
    {
        if (client.available())
        {
            int u = client.readStringUntil('\n').toInt();
            lastPv.data[x] = (uint16_t)u;
        }
        else
        {
            lastPv.data[x] = 0;
        }
    }
    client.stop();
}

void readLastUse()
{
    Serial.println("Getting Usage Histogram");
    if (test)
    {
        for (int i = 0; i < HISTOGRAM_WIDTH; i++)
        {
            lastUse.data[i] = (esp_random() % 9900);
        }
        return;
    }

    WiFiClient client;
    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return;
    }

    doGetRequest(&client, "/energydisplay/lastuse");

    int x = 0;
    while (x < HISTOGRAM_WIDTH)
    {
        if (client.available())
        {
            int u = client.readStringUntil('\n').toInt();
            lastUse.data[x] = (uint16_t)u;
        }
        else
        {
            lastUse.data[x] = 0;
        }
    }
    client.stop();
}

void readState()
{
    if (test)
    {
        state.pv = "8.4";
        state.use = "7.2";
        state.grid = "1.2";
        state.battery = "90";
        state.battuse = "-1.2";
        state.curtime = "11:23";
        state.curdate = "01.01.2000";
        return;
    }

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return;
    }

    doGetRequest(&client, "/energydisplay/state");

    // Read all the lines of the reply from server and print them to Serial
    if (client.available())
    {
        Serial.println("Data received!");
        readPastHeader(&client);
        state.battery = client.readStringUntil('\n');
        state.pv = client.readStringUntil('\n');
        state.use = client.readStringUntil('\n');
        state.grid = client.readStringUntil('\n');
        state.battuse = client.readStringUntil('\n');
        state.curtime = client.readStringUntil('\n');
        state.curdate = client.readStringUntil('\n');
    }
    client.stop();
}

bool doGetRequest(WiFiClient *client, String path)
{
    Serial.println("Getting Data " + path);

    client->print(String("GET ") + path + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Connection: Close\r\n\r\n");

    unsigned long timeout = millis();
    while (client->available() == 0)
    {
        if (millis() - timeout > 5000)
        {
            Serial.println(">>> Client Timeout !");
            client->stop();
            return false;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    if (client->available())
    {
        Serial.println("Data received!");
        readPastHeader(client);
    }
    return true;
}

void readPastHeader(WiFiClient *pClient)
{
    bool isBlank = true;
    while (true)
    {
        if (pClient->available())
        {
            char c = pClient->read();
            if (c == '\r' && isBlank)
            {
                // throw away the /n
                c = pClient->read();
                return;
            }
            if (c == '\n')
                isBlank = true;
            else if (c != '\r')
                isBlank = false;
        }
    }
}

void drawArrows(const uint8_t *image, int num, int x, int y)
{
    for (int i = 0; i < num; i++)
    {
        display.drawBitmap(image, x + (i * 18), y, 18, 24, GxEPD_WHITE);
    }
}

void tinyText(uint16_t x, uint16_t y, String text)
{
    const GFXfont *f = &FreeSans9pt7b;

    display.setRotation(0);
    display.setFont(f);
    display.setTextColor(GxEPD_BLACK);

    display.setCursor(x, y + 9);
    display.print(text);
}

void smallText(uint16_t x, uint16_t y, String text, uint16_t color)
{
    const GFXfont *f = &FreeSans12pt7b;

    display.setRotation(0);
    display.setFont(f);
    display.setTextColor(color);

    display.setCursor(x, y + 20);
    display.print(text);
}

void smallTextBlack(uint16_t x, uint16_t y, String text)
{
    smallText(x, y, text, GxEPD_BLACK);
}

void smallTextWhite(uint16_t x, uint16_t y, String text)
{
    smallText(x, y, text, GxEPD_WHITE);
}

void bigText(uint16_t x, uint16_t y, String text)
{
    const GFXfont *f = &FreeSans18pt7b;

    display.setRotation(0);
    display.setFont(f);
    display.setTextColor(GxEPD_BLACK);

    display.setCursor(x, y + 25);
    display.print(text);
}
