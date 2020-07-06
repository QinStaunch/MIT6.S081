#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint64 uptime_ticks;
    uptime_ticks = uptime();
    printf("%d clock tick interrupts have occured since start.\n", uptime_ticks);
    exit();
}