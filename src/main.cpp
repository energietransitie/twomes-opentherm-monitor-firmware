#include <string>
#include <stdlib.h>
#include "opentherm.h"
#include "driver/timer.h"

#define BOOT_STARTUP_INTERVAL_MS (10 * 1000) // milliseconds ( 10 s * 1000 ms/s)
#define BOOT_STARTUP_INTERVAL_TXT "Wating 10 seconds before next measurement data series is started"

#define DEVICE_TYPE_NAME "OpenTherm-Monitor"
static const char *TAG = "Twomes ESP32 OpenTherm Monitor main";

char variable_upload_payload_template[] = "{\"upload_time\": \"%d\",\"property_measurements\":[ "
    "%s"
    "]}";

static const char supply_temp_property_name[]       = "boilerSupplyTemp";
static const char return_temp_property_name[]       = "boilerReturnTemp";
static const char ch_mode_property_name[]           = "isCentralHeatingModeOn";
static const char dhw_mode_property_name[]          = "isDomesticHotWaterModeOn";
static const char flame_property_name[]             = "isBoilerFlameOn";
static const char room_set_point_property_name[]    = "roomSetpointTemp";
static const char max_rel_mod_lvl_property_name[]   = "maxModulationLevel";
static const char min_mod_lvl_property_name[]       = "minModulationLevel";
static const char rel_mod_lvl_property_name[]       = "relativeModulationLevel";
static const char max_boiler_cap_property_name[]    = "maxBoilerCap";


const int mInPin = MASTER_IN_PIN_GPIO19;  
const int mOutPin = MASTER_OUT_PIN_GPIO26; //not conected

const int sInPin = SLAVE_IN_PIN_GPIO21;
const int sOutPin = SLAVE_OUT_PIN_GPIO23; //not connected

#define BOILER_MEASUREMENT_INTERVAL 10
#define ROOM_TEMP_MEASUREMENT_INTERVAL 300
#define REGULAR_MEASUREMENT_INTERVAL 30
#define OPENTHERM_UPLOAD_INTERVAL 900
#define FLOAT_STRING_LENGTH 7
#define BOILER_MEASUREMENT_MAX OPENTHERM_UPLOAD_INTERVAL / BOILER_MEASUREMENT_INTERVAL
int boiler_measurement_count = 0;

#define ROOM_TEMPERATURE_MEASUREMENT_MAX OPENTHERM_UPLOAD_INTERVAL / ROOM_TEMP_MEASUREMENT_INTERVAL
int room_temp_count = 0;

#define REGULAR_MEASUREMENT_MAX OPENTHERM_UPLOAD_INTERVAL / REGULAR_MEASUREMENT_INTERVAL
int regular_measurement_count = 0;

#define TIMER_DIVIDER 80

// in-memory buffers for interpreted values
int ch_mode_data[REGULAR_MEASUREMENT_MAX];                                 // Central heating mode (ON/OFF)
int dhw_mode_data[REGULAR_MEASUREMENT_MAX];                                // Domestic hot water mode (ON/OFF)
int flame_data[REGULAR_MEASUREMENT_MAX];                                   // Burner status (ON/OFF)
char roomSetpoint_data[REGULAR_MEASUREMENT_MAX][FLOAT_STRING_LENGTH];      // Room temperature setpoint
char roomTemp_data[ROOM_TEMPERATURE_MEASUREMENT_MAX][FLOAT_STRING_LENGTH]; // Current room temperature
char boilerTemp_data[BOILER_MEASUREMENT_MAX][FLOAT_STRING_LENGTH];         // Boiler water temperature
char returnTemp_data[BOILER_MEASUREMENT_MAX][FLOAT_STRING_LENGTH];         // Return water temperature
char maxRelModLvl_data[REGULAR_MEASUREMENT_MAX][FLOAT_STRING_LENGTH];      // Maximum relative modulation level
int minModLvl_data[REGULAR_MEASUREMENT_MAX];                               // Minimum modulation level
char relModLvl_data[REGULAR_MEASUREMENT_MAX][FLOAT_STRING_LENGTH];         // Relative modulation level
int maxBoilerCapStorage_data[REGULAR_MEASUREMENT_MAX];                     // Maximum boiler capacity

OpenTherm mOT(mInPin, mOutPin);
OpenTherm sOT(sInPin, sOutPin, true);

int ch_mode = 0;                // Central heating mode (ON/OFF)
int dhw_mode = 0;               // Domestic hot water mode (ON/OFF)
int flame = 0;                  // Burner status (ON/OFF)
char roomSetpoint[7];           // Room temperature setpoint
char roomTemp[7];               // Current room temperature
char boilerTemp[7];             // Boiler water temperature
char returnTemp[7];             // Return water temperature
char maxRelModLvl[7];           // Maximum relative modulation level
int minModLvl = 0;              // Minimum modulation level
char relModLvl[3];              // Relative modulation level
int maxBoilerCap = 0;           // Maximum boiler capacity

