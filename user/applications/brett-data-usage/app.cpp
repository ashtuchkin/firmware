// from: https://gist.github.com/technobly/e3fa70d9f7ee0b42d165
// and: https://s.slack.com/archives/electron/p1451936316002298

// COMPILE from firmware/modules $
// make clean all PLATFORM_ID=10 APPDIR=~/code/fw-apps/data-usage COMPILE_LTO=n DEBUG_BUILD=y -s program-dfu
//
// NOTE: APPDIR=~/code/fw-apps/data-usage can be APP=data-usage if that's easier

/* Includes ------------------------------------------------------------------*/
#include "application.h"

typedef struct {
    int cid = 0;
    int tx_sess_bytes = 0;
    int rx_sess_bytes = 0;
    int tx_total_bytes = 0;
    int rx_total_bytes = 0;
} MDM_DATA_USAGE;

MDM_DATA_USAGE _data_usage;

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, INFO_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
SerialDebugOutput debugOutput(9600, DEBUG_LEVEL);

SYSTEM_MODE(AUTOMATIC);

static inline int _cbCCID(int type, const char* buf, int len, char* ccid)
{
    if ((type == TYPE_PLUS) && ccid) {
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", ccid) == 1)
            /*nothing*/;
    }
    return WAIT;
}

static inline int _cbUGCNTRD(int type, const char* buf, int len, MDM_DATA_USAGE* data)
{
    if ((type == TYPE_PLUS) && data) {
        int a,b,c,d,e;
        // +UGCNTRD: 31,2704,1819,2724,1839\r\n
        // +UGCNTRD: <cid>,<tx_sess_bytes>,<rx_sess_bytes>,<tx_total_bytes>,<rx_total_bytes>
        if (sscanf(buf, "\r\n+UGCNTRD: %d,%d,%d,%d,%d\r\n", &a,&b,&c,&d,&e) == 5) {
            data->cid = a;
            data->tx_sess_bytes = b;
            data->rx_sess_bytes = c;
            data->tx_total_bytes = d;
            data->rx_total_bytes = e;
        }
    }
    return WAIT;
}

/* This function is called once at start up ----------------------------------*/
void setup()
{

}

/* This function loops forever --------------------------------------------*/
void loop()
{
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        Serial.printf("Hey, you said \'%c\', so I'm gunna: ", c);
        delay(50);
        if (c == 'r') {
            Serial.println("Read counters of sent or received PSD data!");
            if (RESP_OK == Cellular.command(_cbUGCNTRD, &_data_usage, 10000, "AT+UGCNTRD\r\n")) {
                Serial.printlnf("CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d\r\n",
                    _data_usage.cid,
                    _data_usage.tx_sess_bytes, _data_usage.rx_sess_bytes,
                    _data_usage.tx_total_bytes, _data_usage.rx_total_bytes);
            }
        }
        else if (c == 'c') {
            Serial.println("Set/reset counter of sent or received PSD data!");
            Cellular.command("AT+UGCNTSET=?\r\n"); // test available limits, see log output
            Cellular.command("AT+UGCNTSET=%d,0,0\r\n", _data_usage.cid);
        }
        else if (c == 'p') {
            Serial.println("Publish some data!");
            Particle.publish("1234567890","abcdefghijklmnopqrstuvwxyz");
        }
        else if (c == 'i') {
            Serial.println("snack up the SIM card ID (CCID)...");
            char ccid[32] = "";
            if ((RESP_OK == Cellular.command(_cbCCID, ccid, 10000, "AT+CCID\r\n"))
                && (strcmp(ccid,"") != 0))
            {
                Serial.printlnf("Yum! -> %s\r\n", ccid);
            }
        }
        else {
            DEBUG("ignore you because you're not speaking my language!");
        }
        delay(50);
        while (Serial.available()) Serial.read(); // Flush the input buffer
    }

}
