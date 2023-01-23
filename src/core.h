#include <cstdint>

bool setupCore(int chipSelect);
void loopCore();
uint32_t getUnixtime();
struct Config
{
    int timestamp;
    int logFreq;
    char name[64];
    char mDNSName[64];
    char ssid[64];
    char password[64];
};