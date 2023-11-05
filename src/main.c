#include <hw/spi-master.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 250
static char buffer[BUFFER_SIZE] = {0};

int main(int argc, char **argv) {

    int fd = spi_open("/dev/spi0");

    if (fd == -1) {
        fprintf(stderr, "Could not open device with error %d\n", fd);
        exit(EXIT_FAILURE);
    }

    // Do stuff
    spi_cmdread(fd, SPI_DEV_DEFAULT, "sys get ver", 12, buffer, BUFFER_SIZE);

    spi_close(fd);
    return 0;
}
