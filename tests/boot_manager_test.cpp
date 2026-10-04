#include "grid_return.h"
#include "input_gate.h"
#include <cstdio>
#include <cstring>
esp_partition_t factory={0x10000,0x300000},payload={0x310000,0x600000},otadata={0xe000,0x2000};
bool hasFactory=true,hasPayload=true,hasOtaData=true;
const esp_partition_t* running=&factory;
int result=ESP_OK,selects=0,restarts=0,failures=0,otaErases=0,payloadReads=0;
int factoryDesc=ESP_OK,readResult=ESP_OK,eraseResult=ESP_OK;
bool payloadErased=false,flipSecondRead=false;
const esp_partition_t* selected=nullptr;
uint8_t payloadImage[16]={0xe9,0x06,0x02,0x4f};
const esp_partition_t* esp_partition_find_first(int type,int sub,const char* name) {
 if(type==ESP_PARTITION_TYPE_APP && sub==ESP_PARTITION_SUBTYPE_APP_FACTORY && (!name || std::strcmp(name,"factory")==0))return hasFactory?&factory:nullptr;
 if(type==ESP_PARTITION_TYPE_APP && sub==ESP_PARTITION_SUBTYPE_APP_OTA_0 && (!name || std::strcmp(name,"ota_0")==0))return hasPayload?&payload:nullptr;
 if(type==ESP_PARTITION_TYPE_DATA && sub==ESP_PARTITION_SUBTYPE_DATA_OTA && (!name || std::strcmp(name,"otadata")==0))return hasOtaData?&otadata:nullptr;
 return nullptr;
}
const esp_partition_t* esp_ota_get_running_partition(){return running;}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t* p){selected=p;selects++;return result;}
esp_err_t esp_ota_get_partition_description(const esp_partition_t* p, esp_app_desc_t*){return p==&factory?factoryDesc:ESP_OK;}
esp_err_t esp_partition_read(const esp_partition_t* p,size_t offset,void* dst,size_t size){
 if(readResult!=ESP_OK)return readResult;
 if(!p || !dst || p->address!=payload.address || offset+size>sizeof(payloadImage))return ESP_ERR_NOT_FOUND;
 payloadReads++;
 if(flipSecondRead && payloadReads==2)payloadImage[0]^=0xff;
 std::memcpy(dst,payloadImage+offset,size);
 return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t* p,size_t offset,size_t size){
 if(!p)return ESP_ERR_NOT_FOUND;
 if(p->address==payload.address){payloadErased=true;std::memset(payloadImage,0xff,sizeof(payloadImage));return ESP_OK;}
 if(p->address==otadata.address && offset==0 && size==otadata.size){if(eraseResult!=ESP_OK)return eraseResult;otaErases++;return ESP_OK;}
 return ESP_ERR_INVALID_STATE;
}
void esp_restart(){restarts++;}
void check(bool ok,const char* name){if(!ok){std::printf("FAIL %s\n",name);failures++;}}
int main(){
 grid::PressGate gate;
 check(!gate.rising(true),"startup touch ignored");
 check(!gate.rising(true),"continued startup touch ignored");
 check(!gate.rising(false),"release arms input");
 check(gate.rising(true),"fresh tap activates");
 check(!gate.rising(true),"held tap does not repeat");
 gate.rising(false);check(gate.rising(true),"next tap activates");
 int erases=otaErases;
 check(grid::selectPayload()==ESP_OK && selected==&payload && !payloadErased && payloadImage[0]==0xe9 && otaErases==erases,"select payload without rewriting its image");
 running=&payload;check(grid::selectPayload()==ESP_ERR_INVALID_STATE,"reject current target");
 int before=selects;hasFactory=false;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND && selects==before && restarts==0 && otaErases==erases,"missing factory no write/reboot");hasFactory=true;
 factory.address=0x20000;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND,"wrong factory offset");factory.address=0x10000;
 payload.size=0x500000;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND,"wrong payload layout");payload.size=0x600000;
 factoryDesc=ESP_ERR_NOT_FOUND;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND && restarts==0 && otaErases==erases,"factory descriptor failure does not clear boot data");factoryDesc=ESP_OK;
 eraseResult=42;check(gridReturnToLauncher()==42 && restarts==0 && otaErases==erases,"otadata erase failure propagated, no reboot");eraseResult=ESP_OK;
 check(gridReturnToLauncher()==ESP_OK && selected!=&factory && selects==before && otaErases==erases+1 && restarts==1 && payloadImage[0]==0xe9 && !payloadErased,"return clears otadata only and restarts once");
 payloadReads=0;flipSecondRead=true;int restartsBefore=restarts;check(gridReturnToLauncher()==ESP_ERR_INVALID_STATE && restarts==restartsBefore,"changed payload header aborts return");flipSecondRead=false;payloadImage[0]=0xe9;
 hasPayload=false;check(grid::selectLauncher()==ESP_ERR_NOT_FOUND,"missing payload layout");hasPayload=true;
 esp_partition_t unknown={0x990000,0x10000};running=&unknown;check(grid::selectLauncher()==ESP_ERR_INVALID_STATE,"unknown running slot");
 running=nullptr;check(grid::selectLauncher()==ESP_ERR_INVALID_STATE,"missing running partition");
 running=&factory;restartsBefore=restarts;erases=otaErases;check(gridReturnToLauncher()==ESP_ERR_INVALID_STATE && restarts==restartsBefore && otaErases==erases,"return from launcher no reboot loop");
 result=42;running=&factory;check(grid::selectPayload()==42,"payload boot-selection failure propagated");
 std::printf("Boot manager: %d failures\n",failures);return failures?1:0;
}
