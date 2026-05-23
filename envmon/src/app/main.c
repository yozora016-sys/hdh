#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>     

#define DEV_BTNLED  "/dev/btnled"
#define DEV_OLED    "/dev/oled0"


typedef struct {
    int   system_on;
    float last_temp;
    float last_humi;
} shared_state_t;

static shared_state_t   g_state;
static pthread_mutex_t  state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   toggle_cond = PTHREAD_COND_INITIALIZER;

static volatile sig_atomic_t running = 1;


static int wakeup_pipe[2];


static long long read_sysfs_ll(const char *path, int *ok)
{
    char buf[32]; int fd, n; char *end;
    *ok = 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    long long val = strtoll(buf, &end, 10);
    if (end == buf) return 0;
    *ok = 1;
    return val;
}

static int find_iio_dht11(char *temp_path, char *humi_path)
{
    char name_path[128], name_buf[64]; int fd, n, i;
    for (i = 0; i < 10; i++) {
        snprintf(name_path, sizeof(name_path),
                 "/sys/bus/iio/devices/iio:device%d/name", i);
        fd = open(name_path, O_RDONLY);
        if (fd < 0) continue;
        n = read(fd, name_buf, sizeof(name_buf) - 1);
        close(fd);
        if (n <= 0) continue;
        name_buf[n] = '\0';
        if (strncmp(name_buf, "dht11", 5) == 0) {
            snprintf(temp_path, 128,
                     "/sys/bus/iio/devices/iio:device%d/in_temp_input", i);
            snprintf(humi_path, 128,
                     "/sys/bus/iio/devices/iio:device%d/in_humidityrelative_input", i);
            return i;
        }
    }
    return -1;
}

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;

    const char c = 'x';
    write(wakeup_pipe[1], &c, 1);
}


