#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

// https://www.kernel.org/doc/Documentation/hid/hidraw.txt
#include <linux/hidraw.h>

static bool running = true;

void stop_running(int sig)
{
    (void)sig;
    running = false;
}

// Made possible by Henryk Plötz' excellent work:
// https://hackaday.io/project/5301-reverse-engineering-a-low-cost-usb-co-monitor
int main(void)
{
    {
        struct sigaction act = {
            .sa_handler = stop_running,
        };
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGKILL, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
    }

    // NOTE: Included udev rules create this symlink to the appropriate hidraw
    // entry.
    const char *hid_file_path         = "/dev/co2mini0";
    const char *temperature_file_path = "/tmp/co2minimon_temp";
    const char *co2_file_path         = "/tmp/co2minimon_co2";

    int device_handle = -1;
    int again_count = 0;

    while (running)
    {

        if (access(hid_file_path, F_OK) != 0)
        {
            if (close(device_handle) == 0)
            {
                device_handle = -1;
            }

            unlink(co2_file_path);
            unlink(temperature_file_path);

            sleep(30);
            continue;
        }

        if (device_handle == -1)
        {
            device_handle = open(hid_file_path, O_RDWR);
            if (device_handle < 0)
            {
                printf("ERROR: Failed to open HID.\n");
                return 1;
            }

            // TODO: do a timed out blocking read with select(), or if not
            // possible, at least sleep(5) at the end of the loop.
            int flags = fcntl(device_handle, F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(device_handle, F_SETFL, flags);

            // TODO: Try with just a 0
            uint8_t key[8] = {0};
            int result = ioctl(device_handle, HIDIOCSFEATURE(sizeof(key)), key);
            if (result < 0 || result != sizeof(key))
            {
                printf("ERROR: Failed to send feature report.\n");
                return 1;
            }
        }

        uint8_t data[8] = {0};
        int bytes_read = read(device_handle, &data, sizeof(data));
        // TODO: handle bytes_read == 0 aka eof aka pipe closed for writing
        if (bytes_read < 0)
        {
            if ((errno == EAGAIN || errno == EWOULDBLOCK))
            {
                again_count++;
                if (again_count > 10000000) {
                    // If we go too long with an empty pipe, the device may
                    // have stopped sending data. Resend the feature report to
                    // get it to send data again. A situation where I've
                    // observed this happening is when I hibernate and resume
                    // the computer. Upon resumption, all calls to read() will
                    // block until we resend the feature report.
                    //
                    // I'm not sure why this happens, but my guess would be
                    // that the device stops sending new data if the data
                    // hasn't been read in a while.
                    uint8_t key[8] = {0};
                    int result = ioctl(device_handle, HIDIOCSFEATURE(sizeof(key)), key);
                    if (result < 0 || result != sizeof(key))
                    {
                        printf("ERROR: Failed to send feature report.\n");
                        return 1;
                    }
                }
                continue;
            } else {
                // TODO: handle error
                printf("Unhandled error: %i\n", errno);
            }
        }
        again_count = 0;

        //printf("bytes_read: %d\n", bytes_read);

        uint8_t item     = data[0];
        uint8_t msb      = data[1];
        uint8_t lsb      = data[2];
        uint8_t checksum = data[3];
        uint8_t end      = data[4];

        if (bytes_read == sizeof(data)
            && end == 0x0d
            && (item == 0x42 || item == 0x50)
            && ((item + msb + lsb) & 0xFF) == checksum)
        {
            printf("write\n");
            uint16_t value = (((uint16_t)msb) << 8) | lsb;
            char buf[1024] = {0};
            int str_len = 0;
            mode_t create_mode = S_IRUSR | S_IWUSR;
            int open_mode = O_WRONLY | O_CREAT | O_TRUNC;

            switch(item)
            {
                case 0x42:
                {
                    printf("bytes T\n");
                    double t_celsius = value / 16.0 - 273.15;
                    str_len = snprintf(buf, sizeof(buf), "%.2f", t_celsius);
                    assert(str_len > 0);
                    int f = open(temperature_file_path, open_mode, create_mode);
                    if (f == -1)
                    {
                        return 1;
                    }
                    write(f, buf, str_len);
                    close(f);
                } break;

                case 0x50:
                {
                    printf("bytes C\n");
                    str_len = snprintf(buf, sizeof(buf), "%d", value);
                    assert(str_len > 0);
                    int f = open(co2_file_path, open_mode, create_mode);
                    if (f == -1)
                    {
                        return 1;
                    }
                    write(f, buf, str_len);
                    close(f);
                } break;
            }
        }
    }

    unlink(co2_file_path);
    unlink(temperature_file_path);
    close(device_handle);
    return 0;
}
