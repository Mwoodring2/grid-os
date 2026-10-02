#include <Arduino.h>
#include <TFT_eSPI.h>
#include "es3c28p_board.h"
#include "touch_ft6336.h"
#include "grid_return.h"
#include "input_gate.h"

TFT_eSPI tft;
bool touchOk=false,bootHeld=false,bootTriggered=false;
grid::PressGate touchGate;
uint32_t bootStart=0;
String command;
void show(const String& status) {
    tft.fillScreen(0x0841);tft.setTextColor(0x07ff,0x0841);
    tft.drawString("GRID//RETURN TEST",12,12,2);
    tft.setTextColor(0xe73c,0x0841);
    tft.drawString("Compatible payload / ES3C28P",12,48,2);
    tft.drawString(status.substring(0,38),12,80,2);
    tft.drawString("SD card is not needed to return.",12,108,2);
    tft.drawRect(10,145,300,46,0x07ff);
    tft.drawString("RETURN TO GRID//OS",42,160,2);
    tft.drawString("Or hold BOOT 2s / USB: return",12,213,2);
    Serial.println(status);
}
void returnNow() {
    show("SELECTING FACTORY LAUNCHER...");
    esp_err_t result=gridReturnToLauncher();
    // Success restarts immediately; returning means there was an error.
    show("Return failed: "+String(esp_err_to_name(result)));
}
void setup() {
    Serial.begin(115200);
    pinMode(GoblinBoard::LCD_BL,OUTPUT);digitalWrite(GoblinBoard::LCD_BL,HIGH);
    pinMode(GoblinBoard::BOOT_BUTTON,INPUT_PULLUP);
    tft.init();tft.setRotation(1);touchOk=goblinTouchBegin();
    show(grid::layoutCompatible()?"PAYLOAD ONLINE":"PARTITION LAYOUT MISMATCH");
}
void loop() {
    GoblinTouchPoint p;
    if(touchOk && goblinTouchRead(p)) {
        if(touchGate.rising(p.pressed) && p.x>=10 && p.x<310 && p.y>=145 && p.y<191)returnNow();
    }
    // This fallback runs INSIDE this cooperative app. It is not universal
    // bootloader recovery, and BOOT must not be held while resetting.
    bool pressed=digitalRead(GoblinBoard::BOOT_BUTTON)==LOW;
    if(!pressed)bootTriggered=false;
    if(pressed&&!bootHeld)bootStart=millis();
    if(pressed&&bootHeld&&!bootTriggered&&millis()-bootStart>=2000){bootTriggered=true;returnNow();}
    bootHeld=pressed;
    while(Serial.available()) {
        char c=Serial.read();
        if(c=='\n'){command.trim();if(command=="return")returnNow();else Serial.println("Command: return");command="";}
        else if(c!='\r' && command.length()<32)command+=c;
    }
    delay(10);
}
