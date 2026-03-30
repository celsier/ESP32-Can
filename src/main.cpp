#include <M5Unified.h>
#include "driver/twai.h"
#include <esp_now.h>
#include <WiFi.h>

/* 
 * DJI RS4 Universal Controller v8.0 (Session-Locked)
 * - Sequence numbers used only for salting (no comparison).
 * - Master accepts all commands matching the current Session ID.
 * - PSK XOR ensures over-the-air privacy.
 */

const uint32_t PSK = 0x5A2B3C4D; 
uint16_t global_can_seq = 0xCCCC;
uint32_t last_can_rx_time = 0;           
uint32_t tx_rolling_counter = 100;
uint32_t current_session_id = 0;
uint32_t target_session_id = 0;
uint32_t last_beacon_sent = 0;
bool is_moving = false;
int can_tx = -1, can_rx = -1;
const uint16_t DJI_TX_ID = 0x223;

typedef enum { PKT_BEACON = 0x01, PKT_COMMAND = 0x02 } PacketType;
typedef struct { uint8_t type; uint32_t sessionID; } BeaconPacket;
typedef struct { uint8_t type; uint8_t cmdSet; uint8_t cmdID; uint8_t data[8]; uint8_t len; uint32_t seq; uint32_t sessionID; } CommandPacket;

struct Rect { int x, y, w, h; };
Rect btnUp, btnDown, btnLeft, btnRight;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Forward Declarations
void setupUI();
void crypt(uint8_t* data, size_t len);
void sendCANPacket(uint8_t cmdSet, uint8_t cmdID, const uint8_t* data, size_t len);

uint16_t dji_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0x3AA3; 
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

uint32_t dji_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0x00003AA3; 
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return crc;
}

void sendCANPacket(uint8_t cmdSet, uint8_t cmdID, const uint8_t* data, size_t len) {
    size_t totalLen = 10 + 2 + 2 + len + 4;
    uint8_t p[totalLen];
    p[0] = 0xAA; p[1] = totalLen & 0xFF; p[2] = (totalLen >> 8) & 0x03; 
    p[3] = 0x03; p[4] = 0x00; p[5] = 0x00; p[6] = 0x00; p[7] = 0x00; 
    p[8] = (uint8_t)((global_can_seq >> 8) & 0xFF); p[9] = (uint8_t)(global_can_seq & 0xFF);
    global_can_seq++;
    uint16_t c16 = dji_crc16(p, 10);
    p[10] = c16 & 0xFF; p[11] = (c16 >> 8) & 0xFF;
    p[12] = cmdSet; p[13] = cmdID;
    if(len > 0) memcpy(&p[14], data, len);
    uint32_t c32 = dji_crc32(p, totalLen - 4);
    p[totalLen-4] = c32 & 0xFF; p[totalLen-3] = (c32 >> 8) & 0xFF;
    p[totalLen-2] = (c32 >> 16) & 0xFF; p[totalLen-1] = (c32 >> 24) & 0xFF;
    for (size_t i = 0; i < totalLen; i += 8) {
        uint8_t cl = (totalLen - i > 8) ? 8 : (totalLen - i);
        twai_message_t msg = {.identifier = DJI_TX_ID, .data_length_code = cl};
        memcpy(msg.data, &p[i], cl);
        twai_transmit(&msg, pdMS_TO_TICKS(5));
    }
}

void crypt(uint8_t* data, size_t len) {
    uint8_t* key = (uint8_t*)&PSK;
    for(size_t i=0; i<len; i++) data[i] ^= key[i % 4];
}

void dispatchWireless(uint8_t cmdSet, uint8_t cmdID, const uint8_t* data, size_t len) {
    if (target_session_id == 0) return;
    CommandPacket cp = {
        .type = PKT_COMMAND, .cmdSet = cmdSet, .cmdID = cmdID, .len = (uint8_t)len, 
        .seq = tx_rolling_counter++, .sessionID = target_session_id
    };
    memcpy(cp.data, data, len);
    crypt(((uint8_t*)&cp) + 1, sizeof(CommandPacket) - 1);
    esp_now_send(broadcastAddress, (uint8_t*)&cp, sizeof(cp));
}

