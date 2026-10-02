#include "grid_return.h"
#include "input_gate.h"
#include <cstdio>
#include <cstring>
esp_partition_t factory={0x10000,0x300000},payload={0x310000,0x600000};
bool hasFactory=true,hasPayload=true;const esp_partition_t* running=&factory;
int result=ESP_OK,selects=0,restarts=0,failures=0;const esp_partition_t* selected=nullptr;
const esp_partition_t* esp_partition_find_first(int,int sub,const char* name) {
 if(sub==0 && std::strcmp(name,"factory")==0)return hasFactory?&factory:nullptr;
 if(sub==16 && std::strcmp(name,"ota_0")==0)return hasPayload?&payload:nullptr;
 return nullptr;
}
const esp_partition_t* esp_ota_get_running_partition(){return running;}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t* p){selected=p;selects++;return result;}
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
 check(grid::selectPayload()==ESP_OK && selected==&payload,"select existing payload");
 running=&payload;check(grid::selectPayload()==ESP_ERR_INVALID_STATE,"reject current target");
 int before=selects;hasFactory=false;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND && selects==before && restarts==0,"missing factory no write/reboot");hasFactory=true;
 factory.address=0x20000;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND,"wrong factory offset");factory.address=0x10000;
 payload.size=0x500000;check(gridReturnToLauncher()==ESP_ERR_NOT_FOUND,"wrong payload layout");payload.size=0x600000;
 result=42;check(gridReturnToLauncher()==42 && restarts==0,"verification failure propagated, no reboot");
 result=ESP_OK;check(gridReturnToLauncher()==ESP_OK && selected==&factory && restarts==1,"successful return restarts once");
 hasPayload=false;check(grid::selectLauncher()==ESP_ERR_NOT_FOUND,"missing payload layout");hasPayload=true;
 esp_partition_t unknown={0x990000,0x10000};running=&unknown;check(grid::selectLauncher()==ESP_ERR_INVALID_STATE,"unknown running slot");
 running=nullptr;check(grid::selectLauncher()==ESP_ERR_INVALID_STATE,"missing running partition");
 running=&factory;check(gridReturnToLauncher()==ESP_ERR_INVALID_STATE && restarts==1,"return from launcher no reboot loop");
 std::printf("Boot manager: %d failures\n",failures);return failures?1:0;
}