// const char *ch_mode_name = "chmode";
// const char *dhw_mode_name = "dhwmode";
// const char *flame_name = "flame";
// const char *room_setpoint_name = "roomSetpoint";
// const char *room_temp_name = "roomTemp";
// const char *boiler_temp_name = "boilerTemp";
// const char *return_temp_name = "returnTemp";
// const char *max_rel_mod_lvl_name = "maxRelModLvl";
// const char *min_mod_lvl_name = "minModLvl";
// const char *rel_mod_lvl_name = "relModLvl";
// const char *max_boiler_cap_name = "maxBoilerCap";

char hex[5];     // Max 4 hex chars (two bytes) + null-terminator
int pointer = 0; // Used to keep track of position of received character

char dataBufferSlave[10][10];
int bufferSlaveCount = 0;

char dataBufferMaster[10][10];
int bufferMasterCount = 0;

int regular_timer_count = 0;
int boiler_timer_count = 0;
int room_temp_timer_count = 0;
int upload_timer_count = 0;

int buffered_cycles_count = 0;

// char *storage_template = (char *)"OpenTherm-%d";

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} seconds_timer_info_t;

void ICACHE_RAM_ATTR mHandleInterrupt(void *) {
    mOT.handleInterrupt(NULL);
}

void ICACHE_RAM_ATTR sHandleInterrupt(void *) {
    sOT.handleInterrupt(NULL);
}

bool IRAM_ATTR timer_group_isr_callback(void *args) {
    BaseType_t high_task_awoken = pdFALSE;
    // example_timer_info_t *info = (example_timer_info_t *) args;

    // uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);

    /* Prepare basic event data that will be then sent back to task */
    // example_timer_event_t evt = {
    //     .info.timer_group = info->timer_group,
    //     .info.timer_idx = info->timer_idx,
    //     .info.auto_reload = info->auto_reload,
    //     .info.alarm_interval = info->alarm_interval,
    //     .timer_counter_value = timer_counter_value
    // };

    // if (!info->auto_reload) {
    //     timer_counter_value += info->alarm_interval * TIMER_SCALE;
    //     timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
    // }

    /* Now just send the event data back to the main program task */
    // xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);

    regular_timer_count++;
    boiler_timer_count++;
    room_temp_timer_count++;
    upload_timer_count++;
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

//int timer_interval in microseconds
void initialize_opentherm_timer(timer_group_t group, timer_idx_t timer, bool auto_reload, int timer_interval) {
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = (timer_autoreload_t)auto_reload,
        .divider = TIMER_DIVIDER }; // default clock source is APB

    timer_init(group, timer, &config);

    timer_set_alarm_value(group, timer, timer_interval);
    timer_enable_intr(group, timer);

    // example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    // timer_info->timer_group = group;
    // timer_info->timer_idx = timer;
    // timer_info->auto_reload = auto_reload;
    // timer_info->alarm_interval = timer_interval_sec;
    timer_isr_callback_add(group, timer, timer_group_isr_callback, NULL, 0);

    timer_start(group, timer);
}

void processRequest(unsigned long request, OpenThermResponseStatus status) {
    ESP_LOGD(TAG, "Request frame from thermostat: %lX", request); //TODO: Can ESP_LOGD  be used from an interrupt?
    char request_data[10];
    sprintf(request_data, "T%lX", request); 
    strcpy(dataBufferSlave[bufferSlaveCount++], request_data);
    unsigned long response = mOT.sendRequest(request);
    if (response) {
        ESP_LOGD(TAG, "Response frame from boiler: %lX", response); //TODO: Can ESP_LOGD  be used from an interrupt?
        char response_data[10];
        sprintf(response_data, "B%lX", response); 
        strcpy(dataBufferMaster[bufferMasterCount++], response_data);
    }
}