void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t *incomingData, int len) {
    uint8_t type = incomingData[0];
    bool is_m = (millis() - last_can_rx_time < 1000);

    if (type == PKT_BEACON && !is_m) {
        target_session_id = ((BeaconPacket*)incomingData)->sessionID;
    } else if (type == PKT_COMMAND && is_m) {
        CommandPacket cp; memcpy(&cp, incomingData, sizeof(CommandPacket));
        crypt(((uint8_t*)&cp) + 1, sizeof(CommandPacket) - 1);
        
        // v8.0 Logic: Execute if Session ID matches. Ignore seq order.
        if (cp.sessionID == current_session_id) {
            sendCANPacket(cp.cmdSet, cp.cmdID, cp.data, cp.len);
            M5.Display.fillRect(0, 20, M5.Display.width(), 20, ORANGE);
            M5.Display.setTextColor(BLACK);
            M5.Display.setTextDatum(top_left);
            M5.Display.drawString("SEC CMD OK", 5, 24);
        }
    }
}

void setupUI() {
    int w = M5.Display.width(), h = M5.Display.height();
    if (w == 0) return;
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    if (w < 200) {
        M5.Display.setTextDatum(middle_center);
        M5.Display.fillCircle(w/2, h/2, w/3, RED);
        M5.Display.drawString("HOME", w/2, h/2);
    } else {
        int pad = 20, bw = w/3-pad, bh = h/4;
        btnUp = {w/2-bw/2, pad+15, bw, bh}; btnDown = {w/2-bw/2, h-bh-pad, bw, bh};
        btnLeft = {pad, h/2-bh/2, bw, bh}; btnRight = {w-bw-pad, h/2-bh/2, bw, bh};
        M5.Display.fillRect(btnUp.x, btnUp.y, btnUp.w, btnUp.h, BLUE);
        M5.Display.fillRect(btnDown.x, btnDown.y, btnDown.w, btnDown.h, BLUE);
        M5.Display.fillRect(btnLeft.x, btnLeft.y, btnLeft.w, btnLeft.h, BLUE);
        M5.Display.fillRect(btnRight.x, btnRight.y, btnRight.w, btnRight.h, BLUE);
        M5.Display.fillCircle(w/2, h/2, bw/3, RED);
    }
}

void setup() {
    auto cfg = M5.config(); M5.begin(cfg);
    Serial.begin(115200);
    current_session_id = esp_random();
    Serial.println("GIMBAL_CTRL_V8_2_READY");
    
    WiFi.mode(WIFI_STA);
    if (esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_peer_info_t pi = {}; 
        memcpy(pi.peer_addr, broadcastAddress, 6);
        esp_now_add_peer(&pi);
    }
    
    auto board = M5.getBoard();
    if (board == m5::board_t::board_M5AtomS3) { can_tx = 2; can_rx = 1; pinMode(41, INPUT_PULLUP); }
    else { can_tx = 32; can_rx = 33; }
    
    twai_general_config_t gc = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)can_tx, (gpio_num_t)can_rx, TWAI_MODE_NORMAL);
    twai_timing_config_t tc = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t fc = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&gc, &tc, &fc); twai_start();
    setupUI();
}