static void *thread_button(void *arg)
{
    int          btn_fd;
    char         buf[8];
    int          last_state = 0;
    struct pollfd fds[2];

    (void)arg;

    btn_fd = open(DEV_BTNLED, O_RDONLY);
    if (btn_fd < 0) {
        perror("[BUTTON] open btnled");
        running = 0;
        return NULL;
    }

    fds[0].fd     = btn_fd;
    fds[0].events = POLLIN;

    fds[1].fd     = wakeup_pipe[0];
    fds[1].events = POLLIN;

    printf("[BUTTON] thread started (TID=%lu)\n", pthread_self());

    while (running) {
        int ret = poll(fds, 2, -1);

        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (fds[1].revents & POLLIN) {
            printf("[BUTTON] wakeup pipe received → exiting\n");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int n = read(btn_fd, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            buf[n] = '\0';

            int state = atoi(buf);
            if (state == 1 && last_state == 0) {
                pthread_mutex_lock(&state_mutex);
                g_state.system_on = !g_state.system_on;
                printf("[BUTTON] pressed → System %s\n",
                       g_state.system_on ? "ON" : "OFF");
                pthread_cond_signal(&toggle_cond);
                pthread_mutex_unlock(&state_mutex);
            }
            last_state = state;
        }
    }

    close(btn_fd);
    printf("[BUTTON] thread exiting cleanly\n");
    return NULL;
}


static void *thread_sensor(void *arg)
{
    char  temp_path[128], humi_path[128];
    int   iio_num;

    (void)arg;

    printf("[SENSOR] thread started (TID=%lu)\n", pthread_self());

    iio_num = find_iio_dht11(temp_path, humi_path);
    if (iio_num < 0) {
        printf("[SENSOR] ERROR: DHT11 IIO not found\n");

    } else {
        printf("[SENSOR] DHT11 at iio:device%d\n", iio_num);
    }

    while (running) {
        pthread_mutex_lock(&state_mutex);
        int sys_on = g_state.system_on;
        pthread_mutex_unlock(&state_mutex);

        if (!sys_on || iio_num < 0) { 
            sleep(1); 
            continue; 
        }

        int temp_ok = 0, humi_ok = 0;
        long long temp_raw = read_sysfs_ll(temp_path, &temp_ok);
        long long humi_raw = 0;
        
        if (temp_ok) {
            humi_raw = read_sysfs_ll(humi_path, &humi_ok);
        }

        if (temp_ok && humi_ok) {
            float temp = temp_raw / 1000.0f;
            float humi = humi_raw / 1000.0f;
            
            if (temp >= -10 && temp <= 60 && humi >= 0 && humi <= 100) {
                pthread_mutex_lock(&state_mutex);
                g_state.last_temp = temp;
                g_state.last_humi = humi;
                pthread_mutex_unlock(&state_mutex);
                printf("[SENSOR] OK %.1f°C %.1f%%\n", temp, humi);
            }
        }

        sleep(2);
    }

    printf("[SENSOR] thread exiting cleanly\n");
    return NULL;
}

static void *thread_display(void *arg)
{
    int  oled_fd, btn_fd;
    char disp[128];
    int  last_led_state = -1;

    (void)arg;

    oled_fd = open(DEV_OLED,   O_WRONLY);
    btn_fd  = open(DEV_BTNLED, O_WRONLY);
    if (oled_fd < 0) { perror("[DISPLAY] open oled");   running = 0; return NULL; }
    if (btn_fd  < 0) { perror("[DISPLAY] open btnled"); running = 0; return NULL; }

    printf("[DISPLAY] thread started (TID=%lu)\n", pthread_self());
    
    write(oled_fd, "Init Sensor...\n", 15);

    while (running) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 500000000LL;
        if (timeout.tv_nsec >= 1000000000LL) {
            timeout.tv_sec++; timeout.tv_nsec -= 1000000000LL;
        }

        pthread_mutex_lock(&state_mutex);
        pthread_cond_timedwait(&toggle_cond, &state_mutex, &timeout);

        int   sys_on = g_state.system_on;
        float temp   = g_state.last_temp;
        float humi   = g_state.last_humi;
        pthread_mutex_unlock(&state_mutex);

        if (sys_on != last_led_state) {
            write(btn_fd, sys_on ? "1" : "0", 1);
            last_led_state = sys_on;
        }

        if (!sys_on) {
            write(oled_fd, " \n", 2); 
            continue;
        }

        snprintf(disp, sizeof(disp), "Temp: %.1f C\nHumi: %.0f %%\n", temp, humi);
        write(oled_fd, disp, strlen(disp));
    }

    write(btn_fd,  "0", 1);
    write(oled_fd, " \n", 2);
    close(oled_fd);
    close(btn_fd);
    printf("[DISPLAY] thread exiting cleanly\n");
    return NULL;
}


int main(void)
{
    pthread_t tid_btn, tid_sensor, tid_display;
    int ret;

    printf("=== ENV Monitor System (Minimalist UI) ===\n");

    if (pipe(wakeup_pipe) < 0) {
        perror("pipe");
        return 1;
    }

    g_state.system_on = 1;
    g_state.last_temp = 0.0f; 
    g_state.last_humi = 0.0f;  

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    ret = pthread_create(&tid_btn,     NULL, thread_button,  NULL);
    if (ret) { fprintf(stderr, "pthread_create button: %s\n",  strerror(ret)); return 1; }
    ret = pthread_create(&tid_sensor,  NULL, thread_sensor,  NULL);
    if (ret) { fprintf(stderr, "pthread_create sensor: %s\n",  strerror(ret)); return 1; }
    ret = pthread_create(&tid_display, NULL, thread_display, NULL);
    if (ret) { fprintf(stderr, "pthread_create display: %s\n", strerror(ret)); return 1; }

    printf("Threads: BUTTON=%lu SENSOR=%lu DISPLAY=%lu\n",
           tid_btn, tid_sensor, tid_display);

    while (running)
        pause();

    printf("\nShutting down...\n");
    
    pthread_cond_broadcast(&toggle_cond);

    pthread_join(tid_display, NULL);
    pthread_join(tid_sensor,  NULL);
    pthread_join(tid_btn,     NULL);

    close(wakeup_pipe[0]);
    close(wakeup_pipe[1]);
    pthread_mutex_destroy(&state_mutex);
    pthread_cond_destroy(&toggle_cond);

    printf("System stopped.\n");
    return 0;
}
