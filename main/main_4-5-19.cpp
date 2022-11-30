#define VERSION "3-22-19"
#define NAME_DEFAULT "TempLogger"
#define MAX_SAMPLES 1000
#define NUM_CHANNELS 12
#define SLOPE_TIME_SEC_INIT 10

// If debugger connected then don't access MAX31856 thermocouple.
#define DEBUGGER 0

// Change this to force the configuration to be saved to flash.
#define CHECK_VALUE_INIT 18558
#define INVALID_TEMP -20000
#define ZOOM_MAX 10
#define CHART_TEMP_MIN_DIF 10
#define MAX_SUCCESSIVE_INVALID_READS 5

#define SAMPLE_PERIOD_TSEC 10
#define SAMPLE_PERIOD_TSEC_MAX 6000
#define WEB_REFRESH_PERIOD_TSEC 5
#define WEB_REFRESH_PERIOD_TSEC_MAX 50
#define WEB_REFRESH_PERIOD_TSEC_MIN 2
#define CHART_SAMPLES_INIT 100
#define CHART_SAMPLES_MIN 50
#define CHART_SAMPLES_MAX 1000
#define TEMP_MIN -50
#define TEMP_MAX 2000
#define HOSTNAME_MAXLEN 20

/*
 * Temperature Logger by Doug Sisco
 * 
 * The Temperature Logger records the temperature read from a Maxim 31856 Thermocouple IC and
 *		creates a line graph viewable as a web page.
 * 
 * NEXT
 *    Hardware:
 *       Share jtag with led, not TC.
 *    Startup: currently the web server is blocked until the date is set.
 *        The date should be set to 1970 (or an old date) and then updated when the time svr is contacted.
 *    All samples in flash, reboot continues where left off.
 *		Auto reboot if no connection.
 *		Need unit testing.
 *    Need reboot button.
 *    Check that all config settings are within range: I found that one of the axis settings was out of range but was
 *       saved as part of the config.
 *    Select temperatures to include in title (tab).
 *    Setting to read temperatures from another unit.
 *    Setting to enable/disable slope.
 *    Setting to select channel for title.
 *    Zoom should be function of browser, not the same on all browsers.
 *    Arrows in tab and in status pane to show temperature trends.
 *    In main.htm, update values that are invalid with blanks.
 *		WHAT IS CHARTSAMPLES??
 *    Add channel names.
 *		Setting to enable channel on chart.
 *		If ajax does not respond within x seconds, show warning
 *    If last N readings were in error then report.
 *		Signal strength, battery level.
 *		Bluetooth interface (for IP address, etc.)
 *		Label each channel
 *		Assign each channel to a slot.
 *		Assign each channel color.
 *		Warning email when channel temp reaches a high or low value.
 * 
 *		Bluetooth interface (for IP address, etc.)
 * 
 *		Press a push button to enable web server for 10 minutes (make this an option).
 * 
 * 
 *		Show statistics: number of data points, current value.
 * 
 *		Use localized graph js.
 * 
 *		Show battery level.
 * 
 *		Download values as csv file.
 * 
 * 
 *		Add multiple temperature inputs.
 * 
 *		
 * 
 * MAINTENANCE
 *    2-15-19 Added Sensor Type menu to settings panel.
 *			Added slope calculation.
 * 
 *    2-13-19 Added TC selector, removed thermocouple_task() and moved it to Thread_ReadTemperature().
 * 
 *    2.10 Fixed thermocouple sign problem in max31856.c/thermocouple_read_temperature().
 * 
 *    2.9 Added Config::CheckValues().
 *        Increased accuracy of 18b20 sensors in ds18b20_get_temp().
 * 
 *    2.8 Initial update does not print invalid values on left.
 *			 Bigger buttons.
 *        After MAX_SUCCESSIVE_INVALID_READS successive temp reads the value is recorded as INVALID_TEMP (instead of being ignored).
 *        Chart now reflects the same colors as the left panel. 
 *           In ProcessChartRequest() added 'null' entries for invalid values instead of simply skipping the values in the chart data.
 *    2.7 Changed X-axis date format.
 * 
 * 
 * DESIGN NOTES
 *	  Thermocouple pins: 21 (SCK), 22 (SD0), 27 (CS),  (SDI) 2
 *    Thermocouple MUX: 32 (s0), 33 (s1), 23 (s2)
 *    Relay Out: 17
 *    PB1, PB2: 39,36
 *    LED6: 0, 4, 16
 *    DS18B20 SENS: 25, 26, 5, 19, 18
 *    Programmer: D0, TXD, RXD
 *    JTAG: 12,13,14,15
 *    
 *    0 LED, 2 TC, 4 LED, 5 SENS, 12 JTAG, 13 JTAG, 14 JTAG, 15 JTAG, 16 LED, 17 relay, 18 SENS, 
 *       19 SENS, 21 TC, 22 TC, 23 TCM, 25 SENS, 26 SENS, 27 TC, 32 TCM, 33 TCM, 34 (unused), 35 (unused), 36 PB2, 39 PB1
 * 
 *    Channel 0
 *       CH0 can be either a thermocouple or DS1820B depending on the value of Config.ChannelsAr[0].SensorType.
 * 
 *		The MAX31856 driver code is from https://github.com/djkilgore/max31856.
 * 
 * Unusable: 6,7,8,9,10,11
 * Inputs-only: 34,35,36,39
 *   12 is also unusable as an input on startup. * 
 * 
 * TOOLCHAIN NOTES
 * 
 * To send messages to error.txt, use make flash monitor &> error.txt
 * 
 * BUGS
 *    3-28-19 Wlan suddently would not connect until I removed the 'lasertag' wless entry, then it connected imediately.
 */

// This is used for the ESPIDF log.
#define TAG "example"

// This is mail.dougsisco.com.
#define SVR_ADDR_SMTP "192.185.123.196"

// These are defined to make message.h compatible.
#define ESPIDF
#define FLUSH ;

// Thermocouple
// For pin contants see max31856.h.
#define TC_MUX_SEL0 32
#define TC_MUX_SEL1 33
#define TC_MUX_SEL2 23

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS))
#define DelayMS(ms) (vTaskDelay(ms/portTICK_RATE_MS))

#define BEEP_GPIO_NUM GPIO_NUM_17
#define LEDR_GPIO_NUM 0
#define LEDG_GPIO_NUM 4
#define LEDB_GPIO_NUM 16

#define LEDR_ON gpio_set_level((gpio_num_t)LEDR_GPIO_NUM, 0)
#define LEDR_OFF gpio_set_level((gpio_num_t)LEDR_GPIO_NUM, 1)
#define LEDG_ON gpio_set_level((gpio_num_t)LEDG_GPIO_NUM, 0)
#define LEDG_OFF gpio_set_level((gpio_num_t)LEDG_GPIO_NUM, 1)
#define LEDB_ON gpio_set_level((gpio_num_t)LEDB_GPIO_NUM, 0)
#define LEDB_OFF gpio_set_level((gpio_num_t)LEDB_GPIO_NUM, 1)

#include <string.h>
#include <string>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "apps/sntp/sntp.h"
#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/portmacro.h"
#include "tcpip_adapter.h"
//#include "cJSON.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
//#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "ds18b20.h"
#include "driver/uart.h"
#include "esp_deep_sleep.h"
#include "esp_task_wdt.h"
#include <driver/dac.h>
#include <driver/adc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "driver/ledc.h"
#include "message.h"
#include "crypto/base64.h" // For email
#include "sdkconfig.h"
#include "max31856.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "main.htm.h"
#include "settings.htm.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

using std::string;

#define HTML_HEADER_OK "HTTP/1.1 200 OK\n\n"

#define bool int8_t

using namespace std;

typedef struct {
	int8_t rssi;
	uint8_t channel;
} SWifiInfo;

// This is a sample of all the channel values at the given time.
typedef struct {
	time_t time;
	float val[NUM_CHANNELS];	
} S_DateVal;

// Y-Axis settings for the chart
class CChartYAxis {
public:
	int LowTemp;
	int HighTemp;
	int NumTicks;
	void Init() {
		LowTemp = 0;
		HighTemp = 100;
		NumTicks = 10;
	}
	
	int CheckValues() {
		if (LowTemp < TEMP_MIN || LowTemp > TEMP_MAX || HighTemp < TEMP_MIN || HighTemp > TEMP_MAX || NumTicks < 2 || NumTicks > 100)
			return 1;
		return 0;
	}
public:
	void print() {
		printf("ChartYAxis: LowTemp:%u HighTemp:%u NumTicks:%u\n",LowTemp,HighTemp,NumTicks);
	}
};

// Channel
class CChannel {
public:
	// Sensor type
	enum SensorTypeE {T18B20=0, TMAX31856=1} SensorType;
	// GPIO if this is a DS18B20.
	gpio_num_t DS_GPIO;
	// This is the MUX address if this is a TC.
	int MuxAddr; 	
	// Flag to indicate that this device has been initialized. Not used for TC's.
	int DeviceInitialized;
	// Channel Name
	char ChanName[50];

