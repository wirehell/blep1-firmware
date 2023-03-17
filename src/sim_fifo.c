#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <stdio.h>
#include <math.h>

#if CONFIG_BLEP1_SIM_PIPE

#define STACKSIZE 1024
#define PRIORITY 7

extern struct k_pipe rx_pipe; 

LOG_MODULE_REGISTER(sim_fifo, LOG_LEVEL_DBG);

uint8_t buf[8192];

//	.value = sin( (double) t * M_PI / 10.0) * 1000 + 1500,

void sim_update(int t) {
    int pos = 0;
    pos += sprintf(&buf[pos], "/ASD5id123\r\n\r\n");
    pos += sprintf(&buf[pos], "0-0:1.0.0(230313204113W)\r\n"); // TODO 
    pos += sprintf(&buf[pos], "1-0:1.8.0(%012.3f*kWh)\r\n", 0.394 * t);
    pos += sprintf(&buf[pos], "1-0:2.8.0(%012.3f*kWh)\r\n", 0.128 * t);
    pos += sprintf(&buf[pos], "1-0:3.8.0(%012.3f*kWh)\r\n", 0.074 * t);
    pos += sprintf(&buf[pos], "1-0:4.8.0(%012.3f*kWh)\r\n", 0.018 * t);
    pos += sprintf(&buf[pos], "1-0:1.7.0(%08.3f*kW)\r\n",  5 + 5 * sin(M_PI * t / 10.0));
    pos += sprintf(&buf[pos], "1-0:2.7.0(%08.3f*kW)\r\n",  3 + 3 * cos(M_PI * t / 10.0));
    pos += sprintf(&buf[pos], "1-0:3.7.0(%08.3f*kvar)\r\n", 0.5 + 0.5 * sin(M_PI * t / 7.0));
    pos += sprintf(&buf[pos], "1-0:4.7.0(%08.3f*kvar)\r\n", 0.2 + 0.2 * cos(M_PI * t / 7.0));
    pos += sprintf(&buf[pos], "1-0:32.7.0(%05.1f*V)\r\n", 240 + sin(M_PI * t / 10) * 5);
    pos += sprintf(&buf[pos], "1-0:52.7.0(%05.1f*V)\r\n", 240 + sin(M_PI * t / 9) * 5);
    pos += sprintf(&buf[pos], "1-0:72.7.0(%05.1f*V)\r\n", 240 + sin(M_PI * t / 8) * 5);
    pos += sprintf(&buf[pos], "1-0:31.7.0(%05.1f*A)\r\n", 5 + sin(M_PI * t / 10) * 5);
    pos += sprintf(&buf[pos], "1-0:51.7.0(%05.1f*A)\r\n", 5 + sin(M_PI * t / 9) * 5);
    pos += sprintf(&buf[pos], "1-0:71.7.0(%05.1f*A)\r\n", 5 + sin(M_PI * t / 8) * 5);
    pos += sprintf(&buf[pos], "!");
    uint16_t checksum = crc16_reflect(0xa001, 0, buf, pos);
    pos += sprintf(&buf[pos], "%4X\r\n", checksum);
}

void sim_fifo(void *, void *, void *) {
    size_t bytes_written;
    int t = 0;
    while (true) {
        k_sleep(K_MSEC(3000));
        sim_update(t++);
        LOG_INF("Transmitting");
        int len = strlen(buf);
        int ret = k_pipe_put(&rx_pipe, buf, len, &bytes_written, len, K_NO_WAIT);
        if (ret < 0) {
            LOG_ERR("Failed to send simulated data to fifo: %d", ret);
        }
    }
}

K_THREAD_DEFINE(sim_fifo_thread, STACKSIZE,
                sim_fifo, NULL, NULL, NULL,
                PRIORITY, 0, 0);

    
#endif