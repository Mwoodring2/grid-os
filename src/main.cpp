#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include "es3c28p_board.h"
#include "touch_ft6336.h"
#include "image_policy.h"
#include "grid_boot.h"

// GRID//OS v0.2.0-alpha. Serial console remains usable if touch/SD fails.
TFT_eSPI tft;
TFT_eSprite liveRow(&tft);
String cachedRows[5];
bool rowBufferOk=false;
Preferences prefs;
constexpr uint16_t BG=0x0841, TEXT=0xe73c, MUTED=0x7bef, WARN=0xfd20;
uint16_t accent=0x07ff;
bool sdOk=false,touchOk=false,held=false,dirty=true;
int page=0,offset=0,selected=-1,confirm=0,brightness=80;
String status="SYSTEM READY",cwd="/",commandLine;
bool commandOverflow=false;
struct Entry { String name,path; bool folder; size_t size; };
Entry entries[64]; int count=0;
bool installedAvailable=false;
String installedName;
uint32_t lastRefresh=0,lastTouch=0;

void message(const String& s) { status=s; Serial.println(s); dirty=true; }
void label(const String& s,int x,int y,uint16_t color=TEXT) {
    tft.setTextColor(color,BG); tft.drawString(s,x,y,2);
}
void button(const String& s,int x,int y,int w=96) {
    tft.drawRect(x,y,w,30,accent); label(s,x+6,y+6,accent);
}
String clipped(String s,int n=35) { if(s.length()>n) s=s.substring(0,n-3)+"..."; return s; }
void storageBegin() {
    SD_MMC.setPins(GoblinBoard::SD_CLK,GoblinBoard::SD_CMD,GoblinBoard::SD_D0);
    sdOk=SD_MMC.begin("/sdcard",true,false);
    if(sdOk) SD_MMC.mkdir("/apps");
}
void refreshInstalled() {
    installedAvailable=false;
    const esp_partition_t* p=grid::payloadPartition();
    if(!grid::layoutCompatible() || !p)return;
    esp_app_desc_t desc={};
    if(esp_ota_get_partition_description(p,&desc)!=ESP_OK)return;
    char name[33]={};memcpy(name,desc.project_name,32);
    installedName=String(name);
    if(installedName.isEmpty())installedName="PAYLOAD";
    installedAvailable=true; // Descriptor detected; boot selection verifies body.
}
int visibleCount() { return count+((page==1 && installedAvailable)?1:0); }
void launchInstalled() {
    if(!installedAvailable){message("No installed image detected");return;}
    esp_err_t result=grid::selectPayload();
    if(result!=ESP_OK){message("Launch rejected: "+String(esp_err_to_name(result)));return;}
    message("BOOTING INSTALLED PAYLOAD");delay(500);ESP.restart();
}
void enumerate() {
    count=0; offset=0; selected=-1; confirm=0;refreshInstalled();
    if(!sdOk) { message("SD OFFLINE: CFG > RETRY SD"); return; }
    String path=page==1?"/apps":cwd;
    fs::File dir=SD_MMC.open(path);
    if(!dir || !dir.isDirectory()) { message("Directory unavailable"); return; }
    fs::File f=dir.openNextFile();
    while(f && count<64) {
        String n=f.name(); int slash=n.lastIndexOf('/'); if(slash>=0)n=n.substring(slash+1);
        String lower=n; lower.toLowerCase();
        if(page!=1 || (!f.isDirectory() && lower.endsWith(".bin")))
            entries[count++]={n,path+(path=="/"?"":"/")+n,f.isDirectory(),size_t(f.size())};
        f.close(); f=dir.openNextFile();
    }
    bool truncated=bool(f); f.close();dir.close();
    for(int i=0;i<count;i++) for(int j=i+1;j<count;j++)
        if(entries[j].name.compareTo(entries[i].name)<0) { Entry e=entries[i];entries[i]=entries[j];entries[j]=e; }
    message(truncated?"First 64 entries; narrow directory":"SD INDEX READY");
}
const esp_partition_t* payloadSlot() {
    return grid::layoutCompatible()?grid::payloadPartition():nullptr;
}
String validate(fs::File& f) {
    const esp_partition_t* p=payloadSlot();
    if(!p) return "Missing payload partition";
    uint8_t header[36]={}; f.seek(0);
    if(f.read(header,sizeof(header))!=sizeof(header)) return "Cannot read image header";
    const char* error=grid::imageHeaderError(header,24,f.size(),p->size);
    if(error) return error;
    if(!grid::appDescriptorMagic(header+32)) return "Use application .bin, not merged image";
    f.seek(0);return "";
}
void installSelected() {
    if(selected<0 || selected>=count || !sdOk) {message("Select an app first");return;}
    const esp_partition_t* running=esp_ota_get_running_partition();
    if(!running || running->subtype!=ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        message("Installer requires factory launcher");return;
    }
    fs::File f=SD_MMC.open(entries[selected].path);
    if(!f) {message("Cannot open firmware");return;}
    String error=validate(f);
    if(error.length()) {f.close();confirm=0;message(error);return;}
    installedAvailable=false;
    esp_ota_handle_t handle=0;
    esp_err_t result=esp_ota_begin(payloadSlot(),f.size(),&handle);
    if(result!=ESP_OK) {f.close();confirm=0;message("OTA begin failed: "+String(esp_err_to_name(result)));return;}
    tft.fillScreen(BG);label("WRITING//PAYLOAD",10,20,accent);label("DO NOT POWER OFF",10,55,WARN);
    uint8_t buffer[4096];size_t written=0,total=f.size();
    while(written<total) {
        size_t wanted=total-written; if(wanted>sizeof(buffer))wanted=sizeof(buffer);
        size_t got=f.read(buffer,wanted);
        if(got!=wanted) {result=ESP_FAIL;break;}
        result=esp_ota_write(handle,buffer,got);if(result!=ESP_OK)break;
        written+=got;
        tft.fillRect(10,100,300,32,BG);label(String(written*100/total)+"%",10,105,accent);
        delay(1);
    }
    f.close();
    if(result!=ESP_OK) {esp_ota_abort(handle);confirm=0;message("Write failed; launcher retained");return;}
    result=esp_ota_end(handle);
    if(result!=ESP_OK) {confirm=0;message("Image verification failed; launcher retained");return;}
    result=grid::selectPayload();
    if(result!=ESP_OK) {confirm=0;message("Boot selection failed; launcher retained");return;}
    message("VERIFIED: rebooting payload");delay(700);ESP.restart();
}
void draw() {
    // Full redraw only on navigation/action; live SYS values update their own area.
    tft.fillScreen(BG);label("GRID//OS",8,5,accent);label("ES3C28P  R0.2",175,5,MUTED);
    tft.drawFastHLine(0,28,320,accent);
    const char* names[]={"SYS","APPS","FILES","NET","TERM","CFG"};
    for(int i=0;i<6;i++) {tft.drawRect(i*53,207,53,33,i==page?accent:MUTED);label(names[i],i*53+6,215,i==page?accent:MUTED);}
    label(clipped(status,39),8,185,MUTED);
    if(page==0) {
        label("SYSTEM//CORE",8,35,accent);
        label("CPU   "+String(ESP.getCpuFreqMHz())+" MHz",8,61);
        label("HEAP  "+String(ESP.getFreeHeap()/1024)+" KB FREE",8,84);
        label("PSRAM "+String(ESP.getFreePsram()/1024)+" KB FREE",8,107);
        label("FLASH "+String(ESP.getFlashChipSize()/1048576)+" MB",8,130);
        label("SD "+String(sdOk?"ONLINE":"OFFLINE")+"  UP "+String(millis()/1000)+"s",8,153);
    } else if(page==1 || page==2) {
        if(selected==-2 && page==1) {
            label("INSTALLED//PAYLOAD",8,36,accent);
            label(clipped(installedName),8,60);
            label("Launches from internal flash.",8,83,MUTED);
            label("Return needs a compatible app.",8,106,WARN);
            button("BACK",8,140,140);button("BOOT APP",160,140,152);
        } else if(confirm) {
            label("INSTALL//CONFIRM",8,36,WARN);
            label("Replaces the payload slot.",8,60);
            label("Return requires app integration",8,82,WARN);
            label("or USB reflash of GRID//OS.",8,104,WARN);
            button("CANCEL",8,140,140);button("WRITE + BOOT",160,140,152);
        } else if(selected>=0 && page==1) {
            label(clipped(entries[selected].name),8,36,accent);
            label(String(entries[selected].size)+" bytes",8,60);
            label("ESP32-S3 APPLICATION IMAGE ONLY",8,83,MUTED);
            label("Board compatibility is your choice.",8,106,MUTED);
            button("BACK",8,140,140);button("INSTALL",160,140,152);
        } else {
            label(page==1?"GRID//APPLICATIONS":clipped(cwd),8,33,accent);
            for(int r=0;r<3 && offset+r<visibleCount();r++) {
                const int index=offset+r;
                tft.drawRect(6,55+r*30,308,28,MUTED);
                if(page==1 && installedAvailable && index==0)
                    label(clipped("[FLASH] "+installedName),12,60+r*30,accent);
                else {
                    const int fileIndex=index-((page==1&&installedAvailable)?1:0);
                    const Entry& e=entries[fileIndex];
                    label(clipped((e.folder?"[DIR] ":"")+e.name),12,60+r*30);
                }
            }
            button(page==1?"RESCAN":"UP",8,149,96);button("PREV",111,149,96);button("NEXT",214,149,98);
        }
    } else if(page==3) {
        label("NETWORK//WLAN",8,35,accent);
        label(WiFi.status()==WL_CONNECTED?"CONNECTED":"DISCONNECTED",8,62);
        label("SSID "+clipped(WiFi.SSID(),28),8,86);
        label("IP   "+WiFi.localIP().toString(),8,110);
        label(WiFi.status()==WL_CONNECTED?"RSSI "+String(WiFi.RSSI())+" dBm":"RSSI UNAVAILABLE",8,134);
        label("Connect through USB terminal.",8,158,MUTED);
    } else if(page==4) {
        label("TERM//LOCAL  USB 115200",8,35,accent);
        label("help | sysinfo | sd ls | app list",8,64);
        label("wifi connect SSID|PASSWORD",8,87);
        label("brightness 80 | theme cyan",8,110);
        label("reboot | clear",8,133);
        label("Commands run over USB serial.",8,156,MUTED);
    } else {
        label("CONFIG//DISPLAY",8,35,accent);
        button("DIM -",8,64,140);button("BRIGHT +",160,64,152);
        button("THEME",8,106,140);button("RETRY SD",160,106,152);
        label("BRIGHTNESS "+String(brightness)+"%",8,151);
    }
    for(auto& row:cachedRows)row="";
    dirty=false;
}
void touch(int x,int y) {
    if(y>=207) {page=x/53;if(page>5)page=5;selected=-1;confirm=0;if(page==1||page==2)enumerate();dirty=true;return;}
    if(page==1||page==2) {
        if(selected==-2 && page==1) {
            if(y>=140 && y<175){if(x<155){selected=-1;dirty=true;}else launchInstalled();}
            return;
        }
        if(confirm && y>=140 && y<175) {if(x<155){confirm=0;dirty=true;}else installSelected();return;}
        if(selected>=0 && page==1 && y>=140 && y<175){if(x<155)selected=-1;else confirm=1;dirty=true;return;}
        if(y>=55 && y<145 && selected<0) {
            int i=offset+(y-55)/30;if(i>=visibleCount())return;
            if(page==1 && installedAvailable){if(i==0){selected=-2;dirty=true;return;}i--;}
            if(page==1)selected=i;
            else if(entries[i].folder){cwd=entries[i].path;enumerate();}
            else message(clipped(entries[i].name)+" "+String(entries[i].size)+" B");
            dirty=true;
        }
        if(y>=149&&y<179) {
            if(x<104){if(page==2){int slash=cwd.lastIndexOf('/');cwd=slash<=0?"/":cwd.substring(0,slash);}enumerate();}
            else if(x<207){offset-=3;if(offset<0)offset=0;}
            else if(offset+3<visibleCount())offset+=3;
            dirty=true;
        }
    } else if(page==5) {
        if(y>=64&&y<94){brightness+=(x<155?-10:10);brightness=constrain(brightness,10,100);analogWrite(GoblinBoard::LCD_BL,brightness*255/100);prefs.putInt("brightness",brightness);}
        if(y>=106&&y<136){if(x<155){accent=accent==0x07ff?0x07e0:accent==0x07e0?0xfd20:0x07ff;prefs.putUShort("accent",accent);}else{SD_MMC.end();storageBegin();message(sdOk?"SD ONLINE":"SD OFFLINE");}}
        dirty=true;
    }
}
void execute(String cmd) {
    cmd.trim();
    if(cmd=="help") Serial.println("help, sysinfo, sd ls, app list, app installed, app boot, wifi status, wifi connect SSID|PASSWORD, brightness 10..100, theme cyan|green|amber, reboot, clear");
    else if(cmd=="sysinfo") Serial.printf("GRID//OS ESP32-S3 heap=%u psram=%u flash=%u uptime=%lu\n",ESP.getFreeHeap(),ESP.getFreePsram(),ESP.getFlashChipSize(),(unsigned long)(millis()/1000));
    else if(cmd=="sd ls"||cmd=="app list") {int old=page;page=cmd=="app list"?1:2;enumerate();for(int i=0;i<count;i++)Serial.println(entries[i].path);page=old;selected=-1;dirty=true;}
    else if(cmd=="app installed") {refreshInstalled();message(installedAvailable?"INSTALLED: "+installedName:"No installed image detected");}
    else if(cmd=="app boot") {refreshInstalled();launchInstalled();}
    else if(cmd=="wifi status") Serial.println(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"Disconnected");
    else if(cmd.startsWith("wifi connect ")) {String args=cmd.substring(13);int split=args.indexOf('|');if(split<1){message("Use SSID|PASSWORD");return;}WiFi.mode(WIFI_STA);WiFi.begin(args.substring(0,split).c_str(),args.substring(split+1).c_str());message("WiFi connecting...");}
    else if(cmd.startsWith("brightness ")) {String value=cmd.substring(11);bool numeric=value.length()>0;for(size_t i=0;i<value.length();i++)if(value[i]<'0'||value[i]>'9')numeric=false;int v=value.toInt();if(!numeric||v<10||v>100){message("Brightness range: 10..100");return;}brightness=v;analogWrite(GoblinBoard::LCD_BL,v*255/100);prefs.putInt("brightness",v);dirty=true;}
    else if(cmd.startsWith("theme ")) {String value=cmd.substring(6);if(value=="cyan")accent=0x07ff;else if(value=="green")accent=0x07e0;else if(value=="amber")accent=0xfd20;else{message("Theme: cyan green amber");return;}prefs.putUShort("accent",accent);dirty=true;}
    else if(cmd=="reboot")ESP.restart();
    else if(cmd=="clear") {status="SYSTEM READY";dirty=true;}
    else message("Unknown command; type help");
}
void setup() {
    Serial.begin(115200);prefs.begin("grid-os",false);
    brightness=constrain(prefs.getInt("brightness",80),10,100);accent=prefs.getUShort("accent",0x07ff);
    pinMode(GoblinBoard::LCD_BL,OUTPUT);analogWrite(GoblinBoard::LCD_BL,brightness*255/100);
    tft.init();tft.setRotation(1);liveRow.setColorDepth(16);rowBufferOk=liveRow.createSprite(320,23)!=nullptr;touchOk=goblinTouchBegin();storageBegin();refreshInstalled();
    message(touchOk?"SYSTEM READY":"TOUCH OFFLINE: USB CONSOLE READY");draw();
}
void loop() {
    while(Serial.available()) {char c=Serial.read();if(c=='\r')continue;if(c=='\n'){if(commandOverflow)message("Command too long; discarded");else execute(commandLine);commandLine="";commandOverflow=false;}else if(c==8||c==127){if(commandLine.length())commandLine.remove(commandLine.length()-1);}else if(commandLine.length()<160)commandLine+=c;else commandOverflow=true;}
    GoblinTouchPoint p;bool read=touchOk&&goblinTouchRead(p);
    if(read) {if(p.pressed&&!held&&millis()-lastTouch>180){lastTouch=millis();touch(p.x,p.y);}held=p.pressed;}
    if(dirty)draw();
    if(millis()-lastRefresh>=1500) {
        lastRefresh=millis();
        if(page==0 || page==3) {
            String rows[]={"CPU   "+String(ESP.getCpuFreqMHz())+" MHz",
                "HEAP  "+String(ESP.getFreeHeap()/1024)+" KB FREE",
                "PSRAM "+String(ESP.getFreePsram()/1024)+" KB FREE",
                "FLASH "+String(ESP.getFlashChipSize()/1048576)+" MB",
                "SD "+String(sdOk?"ONLINE":"OFFLINE")+"  UP "+String(millis()/1000)+"s"};
            if(page==3) {
                rows[0]=WiFi.status()==WL_CONNECTED?"CONNECTED":"DISCONNECTED";
                rows[1]="SSID "+clipped(WiFi.SSID(),28);
                rows[2]="IP   "+WiFi.localIP().toString();
                rows[3]=WiFi.status()==WL_CONNECTED?"RSSI "+String(WiFi.RSSI())+" dBm":"RSSI UNAVAILABLE";
                rows[4]="Connect through USB terminal.";
            }
            for(int i=0;i<5;i++)if(rows[i]!=cachedRows[i]) {
                if(rowBufferOk) {liveRow.fillSprite(BG);liveRow.setTextColor(TEXT,BG);liveRow.drawString(rows[i],8,2,2);liveRow.pushSprite(0,59+i*23);}
                else {tft.fillRect(0,59+i*23,320,23,BG);label(rows[i],8,61+i*23);}
                cachedRows[i]=rows[i];
            }
        }
    }
    delay(10);
}