	// Last value read.
	float ValueCurrent;
	// Temperature unit
	enum {TUNIT_F=0, TUNIT_C} TempUnit;
	// Y-Axis for chart
	int Axis;
	// Number of successive invalid readings.
	int InvalidReads;
public:
	CChannel(int GPIO_MuxAddr, SensorTypeE sensortype) {
		TempUnit = TUNIT_F;
		SensorType = T18B20;
		DS_GPIO = (gpio_num_t)-1;
		ValueCurrent = INVALID_TEMP;
		Axis = 0;
		InvalidReads = 0;
	}
};

class CSelectVals {
public:
	int Value;
	string Description;
};

// Note: if you change CConfig you must change CheckValue.
class CConfig {
public:
	int SamplePeriodTSec;
	int WebRefreshPeriodTSec;
	// Number of samples to show in X axis.
	int ChartSamples;
	// This skips N samples when drawing the chart.
	int ChartZoom;
	// Scroll back this number of samples (cannot be negative).
	int XOffset;
	// Y-Axis chart settings
	CChartYAxis ChartYAxis[2];
	int CheckValue;
	// This is the index to the value that will be charted.
	int ChartValueSelect;
	char HostName[HOSTNAME_MAXLEN];
	// Sensor configuration.
	CChannel ChannelsAr[NUM_CHANNELS];
	// This enables saving the samples to the sample array.
	int EnableLogging;
	// Time over which the slope is calculated.
	int SlopeTimeSec;
	
	// Test all config values for ranges.
	bool CheckValues() {		
		if (this->SamplePeriodTSec < 1 || this->SamplePeriodTSec > SAMPLE_PERIOD_TSEC_MAX)
			return 1;
		
		if (this->WebRefreshPeriodTSec < WEB_REFRESH_PERIOD_TSEC_MIN || this->WebRefreshPeriodTSec > WEB_REFRESH_PERIOD_TSEC_MAX)
			return 1;
		
		if (this->ChartSamples < CHART_SAMPLES_MIN || this->ChartSamples > CHART_SAMPLES_MAX)
			return 1;
		
		if (this->ChartZoom < 1 || this->ChartZoom > ZOOM_MAX)
			return 1;
		
		if (this->XOffset < 0 || this->XOffset > CHART_SAMPLES_MAX)
			return 1;
		
		if (this->ChartYAxis[0].CheckValues() )
			return 1;
		if (this->ChartYAxis[1].CheckValues() )
			return 1;
		
		if (strlen(HostName) > HOSTNAME_MAXLEN)
			return 1;
		
		if (SlopeTimeSec < 2 || SlopeTimeSec > MAX_SAMPLES-1)
			return 1;
		return 0;
	}
	
public:
	esp_err_t Read();
	esp_err_t Write();
	
	void Init() {
		// Initialize the configuration.
		this->SamplePeriodTSec = SAMPLE_PERIOD_TSEC;
		this->WebRefreshPeriodTSec = WEB_REFRESH_PERIOD_TSEC;
		this->ChartSamples = CHART_SAMPLES_INIT;
		this->ChartZoom = 1;
		this->XOffset = 0;
		this->ChartValueSelect = 0;
		strncpy(HostName,NAME_DEFAULT,20);
		this->CheckValue = CHECK_VALUE_INIT;	
		this->ChartYAxis[0].Init();
		this->ChartYAxis[1].Init();
		this->EnableLogging = 1;
		this->SlopeTimeSec = SLOPE_TIME_SEC_INIT;
	}
};
	
// CONSTANTS
/*
			 [new Date(2015,01,01, 15,0,30), 65],
			 [new Date(2015,01,01, 15,0,31), 66],
			 [new Date(2015,01,01, 15,0,35), 67],
			 [new Date(2015,01,02, 15,0,37), 66],
			 [new Date(2015,01,02, 15,0,39), 68],
			 [new Date(2015,01,02, 15,0,40), 69]
 */
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

// ****************************************************************************************************
// GLOBALS
S_DateVal ValuesArr[MAX_SAMPLES];
time_t BootTime;
volatile int ValuesArrInd = 0;
long UpdateCount=0;
max31856_cfg_t max31856;
gpio_num_t DS_GPIO;
	

static EventGroupHandle_t wifi_event_group;
float temp1;
int Count = 0;
char strftime_buf[64];
//uint8_t *UARTbuf;
uart_config_t myUartConfig;
uint8_t *data;
CClientStats ClientStats;
CMsg *MsgPtr;
#define BUF_SIZE_SKT 300
#define BUF_SIZE (300)
int sock;
int sockSMTP;
int LEDCurrentColor = 0;
int WifiAuthIndex = 0;
int WifiStatus = 0;
int CurrentColor_g;
int ColorBeforeBlink;

int LEDServiceDelay = 0;
int LED_R_Count0 = 0;
int LED_G_Count0 = 0;
int LED_B_Count0 = 0;
int LEDRCount = 0;
int LEDGCount = 0;
int LEDBCount = 0;
int OnPeriod = 0;
int OffPeriod = 0;
int BlinkCount = 0;
int ThermocoupleCurrent = 0;

E_BLINKTYPE BlinkType = BLINK_OFF;
// This is the low level for dimming the LEDs. When set the leds will dim instead of blink.
int LEDDimLowLevel = 0;

char HostName[20];
long TimeSinceBootMS = 0;
long TimeSinceLastMsgMS = 0;
// Indicates that the leds should show power on.
bool PowerBlinkIndictor = 1;

// MAX31856 Configuration.
//CMAX31856 MAX31856;

// This holds the wifi authentication.
typedef struct {
	const char *SSID;
	const char *Pwd;
} s_WifiAuth;

// Configure possible wireless networks here.
#define WIFIAUTH_LAST_INDEX 0 /* Be sure to change this constant or we will crash. */
s_WifiAuth WifiAuth[] = {
//	{"lasertag", ""},
	{"full_quiver", "samrizzo"},
};

CConfig Config;

/* The event group allows multiple bits for each event,
	but we only care about one event - are we connected
	to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

void Thread_ReadTemperature();
static void obtain_time(void);
static void initialize_sntp(void);
static int initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);
void TimeSetup();
void getWifiInfo(SWifiInfo *info);
void NVS_Init();
esp_err_t Write(char *tag, char *value);
esp_err_t Read(char *tag, char *value, size_t len);
void Thread_CPUTimeUpdate();
void Thread_WebServer();
void Thread_BatteryTest();
void Thread_SaveValuesToArray();
static void http_server(void *pvParameters);
static void ProcessHttpRequest(struct netconn *conn);
void Replace(string& source, string const& find, string const& replace);
bool SendMailOK(string ToStr, string FromStr, string SubjectStr, string MsgStr, string UserNameStr, string PwdStr);
int SMTPWaitReply(int WaitTimeMS, string &s);
void ProcessMainPageRequest(struct netconn *conn);
string GetStatusTextStr();
void ProcessChartRequest(struct netconn *conn);
void ThermocoupleSelect(int n);
int GetSample(int offset,S_DateVal &Sample);
float GetSlope(int chan);
static void http_get_task(void *pvParameters);

extern "C" {
	void app_main(void);
	max31856_cfg_t max31856_init();
	void thermocouple_set_type(max31856_cfg_t *max31856, max31856_thermocoupletype_t tc_type);
	max31856_thermocoupletype_t thermocouple_get_type(max31856_cfg_t *max31856);
	uint8_t thermocouple_read_fault(max31856_cfg_t *max31856, bool log_fault);
	float thermocouple_read_coldjunction(max31856_cfg_t *max31856);
	float thermocouple_read_temperature(max31856_cfg_t *max31856);
	void thermocouple_set_temperature_fault(max31856_cfg_t *max31856, float temp_low, float temp_high);
}

static void obtain_time(void) {
	//ESP_ERROR_CHECK( nvs_flash_init() );
	//initialise_wifi();
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	initialize_sntp();

	// wait for time to be set
	time_t now = 0;
	struct tm timeinfo = {0,0,0,0,0,1970,0,0,0};
	int retry = 0;
	//const int retry_count = 10;
	// while (++retry < retry_count) { // Removed 3-14-19 dgs
		ESP_LOGI(TAG, "Setting system time..\n");
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	// }

	//ESP_ERROR_CHECK( esp_wifi_stop() );
}

static void initialize_sntp(void) {
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
		case SYSTEM_EVENT_STA_START:
			printf("event_handler() SYSTEM_EVENT_STA_START\n");
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			printf("event_handler() SYSTEM_EVENT_STA_GOT_IP\n");
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
			//        printf("netmask: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.netmask));
			//        printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
			//        printf("\n");
			fflush(stdout);
			WifiStatus = 1;
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			printf("event_handler() SYSTEM_EVENT_STA_DISCONNECTED\n");

			// If wifi was never connected..
			if (WifiStatus == 0) {
				//printf("event_handler() Trying the next wless network..\n");
				// Try the next wireless network.
				WifiAuthIndex++;
				if (WifiAuthIndex > WIFIAUTH_LAST_INDEX)
					WifiAuthIndex = 0;

				//printf("event_handler() WifiAuthIndex=%u\n",WifiAuthIndex);
				printf("event_handler() Wifi trying to connect to %s with pwd %s\n", WifiAuth[WifiAuthIndex].SSID, WifiAuth[WifiAuthIndex].Pwd);
				wifi_config_t sta_config;
				sta_config.sta.bssid_set = false;
				strcpy((char*) sta_config.sta.ssid, WifiAuth[WifiAuthIndex].SSID);
				strcpy((char*) sta_config.sta.password, WifiAuth[WifiAuthIndex].Pwd);

				//printf("event_handler() esp_wifi_set_config\n");
				esp_wifi_set_config(WIFI_IF_STA, &sta_config);
			}

			WifiStatus = 0;
			/* This is a workaround as ESP32 WiFi libs don't currently
				auto-reassociate. */
			printf("event_handler() esp_wifi_connect\n");
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
			break;
		default:
			break;
	}
	return ESP_OK;
}

