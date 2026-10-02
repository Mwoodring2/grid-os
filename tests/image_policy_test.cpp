#include "image_policy.h"
#include <cstdio>
#include <cstring>
int failures=0;
void check(bool ok,const char* name){if(!ok){std::printf("FAIL %s\n",name);failures++;}}
int main(){
 uint8_t h[36]={};h[0]=0xe9;h[1]=3;h[12]=9;
 check(grid::imageHeaderError(h,24,4096,8192)==nullptr,"S3 accepted");
 check(grid::imageHeaderError(h,23,4096,8192)!=nullptr,"short read rejected");
 check(grid::imageHeaderError(h,24,9000,8192)!=nullptr,"oversize rejected");
 h[12]=0;check(grid::imageHeaderError(h,24,4096,8192)!=nullptr,"ESP32 rejected");h[12]=9;
 h[1]=0;check(grid::imageHeaderError(h,24,4096,8192)!=nullptr,"no segments rejected");h[1]=17;check(grid::imageHeaderError(h,24,4096,8192)!=nullptr,"excess segments rejected");
 check(!grid::appDescriptorMagic(h+32),"bootloader rejected");
 uint8_t magic[]={0x32,0x54,0xcd,0xab};check(grid::appDescriptorMagic(magic),"app descriptor accepted");
 std::printf("Image policy: %d failures\n",failures);return failures?1:0;
}
