#include "sys.h"
void user_main(void)
{
    if (hello() != 0) for (;;) {}
    if (hello() != 0) for (;;) {}
    for (;;) {}
}
