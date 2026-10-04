#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Static RGB565 chrome. No timers, heap allocations, or animation task.
namespace gridui {
constexpr uint16_t Background=0x0841, Panel=0x1083, Edge=0x2947;
constexpr uint16_t Text=0xe73c, Muted=0x9cd3, Warning=0xfd20, Magenta=0xd81f;
inline void frame(TFT_eSPI& t,int x,int y,int w,int h,uint16_t color) {
    constexpr int cut=5;
    t.drawFastHLine(x+cut,y,w-cut,color);
    t.drawLine(x,y+cut,x+cut,y,color);
    t.drawFastVLine(x,y+cut,h-cut,color);
    t.drawFastHLine(x,y+h-1,w-cut,color);
    t.drawLine(x+w-cut-1,y+h-1,x+w-1,y+h-cut-1,color);
    t.drawFastVLine(x+w-1,y,h-cut,color);
}
inline void text(TFT_eSPI& t,const String& s,int x,int y,uint16_t color=Text,
                 uint16_t background=Background,int font=2) {
    t.setTextColor(color,background);t.drawString(s,x,y,font);
}
inline void panel(TFT_eSPI& t,int x,int y,int w,int h,uint16_t color=Edge) {
    t.fillRect(x,y,w,h,Panel);frame(t,x,y,w,h,color);
}
inline void button(TFT_eSPI& t,const String& s,int x,int y,int w,uint16_t accent) {
    panel(t,x,y,w,36,accent);
    t.fillRect(x+5,y+8,2,20,accent);
    text(t,s,x+12,y+10,accent,Panel);
}
inline void shell(TFT_eSPI& t,int page,uint16_t accent,const String& status) {
    t.fillScreen(Background);
    t.fillRect(0,0,320,28,Panel);
    text(t,"GRID//OS",8,5,accent,Panel);
    text(t,"ES3C28P / LINK",218,10,Muted,Panel,1);
    t.drawFastHLine(0,28,320,Edge);t.drawFastHLine(8,28,78,accent);
    t.drawFastHLine(280,28,32,Magenta);
    t.drawFastHLine(8,181,304,Edge);
    text(t,status,8,187,Muted,Background,1);
    const char* names[]={"SYS","APPS","FILES","NET","TERM","CFG"};
    for(int i=0;i<6;i++) {
        const int x=i*53,w=i==5?55:53;
        t.fillRect(x,207,w,33,page==i?Panel:Background);
        t.drawFastHLine(x+3,207,w-6,page==i?accent:Edge);
        text(t,names[i],x+6,216,page==i?accent:Muted,page==i?Panel:Background);
        if(page==i)t.fillRect(x+3,236,w-6,2,accent);
    }
}
inline void title(TFT_eSPI& t,const String& s,uint16_t accent) {
    text(t,s,8,35,accent);
    t.fillRect(302,36,3,12,Magenta);t.fillRect(308,36,3,12,accent);
}
inline void progress(TFT_eSPI& t,size_t done,size_t total,uint16_t accent) {
    const unsigned percent=total?unsigned(done*100/total):0;
    t.fillRect(10,96,300,65,Background);
    frame(t,10,100,300,18,Edge);
    t.fillRect(13,103,int(percent*294/100),12,accent);
    text(t,String(percent)+"%  /  "+String(done)+" B",10,130,accent);
}
}