void loop() {
    M5.update();
    twai_message_t can_rx_msg; while (twai_receive(&can_rx_msg, 0) == ESP_OK) last_can_rx_time = millis();
    bool is_m = (millis() - last_can_rx_time < 1000);

    // Serial Command Parsing
    // Format: CAN:XX:YY:DDDDDDDD\n (XX=cmdSet, YY=cmdID, DDDDDDDD=hex data)
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("CAN:")) {
            int firstColon = line.indexOf(':');
            int secondColon = line.indexOf(':', firstColon + 1);
            int thirdColon = line.indexOf(':', secondColon + 1);
            if (firstColon != -1 && secondColon != -1 && thirdColon != -1) {
                uint8_t cSet = (uint8_t) strtol(line.substring(firstColon + 1, secondColon).c_str(), NULL, 16);
                uint8_t cID = (uint8_t) strtol(line.substring(secondColon + 1, thirdColon).c_str(), NULL, 16);
                String dataStr = line.substring(thirdColon + 1);
                size_t dLen = dataStr.length() / 2;
                uint8_t dBuf[16];
                for (size_t i = 0; i < dLen && i < 16; i++) {
                    String byteStr = dataStr.substring(i * 2, i * 2 + 2);
                    dBuf[i] = (uint8_t) strtol(byteStr.c_str(), NULL, 16);
                }
                
                // If we are MASTER, send directly. If REMOTE, forward via ESP-NOW.
                if (is_m) {
                    sendCANPacket(cSet, cID, dBuf, dLen);
                    M5.Display.fillRect(0, 20, M5.Display.width(), 20, TFT_PURPLE);
                    M5.Display.setTextColor(TFT_WHITE);
                    M5.Display.setTextDatum(top_left);
                    M5.Display.drawString("SER CMD TX", 5, 24);
                } else {
                    dispatchWireless(cSet, cID, dBuf, dLen);
                }
            }
        }
    }

    static bool was_m = false;
    if (is_m != was_m) {
        was_m = is_m;
        M5.Display.fillRect(0, 0, M5.Display.width(), 20, is_m ? GREEN : BLUE);
        M5.Display.setTextColor(BLACK);
        M5.Display.setTextDatum(top_center);
        M5.Display.drawString(is_m ? "MASTER" : "REMOTE", M5.Display.width()/2, 4);
    }

    if (is_m && (millis() - last_beacon_sent > 2000)) {
        BeaconPacket bp = {.type = PKT_BEACON, .sessionID = current_session_id};
        esp_now_send(broadcastAddress, (uint8_t*)&bp, sizeof(bp));
        last_beacon_sent = millis();
    }

    bool hit = M5.BtnA.wasPressed();
    if (M5.getBoard() == m5::board_t::board_M5AtomS3 && digitalRead(41) == LOW) { delay(150); hit = true; }
    if (hit) {
        uint8_t d[8] = {0,0,0,0,0,0,1,10};
        if (is_m) sendCANPacket(0x0E, 0x00, d, 8);
        else dispatchWireless(0x0E, 0x00, d, 8);
        M5.Display.fillCircle(M5.Display.width()/2, M5.Display.height()/2, M5.Display.width()/4, WHITE);
        delay(50); setupUI();
    }

    static uint32_t last_t_cmd = 0;
    auto t = M5.Touch.getDetail();
    if (t.isPressed() && M5.Display.width() > 200 && !is_m) {
        if (millis() - last_t_cmd > 100) {
            int x = t.x, y = t.y, s = 400;
            if (y < 80) dispatchWireless(0x0E, 0x01, (uint8_t[]){0,0,0,0,(uint8_t)(s&0xFF),(uint8_t)(s>>8),0x80}, 7);
            else if (y > 160) dispatchWireless(0x0E, 0x01, (uint8_t[]){0,0,0,0,(uint8_t)((-s)&0xFF),(uint8_t)((-s)>>8),0x80}, 7);
            else if (x < 100) dispatchWireless(0x0E, 0x01, (uint8_t[]){(uint8_t)((-s)&0xFF),(uint8_t)((-s)>>8),0,0,0,0,0x80}, 7);
            else if (x > 220) dispatchWireless(0x0E, 0x01, (uint8_t[]){(uint8_t)(s&0xFF),(uint8_t)(s>>8),0,0,0,0,0x80}, 7);
            is_moving = true; last_t_cmd = millis();
        }
    } else if (is_moving) {
        dispatchWireless(0x0E, 0x01, (uint8_t[]){0,0,0,0,0,0,0x80}, 7);
        is_moving = false;
    }
}
