#include "os.h"
#include "sensor.h"

struct sensor_data current_env;

static uint32_t rand_seed = 12345;
static uint32_t pseudo_rand() {
    rand_seed = (1103515245 * rand_seed + 12345) & 0x7fffffff;
    return rand_seed;
}

void sensor_init(void) {
    current_env.temperature = 25;
    current_env.humidity = 50;
    current_env.smoke_alarm = 0;
    uart_puts("HAL: Simulated Sensors Initialized.\n");
}

void sensor_read_all(void) {
    // 模拟温度在 20°C ~ 35°C 之间波动
    current_env.temperature = 20 + (pseudo_rand() % 16);
    
    // 模拟湿度在 40% ~ 80% 之间波动
    current_env.humidity = 40 + (pseudo_rand() % 41);
    
    // 模拟极小概率 (约 1/20) 触发烟雾报警
    if ((pseudo_rand() % 20) == 0) {
        current_env.smoke_alarm = 1;
    } else {
        current_env.smoke_alarm = 0;
    }
}