static int initialise_wifi(void) {
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	wifi_config_t sta_config;
	sta_config.sta.bssid_set = false;
	strcpy((char*) sta_config.sta.ssid, WifiAuth[0].SSID);
	strcpy((char*) sta_config.sta.password, WifiAuth[0].Pwd);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

	printf("esp_wifi_start()\n");
	int r = esp_wifi_start();
	printf("esp_wifi_start() done, r=%u\n", r);
	return r;
}

void TimeSetup() {
	time_t now;
	struct tm timeinfo;

	time(&now);
	localtime_r(&now, &timeinfo);
	// Is time set?
	if (timeinfo.tm_year < (2016 - 1900)) {
		ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
		obtain_time();
		// update 'now' variable with current time
		time(&now);
	}

	// Set timezone to Eastern Standard Time and print local time
	setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
	tzset();
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof (strftime_buf), "%c", &timeinfo);
	printf ("The current time is: %s\n", strftime_buf);
}

//static void UART_Init() {
//	printf("UART_Init()\n");
//
//	/* Configure parameters of an UART driver,
//	 * communication pins and install the driver */
//	uart_config_t uart_config = {
//		.baud_rate = 1200,
//		.data_bits = UART_DATA_8_BITS,
//		.parity = UART_PARITY_DISABLE,
//		.stop_bits = UART_STOP_BITS_1,
//		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//	};
//	uart_param_config(UART_NUM_1, &uart_config);
//	// RXD pin is GPIO 5
//	uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, GPIO_NUM_5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//	uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
//
//	// Configure a temporary buffer for the incoming data
//	data = (uint8_t *) malloc(BUF_SIZE);
//}

/**
 * Get the WIFI channel and signal strength.
 * 
 * @param info
 */
void getWifiInfo(SWifiInfo *info) {
	info->rssi = 0;
	info->channel = 0;
	wifi_ap_record_t wifidata;
	esp_wifi_sta_get_ap_info(&wifidata);
	if (wifidata.primary != 0) {
		info->rssi = wifidata.rssi;
		info->channel = wifidata.primary;
	}
}

void SetGPOutput(int n) {
	gpio_pad_select_gpio(n);
	gpio_set_direction((gpio_num_t) n, GPIO_MODE_OUTPUT);
}

