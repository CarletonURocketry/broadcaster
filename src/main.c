#include <hw/spi-master.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 250
static char buffer[BUFFER_SIZE] = {0};
static const char RN2483[] = "/dev/ser3";

int main(int argc, char **argv) {

    int fd = open(RN2483, O_RDWR | O_NDELAY | O_NOCTTY);

    if (fd == -1) {
        fprintf(stderr, "Could not open device with error %d\n", fd);
        exit(EXIT_FAILURE);
    }
    printf("Successfully opened %s with file descriptor %d\n", RN2483, fd);

    // Do stuff
    const char *cmd = "sys get ver\n";
    write(fd, cmd, strlen(cmd));

    size_t chars_read = read(fd, buffer, BUFFER_SIZE);
    printf("Read %lu bytes\n", chars_read);
    if (chars_read) {
        printf("%s\n", buffer);
    }

    close(fd);
    return 0;
}
