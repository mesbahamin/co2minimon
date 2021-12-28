#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>     // open()
#include <sys/ioctl.h> // ioctl()
#include <unistd.h>    // close()

#include <linux/hidraw.h>

typedef uint8_t buffer[8];

// Made possible by Henryk Plötz' excellent work:
// https://hackaday.io/project/5301-reverse-engineering-a-low-cost-usb-co-monitor

// references
// https://www.kernel.org/doc/Documentation/hid/hidraw.txt
int main(void) {
    // NOTE: Included udev rules create this symlink to the appropriate hidraw
    // entry.
    int device_handle = open("/dev/co2mini0", O_RDWR);
    if (device_handle < 0) {
        printf("ERROR: Failed to open hid\n");
        return 1;
    }

    buffer key = {0};
    int result = ioctl(device_handle, HIDIOCSFEATURE(sizeof(key)), key);
    if (result < 0 || result != sizeof(buffer)) {
        printf("ERROR: Failed to send feature report\n");
        return 1;
    }

    while (true) {
        buffer data = {0};
        int bytes_read = read(device_handle, &data, sizeof(buffer));
        if (bytes_read < 0 && (errno == EAGAIN || errno == EINPROGRESS)) {
            bytes_read = 0;
        } else {
            assert(bytes_read == sizeof(buffer));
        }

        if (bytes_read == sizeof(buffer)) {
            buffer decoded = {0};
            {
                {
                    uint8_t tmp;
                    tmp = data[0]; data[0] = data[2]; data[2] = tmp;
                    tmp = data[1]; data[1] = data[4]; data[4] = tmp;
                    tmp = data[3]; data[3] = data[7]; data[7] = tmp;
                    tmp = data[5]; data[5] = data[6]; data[6] = tmp;
                }

                for (int i = 0; i < sizeof(buffer); ++i) {
                    data[i] ^= key[i];
                }

                {
                    uint8_t tmp = (data[7] << 5);
                    decoded[7] = (data[6] << 5) | (data[7] >> 3);
                    decoded[6] = (data[5] << 5) | (data[6] >> 3);
                    decoded[5] = (data[4] << 5) | (data[5] >> 3);
                    decoded[4] = (data[3] << 5) | (data[4] >> 3);
                    decoded[3] = (data[2] << 5) | (data[3] >> 3);
                    decoded[2] = (data[1] << 5) | (data[2] >> 3);
                    decoded[1] = (data[0] << 5) | (data[1] >> 3);
                    decoded[0] = tmp | (data[0] >> 3);
                }

                buffer dumb_proprietary_bullshit_secret_word = "Htemp99e";
                for (int i = 0; i < sizeof(buffer); ++i)
                {
                    decoded[i] -= (dumb_proprietary_bullshit_secret_word[i] << 4) | (dumb_proprietary_bullshit_secret_word[i] >> 4);
                }
            }

            if (decoded[4] != 0x0d) {
                printf("BAD DATA: ");
            } else {
                switch(decoded[0]) {
                    case 0x42:
                        printf("Temperature: ");
                    break; case 0x50:
                        printf("CO2:         ");
                    break;
                }
            }

            for (int i = 0; i < sizeof(buffer); i++) {
                printf("[%i]: %X ", i, decoded[i]);
            }

            printf("\n");
        }
    }

    close(device_handle);
    return 0;
}
