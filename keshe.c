#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>

#define BUZZER_CTL "/dev/buzzer_ctl"
#define BUZZER_ON 1
#define BUZZER_OFF 0

int fd;
int running = 1;
int mode = 0; // 0: periodic, 1: always on, 2: always off
int interval_on = 500000; // default on interval in microseconds
int interval_off = 1000000; // default off interval in microseconds

void* control_buzzer(void* arg) {
    while (running) {
        if (mode == 0) { // periodic
            ioctl(fd, BUZZER_ON, 1);
            usleep(interval_on);
            ioctl(fd, BUZZER_OFF, 1);
            usleep(interval_off);
        } else if (mode == 1) { // always on
            ioctl(fd, BUZZER_ON, 1);
            usleep(100000);
        } else { // always off
            ioctl(fd, BUZZER_OFF, 1);
            usleep(100000);
        }
    }
    return NULL;
}

void handle_signal(int signal) {
    running = 0;
    close(fd);
    exit(0);
}

int main(int argc, char* argv[]) {
    struct termios options;
    char buffer[256];
    int n;

    if ((fd = open(BUZZER_CTL, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        perror("Open buzzer_ctl failed");
        return 1;
    }

    signal(SIGINT, handle_signal);

    pthread_t buzzer_thread;
    pthread_create(&buzzer_thread, NULL, control_buzzer, NULL);

    while (running) {
        // Read commands from the serial port
        n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            if (strncmp(buffer, "always on", 9) == 0) {
                mode = 1;
            } else if (strncmp(buffer, "always off", 10) == 0) {
                mode = 2;
            } else if (sscanf(buffer, "periodic %d %d", &interval_on, &interval_off) == 2) {
                mode = 0;
                interval_on *= 1000;
                interval_off *= 1000;
            } else {
                printf("Unknown command: %s\n", buffer);
            }
        }
    }

    close(fd);
    return 0;
}
