#include "installed_app.h"
#include <cstdio>
#include <cstring>
int failures=0;
void check(bool ok,const char* name){if(!ok){std::printf("FAIL %s\n",name);failures++;}}
void fillValid(uint8_t* image){
 std::memset(image,0,112);
 image[0]=0xe9;image[1]=6;image[12]=9;image[13]=0;
 image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
 std::memcpy(image+48,"0.1.0",5);
 std::memcpy(image+80,"esp-goblin",10);
}
int main(){
 uint8_t image[112];
 fillValid(image);
 // No boot-selection or SD argument: an erased otadata record must not hide the image.
 grid::InstalledApp app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(app.present && app.reason==nullptr,"valid payload discovered while factory is selected and SD is absent");
 check(std::strcmp(app.project,"esp-goblin")==0,"project name read from payload descriptor");
 check(std::strcmp(app.version,"0.1.0")==0,"version read from payload descriptor");
 image[0]=0xff;app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"payload image missing")==0,"erased payload reports image missing");
 fillValid(image);image[0]=0x00;app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"payload image missing")==0,"blank payload reports image missing");
 fillValid(image);image[12]=0;app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"Requires ESP32-S3 image")==0,"wrong chip rejected");
 fillValid(image);image[32]=0;app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"descriptor magic mismatch")==0,"bad descriptor rejected");
 fillValid(image);app=grid::inspectPayloadImage(image,36,0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"payload header unreadable")==0,"short header rejected");
 fillValid(image);std::memset(image+80,0,32);app=grid::inspectPayloadImage(image,sizeof(image),0x600000);
 check(app.present && std::strcmp(app.project,"PAYLOAD")==0,"empty project name stays installed");
 app=grid::inspectPayloadImage(nullptr,112,0x600000);
 check(!app.present && app.reason && std::strcmp(app.reason,"payload header unreadable")==0,"null image rejected");
 std::printf("Installed app: %d failures\n",failures);return failures?1:0;
}
