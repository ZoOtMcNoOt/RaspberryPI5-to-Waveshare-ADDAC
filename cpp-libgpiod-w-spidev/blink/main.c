#include <gpiod.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#ifndef CONSUMER
#define CONSUMER "consumer"
#endif

int main(int argc, char **argv) {
    char *chipname = "gpiochip4";
    unsigned int line_num=21;
    unsigned int val;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int i,ret;

    chip =gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("open chip failed\n");
        goto end;
    }

    line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("get line failed\n");
        goto close_chip;
    }

    ret =  gpiod_line_request_output(line, CONSUMER, 0);
    if (ret < 0) {
        perror("request output failed\n");
        goto release_line;
    }

    val = 0;
    for (i = 0; i < 20; i++) {
        ret = gpiod_line_set_value(line, val);
        if (ret < 0) {
            perror("set value failed\n");
            goto release_line;
        }
        printf("Set line %u to #%u\n", val, line_num);
        val = !val;
        sleep(1); 
    }

    release_line:
        gpiod_line_release(line);

    close_chip:
        gpiod_chip_close(chip);
    
    end:
        return 0;
}