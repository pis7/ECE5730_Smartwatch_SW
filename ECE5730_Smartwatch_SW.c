/**
 * Parker Schless (pis7), Katarina Duric (kd374), George Maidhof (gpm58)
 *
 * Smartwatch Code
 *
 */

 #include "common.h"

 // Include custom libraries
 #include "pico/stdlib.h"
 #include "hardware/uart.h"
 #include "hardware/timer.h"
 // #include "pt_cornell_rp2040_v1_3.h"
 #include "font_ssh1106.h"
 #include "ssh1106.h"
 #include "i3g4250d.h"
 
 // Include custom images
 #include "dat/mic_img.h"
 #include "dat/heartrate_img.h"
 #include "dat/activity_img.h"
 #include "dat/weather_img.h"
 #include "dat/wifi_img.h"
 #include "dat/no_wifi_img.h"
 #include "dat/message_img.h"
 #include "dat/info_img.h"
 #include "dat/stopwatch_img.h"
 
 // Include WiFi library
 #include "wifi_udp.h"
 #include "hardware/rtc.h"
 
 // Include microphone library
 #include "pdm_microphone.h"
 
 // Heart rate library
 #include "heart_sensor_bpm.h"

 //SPO2 library
 #include "spo2_algorithm.h"
 
 // Some macros for max/min/abs
 #define min(a, b) ((a < b) ? a : b)
 #define max(a, b) ((a < b) ? b : a)
 #define abs(a) ((a > 0) ? a : -a)
 
 // GPIO Config
 #define CYCLE_BUTTON 14
 #define SELECT_BUTTON 15
 
 // ADC Config
 #define ADC_CHAN 0
 #define ADC_PIN 26
 
 // Battery values
 #define ADC_MAX 2800
 #define ADC_MIN 2050
 #define BATT_MIN 0
 #define BATT_MAX 100
 
 // Audio variables
 #define DELAY 125 // 1/8000 (in microseconds)
 #define DAC_config_chan_A 0b0001000000000000
 
 // Date and time variables
 extern datetime_t ntp_time;
 static bool ntp_time_initialized = false;
 
 // Text message variables
 #define BEACON_MSG_LEN_MAX 127
 static char last_message[BEACON_MSG_LEN_MAX] = "";
 char filtered_message[BEACON_MSG_LEN_MAX] = "\0";
 
 char* ip = NULL;
 char* mac = NULL;
 int port;
 int connect_status;
 
 typedef enum {
   MM_TIME,
   MM_HEART_RATE,
   MM_ACTIVITY,
   MM_STOPWATCH,
   MM_AUDIO,
   MM_TEXT,
   MM_INFO,
 } main_menu_state_t;
 main_menu_state_t main_menu_state = MM_TIME;
 
 typedef enum {
   DB_NOT_PRESSED,
   DB_MAYBE_PRESSED,
   DB_PRESSED,
   DB_MAYBE_NOT_PRESSED
 } debounce_state_t;
 debounce_state_t select_button_state = DB_NOT_PRESSED;
 debounce_state_t cycle_button_state = DB_NOT_PRESSED;
 int in_sub_menu = 0;
 
 // Mic config
 #define SAMPLE_RATE 8000
 #define SAMPLE_BUFFER_SIZE 8
 #define NUM_BURSTS 5 * (SAMPLE_RATE / SAMPLE_BUFFER_SIZE) // 5 second recording
 const struct pdm_microphone_config mic_config = {
     .gpio_data = 7,
     .gpio_clk = 6,
     .pio = pio0,
     .pio_sm = 0,
     .sample_rate = SAMPLE_RATE,
     .sample_buffer_size = SAMPLE_BUFFER_SIZE,
 };
 uint16_t mic_samp_buff[NUM_BURSTS][SAMPLE_BUFFER_SIZE];
 volatile int samp_idx = 0;
 volatile int samp_idx_inner = 0;
 
 int done_rec = 0;
 
 // Heart rate variables
 sense_struct sense;
 sense_struct sense_hr;
 #define RATE_SIZE 4
 uint8_t rates[RATE_SIZE];
 uint8_t rateSpot = 0;
 long lastBeat = 0;
 float beatsPerMinute;
 int beatAvg;
 int16_t IR_AC_Max = 20;
 int16_t IR_AC_Min = -20;
 int16_t IR_AC_Signal_Current = 0;
 int16_t IR_AC_Signal_Previous;
 int16_t IR_AC_Signal_min = 0;
 int16_t IR_AC_Signal_max = 0;
 int16_t IR_Average_Estimated;
 int16_t positiveEdge = 0;
 int16_t negativeEdge = 0;
 int32_t ir_avg_reg = 0;
 int16_t cbuf[32];
 uint8_t offset = 0;
 static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 
                                        2916, 3391, 3768, 4012, 4096};
 bool heartrate_screen = true;
                            
