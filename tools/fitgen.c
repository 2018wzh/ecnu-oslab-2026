/* 输出 U-Boot FIT 描述，二进制与描述位于同一目录。 */
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    unsigned long load = strtoul(argv[1], NULL, 0);
    printf("/dts-v1/;\n/ { description = \"OSLab\"; #address-cells = <1>;\n"
           "images { kernel { description = \"kernel\"; data = /incbin/(\"kernel.bin\"); "
           "type = \"kernel\"; arch = \"riscv\"; os = \"linux\"; compression = \"none\"; "
           "load = <0x%lx>; entry = <0x%lx>; }; };\n"
           "configurations { default = \"conf\"; conf { kernel = \"kernel\"; }; }; };\n", load, load);
    return ferror(stdout) ? 1 : 0;
}