void processSendRequest(char data[9]) {
    ESP_LOGD(TAG, "Processing data: %c%c%c%c%c%c%c%c%c",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
    // http://otgw.tclcode.com/firmware.html#operation
    // T = received from thermostat
    // B = received from boiler
    // R = sent to boiler
    // A = sent to thermostat
    // E = error

    // Only process messages received from thermostat or boiler, since we're in listening mode (not sending anything)
    // WARNING: one possible issue is BV which stands for Bad Value and also starts with B.

    // https://www.domoticaforum.eu/uploaded/Ard%20M/Opentherm%20Protocol%20v2-2.pdf
    // Messages are received in 4 bytes, converted to hex for a total of 8 characters

    // Bit 5, 6 and 7 of the the first byte (counted from LSB) contains the Message Type:
    // Master-to-Slave Messages      Slave-to-Master Messages
    // Value Message type            Value Message type
    // 000   READ-DATA               100 READ-ACK
    // 001   WRITE-DATA              101 WRITE-ACK
    // 010   INVALID-DATA            110 DATA-INVALID
    // 011   -reserved-              111 UNKNOWN-DATAID

    // Since we're only interested in gathering actual values, we filter on READ-ACK and WRITE-ACK.
    strncpy(hex, &data[1], 2);           // Copy 2 chars, starting from 1st character
    hex[2] = '\0';                       // null-terminated string
    int msgType = strtol(hex, NULL, 16); // Convert hex to dec

    // Now we need to take bit 5, 6 and 7. One way of doing that, is bit-shifting to the right by 4 places
    // so that ?xxx???? becomes 0000?xxx where xxx are the bits we're after.
    msgType = msgType >> 4;

    // And then bitwise AND compare it with 7, which leaves us with the first 3 bits:
    // Take the shifted msgType value   0000?xxx
    // Take the number 7                00000111
    // Bitwise AND compare              -------- &
    // Results in                       00000xxx
    msgType = msgType & 7;

    // Another option would be to bitwise AND compare it with 01110000 (112) without shifting,
    // and check the result for 01000000 (64 for READ-ACK) or 01010000 (80 for WRITE-ACK)

    // if (msgType == 4 || msgType == 5)
    // { // Filter on READ-ACK (100 = 4) and WRITE-ACK (101 = 5)
    ESP_LOGD(TAG, "Processing READ-ACK or WRITE-ACK");
    // The second byte (char 3 and 4 if you don't take into account TBRAE) holds the Data-ID
    strncpy(hex, &data[3], 2);          // Copy 2 chars, starting from 3rd character
    hex[2] = '\0';                      // null-terminated string
    int dataId = strtol(hex, NULL, 16); // Convert hex to dec

    // Filter on specific Data ID's
    // 0 = Slave status (low byte)
    // 16 = Room Setpoint (f8.8)
    // 24 = Room Temperature (f8.8)
    // 25 = Boiler Water Temperature (f8.8)
    // 28 = Return Water Temperature (f8.8)
    if (dataId == 0) {
        ESP_LOGD(TAG, "Processing Slave Status");
        // bit : description            [clear/0, set/1]
        // 0   : fault indication       [no fault, fault]
        // 1   : CH mode                [CH not active, CH active]
        // 2   : DHW mode               [DHW not active, DHW active]
        // 3   : Flame status           [flame off, flame on]
        // 4   : Cooling status         [cooling mode not active, cooling mode active]
        // 5   : CH2 mode               [CH2 not active, CH2 active]
        // 6   : diagnostic indication  [no diagnostics, diagnostic event]
        // 7   : reserved

        strncpy(hex, &data[7], 2);           // Copy 2 characters, starting from 7th char of the message (effectively take the last 2 hex chars: the low byte)
        hex[2] = '\0';                       // null-terminated string
        int lowbyte = strtol(hex, NULL, 16); // Convert hex to dec

        // Bitwise compare with 00000010 (2) to get bit number 2. The result is either 2 or 0.
        ch_mode = ((lowbyte & 2) == 2) ? 1 : 0;

        // Bitwise compare with 00000100 (4) to get bit number 3. The result is either 4 or 0.
        dhw_mode = ((lowbyte & 4) == 4) ? 1 : 0;

        // Bitwise compare with 00001000 (8) to get bit number 4. The result is either 8 or 0.
        flame = ((lowbyte & 8) == 8) ? 1 : 0;
        ESP_LOGD(TAG, "Processing ch_mode %d dhw_mode %d, flame %d", ch_mode, dhw_mode, flame);
    }
    else if (dataId == 14 || dataId == 16 || dataId == 17 || dataId == 24 || dataId == 25 || dataId == 28) {
        ESP_LOGD(TAG, "Processing regular data values");
        // These values are f8.8 signed fixed point value : 1 sign bit, 7 integer bit, 8 fractional bits
        // (two’s compliment ie. the LSB of the 16 bit binary number represents 1/256th of a unit).
        strncpy(hex, &data[5], 4); // Copy 4 characters, starting from 5th char of the message (or: take the last 4 chars of the message)

        hex[4] = '\0';                    // null-terminated string
        long lng = strtol(hex, NULL, 16); // Convert hex to dec

        if ((lng & 32768) == 32768) { // if the first bit (the sign bit) is a 1, then this a negative value (again, same trick as above)
            lng = (65536 - lng) * -1;
        }

        float tmp = (float)lng / 256.0; // As the last 8 bits are fractional, divide by 2^8 = 256

        // Format the floating point value to a string with 6 characters using 2 decimals
        switch (dataId) {
            case 14:
                sprintf(maxRelModLvl, "%.0f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing maxRelModLvl %s", maxRelModLvl);
                break;
            case 16:
                sprintf(roomSetpoint, "%.2f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing roomSetpoint %s", roomSetpoint);
                break;
            case 17:
                sprintf(relModLvl, "%.0f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing relModLvl %s", relModLvl);
                break;
            case 24:
                sprintf(roomTemp, "%.2f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing roomTemp %s", roomTemp);
                break;
            case 25:
                sprintf(boilerTemp, "%.2f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing boilerTemp %s", boilerTemp);
                break;
            case 28:
                sprintf(returnTemp, "%.2f", tmp); // TODO: relax the 6 character demand; make this dynamic?
                ESP_LOGD(TAG, "Processing returnTemp %s", returnTemp);
                break;
            default:
                break;
        }
    }
    else if (dataId == 15) {
        // HB : max. boiler capacity       u8     0..255kW
        // LB : min. modulation level      u8     0..100% expressed as a percentage of the maximum capacity

        strncpy(hex, &data[5], 2);            // Copy 2 characters, starting from 5th char of the message (high byte)
        hex[2] = '\0';                        // null-terminated string
        maxBoilerCap = strtol(hex, NULL, 16); // Straight forward unsigned 8 bit integer. Simply convert from hex to dec.

        strncpy(hex, &data[7], 2);         // Copy 2 characters, starting from 7th char of the message (low byte)
        hex[2] = '\0';                     // null-terminated string
        minModLvl = strtol(hex, NULL, 16); // Same here, 8 bit signed integer, so just convert from hex to dec.
        ESP_LOGD(TAG, "Processing maxBoilerCap %d and minModLvl %d", maxBoilerCap, minModLvl);
    }
    // }
}

// Work In-Progress
// void create_variable_interval_msg()
// {
//     int ch_mode;                // Central heating mode (ON/OFF)
//     int dhw_mode;               // Domestic hot water mode (ON/OFF)
//     int flame;                  // Burner status (ON/OFF)
//     char roomSetpoint[]; // Room temperature setpoint
//     char roomTemp[];     // Current room temperature
//     char boilerTemp[];   // Boiler water temperature
//     char returnTemp[];   // Return water temperature
//     char maxRelModLvl[buffered_cycles_count][7]; // Maximum relative modulation level
//     int minModLvl[buffered_cycles_count];              // Minimum modulation level
//     char relModLvl[buffered_cycles_count][7];    // Relative modulation level
//     int maxBoilerCap[buffered_cycles_count];           // Maximum boiler capacity
//     //Add main JSON template
//     char *msg_template = "{\"upload_time\": \"%d\",\"property_measurements\":["
//                          "%s"
//                          "]}]}";
//     char *msg_measurement_property_template = "{\"property_name\": %s,"
//                                               "\"measurements\": ["
//                                               "{ \"timestamp\":\"%d\","
//                                               "\"value\":\"%d\"}";
//     for (int i = 0; i < buffered_cycles_count; i++)
//     {
//     }
// }

//Function to initialise the buttons and LEDs, with interrupts on the buttons
void intialize_button_and_led_gpio() {
    ESP_LOGD(TAG, "starting intialize_button_and_led_gpio();");

    gpio_config_t io_conf;
    //CONFIGURE OUTPUTS:
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BUTTON_AND_LED_OUTPUT_BITMASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    //CONFIGURE INPUTS:
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BUTTON_AND_LED_INPUT_BITMASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGD(TAG, "exiting intialize_button_and_led_gpio()");

}

void begin_opentherm() {

    ESP_LOGD(TAG, "calling start_opentherm_interrupts_handling()");
    start_opentherm_interrupt_handling();

    ESP_LOGD(TAG, "Wating 5 seconds"); 
    vTaskDelay( (5 * 1000)  / portTICK_PERIOD_MS); // debug purposes

    ESP_LOGD(TAG, "calling mOT.begin(mHandleInterrupt, processMasterRequest)");
    mOT.begin(mHandleInterrupt);
    ESP_LOGD(TAG, "sOT.begin(sHandleInterrupt, processSlaveRequest);");
    sOT.begin(sHandleInterrupt, processRequest);

    ESP_LOGD(TAG, "calling initialize_opentherm_timer()");
    initialize_opentherm_timer(TIMER_GROUP_0, TIMER_0, true, 1000000);

    ESP_LOGD(TAG, "Wating 5 seconds"); 
    vTaskDelay( (5 * 1000)  / portTICK_PERIOD_MS); // debug purposes

    ESP_LOGD(TAG, "starting main OpenTherm processing loop");

    
}


// Work In-Progress
// esp_err_t store_char_persistent(const char *storage_loc, const char *name, char *data) {
//     esp_err_t err;
//     nvs_handle_t char_handle;
//     err = nvs_open(storage_loc, NVS_READWRITE, &char_handle);
//     if (err) {
//         ESP_LOGE(TAG, "Failed to open NVS twomes_storage: %s", esp_err_to_name(err));
//     }
//     else {
//         ESP_LOGE(TAG, "Succesfully opened NVS twomes_storage!");
//         err = nvs_set_str(char_handle, name, data);
//         switch (err) {
//             case ESP_OK:
//                 ESP_LOGD(TAG, "The char was written!\n");
//                 ESP_LOGD(TAG, "The char is: %s\n", data);
//                 break;
//             default:
//                 ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
//         }
//         err = nvs_commit(char_handle);
//         nvs_close(char_handle);
//     }
//     return err;
// }

// Work In-Progress
// esp_err_t store_int_persistent(const char *storage_loc, const char *name, uint16_t data) {
//     esp_err_t err;
//     nvs_handle_t int_handle;
//     err = nvs_open(storage_loc, NVS_READWRITE, &int_handle);
//     if (err) {
//         ESP_LOGE(TAG, "Failed to open NVS twomes_storage: %s", esp_err_to_name(err));
//     }
//     else {
//         ESP_LOGE(TAG, "Succesfully opened NVS twomes_storage!");
//         err = nvs_set_i16(int_handle, name, data);
//         switch (err) {
//             case ESP_OK:
//                 ESP_LOGD(TAG, "The int was written!\n");
//                 ESP_LOGD(TAG, "The int is: %d\n", data);
//                 break;
//             default:
//                 ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
//         }
//         err = nvs_commit(int_handle);
//         nvs_close(int_handle);
//     }
//     return err;
// }

// // Work In-Progress
// uint16_t retrieve_int_persistent(const char *storage_loc, const char *name) {
//     esp_err_t err;
//     uint16_t value = 0;
//     nvs_handle_t int_handle;
//     err = nvs_open("twomes_storage", NVS_READONLY, &int_handle);
//     if (err) {
//         ESP_LOGE(TAG, "Failed to open NVS twomes_storage: %s", esp_err_to_name(err));
//     }
//     else {
//         ESP_LOGE(TAG, "Succesfully opened NVS twomes_storage!");
//         err = nvs_get_u16(int_handle, name, &value);
//         switch (err) {
//             case ESP_OK:
//                 ESP_LOGD(TAG, "The int was read!\n");
//                 ESP_LOGD(TAG, "The int  is: %d\n", value);
//                 break;
//             case ESP_ERR_NVS_NOT_FOUND:
//                 ESP_LOGE(TAG, "The int was not initialized yet!");
//                 break;
//             default:
//                 ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
//         }
//         nvs_close(int_handle);
//     }
//     return value;
// }

// Work In-Progress
// char *retrieve_char_persistent(const char *storage_loc, const char *name) {
//     esp_err_t err;
//     char *value = NULL;
//     nvs_handle_t char_handle;
//     err = nvs_open("twomes_storage", NVS_READONLY, &char_handle);
//     if (err) {
//         ESP_LOGE(TAG, "Failed to open NVS twomes_storage: %s", esp_err_to_name(err));
//     }
//     else {
//         ESP_LOGE(TAG, "Succesfully opened NVS twomes_storage!");
//         size_t value_size;
//         nvs_get_str(char_handle, name, NULL, &value_size);
//         value = (char *)malloc(value_size);
//         err = nvs_get_str(char_handle, name, value, &value_size);
//         switch (err) {
//             case ESP_OK:
//                 ESP_LOGD(TAG, "The char was read!\n");
//                 ESP_LOGD(TAG, "The char is: %s\n", value);
//                 break;
//             case ESP_ERR_NVS_NOT_FOUND:
//                 ESP_LOGE(TAG, "The char was not initialized yet!");
//                 value = "";
//                 break;
//             default:
//                 value = NULL;
//                 ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
//         }
//         nvs_close(char_handle);
//     }
//     return value;
// }

//WIP Will be radically different later
void upload_data() {
    std::string room_temp_str("");
    std::string return_temp_str("");
    std::string boiler_temp_str("");
    std::string ch_mode_str("");
    std::string dhw_mode_str("");
    std::string flame_str("");
    std::string room_set_point_str("");
    std::string max_rel_mod_lvl_str("");
    std::string min_mod_lvl_str("");
    std::string rel_mod_lvl_str("");
    std::string max_boiler_cap_str("");
    for (int i = 0; i < regular_measurement_count; i++) {
        if (i) {
            ch_mode_str.append(",");
            dhw_mode_str.append(",");
            flame_str.append(",");
            room_set_point_str.append(",");
            max_rel_mod_lvl_str.append(",");
            min_mod_lvl_str.append(",");
            min_mod_lvl_str.append(",");
            max_boiler_cap_str.append(",");
        }
        ch_mode_str.append(std::to_string(ch_mode_data[i]));
        dhw_mode_str.append(std::to_string(dhw_mode_data[i]));
        flame_str.append(std::to_string(flame_data[i]));
        room_set_point_str.append(roomSetpoint_data[i]);
        max_rel_mod_lvl_str.append(maxRelModLvl_data[i]);
        min_mod_lvl_str.append(std::to_string(minModLvl_data[i]));
        min_mod_lvl_str.append(relModLvl_data[i]);
        max_boiler_cap_str.append(std::to_string(maxBoilerCapStorage_data[i]));
    }
    for (int i = 0; i < room_temp_count; i++) {
        if (i) {
            room_temp_str += std::string(", ") + std::string(roomTemp_data[i]);
        }
        else {
            room_temp_str += std::string(roomTemp_data[i]);
        }
    }
    for (int i = 0; i < boiler_measurement_count; i++) {
        if (i) {
            boiler_temp_str += std::string(", ") + std::string(boilerTemp_data[i]);
            return_temp_str += std::string(", ") + std::string(returnTemp_data[i]);
        }
        else {
            boiler_temp_str += boilerTemp_data[i];
            return_temp_str += returnTemp_data[i];
        }
    }
    ESP_LOGD(TAG, "Temp Data: %s\n", room_temp_str.c_str());
    ESP_LOGD(TAG, "Boiler Temp: %s\n", boiler_temp_str.c_str());
    ESP_LOGD(TAG, "Return Temp: %s\n", return_temp_str.c_str());
    ESP_LOGD(TAG, "CH_MODE: %s\n", ch_mode_str.c_str());
    ESP_LOGD(TAG, "DHW_MODE: %s\n", dhw_mode_str.c_str());
    ESP_LOGD(TAG, "flame: %s\n", flame_str.c_str());
    ESP_LOGD(TAG, "room_set_point: %s\n", room_set_point_str.c_str());
    ESP_LOGD(TAG, "max_rel: %s\n", max_rel_mod_lvl_str.c_str());
    ESP_LOGD(TAG, "min_mod: %s\n", min_mod_lvl_str.c_str());
    ESP_LOGD(TAG, "rel_mod: %s\n", rel_mod_lvl_str.c_str());
    ESP_LOGD(TAG, "boiler cap: %s\n", max_boiler_cap_str.c_str());
}

char *get_property_string_char(const char *prop_name, uint32_t time, char *value) {
    char property_string_char[] = "{\"property_name\": \"%s\","
        "\"measurements\": ["
        "{ \"timestamp\":\"%d\","
        "\"value\":\"%s\"}]}";

    int property_size = variable_sprintf_size(property_string_char, 3, prop_name, time, value);
    //Allocating enough memory so inputting the variables into the string doesn't overflow
    char *property_str = (char *)malloc(property_size);
    //Inputting variables into the plain json string from above(msgPlain).
    snprintf(property_str, property_size, property_string_char, prop_name, time, value);
    return property_str;
}

char *get_property_string_integer(const char *prop_name, uint32_t time, int value) {
    char property_string_integer[] = "{\"property_name\": \"%s\","
        "\"measurements\": ["
        "{ \"timestamp\":\"%d\","
        "\"value\":\"%d\"}]}";

    int property_size = variable_sprintf_size(property_string_integer, 3, prop_name, time, value);
    //Allocating enough memory so inputting the variables into the string doesn't overflow
    char *property_str = (char *)malloc(property_size);
    //Inputting variables into the plain json string from above(msgPlain).
    snprintf(property_str, property_size, property_string_integer, prop_name, time, value);
    return property_str;
}

char *get_boiler_measurements_data(uint32_t time) {
    //Get size of the message after inputting variables.
    std::string supply_temp_property(get_property_string_char(supply_temp_property_name, time, boilerTemp));
    std::string return_temp_property(get_property_string_char(return_temp_property_name, time, returnTemp));

    std::string combined_properties = supply_temp_property + ", " + return_temp_property;

    int msgSize = variable_sprintf_size(variable_upload_payload_template, 2, time, combined_properties.c_str());
    char *msg = (char *)malloc(msgSize);
    snprintf(msg, msgSize, variable_upload_payload_template, time, combined_properties.c_str());
    ESP_LOGD(TAG, "Data: %s", msg);

    // memcpy(&ch_mode_data[regular_measurement_count], &ch_mode, sizeof(ch_mode));                       // Central heating mode (ON/OFF)
    // memcpy(&dhw_mode_data[regular_measurement_count], &dhw_mode, sizeof(dhw_mode));                    // Domestic hot water mode (ON/OFF)
    // memcpy(&flame_data[regular_measurement_count], &flame, sizeof(flame));                             // Burner status (ON/OFF)
    // strcpy(roomSetpoint_data[regular_measurement_count], roomSetpoint);                                // Return water temperature
    // strcpy(maxRelModLvl_data[regular_measurement_count], maxRelModLvl);                                // Maximum relative modulation level
    // memcpy(&minModLvl_data[regular_measurement_count], &minModLvl, sizeof(minModLvl));                 // Minimum modulation level
    // strcpy(relModLvl_data[regular_measurement_count], relModLvl);                                      // Relative modulation level
    // memcpy(&maxBoilerCapStorage_data[regular_measurement_count], &maxBoilerCap, sizeof(maxBoilerCap));
    // free(boiler_temp_property);
    // free(return_temp_property);
    return msg;
}

char *get_regular_measurements_data(uint32_t time) {
    //Get size of the message after inputting variables.
    std::string ch_mode_property(get_property_string_integer(ch_mode_property_name, time, ch_mode));
    std::string dhw_mode_property(get_property_string_integer(dhw_mode_property_name, time, dhw_mode));
    std::string flame_property(get_property_string_integer(flame_property_name, time, flame));
    std::string room_set_point_property(get_property_string_char(room_set_point_property_name, time, roomSetpoint));
    std::string max_rel_mod_lvl_property(get_property_string_char(max_rel_mod_lvl_property_name, time, maxRelModLvl));
    std::string min_mod_lvl_property(get_property_string_integer(min_mod_lvl_property_name, time, minModLvl));
    std::string rel_mod_lvl_property(get_property_string_char(rel_mod_lvl_property_name, time, relModLvl));
    std::string max_boiler_cap_property(get_property_string_integer(max_rel_mod_lvl_property_name, time, maxBoilerCap));

    std::string combined_properties = ch_mode_property + ", " + dhw_mode_property + ", " + flame_property + ", " + room_set_point_property + ", " + max_rel_mod_lvl_property + ", " + min_mod_lvl_property + ", " + rel_mod_lvl_property + ", " + max_boiler_cap_property;

    int msgSize = variable_sprintf_size(variable_upload_payload_template, 2, time, combined_properties.c_str());
    char *msg = (char *)malloc(msgSize);
    snprintf(msg, msgSize, variable_upload_payload_template, time, combined_properties.c_str());
    ESP_LOGD(TAG, "Data: %s", msg);


    // memcpy(&ch_mode_data[regular_measurement_count], &ch_mode, sizeof(ch_mode));                       // Central heating mode (ON/OFF)
    // memcpy(&dhw_mode_data[regular_measurement_count], &dhw_mode, sizeof(dhw_mode));                    // Domestic hot water mode (ON/OFF)
    // memcpy(&flame_data[regular_measurement_count], &flame, sizeof(flame));                             // Burner status (ON/OFF)
    // strcpy(roomSetpoint_data[regular_measurement_count], roomSetpoint);                                // Return water temperature
    // strcpy(maxRelModLvl_data[regular_measurement_count], maxRelModLvl);                                // Maximum relative modulation level
    // memcpy(&minModLvl_data[regular_measurement_count], &minModLvl, sizeof(minModLvl));                 // Minimum modulation level
    // strcpy(relModLvl_data[regular_measurement_count], relModLvl);                                      // Relative modulation level
    // memcpy(&maxBoilerCapStorage_data[regular_measurement_count], &maxBoilerCap, sizeof(maxBoilerCap));
    return msg;
}

extern "C" {
    void app_main();
}

void app_main()
{
	gpio_install_isr_service(0);

    ESP_LOGD(TAG, "calling initialize_intialize_button_and_led_gpio_gpio()");
    intialize_button_and_led_gpio();

    ESP_LOGD(TAG, "calling intialize_opentherm_gpio()");
    intialize_opentherm_gpio();

    twomes_device_provisioning(DEVICE_TYPE_NAME);

    ESP_LOGD(TAG, "Starting heartbeat task");
    xTaskCreatePinnedToCore(&heartbeat_task, "heartbeat_task", 4096, NULL, 1, NULL, 1);

    ESP_LOGD(TAG, BOOT_STARTUP_INTERVAL_TXT);
    vTaskDelay(BOOT_STARTUP_INTERVAL_MS / portTICK_PERIOD_MS);
    
    ESP_LOGD(TAG, "Starting timesync task");
    xTaskCreatePinnedToCore(&timesync_task, "timesync_task", 4096, NULL, 1, NULL, 1);

    ESP_LOGD(TAG, BOOT_STARTUP_INTERVAL_TXT);
    vTaskDelay(BOOT_STARTUP_INTERVAL_MS / portTICK_PERIOD_MS);

    #ifdef CONFIG_TWOMES_PRESENCE_DETECTION
    ESP_LOGD(TAG, "Starting presence detection");
    start_presence_detection();
    #endif

    ESP_LOGD(TAG, BOOT_STARTUP_INTERVAL_TXT);
    vTaskDelay(BOOT_STARTUP_INTERVAL_MS / portTICK_PERIOD_MS);

    ESP_LOGD(TAG, "Generic Setup complete");

    ESP_LOGD(TAG, "calling begin_opentherm()");
    begin_opentherm();

    while (1) {
        sOT.process();
        //mOT.process();
        if (bufferSlaveCount != 0) {
            ESP_LOGD(TAG, "Slave Count: %d", bufferSlaveCount);
            processSendRequest(dataBufferSlave[--bufferSlaveCount]);
            // ESP_LOGD(TAG, "upload_timer_count: %d", upload_timer_count);
            // ESP_LOGD(TAG, "boiler_timer_count: %d", boiler_timer_count );
            // ESP_LOGD(TAG, "room_temp_timer_count: %d", room_temp_timer_count );
            // ESP_LOGD(TAG, "regular_timer_count: %d", regular_timer_count );
        }
        if (bufferMasterCount != 0) {
            ESP_LOGD(TAG, "Master Count: %d", bufferMasterCount);
            processSendRequest(dataBufferMaster[--bufferMasterCount]);
            // ESP_LOGD(TAG, "upload_timer_count: %d", upload_timer_count);
            // ESP_LOGD(TAG, "boiler_timer_count: %d", boiler_timer_count );
            // ESP_LOGD(TAG, "room_temp_timer_count: %d", room_temp_timer_count );
            // ESP_LOGD(TAG, "regular_timer_count: %d", regular_timer_count );
        }
        if (upload_timer_count >= OPENTHERM_UPLOAD_INTERVAL) {
            // ESP_LOGE(TAG, "Uploading all buffered data!");
            // regular_measurement_count = 0;
            // boiler_measurement_count = 0;
            // room_temp_count = 0;
            upload_timer_count = 0;
        }
        //boilerSupplyTemp & boilerReturnTemp timer
        if (boiler_timer_count >= BOILER_MEASUREMENT_INTERVAL) {
            ESP_LOGD(TAG, "Measuring and uploading boilerSupplyTemp and boilerReturnTemp");
            char *msg = get_boiler_measurements_data(time(NULL));
            upload_data_to_server(VARIABLE_UPLOAD_ENDPOINT, POST_WITH_BEARER, msg, NULL, 0);
            ESP_LOGD(TAG, "Free Heap before free(msg): %d", esp_get_free_heap_size());
            free(msg);
            ESP_LOGD(TAG, "Free Heap after free(msg): %d", esp_get_free_heap_size());
            // strcpy(boilerTemp_data[boiler_measurement_count], boilerTemp);
            // strcpy(returnTemp_data[boiler_measurement_count], returnTemp);
            // boiler_measurement_count++;
            boiler_timer_count = 0;
        }
        if (room_temp_timer_count >= ROOM_TEMP_MEASUREMENT_INTERVAL) {
            ESP_LOGD(TAG, "Measuring and uploading room temp");
            //Get size of the message after inputting variables.
            char *room_temp_property = get_property_string_char("roomTemp", time(NULL), roomTemp);
            int msgSize = variable_sprintf_size(variable_upload_payload_template, 2, time(NULL), room_temp_property);
            //Allocating enough memory so inputting the variables into the string doesn't overflow
            char *msg = (char *)malloc(msgSize);
            //Inputting variables into the plain json string from above(msgPlain).
            snprintf(msg, msgSize, variable_upload_payload_template, time(NULL), room_temp_property);
            //Posting data over HTTPS, using url, msg and bearer token.
            ESP_LOGD(TAG, "Data: %s", msg);
            upload_data_to_server(VARIABLE_UPLOAD_ENDPOINT, POST_WITH_BEARER, msg, NULL, 0);
            ESP_LOGD(TAG, "Free Heap before free(msg): %d", esp_get_free_heap_size());
            free(msg);
            ESP_LOGD(TAG, "Free Heap after free(msg): %d", esp_get_free_heap_size());
            room_temp_timer_count = 0;
        }
        if (regular_timer_count >= REGULAR_MEASUREMENT_INTERVAL) {
            ESP_LOGD(TAG, "Measuring and uploading regular measurements");
            char *msg = get_regular_measurements_data(time(NULL));
            upload_data_to_server(VARIABLE_UPLOAD_ENDPOINT, POST_WITH_BEARER, msg, NULL, 0);
            ESP_LOGD(TAG, "Free Heap before free(msg): %d", esp_get_free_heap_size());
            free(msg);
            ESP_LOGD(TAG, "Free Heap after free(msg): %d", esp_get_free_heap_size());
            // memcpy(&ch_mode_data[regular_measurement_count], &ch_mode, sizeof(ch_mode));                       // Central heating mode (ON/OFF)
            // memcpy(&dhw_mode_data[regular_measurement_count], &dhw_mode, sizeof(dhw_mode));                    // Domestic hot water mode (ON/OFF)
            // memcpy(&flame_data[regular_measurement_count], &flame, sizeof(flame));                             // Burner status (ON/OFF)
            // strcpy(roomSetpoint_data[regular_measurement_count], roomSetpoint);                                // Return water temperature
            // strcpy(maxRelModLvl_data[regular_measurement_count], maxRelModLvl);                                // Maximum relative modulation level
            // memcpy(&minModLvl_data[regular_measurement_count], &minModLvl, sizeof(minModLvl));                 // Minimum modulation level
            // strcpy(relModLvl_data[regular_measurement_count], relModLvl);                                      // Relative modulation level
            // memcpy(&maxBoilerCapStorage_data[regular_measurement_count], &maxBoilerCap, sizeof(maxBoilerCap)); // Maximum boiler capacity
            // regular_measurement_count++;
            regular_timer_count = 0;
        }
        // ESP_LOGD(TAG, "Upload Timer Count: %d", upload_timer_count);
        // ESP_LOGD(TAG, "Boiler Timer Count: %d", boiler_timer_count);
        // ESP_LOGD(TAG, "Room Temp Timer Count: %d", room_temp_timer_count);
        // ESP_LOGD(TAG, "Regular Timer Count: %d", regular_timer_count);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // ESP_LOGD(TAG, "I'm looping in the opentherm loop!");
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