void Thread_ReadTemperature(void *pvParameter) {
	// Initialize the thermocouple queue.
	QueueHandle_t thermocouple_queue = xQueueCreate(5, sizeof (struct max31856_cfg_t));

	if (thermocouple_queue == NULL) {
		ESP_LOGI("queue", "Not Ready: thermocouple_queue");
		return;
	}
	
	// Init the MAX31856.
#if !DEBUGGER
	max31856_cfg_t max31856 = max31856_init();
	thermocouple_set_type(&max31856, MAX31856_TCTYPE_K);
#endif
	
	while (1) {
		// Now get the other sensor values.
		for (int i=0; i<NUM_CHANNELS; i++) {
			// If this is a thermocouple..
			if (Config.ChannelsAr[i].SensorType == CChannel::SensorTypeE::TMAX31856) {
				// Read the TC temperature.
#if !DEBUGGER				
				thermocouple_read_temperature(&max31856);
#endif				
				// Select the next TC.
				ThermocoupleSelect(i);
				// Delay while the next thermocouple is selected.
				vTaskDelay(200 / portTICK_PERIOD_MS);

				if (max31856.thermocouple_f > 10000)
					Config.ChannelsAr[i].ValueCurrent = INVALID_TEMP;
				else {
					Config.ChannelsAr[i].ValueCurrent = max31856.thermocouple_f;
					printf("Ch%u is TC: %0.2f\n", i, max31856.thermocouple_f);
				}
				
			} else {
				// If this is a DS1820B..
				float Temp = Config.ChannelsAr[i].ds18b20_get_temp();

				// If the temp is valid then record it.
				if (Temp > -50) {
					printf("Ch%u is DS1820B: %0.2f\n",i,Temp);
					Config.ChannelsAr[i].InvalidReads = 0;
					Config.ChannelsAr[i].ValueCurrent = Temp;
				} else {
					// If the value is invalid then inc the InvalidReads count.
//					printf("*****Config.ChannelsAr[%u] INVALID: count=%u *****\n",i,Config.ChannelsAr[i].InvalidReads);		
					if (Config.ChannelsAr[i].InvalidReads < MAX_SUCCESSIVE_INVALID_READS)
						Config.ChannelsAr[i].InvalidReads++;
					else
						// There have been too many successive invalid values so set the current value to INVALID_TEMP.
						Config.ChannelsAr[i].ValueCurrent = INVALID_TEMP;
				}
			}
		}
					  
		// Pause 1/2 second.
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void Thread_SaveValuesToArray() {
	time_t now;
	struct tm timeinfo;	
	
	while (1) {
		// update 'now' variable with current time
		time(&now);
		localtime_r(&now, &timeinfo);

		if (Config.EnableLogging) {
			printf("Thread_SaveValuesToArray: %u:%u:%u t0=%.1f, t1=%.1f, t2=%.1f, t3=%.1f, t4=%.1f ValuesArrInd=%u\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
					  Config.ChannelsAr[0].ValueCurrent,Config.ChannelsAr[1].ValueCurrent,Config.ChannelsAr[2].ValueCurrent,Config.ChannelsAr[3].ValueCurrent,Config.ChannelsAr[4].ValueCurrent, ValuesArrInd);

				// Add the new value to the array.
			for (int i=0; i<NUM_CHANNELS; i++)
				ValuesArr[ValuesArrInd].val[i] = Config.ChannelsAr[i].ValueCurrent;

			ValuesArr[ValuesArrInd].time = now;

			ValuesArrInd++;
			if (ValuesArrInd >= MAX_SAMPLES)
				ValuesArrInd = 0;	
		} else 
			printf("Logging disabled\n");
		
		// $$$ Use vTaskDelayUntil instead.
		vTaskDelay(pdMS_TO_TICKS(Config.SamplePeriodTSec * 100));	
	}
}

void NVS_Init() {
	// Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
}

esp_err_t Read(char *tag, char *value, size_t len) {
	nvs_handle my_handle;

	// Get this target's name.
	esp_err_t err = nvs_open("preferences", NVS_READWRITE, &my_handle);

	if (err != ESP_OK)
		printf("Error (%d) opening NVS handle!\n", err);
	else {
		err = nvs_get_str(my_handle, tag, value, &len);
		//printf("nvs_get_str: err=%X, '%s'\n",err,value);
	}

	nvs_close(my_handle);
	return err;
}

esp_err_t Write(char *tag, char *value) {
	nvs_handle my_handle;

	// Get this target's name.
	esp_err_t err = nvs_open("preferences", NVS_READWRITE, &my_handle);

	if (err == ESP_OK) {
		err = nvs_set_str(my_handle, tag, value);
	} else
		printf("Error (%d) opening NVS handle!\n", err);

	nvs_close(my_handle);
	return err;
}

void Thread_CPUTimeUpdate() {
	while (1) {
		vTaskDelay(10 / portTICK_PERIOD_MS);

		TimeSinceBootMS++;
	}
}

void Replace(string& source, string const& find, string const& replace) {
	for (string::size_type i = 0; (i = source.find(find, i)) != string::npos;) {
		source.replace(i, find.length(), replace);
		i += replace.length();
	}
}

/**
 * Open a socket to the server.
 */
void SocketInitSMTP() {

	// Close first in case we are reconnecting because of a disconnect.
	close(sockSMTP);

	// Open a socket.
	sockSMTP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, SVR_ADDR_SMTP, &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(26);
	connect(sockSMTP, (struct sockaddr *) &serverAddress, sizeof (struct sockaddr_in));
	listen(sockSMTP, 1);
}

void Parse(char *buf, int buflen, string StrArr[], int StrMaxlen);

/**
 * Parse an html header string.
 * 
 * @param buf
 * @param buflen
 * @param StrArr
 * @param StrMaxlen
 */
///
void Parse(char *buf, int buflen, string StrArr[], int StrMaxlen) {
	char *ptr = buf;
	int i = 0;
	char s[5];

	for (i=0; i<StrMaxlen; i++)
		StrArr[i].clear();

	i=0;
	// While EOL not detected and we are not at the end of the buffer..
	while (*ptr != '\n' && (int)(ptr-buf) < buflen) {
		//printf("While\n");
		
		//printf("i=%u *ptr=%c\n",i,*ptr);
		// If this is a separator..
		if ( strchr("?=& ",*ptr) ) {
			// Save the separator as the current array token.
			if (StrArr[i].size() > 0 && i < StrMaxlen-1)
				i++;
			s[0] = *ptr; s[1] = 0;
			StrArr[i] = s;
			if (i < StrMaxlen-1)
				i++;
		}
		else {
			// Add this char to the current array value.
			StrArr[i] += (char)(*ptr);
		}
		ptr++;
	}
}

/**
 * Send the main page.
 */
/// 
void ProcessMainPageRequest(struct netconn *conn) {
	printf("Main page..\n");

	char s200[200], s100[100];
	struct tm timeinfo;
	
	// Create the Options section.
	string OptionsStr = CHART_OPTIONS;
	sprintf(s200,"%u",Config.ChartYAxis[0].LowTemp);
	Replace(OptionsStr,string("$$$VAXES_0_MIN"),string(s200));
	
	sprintf(s200,"%u",Config.ChartYAxis[0].HighTemp);
	Replace(OptionsStr,string("$$$VAXES_0_MAX"),string(s200));

	sprintf(s200,"%u",Config.ChartYAxis[1].LowTemp);
	Replace(OptionsStr,string("$$$VAXES_1_MIN"),string(s200));
	
	sprintf(s200,"%u",Config.ChartYAxis[1].HighTemp);
	Replace(OptionsStr,string("$$$VAXES_1_MAX"),string(s200));
	
	// Set the chart axis for each sensor.
	for (int i=0; i<NUM_CHANNELS; i++) {
		sprintf(s100,"%u",Config.ChannelsAr[i].Axis);
		sprintf(s200,"$$$SENSOR_%u_AXIS",i);
		Replace( OptionsStr,string(s200),string(s100) );
	}
	
	// Build the vertical ticks string.
//	string TicksStr;
//	int TicksDiv = (Config.ChartYAxis[0].HighTemp-Config.ChartYAxis[0].LowTemp)/Config.ChartYAxis[0].NumTicks;
//	for (int i=Config.ChartYAxis[0].LowTemp; i<Config.ChartYAxis[0].HighTemp; i += TicksDiv) {
//		sprintf(s200,"%i,",i);
//		TicksStr += s200;
//	}
//	if ( !TicksStr.empty() ) {
//		TicksStr.erase(TicksStr.size()-1);
//	}
//	
//	Replace(str,string("$$$VTICKS"),string(TicksStr));	
	string MainStr = MAIN_HTML;
	
	// Set the current value for each channel.
	for (int i=0; i<NUM_CHANNELS; i++) {
		if (Config.ChannelsAr[i].ValueCurrent != INVALID_TEMP)
			sprintf(s100,"%.1f",Config.ChannelsAr[i].ValueCurrent);
		else
			*s100 = 0;
		sprintf(s200,"$$$CURVAL%u",i);
		Replace( MainStr,string(s200),string(s100) );
	}	

	// Insert the options into the main string.
	Replace(MainStr,"$$$CHART_OPTIONS",OptionsStr);
	
	sprintf(s200,"%u",100*Config.WebRefreshPeriodTSec);
	Replace(MainStr,string("$$$REFRESH_PERIOD"),string(s200));

	// Boot time.
	localtime_r(&BootTime, &timeinfo);
	sprintf(s200,"%u-%u-%u %2u:%2u:%2u",timeinfo.tm_year+1900,timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
	printf("BootTimeStr=%s\n",s200);
	Replace(MainStr,string("$$$BOOTTIME"),string(s200));

	// Initial status text.
	Replace(MainStr,string("$$$STATUS_TEXT"),GetStatusTextStr());
	
	// Send the HTML header
	netconn_write(conn, http_html_hdr, sizeof (http_html_hdr) - 1, NETCONN_NOCOPY);

	netconn_write(conn, MainStr.c_str(), strlen(MainStr.c_str()) - 1, NETCONN_NOCOPY);
}


/**
 * Create the text shown at the bottom of the chart page.
 * 
 * @return 
 */
///
string GetStatusTextStr() {
	char s200[200];
	time_t now;
	struct tm timeinfo, timeinfo_now;	
	
	localtime_r(&ValuesArr[ValuesArrInd].time, &timeinfo);
	time(&now);
	localtime_r(&now, &timeinfo_now);
	
	sprintf(s200,"<b>ChartSamples:</b>%u <b>Zoom:</b>%u <b>SampleP:</b>%.1f s <b>XOffset:</b>%u <b>ValuesArrInd:</b>%u "
			  "<b>ChartTime:</b>%u:%02u:%02u <b>Time:</b>%u:%02u:%02u <b>Ver</b> %s <br />"
			  "<b>Slope0:</b>%.1f <b>Slope1:</b>%.1f <b>Slope2:</b>%.1f<b>Slope3:</b>%.1f<b>Slope4:</b>%.1f<b>Slope5:</b>%.1f",
		Config.ChartSamples,Config.ChartZoom,(float)Config.SamplePeriodTSec/10,Config.XOffset,
		ValuesArrInd,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec, timeinfo_now.tm_hour,timeinfo_now.tm_min,timeinfo_now.tm_sec,VERSION,
		GetSlope(0),GetSlope(1),GetSlope(2),GetSlope(3),GetSlope(4),GetSlope(5) );
	
	return (string)s200;
}

/**
 * Process the settings page.
 */
///
void ProcessSettingsPage(struct netconn *conn) {
	char s100[100], s200[200];
	printf("Settings page..\n");

	// Send the HTML header
	netconn_write(conn, http_html_hdr, sizeof (http_html_hdr) - 1, NETCONN_NOCOPY);

	// Get the settings page.
	string SettingsPageStr = SETTINGS_HTML;
	string SelectOptionsStr;
	
	// Create the SamplePeriod menu;
	CSelectVals SelectValsAr[10] = { {10,"1 sec"},{20,"2 sec"},{50,"5 sec"},{30,"30 sec"},{600,"1 min"},{1200,"2 min"},{3000,"5 min"},{6000,"10 min"},{0,""} };
	
	string SelectedStr;
	for (int i=0; SelectValsAr[i].Value != 0 && i<10; i++) {
		sprintf(s100,"%u",SelectValsAr[i].Value);
		SelectedStr = "";
		if (SelectValsAr[i].Value == Config.SamplePeriodTSec)
			SelectedStr = " selected";
		SelectOptionsStr += (string)"<option value='" + s100 + "'" + SelectedStr + ">" + SelectValsAr[i].Description + "</option>\n";
	}
	
	Replace(SettingsPageStr,string("$$$SAMPLE_PERIOD_OPTIONS"),SelectOptionsStr);
	
	// Logging Enable.
	*s100 = 0;
	if (Config.EnableLogging)
		strcpy(s100,"checked");
	Replace(SettingsPageStr,string("$$$LOGGING_ENABLE"),s100);
	
	// Set the Chart Samples default value.
	sprintf(s100,"%u",Config.ChartSamples);
	string ChartSamples = s100;
	Replace(SettingsPageStr,string("$$$CHARTSAMPLES"),ChartSamples);
	
	// Set the vertical axis 1 low and high values.
	sprintf(s100,"%u",Config.ChartYAxis[0].LowTemp);
	string ChartYAxisLowStr = s100;
	Replace(SettingsPageStr,string("$$$CHARTLOWTEMP1"),ChartYAxisLowStr);

	sprintf(s100,"%u",Config.ChartYAxis[0].HighTemp);
	string ChartYAxisHighStr = s100;
	Replace(SettingsPageStr,string("$$$CHARTHIGHTEMP1"),ChartYAxisHighStr);

	// Set the vertical axis 2 low and high values.
	sprintf(s100,"%u",Config.ChartYAxis[1].LowTemp);
	ChartYAxisLowStr = s100;
	Replace(SettingsPageStr,string("$$$CHARTLOWTEMP2"),ChartYAxisLowStr);

	sprintf(s100,"%u",Config.ChartYAxis[1].HighTemp);
	ChartYAxisHighStr = s100;
	Replace(SettingsPageStr,string("$$$CHARTHIGHTEMP2"),ChartYAxisHighStr);

	Replace(SettingsPageStr,string("$$$HOSTNAME"),Config.HostName);
	
	// Slope Time Seconds
	sprintf(s100,"%u",Config.SlopeTimeSec);
	Replace(SettingsPageStr,string("$$$SLOPETIMESEC"),s100);
	
	// Update channel options
	// For each channel..
	for (int i=0; i<NUM_CHANNELS; i++) {	
		// Update each axis menu..
		SelectOptionsStr = "";
		// For each axis menu option.
		for (int AxisI=0; AxisI<2; AxisI++) {
			SelectedStr = "";
			if (Config.ChannelsAr[i].Axis == AxisI) {
				SelectedStr = " selected";
				//printf("Config.ChannelsAr[%u].Axis=%u, AxisI=%u\n",i,Config.ChannelsAr[i].Axis,AxisI);
			}
			sprintf(s200,"%u",AxisI);

			sprintf(s100,"<option value=%u %s>%s</option>\n",AxisI,SelectedStr.c_str(),s200);
			SelectOptionsStr += s100;
		}
		sprintf(s200,"$$$AXES_OPTIONS_%u",i);
		Replace(SettingsPageStr,string(s200),SelectOptionsStr);
		
		// Update each device menu..
		SelectOptionsStr = "";
		// For each sensor type menu option.
		SelectedStr = "";
		// If this sensor is a DS18B20..
		if (Config.ChannelsAr[i].SensorType == CChannel::SensorTypeE::T18B20)
			SelectedStr = " selected";

			sprintf(s100,"<option value=0 %s>DS18B20</option>\n",SelectedStr.c_str());
		SelectOptionsStr += s100;

		SelectedStr = "";
		// If this sensor is a TMAX31856..
		if (Config.ChannelsAr[i].SensorType == CChannel::SensorTypeE::TMAX31856)
			SelectedStr = " selected";

		sprintf(s100,"<option value=1 %s>Thermocouple</option>\n",SelectedStr.c_str());
		SelectOptionsStr += s100;
		
		sprintf(s200,"$$$SENSOR_TYPE_OPTIONS_%u",i);
		Replace(SettingsPageStr,string(s200),SelectOptionsStr);
	}
		
	netconn_write(conn, SettingsPageStr.c_str(), strlen(SettingsPageStr.c_str()) - 1, NETCONN_NOCOPY);
}

/***************************************************************************************
 * Process the html request.
 * 
 * 
 * NOTE: if it looks like the browser is sending more than one request per page then 
 *		the browser is probably requesting a particular piece of data using a GET.
 * 
 * This server only handles GET requests because the POST request appears much farther down in the 
 * data request stream.
 */
static void ProcessHttpRequest(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;
	char s200[200], s100[100];
	
	UpdateCount++;

	// Set timezone to Eastern Standard Time and print local time
	setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
	tzset();
	
//	printf("ProcessHttpRequest UpdateCount=%lu\n",UpdateCount);

	// Read the data from the port, blocking if nothing yet there.
	// We assume the request (the part we care about) is in one netbuf .
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);

		//printf("buflen=%u buf=%s\n",buflen,buf);

		// NOTE that we could use a POST here but GET is easier since the GET argument appears earlier in the data stream.
		// 
		// We handle two types of requests; the favicon request is ignored.
		// Data request: GET /index.php?curval1 HTTP/1.1
		// Page request: GET / HTTP/1.1
		// Ignored: GET /favicon.ico HTTP/1.1

		// Parse the first line of the request.
		// A typical reponse is
		//    TokenArr[0]:GET
		//    TokenArr[1]:
		//    TokenArr[2]:/index.php
		//    TokenArr[3]:?
		//    TokenArr[4]:value
		//    TokenArr[5]:=
		//    TokenArr[6]:333
		//    TokenArr[7]:
		//    TokenArr[8]:HTTP/1.1
		
		string TokenArr[20];
		Parse(buf, buflen, TokenArr, 20);

		// If this is a GET..
		if (TokenArr[0] == "GET") {
			// If this is a data request..
			if (TokenArr[3] == "?") {
#if 0
		// Print the token array.
		printf("*** TokenArr ***\n");
		for (int i=0; TokenArr[i].length() > 0; i++)
			printf("TokenArr[%u]='%s'\n",i,TokenArr[i].c_str());
		printf("\n");
#endif
			
				// If request to set a var..
				if (TokenArr[5] == "=") {
					// ************************
					// Channel Options
					//
					// ?channel=X&axis=X
					// ^   ^   ^^^ ^  ^^
					// 3   4   567 8  90
					// ************************
					if (TokenArr[4] == "channel" && TokenArr[7] == "&" && TokenArr[9] == "=") {
						int SensorI = atoi( TokenArr[6].c_str() );
						if (SensorI < 0 || SensorI > NUM_CHANNELS) {
							printf("ERROR: Sensor index '%u' out of range!\n",SensorI);
							return;
						}
						printf("Sensor Options, sensor %u\n",SensorI);
						// Axis
						if (TokenArr[8] == "axis") {
							int AxisI = atoi( TokenArr[10].c_str() );
							if (AxisI == 0 || AxisI == 1) {
								Config.ChannelsAr[SensorI].Axis = AxisI;
								printf("Config.ChannelsAr[%u].Axis -> %u\n",SensorI,AxisI);
							}
						}
						// sensortype
						if (TokenArr[8] == "sensortype") {
							CChannel::SensorTypeE SensorType = (CChannel::SensorTypeE)(atoi( TokenArr[10].c_str() ) );
							if (SensorType == CChannel::SensorTypeE::T18B20 || SensorType == CChannel::SensorTypeE::TMAX31856) {
								Config.ChannelsAr[SensorI].SensorType = SensorType;
								// If this is a DS18B20..
								if (SensorType == CChannel::SensorTypeE::TMAX31856)
									Config.ChannelsAr[SensorI].T18b20.ds18b20_init();

								printf("Config.ChannelsAr[%u].SensorType -> %u\n",SensorI,(int)SensorType);
							} else {
								printf("ERROR: SensorType out of range!\n");
							}
						}
					}
					// ************************
					// enablelogging
					//
					// ?enablelogging=X
					// ^   ^         ^^
					// 3   4         56
					// ************************
					if (TokenArr[4] == "enablelogging") {
						Config.EnableLogging = atoi( TokenArr[6].c_str() );
						printf("Config.EnableLogging -> %u\n",Config.EnableLogging);
					}
					// ************************
					// hostname
					//
					// ?hostname=X
					// ^   ^    ^^
					// 3   4    56
					// ************************
					if (TokenArr[4] == "hostname") {
						if (TokenArr[6].length() > 1 && TokenArr[6].length() < 20)
							strncpy(Config.HostName, TokenArr[6].c_str(),20);
						printf("Config.HostName -> %s\n",Config.HostName);
					}
					// ************************
					// channel
					// ************************
					// Handle a single channel data request.
					if (TokenArr[4] == "channel") {
						int n = atoi( TokenArr[6].c_str() );
						printf("ChannelReq %u:%.1f\n",n,Config.ChannelsAr[n].ValueCurrent);
						sprintf(s100,"%.1f",Config.ChannelsAr[n].ValueCurrent);
						netconn_write(conn, HTML_HEADER_OK, strlen(HTML_HEADER_OK), NETCONN_NOCOPY);
						netconn_write(conn, s100, strlen(s100), NETCONN_NOCOPY);						
					} 
					// ************************
					// SamplePeriodTSec
					// ************************
					if (TokenArr[4] == "sampleperiod") {
						Config.SamplePeriodTSec = atoi( TokenArr[6].c_str() );
						printf("Config.SamplePeriodTSec -> %u\n",Config.SamplePeriodTSec);
					}
					// ************************
					// WebRefreshPeriodTSec
					// ************************
					if (TokenArr[4] == "webrefreshperiod") {
						Config.WebRefreshPeriodTSec = atoi( TokenArr[6].c_str() );
						printf("Config.WebRefreshPeriodTSec -> %u\n",Config.WebRefreshPeriodTSec);
					}
					// ************************
					// ChartSamples
					// ************************
					if (TokenArr[4] == "chartsamples") {
						Config.ChartSamples = atoi( TokenArr[6].c_str() );
						printf("Config.ChartSamples -> %u\n",Config.ChartSamples);
					}
					// ************************
					// ZoomTo
					// ************************
					if (TokenArr[4] == "zoomto") {
						Config.ChartZoom = atoi( TokenArr[6].c_str() );
						printf("Config.ChartZoom -> %u\n",Config.ChartZoom);
					}
					// ************************
					// ZoomIn
					// ************************
					if (TokenArr[4] == "zoomin") {
						int zlevel = Config.ChartZoom;
						if (zlevel < ZOOM_MAX)
							zlevel++;
						Config.ChartZoom = zlevel;
						printf("Config.ChartZoom -> %u\n",Config.ChartZoom);
					}
					// ************************
					// ZoomOut
					// ************************
					if (TokenArr[4] == "zoomout") {
						int zlevel = Config.ChartZoom;
						if (zlevel > 1)
							zlevel--;
						Config.ChartZoom = zlevel;
						printf("Config.ChartZoom -> %u\n",Config.ChartZoom);
					}
					// ************************
					// XOffset
					// ************************
					if (TokenArr[4] == "xoffset") {
						Config.XOffset = atoi( TokenArr[6].c_str() );
						printf("Config.XOffset -> %u\n",Config.XOffset);
					}
					// ************************
					// ChartHighTemp1
					// ************************
					if (TokenArr[4] == "charthightemp1") {
						Config.ChartYAxis[0].HighTemp = atoi( TokenArr[6].c_str() );
						printf("Config.ChartYAxis[0].HighTemp -> %u\n",Config.ChartYAxis[0].HighTemp);
						if (Config.ChartYAxis[0].LowTemp > Config.ChartYAxis[0].HighTemp-CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[0].LowTemp = Config.ChartYAxis[0].HighTemp-CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartLowTemp1
					// ************************
					if (TokenArr[4] == "chartlowtemp1") {
						Config.ChartYAxis[0].LowTemp = atoi( TokenArr[6].c_str() );
						printf("Config.ChartYAxis[0].LowTemp -> %u\n",Config.ChartYAxis[0].LowTemp);
						if (Config.ChartYAxis[0].HighTemp < Config.ChartYAxis[0].LowTemp+CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[0].HighTemp = Config.ChartYAxis[0].LowTemp+CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartHighTemp2
					// ************************
					if (TokenArr[4] == "charthightemp2") {
						Config.ChartYAxis[1].HighTemp = atoi( TokenArr[6].c_str() );
						printf("Config.ChartYAxis[1].HighTemp -> %u\n",Config.ChartYAxis[1].HighTemp);
						if (Config.ChartYAxis[1].LowTemp > Config.ChartYAxis[1].HighTemp-CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[1].LowTemp = Config.ChartYAxis[1].HighTemp-CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartLowTemp2
					// ************************
					if (TokenArr[4] == "chartlowtemp2") {
						Config.ChartYAxis[1].LowTemp = atoi( TokenArr[6].c_str() );
						printf("Config.ChartYAxis[1].LowTemp -> %u\n",Config.ChartYAxis[1].LowTemp);
						if (Config.ChartYAxis[1].HighTemp < Config.ChartYAxis[1].LowTemp+CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[1].HighTemp = Config.ChartYAxis[1].LowTemp+CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartValueSelect
					// ************************
					if (TokenArr[4] == "chartvalueselect") {
						Config.ChartValueSelect = atoi( TokenArr[6].c_str() );
						printf("Config.ChartValueSelect -> %u\n",Config.ChartValueSelect);
					}
					// ************************
					// SlopeTimeSec
					// ************************
					if (TokenArr[4] == "slopetimesec") {
						Config.SlopeTimeSec = atoi( TokenArr[6].c_str() );
						if (Config.SlopeTimeSec < 2)
							Config.SlopeTimeSec = 10;
						printf("Config.SlopeTimeSec -> %u\n",Config.SlopeTimeSec);
					}
					
					// ************************
					// Test1
					//
					// ************************					
					
					// ************************
					// TestData
					//
					// Fill the sample array with test data.
					// ************************
					time_t now;
					time(&now);
					if (TokenArr[4] == "testdata") {
						printf("*** Filling sample array with test data: MAX_SAMPLES=%u\n",MAX_SAMPLES);
						for (int i=0; i<MAX_SAMPLES; i++) {
							ValuesArr[i].time = now + (time_t)i;
							ValuesArr[i].val[0] = ValuesArr[i].val[1] = (50.0 * sin( i/10.0 )) + 50.0;
						}
						ValuesArrInd = 0;
					}					
						
					// Now send the main page.
					ProcessMainPageRequest(conn);					
					// Save the configuration.
					Config.Write();
//					Config.Print();
				}
				
				// ************************
				// status
				//
				// Handle "status" requests.
				// ************************
				// { "curval0":"4", "slope0", "curval1":"838", "slope1", ... , "statustext":"<b>...</b>" }
				//
				if (TokenArr[4] == "status") {
					printf("----------------------------------------\nProcessing status data request.\n");
					
					string ValStr = "{ ";
					// For each channel..
					for (int chan=0; chan < NUM_CHANNELS; chan++) {
						// NOTE that the response must start with a \n or iphone browsers will not accept the response. This is 
						//  probably due to the lack of response header.

						// Get this channel's slope.
						float slope = GetSlope(chan);
						
						// If the value is valid..
						if (Config.ChannelsAr[chan].ValueCurrent != INVALID_TEMP) {
							// Json encode this channel's current value.
							sprintf(s200, "\"curval%u\":\"%.1f\",", chan, Config.ChannelsAr[chan].ValueCurrent);
							ValStr += s200;
						}
						
						// Json encode this channel's slope.
						sprintf(s200,"\"slope%u\":\"%.1f\",",chan, slope);
						ValStr += s200;
					}
					// Add the status string.
					ValStr += "\"statustext\":\"" + GetStatusTextStr() + "\"";

					ValStr += "}";
					
//					printf("response=%s\n",ValStr.c_str());
					// Send the data.
					netconn_write(conn, HTML_HEADER_OK, strlen(HTML_HEADER_OK), NETCONN_NOCOPY);
					netconn_write(conn, ValStr.c_str(), ValStr.length(), NETCONN_NOCOPY);
				}

				// Handle the chart data request.
				if (TokenArr[4] == "chart") {
					ProcessChartRequest(conn);
				}
				
				// Settings page request.
				if (TokenArr[4] == "settingspage") {
					ProcessSettingsPage(conn);
				}
			} else {
				// A '?' was not found.
				
				// If this is a page request..
				if (TokenArr[2] == "/" || TokenArr[2] == "/index.php") {
					ProcessMainPageRequest(conn);
				}
			}
		}
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	/* Delete the buffer (netconn_recv gives us ownership,
	 so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
}

/*
	echo '{
  "cols":[ 
		{"label":"date","type":"date"},
		{"label":"t1","type":"number"},
		{"label":"t2","type":"number"},
		{"label":"t3","type":"number"},
		{"label":"t4","type":"number"},
		{"label":"t5","type":"number"}
  ],
  "rows":[
		{"c":[{"v":"Date(2000,0,02, 15,0,30)"}, {"v":10}, {"v":15}, {"v":15}, {"v":55}]},
		{"c":[{"v":"Date(2000,0,02, 15,1,30)"}, {"v":20}, {"v":25}, {"v":15}, {"v":65}]},
		{"c":[{"v":"Date(2000,0,02, 15,2,30)"}, {"v":30}, {"v":35}, {"v":15}, {"v":75}]},
		{"c":[{"v":"Date(2000,0,02, 15,3,30)"}, {"v":40}, {"v":45}, {"v":15}, {"v":85}]}
  ]}';
 */
const string JSON_CHART1 = R"(	
	{ 
		"cols":[
		{"label":"date","type":"date"},
		{"label":"t1","type":"number"},
		{"label":"t2","type":"number"},
		{"label":"t3","type":"number"},
		{"label":"t4","type":"number"},
		{"label":"t5","type":"number"}
  ],
  "rows":[
)";
	
	void ProcessChartRequest(struct netconn *conn) {
	struct tm timeinfo;
	char s200[200];
	
	printf("----------------------------------------\nProcessChartRequest()\n");
	string ValStr;

	// Start chart JSON with cols definition.
	ValStr = JSON_CHART1;

	// Calculate the index of the last chart value.
	int i = ValuesArrInd - 1;
	if (i < 0)
		i = MAX_SAMPLES - 1;
	if (i >= MAX_SAMPLES)
		i = 0;

	// List the values backward so that the graph is created with later values on the right.
	for (int count = 0; count < Config.ChartSamples; i-=Config.ChartZoom, count++) {
		if (i <= 0)
			i = MAX_SAMPLES - 1 + i;

			localtime_r(&ValuesArr[i].time, &timeinfo);
		// If this is a valid date..
		if (ValuesArr[i].time > 0) {
			localtime_r(&ValuesArr[i].time, &timeinfo);

			// Start each JSON row with the date string.
			// {"c":[{"v":"Date(2000,0,02, 15,0,30)"},
			sprintf(s200, "{\"c\":[{\"v\":\"Date(%u,%u,%u, %u,%u,%u)\"}, ",
					  timeinfo.tm_year+1900,timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
			ValStr += s200;
			
			// Now add each value.
			// {"v":150},
			// If invalid:  {"v":null},
			for (int Vali = 0; Vali < NUM_CHANNELS; Vali++) { // $$$ max i???
				if (ValuesArr[i].val[Vali] != INVALID_TEMP)
					sprintf(s200,"{\"v\":%.1f},",ValuesArr[i].val[Vali]);
				else
					// If this is an invalid value..
					sprintf(s200,"{\"v\":null},");					
				ValStr += s200;
			}
			// Remove the last ','.
			ValStr.erase(ValStr.length()-1);// = ValStr.substr(0, ValStr.size()-1);
			ValStr += "]},\n";
		}
	}
	// Remove the last ','.
	ValStr.erase(ValStr.length()-2); //ValStr = ValStr.substr(0, ValStr.size()-2);
	
	ValStr += "\n]};";
	
	//printf("\n********************\nValStr=%s\n\n********************\n",ValStr.c_str());

	// Send the chart data.
	netconn_write(conn, HTML_HEADER_OK, strlen(HTML_HEADER_OK), NETCONN_NOCOPY);
	netconn_write(conn, ValStr.c_str(), ValStr.length()-1, NETCONN_NOCOPY);	
	printf("END ProcessChartRequest()\n----------------------------------------\n");
}

static void http_server(void *pvParameters) {
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			ProcessHttpRequest(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}

void Thread_BatteryTest() {
	adc1_config_width(ADC_WIDTH_9Bit);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

	// https://esp-idf.readthedocs.io/en/v2.0/api/peripherals/dac.html
	// For curve fitting see https://mycurvefit.com/
	while (1) {
		float val = adc1_get_raw(ADC1_CHANNEL_0); // dgs: was adc1_get_voltage
		ClientStats.BatteryVoltage = (0.00951 * val + 1.263)*10;
		//printf("Battery=%d\n",ClientStats.BatteryVoltage);
		//dac_out_voltage(DAC_CHANNEL_1, val/2);
		delay(1000);
	}
}

int SMTPWaitReply(int WaitTimeMS, string &s) {
	char SMTPBuf[200];
	int len;

	// While there is no reply..
	do {
		// Get a CMsg from the server.
		len = read(sockSMTP, SMTPBuf, BUF_SIZE_SKT);

		// If disconnect occurred..
		if (len < 0) {
			printf("SMTP Disconnect\n");
			return len;
		}

		DelayMS(1);
	} while (len == 0 && WaitTimeMS-- > 0);

	if (len > 0) {
		SMTPBuf[len] = 0;
		s = SMTPBuf;
		printf("SMTP SVR: %s\n", SMTPBuf);
	}
	return len;
}

bool SendMailOK(string ToStr, string FromStr, string SubjectStr, string MsgStr, string UserNameStr, string PwdStr) {
	int len, iResult;
	string s;

	SocketInitSMTP();

	s = "EHLO\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "EHLO\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "AUTH LOGIN\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "ZG91Z3NAZG91Z3Npc2NvLmNvbQ==\r\n"; // dougs@dougsisco.com
	//s = UserNameEncStr;
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "bm9DYWxlYjU=\r\n"; // noCaleb5
	//s = PwdEncStr;
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "MAIL FROM:" + FromStr + "\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "RCPT TO:" + ToStr + "\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "DATA\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "From:" + FromStr + "\r\nTo:" + ToStr + "\r\nSubject:" + SubjectStr + "\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = MsgStr + "\r\n.\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	s = "QUIT\r\n";
	printf("Send: %s", s.c_str());
	iResult = send(sockSMTP, (char *) s.c_str(), s.length(), 0);
	len = SMTPWaitReply(200, s);
	if (len < 0)
		return 0;

	closesocket(sockSMTP);

	return 1;
}

void app_main(void) {
	printf("\n\nTemperature Logger\n   (C) 2019 Sisco Circuits\n\n");
	
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
	
	for (int i=0; i<MAX_SAMPLES; i++)
		ValuesArr[i].time = 0;
		
	// Don't log anything.
	esp_log_level_set(TAG, ESP_LOG_INFO);
	nvs_flash_init();
//	system_init();

// *    Thermocouple MUX
	gpio_pad_select_gpio(TC_MUX_SEL0);
	gpio_set_direction((gpio_num_t) TC_MUX_SEL0, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)TC_MUX_SEL0, 0);

	gpio_pad_select_gpio(TC_MUX_SEL1);
	gpio_set_direction((gpio_num_t) TC_MUX_SEL1, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)TC_MUX_SEL1, 0);

	gpio_pad_select_gpio(TC_MUX_SEL2);
	gpio_set_direction((gpio_num_t) TC_MUX_SEL2, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)TC_MUX_SEL2, 0);

	// Init LED I/O
	gpio_pad_select_gpio(LEDR_GPIO_NUM);
	gpio_set_direction((gpio_num_t) LEDR_GPIO_NUM, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)LEDR_GPIO_NUM, 0);
	
	gpio_pad_select_gpio(LEDG_GPIO_NUM);
	gpio_set_direction((gpio_num_t) LEDG_GPIO_NUM, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)LEDG_GPIO_NUM, 1);
	
	gpio_pad_select_gpio(LEDB_GPIO_NUM);
	gpio_set_direction((gpio_num_t) LEDB_GPIO_NUM, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)LEDB_GPIO_NUM, 1);
	
	// Init the configuration.
	esp_err_t err = Config.Read();
		 
	// If we are forcing a config write or there was an error reading the config or if the check value is incorrect..
	if (err || Config.CheckValue != CHECK_VALUE_INIT) {
		printf("\n---> CONFIG READ ERROR <---\n\n");
		Config.Init();
		Config.Write();
	}
	
	//*******************************************************************
	// IMPORTANT: THE FOLLOWING NEEDS TO BE DONE AFTER Config.Init()!
	// Init the DS18B20 sensors.
	Config.ChannelsAr[0].T18b20.DS_GPIO = (gpio_num_t)25;
	Config.ChannelsAr[1].T18b20.DS_GPIO = (gpio_num_t)26;
	Config.ChannelsAr[2].T18b20.DS_GPIO = (gpio_num_t)5;
	Config.ChannelsAr[3].T18b20.DS_GPIO = (gpio_num_t)19;
	Config.ChannelsAr[4].T18b20.DS_GPIO = (gpio_num_t)18;
	Config.ChannelsAr[5].T18b20.DS_GPIO = (gpio_num_t)33;
	Config.ChannelsAr[6].T18b20.DS_GPIO = (gpio_num_t)23;
	
	for (int i = 0; i < 7; i++)
		Config.ChannelsAr[0].SensorType = Config.ChannelsAr[1].SensorType = CChannel::SensorTypeE::TMAX31856;		
	
	// Init all DS1820B sensors (even if these channels are not DS18B20 devices)
	for (int i=0; i<NUM_CHANNELS; i++) {
			Config.ChannelsAr[i].T18b20.ds18b20_init();
	}
	//*******************************************************************
	
	// $$$ THIS NEEDS TO BE A SETUP PARM.
//	if (CH0_IS_THERMOCOUPLE) {
//		Config.ChannelsAr[0].SensorType = Config.ChannelsAr[1].SensorType = CChannel::SensorTypeE::TMAX31856;
//	}
	
//	Config.Print();

#if 0
	// Write the target name to the VFS.
	err = Write("hostname", HOSTNAME);
	if (err)
		printf("Error saving HostName name: %X.\n", err);
	else
		printf("Hostname saved as %s.\n", HOSTNAME);

	SetLEDParms(GREEN, 100);
	OnPeriod = 50;
	BlinkCount = -1;
	Beep(10, 5);

	while (1)
		;
#endif

	// Read the target name from the VFS.

	// NOTE: if compiling as a .c file, this function takes an argument of size_t*, but for some 
	//   reason, probably a bug, if compiling as a .cpp it takes an argument of size_t. However if you send
	//   a size_t then it returns a token-length error.
//	err = Read("hostname", HostName, (size_t) & len);
//	if (err)
//		printf("Error reading HostName name: 0x%X\n", err);
//	else
	printf("Hello, Doug, I'm %s.\n", Config.HostName);

	initialise_wifi();
	
	// Red
	LEDR_ON;
	LEDG_OFF;
	LEDB_OFF;

	// Set the host name.
	// Don't put a space in the hostname or it won't update!
	// If done properly the router will show the new hostname immediately.
	string s(Config.HostName);
	Replace(s, " ", "_");
	printf("s=%s\n", s.c_str());
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, s.c_str());

	// I'm not sure why, but without this we get watchdog timer errors.
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	// Set low power wifi mode. This alternates between 50mA and 100mA.
	// See https://github.com/espressif/esp-idf/blob/master/examples/wifi/power_save/main/power_save.c
	esp_wifi_set_ps(WIFI_PS_MODEM);

	xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);

	// see http://esp32.info/docs/esp_idf/html/dd/d3c/group__xTaskCreate.html
	//	xTaskCreatePinnedToCore((TaskFunction_t) & Thread_BatteryTest, "Thread_BatteryTest", 2048, NULL, 1, NULL, 0);
	//	xTaskCreatePinnedToCore((TaskFunction_t) & Thread_Beep, "Thread_Beep", 2048, NULL, 1, NULL, 0);
	xTaskCreate(&http_server, "http_server", 15000, NULL, 5, NULL);

	// Get the current time and set up the time server.
	TimeSetup();
	
	// Led yellow
	LEDR_ON;
	LEDG_ON;
	LEDB_OFF;

	xTaskCreate(&Thread_ReadTemperature, "Thread_ReadTemperature", 2048, NULL, 5, NULL);
	xTaskCreatePinnedToCore((TaskFunction_t) & Thread_SaveValuesToArray, "Thread_SaveValuesToArray", 2048, NULL, 1, NULL, 0);

	// Led green.
	LEDR_OFF;
	LEDB_OFF;
	LEDG_ON;
		
	// Set BootTime.
	struct tm timeinfo;
	localtime_r(&BootTime, &timeinfo);
	strftime(strftime_buf, sizeof (strftime_buf), "%c", &timeinfo);
	
}

/**
 * Read the configuration from the VFS.
 * 
 * See https://github.com/espressif/esp-idf/tree/master/examples/storage/nvs_rw_blob
 * 
 * @return 
 */
esp_err_t CConfig::Read() {
	printf("***** Restoring Config..\n");
	esp_err_t err;
	size_t required_size;
	nvs_handle my_handle;
	
	// Open the NVS.
	err = nvs_open("preferences", NVS_READWRITE, &my_handle);

	if (err != ESP_OK)
		printf("Error (%d) opening NVS handle!\n", err);
	else {
		required_size = sizeof(Config);
		if (required_size > 0) {
			err = nvs_get_blob(my_handle, "config", this, &required_size);
			if (err != ESP_OK) {
				printf("Error nvs_get_blob.\n");
			}
		}
	}	
	nvs_close(my_handle);
	
	// Test all config values for ranges.
	if ( Config.CheckValues() )
		err = ESP_FAIL;
	
	return err;
}

/**
 * Write the configuration to the VFS.
 * 
 * @return 
 */
esp_err_t CConfig::Write() {
	esp_err_t err;
	size_t required_size;
	nvs_handle my_handle;

	printf("**** Writing Config..\n");
	
	// Open the NVS.
	err = nvs_open("preferences", NVS_READWRITE, &my_handle);
	
	// Write the config to the VFS.
	required_size = sizeof(Config);
	err = nvs_set_blob(my_handle, "config", this, required_size);

	if (err != ESP_OK) 
		printf("Error in nvs_set_blob.\n");

	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) 
		printf("NVS commit error.\n");

	nvs_close(my_handle);
	
	return err;
}

/**
 * Select a thermocouple input.
 * 
 * @param n
 */
void ThermocoupleSelect(int n) {
	gpio_set_level((gpio_num_t)TC_MUX_SEL0, 0);
	gpio_set_level((gpio_num_t)TC_MUX_SEL1, 0);
	gpio_set_level((gpio_num_t)TC_MUX_SEL2, 0);
	
	if (n & 1)
		gpio_set_level((gpio_num_t)TC_MUX_SEL0, 1);
	if (n & 2)
		gpio_set_level((gpio_num_t)TC_MUX_SEL1, 1);
	if (n & 4)
		gpio_set_level((gpio_num_t)TC_MUX_SEL2, 1);
}

///
/*		
 * Get a sample given the offset from the current index.
 * 
 * @param offset
 * @param Sample
 * @return -1 on error.
 */
int GetSample(int offset,S_DateVal &Sample) {
	if (offset < 0 || offset > MAX_SAMPLES-1)
		return -1;
	
	int i = ValuesArrInd - offset - 1;
	
	if (i < 0)
		i += MAX_SAMPLES;
	
	Sample = ValuesArr[i];
	return 0;
}

/// Get this channel's slope over the SlopeTimeSec time.
float GetSlope(int chan) {
	float Yavg0 = 0;
	float Yavg1 = 0;
	S_DateVal DateVal;
	
	// SlopeSamples is the number of samples that will be used to calculate the slope.
	int SlopeSamples = (int)((float)Config.SlopeTimeSec *10.0) / (float)Config.SamplePeriodTSec;
	
	// Force SlopeSamples to be even.
	if (SlopeSamples %2 == 1)
		SlopeSamples += 1;
//	printf("GetSlope: SlopeSamples=%u\n",SlopeSamples);
	
	// Slope = (Yavg0 - Yavg1) / SlopeSamples
	//   where Yavg = avg over SlopeSamples/2 samples

	// Get Yavg0
	// 9,8,7,6,5
//	printf("\nGetSlope1: ");
	for (int i = SlopeSamples-1; i >= SlopeSamples/2; i--) {
		GetSample(i,DateVal);
		Yavg0 += DateVal.val[chan];
//		printf("[%u,%.1f], ",i,DateVal.val[chan]);
	}
	
	// Get Yavg1
	// 4,3,2,1,0
//	printf("\nGetSlope2: ");
	for (int i = (SlopeSamples/2)-1; i >= 0; i--) {
		GetSample(i,DateVal);
		Yavg1 += DateVal.val[chan];
//		printf("[%u,%.1f], ",i,DateVal.val[chan]);
	}
	
	float slope = ( (Yavg1 - Yavg0)*2.0 )/(float)SlopeSamples;	

	return slope;
}
		
/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "192.168.100.250"
#define WEB_PORT 80
#define WEB_URL "/index.php"
		
static const char *REQUEST1 = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: " WEB_SERVER "\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";		
		
static const char *REQUEST2 = "POST " WEB_URL " HTTP/1.0\r\n"
"Host: " WEB_SERVER "\r\n"
"From : frog@jmarshall.com\r\n"
"User-Agent: HTTPTool/1.0\r\n"
"Content-Type: application/x-www-form-urlencoded\r\n";

static const char *REQUEST = "POST " WEB_URL " HTTP/1.0\r\n"
"Accept: application/x-www-form-urlencoded, text/javascript, */*; q=0.01\r\n"
"X-Requested-With: XMLHttpRequest\r\n"
"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";

//"Content-Length: 32\r\n"
//"\r\n";
//"home=Cosby&favoriteflavor=flies\r\n";

string ToJSON(string Name, float val, string &Str);
		
string ToJSON(string Name,float val,string &Str) {
	char s200[200];
	sprintf(s200, "\"%s\":\"%.1f\"",Name.c_str(), val);
	Str = s200;
	return Str;
}
			
// See https://stackoverflow.com/questions/14551194/how-are-parameters-sent-in-an-http-post-request		
static void http_get_task(void *pvParameters) {
	struct addrinfo *res;
	struct in_addr *addr;
	int s, r;
	char recv_buf[64];

	const struct addrinfo hints = {
		.ai_flags = 0,
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_addrlen = 0,
		.ai_addr = 0,
		.ai_canonname = 0,
		.ai_next = 0
	};
	
	while (1) {
		// Wait for the internet to be connected.
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,portMAX_DELAY);

		// Resolve web server address.
		int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

		if (err != 0 || res == NULL) { ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		// Print the resolved IP.
		//   Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
		addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
		ESP_LOGI(TAG, "**** DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

		// Allocate a sockeet.
		s = socket(res->ai_family, res->ai_socktype, 0);
		if (s < 0) {
			ESP_LOGE(TAG, "... Failed to allocate socket.");
			freeaddrinfo(res);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		// Connect to the socket.
		if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
			ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
			close(s);
			freeaddrinfo(res);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
			continue;
		}

		freeaddrinfo(res);
		
		string str1;
		string Values = "p={" + ToJSON("doug", 5.123, str1) + "}";
		char s200[200];
		sprintf(s200, "Content-Length:%u\r\n", Values.length());
		string Request = (string)REQUEST + s200 + "\r\n" + Values + "\n\r";
		
		printf("*****\n\r%s\n\r*****\n\r", Request.c_str());

		// Write the http request.
		if( write(s, Request.c_str(), Request.length() ) < 0) {
			ESP_LOGE(TAG, "... socket send failed");
			close(s);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
			continue;
		}

		// Set the rx timeout.
		struct timeval receiving_timeout;
		receiving_timeout.tv_sec = 5;
		receiving_timeout.tv_usec = 0;
		if (setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&receiving_timeout,sizeof(receiving_timeout)) < 0) {
			ESP_LOGE(TAG, "... failed to set socket receiving timeout");
			close(s);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "... set socket receiving timeout success");

		// Read the http response.
		do {
			// Clear the buffer.
			bzero(recv_buf, sizeof(recv_buf));
			// Partially read the response.
			r = read(s, recv_buf, sizeof(recv_buf) - 1);
			// Print the buffer one char at a time.
			for (int i = 0; i < r; i++) {
//				putchar(recv_buf[i]);
				printf("%c", recv_buf[i]);
			}
		} while (r > 0);

		ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
		close(s);
 
		delay(5000);  //Send a request every 10 seconds		
		
		// 5 second delay.
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "**** Starting again!");
	}
}