//SPO2 variables
#define HR_BUFFER_LENGTH 100
int32_t pun_ir_buffer[HR_BUFFER_LENGTH];
int32_t pun_red_buffer[HR_BUFFER_LENGTH];
int32_t pn_spo2 = 0;
int32_t pn_spo2_prev = 0;
int8_t pch_spo2_valid = 0;
int32_t pn_heart_rate = 0;
int8_t pch_hr_valid = 0;
static int spo2_index = 0;
uint32_t red;
long spo2_time_offset = 0;
 
 // Stopwatch variables
 uint32_t sw_hours;
 uint32_t sw_mins;
 uint32_t sw_secs;
 uint32_t sw_decisecs;
 bool sw_en = false;
 absolute_time_t sw_start_time;
 
 // Screen string
 char screen_str[20];
 
 // Step tracking
 float prev_z;
 int step_count;
 
 void update_menu() {
   int sel_pressed = gpio_get(SELECT_BUTTON);
   switch (select_button_state) {
     case DB_NOT_PRESSED:
       if (sel_pressed)
         select_button_state = DB_MAYBE_PRESSED;
       else
         select_button_state = DB_NOT_PRESSED;
       break;
     case DB_MAYBE_PRESSED:
       if (sel_pressed)
       {
         if (main_menu_state == MM_ACTIVITY ||
             main_menu_state == MM_AUDIO ||
             main_menu_state == MM_TEXT ||
             main_menu_state == MM_INFO ||
             main_menu_state == MM_HEART_RATE ||
             main_menu_state == MM_STOPWATCH) {
           if (in_sub_menu == 0) SSH1106_Clear();
           if (main_menu_state == MM_AUDIO) done_rec = 0;
           else if (main_menu_state == MM_HEART_RATE && in_sub_menu == 0) {
            hr_on();
            sprintf(screen_str, "BPM: %d", pn_heart_rate);
            SSH1106_GotoXY(15, 15);
            SSH1106_Puts(screen_str, &Font_11x18, 1);
            sprintf(screen_str, "SPO2: %d", pn_spo2 < 0 ? 0 : pn_spo2);
            SSH1106_GotoXY(15, 35);
            SSH1106_Puts(screen_str, &Font_11x18, 1);
            SSH1106_UpdateScreen();
           } else if (main_menu_state == MM_HEART_RATE && in_sub_menu == 1) {
            heartrate_screen = !heartrate_screen;
           }
           else if (main_menu_state == MM_ACTIVITY && in_sub_menu == 1) step_count = 0;
           else if (main_menu_state == MM_STOPWATCH && in_sub_menu == 0) {
             sprintf(screen_str, "%01u:%02u:%02u.%d", sw_hours, sw_mins, sw_secs, sw_decisecs);
             SSH1106_GotoXY(20, 25);
             SSH1106_Puts(screen_str, &Font_11x18, 1);
             SSH1106_UpdateScreen();
             if (in_sub_menu == 1) {
               sw_start_time = get_absolute_time();
               sw_en = !sw_en;
             }
           }
           in_sub_menu = 1;
         }
         select_button_state = DB_PRESSED;
       }
       else select_button_state = DB_NOT_PRESSED;
       break;
     case DB_PRESSED:
       if (sel_pressed)
         select_button_state = DB_PRESSED;
       else
         select_button_state = DB_MAYBE_NOT_PRESSED;
       break;
     case DB_MAYBE_NOT_PRESSED:
       if (sel_pressed)
         select_button_state = DB_PRESSED;
       else
         select_button_state = DB_NOT_PRESSED;
       break;
     default:
       select_button_state = DB_NOT_PRESSED;
       break;
   }
 
   int cyc_pressed = gpio_get(CYCLE_BUTTON);
   switch (cycle_button_state) {
     case DB_NOT_PRESSED:
       if (cyc_pressed)
         cycle_button_state = DB_MAYBE_PRESSED;
       else
         cycle_button_state = DB_NOT_PRESSED;
       break;
     case DB_MAYBE_PRESSED:
       if (cyc_pressed) {
         if (in_sub_menu) {
           in_sub_menu = 0;
           if (main_menu_state == MM_HEART_RATE) {
            hr_off();
            heartrate_screen = true;
           }
           else if (main_menu_state == MM_STOPWATCH) {
             sw_en = false;
             sw_hours = 0;
             sw_mins = 0;
             sw_secs = 0;
             sw_decisecs = 0;
           }
           done_rec = 0;
         } else 
           main_menu_state = (main_menu_state == MM_INFO) ? MM_TIME : (main_menu_state + 1);
         SSH1106_Clear();
         cycle_button_state = DB_PRESSED;
       }
       else
         cycle_button_state = DB_NOT_PRESSED;
       break;
     case DB_PRESSED:
       if (cyc_pressed)
         cycle_button_state = DB_PRESSED;
       else
         cycle_button_state = DB_MAYBE_NOT_PRESSED;
       break;
     case DB_MAYBE_NOT_PRESSED:
       if (cyc_pressed)
         cycle_button_state = DB_PRESSED;
       else
         cycle_button_state = DB_NOT_PRESSED;
       break;
     default:
       cycle_button_state = DB_NOT_PRESSED;
       break;
   }
 }
 
 int map_batt(int input, int input_min, int input_max, int output_min, int output_max) {
   int output = (int)((float)(input - input_min) / (float)(input_max - input_min) * (float)(output_max - output_min) + (float)output_min);
   output = ((output + 5) / 10) * 10;
   return output;
 }
 
 void on_pdm_samples_ready() {
   pdm_microphone_read(mic_samp_buff[samp_idx++], SAMPLE_BUFFER_SIZE);
 }
 
 void core1_entry() {
 
   // Time variables
   absolute_time_t last_time_update = get_absolute_time();
   uint64_t uptime_ms = 0;
   uint32_t hours = 0;
   uint32_t minutes = 0;
   uint32_t seconds = 0;
   uint32_t month = 0;
   uint32_t day = 0;
   uint32_t prev_seconds = -1;
   char prev_time_str[20];
   char time_str[20];
   char date_str[20];
   sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
   sprintf(prev_time_str, "%s", time_str);
 
   // Audio variables
   uint16_t dac_value, dac_msg;
 
   // Screen status variables
   int screen_rd = 0;
   int screen_status = 1;
 
   while (true) {
     update_menu();
     update_step(&prev_z, &step_count);
     screen_rd = check_screen();
     if (screen_rd == 1)  screen_status = 1;
     if (screen_rd == -1) screen_status = 0;
     if (screen_status) {
       switch (main_menu_state) {
         case MM_TIME:
           if (!ntp_time_initialized && ntp_time_ready)
           {
             datetime_t now;
             get_current_ntp_time(&now);
             hours = now.hour;
             minutes = now.min;
             seconds = now.sec;
             month = now.month;
             day = now.day;
             prev_seconds = seconds;
             ntp_time_initialized = true;
             last_time_update = get_absolute_time();
             sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
           } else if (ntp_time_initialized) {
             if (absolute_time_diff_us(last_time_update, get_absolute_time()) >= 1000000) {
               sprintf(prev_time_str, "%s", time_str);
               last_time_update = get_absolute_time();
               seconds++;
               if (seconds >= 60) {
                 seconds = 0;
                 minutes++;
               }
               if (minutes >= 60) {
                 minutes = 0;
                 hours++;
               }
               if (hours >= 24){
                 hours = 0;
               }
               sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
             }
             sprintf(date_str, "%02u/%02u", month, day);
           } else {
             sprintf(prev_time_str, "%s", time_str);
             uptime_ms = time_us_64() / 1000;
             hours = (uptime_ms / (1000 * 60 * 60)) % 24;
             minutes = (uptime_ms / (1000 * 60)) % 60;
             seconds = (uptime_ms / 1000) % 60;
             sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
             sprintf(date_str, "%02u/%02u", month, day);
           }
       
           if (seconds != prev_seconds) {
             prev_seconds = seconds;
 
             SSH1106_GotoXY(25, 10);
             SSH1106_Puts(prev_time_str, &Font_11x18, 0);
             SSH1106_GotoXY(25, 10);
             SSH1106_Puts(time_str, &Font_11x18, 1);
 
             SSH1106_GotoXY(47, 42);
             SSH1106_Puts(date_str, &Font_7x10, 1);
 
             if (!connect_status) {
               for (int i = 0; i < sizeof(wifi_img) / sizeof(wifi_img[0]); i++)
                 SSH1106_DrawPixel(wifi_img[i][0], wifi_img[i][1], 1);
             } else {
               for (int i = 0; i < sizeof(no_wifi_img) / sizeof(no_wifi_img[0]); i++)
                 SSH1106_DrawPixel(no_wifi_img[i][0], no_wifi_img[i][1], 1);
             }
             sprintf(screen_str, "%02d%%", map_batt(adc_read(), ADC_MIN, ADC_MAX, BATT_MIN, BATT_MAX));
             SSH1106_GotoXY(92, 38);
             SSH1106_Puts(screen_str, &Font_11x18, 1);
             SSH1106_UpdateScreen();
           }
           break;
           case MM_HEART_RATE:
            if (in_sub_menu)
            {
              for (int i = 0; i < 128; i++)
                for (int j = 0; j < 64; j++)
                  SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
              if (heartrate_screen) {
                if (checkForBeat(getIR(&sense_hr), &IR_AC_Max, &IR_AC_Min,
                  &IR_AC_Signal_Current, &IR_AC_Signal_Previous,
                  &IR_AC_Signal_min, &IR_AC_Signal_max, &IR_Average_Estimated,
                  &positiveEdge, &negativeEdge, &ir_avg_reg,
                  cbuf, &offset, FIRCoeffs)) {
    
                  long delta = to_ms_since_boot(get_absolute_time()) - lastBeat;
                  lastBeat = to_ms_since_boot(get_absolute_time());
              
                  beatsPerMinute = 60 / (delta / 1000.0);
              
                  if (beatsPerMinute < 255 && beatsPerMinute > 20)
                  {
                    rates[rateSpot++] = (uint8_t)beatsPerMinute; //Store this reading in the array
                    rateSpot %= RATE_SIZE; //Wrap variable
              
                    //Take average of readings
                    beatAvg = 0;
                    for (uint8_t x = 0 ; x < RATE_SIZE ; x++)
                      beatAvg += rates[x];
                    beatAvg /= RATE_SIZE;
                  }
                }
                sprintf(screen_str, "BPM: %d", beatAvg);
                SSH1106_GotoXY(15, 25);
                SSH1106_Puts(screen_str, &Font_11x18, 1);
              } else {
                for (int i = HR_BUFFER_LENGTH/4; i < HR_BUFFER_LENGTH; i++) {
                  pun_ir_buffer[i - HR_BUFFER_LENGTH/4] = pun_ir_buffer[i];
                  pun_red_buffer[i - HR_BUFFER_LENGTH/4] = pun_red_buffer[i];
                }
                for (int i = 3*HR_BUFFER_LENGTH/4; i < HR_BUFFER_LENGTH; i++) {
                  while (!MAX3010x_available(&sense)) check_hr(&sense);
                  pun_red_buffer[i] = getRed(&sense);
                  pun_ir_buffer[i] = getIR(&sense);
                  MAX3010x_next_sample(&sense);
                  maxim_heart_rate_and_oxygen_saturation(
                    pun_ir_buffer, 
                    HR_BUFFER_LENGTH, 
                    pun_red_buffer, 
                    &pn_spo2, 
                    &pch_spo2_valid, 
                    &pn_heart_rate, 
                    &pch_hr_valid
                  );
                }
                if (!pch_spo2_valid) {
                  sprintf(screen_str, "SPO2: %d", pn_spo2_prev);
                } else {
                  sprintf(screen_str, "SPO2: %d", pn_spo2);
                  pn_spo2_prev = pn_spo2;
                }
                SSH1106_GotoXY(15, 25);
                SSH1106_Puts(screen_str, &Font_11x18, 1);
              }
            } else {
              for (int i = 0; i < sizeof(heartrate_img) / sizeof(heartrate_img[0]); i++)
                SSH1106_DrawPixel(heartrate_img[i][0], heartrate_img[i][1], 1);
              }
           SSH1106_UpdateScreen();
           break;
         case MM_ACTIVITY:
           if (in_sub_menu) {
             for (int i = 0; i < 128; i++)
               for (int j = 0; j < 64; j++)
                 SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
             sprintf(screen_str, "Steps: %d", step_count);
             SSH1106_GotoXY(10, 25);
             SSH1106_Puts(screen_str, &Font_11x18, 1);
           } else {
             for (int i = 0; i < sizeof(activity_img) / sizeof(activity_img[0]); i++)
               SSH1106_DrawPixel(activity_img[i][0], activity_img[i][1], 1);
           }
           SSH1106_UpdateScreen();
           break;
         case MM_STOPWATCH:
           if (in_sub_menu) {
            for (int i = 0; i < 128; i++)
              for (int j = 0; j < 64; j++)
                SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
             if (sw_en) {
               if (sw_decisecs > 9) {
                 sw_decisecs = 0;
                 sw_secs++;
               }
               if (sw_secs > 59) {
                 sw_secs = 0;
                 sw_mins++;
               }
               if (sw_mins > 59) {
                 sw_mins = 0;
                 sw_hours++;
               }
               if (sw_hours > 9) {
                 sw_hours = 0;
               }
               if (absolute_time_diff_us(sw_start_time, get_absolute_time()) >= 100000) {
                 sw_start_time = get_absolute_time();
                 sprintf(screen_str, "%01u:%02u:%02u.%d", sw_hours, sw_mins, sw_secs, sw_decisecs);
                 SSH1106_GotoXY(20, 25);
                 SSH1106_Puts(screen_str, &Font_11x18, 1);
                 SSH1106_UpdateScreen();
                 sw_decisecs++;
               }
             }
           } else {
             for (int i = 0; i < sizeof(stopwatch_img) / sizeof(stopwatch_img[0]); i++)
               SSH1106_DrawPixel(stopwatch_img[i][0], stopwatch_img[i][1], 1);
             SSH1106_UpdateScreen();
           }
           break;
         case MM_AUDIO:
           if (in_sub_menu) {
             if (!done_rec) {
               SSH1106_Clear();
               sprintf(screen_str, "Recording");
               SSH1106_GotoXY(20, 25);
               SSH1106_Puts(screen_str, &Font_11x18, 1);
               SSH1106_UpdateScreen();
               samp_idx = 0;
               pdm_microphone_start();
               while (samp_idx < NUM_BURSTS) tight_loop_contents();
               pdm_microphone_stop();
               SSH1106_Clear();
               sprintf(screen_str, "Playback");
               SSH1106_GotoXY(20, 25);
               SSH1106_Puts(screen_str, &Font_11x18, 1);
               SSH1106_UpdateScreen();
               samp_idx = 0;
               samp_idx_inner = 0;
               while (samp_idx < NUM_BURSTS) {
                 dac_value = (mic_samp_buff[samp_idx][samp_idx_inner++] + 32768) >> 4;
                 dac_msg = (DAC_config_chan_A | (dac_value & 0x0FFF));
                 spi_write16_blocking(SPI_PORT, &dac_msg, 1);
                 if (samp_idx_inner == SAMPLE_BUFFER_SIZE) {
                   samp_idx_inner = 0;
                   samp_idx++;
                 }
                 busy_wait_us(DELAY);
               }
               samp_idx = 0;
               samp_idx_inner = 0;
               done_rec = 1;
             }
           } else {
             for (int i = 0; i < sizeof(mic_img) / sizeof(mic_img[0]); i++)
               SSH1106_DrawPixel(mic_img[i][0], mic_img[i][1], 1);
           }
           SSH1106_UpdateScreen();
           break;
         case MM_TEXT:
           if (in_sub_menu) {
             int ypos = 10;
             for (int i = 0; i < 128; i++)
               for (int j = 0; j < 64; j++)
                 SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
             char line_buffer[17]; // Buffer to hold each line (16 chars + null terminator)
             int line_start = 0;
             int line_end = 0;
 
             while (filtered_message[line_end] != '\0') {
               // Look for the next newline or end of string
               while (filtered_message[line_end] != '\n' && filtered_message[line_end] != '\0') {
                 line_end++;
               }
 
               // Copy the line into the buffer
               int line_length = line_end - line_start;
               strncpy(line_buffer, &filtered_message[line_start], line_length);
               line_buffer[line_length] = '\0'; // Null-terminate the line
 
               // Display the line on the screen
               SSH1106_GotoXY(10, ypos);
               SSH1106_Puts(line_buffer, &Font_7x10, 1);
               ypos += 10; // Move to the next line position
 
               // If we hit a newline, skip it
               if (filtered_message[line_end] == '\n') {
                 line_end++;
               }
 
               // Update the start position for the next line
               line_start = line_end;
             }
           } else {
             for (int i = 0; i < sizeof(message_img) / sizeof(message_img[0]); i++)
               SSH1106_DrawPixel(message_img[i][0], message_img[i][1], 1);
           }
           SSH1106_UpdateScreen();
           break;
         case MM_INFO:
           if (in_sub_menu) {
             sprintf(screen_str, "IP: %s", ip);
             SSH1106_GotoXY(10, 10);
             SSH1106_Puts(screen_str, &Font_7x10, 1);
             sprintf(screen_str, "Port: %d, MAC: ", port);
             SSH1106_GotoXY(10, 25);
             SSH1106_Puts(screen_str, &Font_7x10, 1);
             sprintf(screen_str, "%s", mac);
             SSH1106_GotoXY(10, 40);
             SSH1106_Puts(screen_str, &Font_7x10, 1);
           } else {
             for (int i = 0; i < sizeof(info_img) / sizeof(info_img[0]); i++)
               SSH1106_DrawPixel(info_img[i][0], info_img[i][1], 1);
           }
           SSH1106_UpdateScreen();
           break;
       }
     } else {
       for (int i = 0; i < 128; i++)
         for (int j = 0; j < 64; j++)
           SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
       SSH1106_UpdateScreen(); 
     }
   }
 }
 
 int main()
 {
 
   // POR delay
   sleep_ms(10);
 
   // Initialize standard I/O
   stdio_init_all();
 
   // Initialize I2C
   i2c_init(I2C_CHAN, I2C_BAUD_RATE);
   gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
   gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
 
   // Initialize SPI
   // bitwise_spi0_init();
   spi_init(SPI_PORT, 20000000); // 20MHz
   spi_set_format(SPI_PORT, 16, 0, 0, 0);
   gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
   gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
   gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
 
   // Initialize GPIO
   gpio_init(CYCLE_BUTTON);
   gpio_set_dir(CYCLE_BUTTON, GPIO_IN);
   gpio_pull_down(CYCLE_BUTTON);
 
   gpio_init(SELECT_BUTTON);
   gpio_set_dir(SELECT_BUTTON, GPIO_IN);
   gpio_pull_down(SELECT_BUTTON);
 
   // Initialize ADC
   adc_gpio_init(ADC_PIN);
   adc_init();
   adc_select_input(ADC_CHAN);
 
   // Initialize OLED display
   SSH1106_Init();
 
   // Initialize I3G4250D
   init_i3g4250();
 
   // Initialize microphone
   pdm_microphone_init(&mic_config);
   pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
 
   // Heart rate sensor initialization
   MAX3010x_Init();
   hr_off();
 
   // Connect to WiFi
   SSH1106_GotoXY(20, 15);
   SSH1106_Puts("Powering", &Font_11x18, 1);
   SSH1106_GotoXY(20, 35);
   SSH1106_Puts("   Up   ", &Font_11x18, 1);
   SSH1106_UpdateScreen();
   connect_status = wifi_udp_init(&ip, &port, &mac);
   start_wifi_threads();
   SSH1106_Clear();
 
   if (!connect_status)
   {
     SSH1106_GotoXY(20, 15);
     SSH1106_Puts(" Connect ", &Font_11x18, 1);
     SSH1106_GotoXY(20, 35);
     SSH1106_Puts(" Success ", &Font_11x18, 1);
     SSH1106_UpdateScreen();
   }
   else
   {
     SSH1106_GotoXY(20, 15);
     SSH1106_Puts(" Connect ", &Font_11x18, 1);
     SSH1106_GotoXY(20, 35);
     SSH1106_Puts("   Fail  ", &Font_11x18, 1);
     SSH1106_UpdateScreen();
   }
   sleep_ms(2000);
   SSH1106_Clear();
 
   multicore_reset_core1();
   multicore_launch_core1(core1_entry);
 
   while(1) {
     const char *msg = get_latest_udp_message();
     if (strcmp(msg, last_message) != 0) {
       int j = 0;
       int ypos = 10;
       for (int i = 0; i < BEACON_MSG_LEN_MAX && msg[i] != '\0' && msg[i] != '\r' && msg[i] != '\n'; i++) {
         if (msg[i] >= 32 && msg[i] <= 126) {
           filtered_message[j++] = msg[i];
           if (j % 16 == 0) {
             filtered_message[j++] = '\n';
           }
         }
       }
       filtered_message[j] = '\0';
       strncpy(last_message, filtered_message, BEACON_MSG_LEN_MAX);
     }
   }
 }