#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: Use syslog.h for logging
// TODO: Make a systemd unit for this

// https://www.kernel.org/doc/Documentation/hid/hidraw.txt
#include <linux/hidraw.h>

volatile static bool running = true;

static void stop_running(int signal)
{
    (void)signal;
    running = false;
}

// Made possible by Henryk Plötz' excellent work:
// https://hackaday.io/project/5301-reverse-engineering-a-low-cost-usb-co-monitor
int main(void)
{
    {
        struct sigaction act =
        {
            .sa_handler = stop_running,
        };
        int failure = sigaction(SIGINT, &act, NULL);
        failure |= sigaction(SIGTERM, &act, NULL);
        if (failure)
        {
            printf("ERROR: Failed to register signal handler.\n");
            return 1;
        }
    }


    // NOTE: Included udev rules create this symlink to the appropriate hidraw
    // entry.
    const char *hid_file_path         = "/dev/co2mini0";
    const char *temperature_file_path = "/tmp/co2minimon_temp";
    const char *co2_file_path         = "/tmp/co2minimon_co2";

    bool error_occurred = false;
    int device_handle = -1;
    bool need_send_feature_report = true;

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
                goto error;
            }

            need_send_feature_report = true;
        }

        if (need_send_feature_report) {
            uint8_t key[8] = {0};
            int result = ioctl(device_handle, HIDIOCSFEATURE(sizeof(key)), key);
            if (result < 0 || result != sizeof(key))
            {
                printf("ERROR: Failed to send feature report.\n");
                goto error;
            }
            need_send_feature_report = false;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(device_handle, &read_fds);
        errno = 0;
        int ready = select(device_handle + 1, &read_fds, NULL, NULL, &(struct timeval){ .tv_sec = 15 });
        if (ready == -1 && errno != EINTR)
        {
            printf("ERROR: Failed to select() device handle.\n");
            goto error;
        }
        else if (ready == 0)
        {
            // select() timed out
            //
            // If we go too long with an empty pipe, the device may have
            // stopped sending data. One situation where I've observed this can
            // happen is when the system resumes from hibernation. In this
            // case, all calls to read() will block until we resend the feature
            // report.
            //
            // I'm not sure why this happens, but my guess would be that the
            // device stops sending new data if the data hasn't been read in a
            // while.
            need_send_feature_report = true;
        }
        else
        {
            uint8_t data[8] = {0};
            int bytes_read = read(device_handle, &data, sizeof(data));

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
                need_send_feature_report = false;
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
                            printf("ERROR: Failed to open output file for temperature.\n");
                            goto error;
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
                            printf("ERROR: Failed to open output file for CO2.\n");
                            goto error;
                        }
                        write(f, buf, str_len);
                        close(f);
                    } break;
                }
            }
        }
    }

    goto cleanup;

error:
    error_occurred = true;
cleanup:
    unlink(co2_file_path);
    unlink(temperature_file_path);
    close(device_handle);

    return error_occurred;
}
