#include <stdio.h>
#include <ulib.h>

const char buf[] = {0x01, 0x02, 0x03};

int
main(void) {
    cprintf("Testing gwrite.\n");
    int ret = gwrite(buf, 3);
    cprintf("gwrite returns %d.\n", ret);
    return 0;
}

