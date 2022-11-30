#define VERSION "5.6.2"
#define NEW_REV_BOARD 0

// If debugger connected then don't configure the tricolor LED.
#define DEBUGGER 0
#define COPYRIGHT "SISCO CIRCUITS CloudLogger (C) 2021"

// This is extra code which really does nothing except help to fix the 
// intermittent wifi connection problem.
// If wifi doesn't connect then either enable or disable this flag.
#define FIX_WIFI_CONNECTION_PROBLEM 0
#define PRINT_TEMPERATURE_VALUES 0
#define PRINT_SENSOR_ERRORS 0
#define PRINT_ONEWIRE_RESET_PULSE 0
#define PRINT_CURRENT_DATAPOINT_TIME 0
#define PRINT_HTML_SVR_RESPONSE 0
#define PRINT_WEB_CONN_INFO 0
#define PRINT_HTML_HTTP_REQUEST 0

// The system will reboot after 10 min if a WIFI connection error persists.
#define ERROR_REBOOT_TIME_SEC 60*10
// This needed to disable informational logging.
//#define CONFIG_LOG_DEFAULT_LEVEL ESP_LOG_NONE

#define WEB_PORT "80"
#if 1
// This will save data points to the HostGator svr.
    #define WEB_SERVER "siscocircuits.com"
    #define WEB_URL "http://siscocircuits.com/tl/post_data.php"
#else
// This will save using homedgs2 (only for devices on the local wless)
    #define WEB_SERVER "cl.dsisco.no-ip.info"
    #define WEB_URL "/post_data.php"
#endif

// IMPORTANT: FIRMWARE_UPGRADE_PORT is defined in C:\SysGCC\esp32\esp-idf\v3.3\components\esp_http_client\esp_http_client.c
// I had to hard code this because it overrides the URL port nor the client port.
// See also the change to C:\SysGCC\esp32\esp-idf\v3.3\components\esp_https_ota\src\esp_https_ota.c to prevent an 
//   infinite loop in the case of a bad URL.
#define FIRMWARE_UPGRADE_URL "http://dsisco.no-ip.info/tl.bin"

#define NAME_DEFAULT "Sisco CloudLogger"
#define MAX_SAMPLES 1000
#define SLOPE_TIME_SEC_INIT 10

#define INVALID_TEMP -20000
#define TEMP_MIN -50
#define TEMP_MAX 2000
#define NUM_CHANNELS 12
#define NUM_CHART_VALS 5

#define SAMPLE_PERIOD_TSEC 10
#define SAMPLE_PERIOD_TSEC_MAX 6000
#define WEB_REFRESH_PERIOD_TSEC 5
#define WEB_REFRESH_PERIOD_TSEC_MAX 50
#define WEB_REFRESH_PERIOD_TSEC_MIN 2
#define CHART_SAMPLES_INIT 100
#define CHART_SAMPLES_MIN 50
#define CHART_SAMPLES_MAX 1000
#define HOSTNAME_MAXLEN 20

#define DATAPOINT_SVR_QUEUE_MAX_SIZE 30

#define LOG_LEVEL_DEFAULT ELogLevel::DEBUG
#define PRINT_ALL_LOGS 1
#define LOG_QUEUE_SIZE 50

// Device Flags
// These are bitmapped flags from server to client.
#define DEVICE_FLAG_RESET 1		
#define DEVICE_FLAG_FIRMWARE_UPGRADE 2
#define DEVICE_FLAG_IDENTIFY_ON 4
#define DEVICE_FLAG_IDENTIFY_OFF 8

#define REBOOT_DELAY_TIME 4
#define IDENTIFY_TIME_SEC 10

// Change this to force the configuration to be saved to flash.
#define CHECK_VALUE_INIT 19370
#define ZOOM_MAX 10
#define CHART_TEMP_MIN_DIF 10
#define MAX_SUCCESSIVE_INVALID_READS 3
#define MAX_SUCCESSIVE_SOCKET_ERRORS 3
#define MAX_SUCCESSIVE_SENDHTTP_ERRORS 3

#define HTML_HEADER_OK "HTTP/1.1 200 OK\n\n"

#define AP_SSID "XXXXXX"
#define AP_PASSPHARSE "XXXXXXXX"
#define AP_SSID_HIDDEN 0
#define AP_MAX_CONNECTIONS 4
#define AP_AUTHMODE WIFI_AUTH_WPA2_PSK // the passpharese should be atleast 8 chars long
#define AP_BEACON_INTERVAL 100 // in milli seconds

// This is the SSID when in captive portal AP mode.
#define CAPTIVE_PORTAL_SSID "CLOUDLOGGER"
#define SSID_NOT_INITIALIZED "not initialized"

// This is used for the ESPIDF log.
#define TAG "tlogger"

#define BRIGHT_RED 0x640000
#define RED 0x200000
#define BRIGHT_GREEN 0x006400
#define GREEN 0x001000
#define BLUE 0x000064
#define YELLOW 0x305500
#define ORANGE 0xf0ff00
#define WHITE 0x346454 
#define OFF 0

#define SYS_STATUS_OK 0
#define SYS_STATUS_WARNING 1
#define SYS_STATUS_ERR 2

// ***************************** PIN DEFINITIONS *******************************
#define I2C_SDA	33
#define I2C_SCL 32

// Tricolor LED
#define LEDR_GPIO_NUM (gpio_num_t)14
#define LEDG_GPIO_NUM (gpio_num_t)13
#define LEDB_GPIO_NUM (gpio_num_t)12

// If this is a new board.
#if NEW_REV_BOARD == 1
 #define SW1_GPIO_NUM  (gpio_num_t)39
#else
 #define SW1_GPIO_NUM  (gpio_num_t)22
#endif
#define SW2_GPIO_NUM  (gpio_num_t)36

// DS18B20 CHANNELS
//#define DS18B20_0_GPIO_NUM (gpio_num_t)25
//#define DS18B20_1_GPIO_NUM (gpio_num_t)26
//#define DS18B20_2_GPIO_NUM (gpio_num_t)5
//#define DS18B20_3_GPIO_NUM (gpio_num_t)19
//#define DS18B20_4_GPIO_NUM (gpio_num_t)18
//
//    // GPIO 14 and 15 are shared with the LED!
//#ifndef NEW_SPI_DESIGN
//    #define DS18B20_5_GPIO_NUM (gpio_num_t)0
//    #define DS18B20_6_GPIO_NUM (gpio_num_t)4
//#else
//#define DS18B20_5_GPIO_NUM (gpio_num_t)17
//#define DS18B20_6_GPIO_NUM (gpio_num_t)17
//#endif

#define LEDR_ON gpio_set_level((gpio_num_t)LEDR_GPIO_NUM, 0)
#define LEDR_OFF gpio_set_level((gpio_num_t)LEDR_GPIO_NUM, 1)
#define LEDG_ON gpio_set_level((gpio_num_t)LEDG_GPIO_NUM, 0)
#define LEDG_OFF gpio_set_level((gpio_num_t)LEDG_GPIO_NUM, 1)
#define LEDB_ON gpio_set_level((gpio_num_t)LEDB_GPIO_NUM, 0)
#define LEDB_OFF gpio_set_level((gpio_num_t)LEDB_GPIO_NUM, 1)

#define FLUSH fflush(stdout)

// **************** INCLUDES ****************
#include <string.h>
#include <algorithm>
#include <string>
#include <time.h>
#include <math.h>
#include <cstdlib>
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
#include "esp_intr_alloc.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"
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
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include <driver/dac.h>
#include <driver/adc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

//#include "driver/ledc.h"
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
#include <queue>
#include "driver/i2c.h"
#include "mcp23017.h"

#define CONFIG_OTA_ALLOW_HTTP 1 // THIS IS ALSO DEFINED IN esp_https_ota.c

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "driver/ledc.h"

//using std::string;
using namespace std;

#define bool int8_t

typedef struct {
	int8_t rssi;
	uint8_t channel;
} SWifiInfo;

// This is a sample of all the channel values at the given time.
class CDataPoint {
public:
	time_t time;
	float val[NUM_CHANNELS];	
public:
	void Init() { time=0; for (int i=0; i<NUM_CHANNELS; i++) val[i] = INVALID_TEMP; }
	
};

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
		printf("ChartYAxis: LowTemp:%u HighTemp:%u NumTicks:%u\n", LowTemp, HighTemp, NumTicks);
	}
};

class CMAX31856 {
public:
	int cs;
	int miso;
	int mosi;
	int clk;
};

// The sensor can be one of two types.
class CChannel {
public:
	// DS18B20 object (used only if this is a DS18B20).
	C18b20 T18b20;
	// The MUX channel number (used only if this is a thermocouple).
	int MUXNum;
	// The last reading.
	float ValueCurrent;
	// Temperature unit
	enum {TUNIT_F = 0, TUNIT_C} TempUnit;
	// Indicates that this sensor is connected (at least one valid read has occurred).
	int Connected;
	// Sensor type
	enum SensorTypeE {T18B20 = 0, TMAX31856 = 1} SensorType;
	// Number of successive invalid readings.
	int InvalidReads;
    // Indicates that this channel is in an error state (used in UpdateInvalidReadCountStatusChanged() ).
    bool Error;
public:
	CChannel() {
		TempUnit = TUNIT_F;
		SensorType = T18B20;
		T18b20.DS_GPIO = (gpio_num_t) - 1;
		MUXNum = 0;
		Connected = 0;
		ValueCurrent = INVALID_TEMP;
    	InvalidReads = 0;
    	Error = 1;
	}
	void SetChannel18b20(gpio_num_t GPIO) { SensorType = SensorTypeE::T18B20; T18b20.DS_GPIO = GPIO; gpio_pad_select_gpio(GPIO); T18b20.ds18b20_init(); /* dgs added 4-25-21 */ /*T18b20.init = 1;*/ }
	void SetChannelMAX31856(int muxnum) { SensorType = SensorTypeE::TMAX31856; MUXNum = muxnum; }
    
    bool UpdateInvalidReadCountStatusChanged(bool Invalid);
};

// These are the values associated with the chart.
class CChartVal
{
public:
	// This is the channel index. -1 if not assigned.
	int ChannelIndex;
	// This indicates which chart Y-Axis is used.
	int Axis; 
public:
	void CChartval()	{
		// Set this value to 'unassigned'.
		ChannelIndex = -1;
		Axis = 0;
	}
};

// Note: if you change CConfig you must change CheckValue.
class CConfig {
public:
	// NOTE THAT SamplePeriodTSec IS NOT USED!
//	int SamplePeriodTSec;
	int CheckValue;
	// This is the index to the value that will be charted.
	int ChartValueSelect;
	// Sensor configuration.
	CChannel ChannelsAr[NUM_CHANNELS];
	// This enables saving the samples to the sample array.
	int EnableLogging;
	// Time over which the slope is calculated.
	
public:
	CConfig() {
		// Initialize the configuration.
//		this->SamplePeriodTSec = SAMPLE_PERIOD_TSEC;
		this->ChartValueSelect = 0;
		this->CheckValue = CHECK_VALUE_INIT;	
		this->EnableLogging = 1;
	}
};

class CWifiSTACredentials
{
public:
    char SSID[33];
    char Pwd[65];
    
public:
    esp_err_t Write();
    esp_err_t Read();
};

class CSelectVals {
public:
	int Value;
	string Description;
};

class CStatusLED {
public:
    // This indicates the current color of the LED.
	int LEDColor;
    // This is the last color if it was blanked.
    bool Blanked;

public:
	CStatusLED() { LEDColor = Blanked = 0;}
    void SetStatus(int status, string DetailStr="");
	void SetColor(int color);	
	void Blank();
    void Invert();
	void Restore();
};

enum ELogLevel {
    DEBUG = 0, // Very low level events, only used for debugging
     
    INFO,     // Events normal to operation such as "Created new table".
         
    WARNING, // Events that are abnormal but not important such as "Bad packet received".
       
    ERROR,   // Events that should never occur and represent a program bug.
        
    CRITICAL // Events that will prevent normal operation.
};

class CEventLogEntry
{
public:
    ELogLevel level;
    string text;
    time_t time;
    
public:
    string Json();
};

/*
 * The event log stores events whose level is at least as high as the current level
 * where 'higher' means 'more critical'. 
 * The current level can be set from the "set log level" command during an html reponse
 * The event log queue is cleared when it is dumped during an html post.
 */
class CEventLog
{      
private:
    std::queue<CEventLogEntry> EventLogQueue;
    
    ELogLevel CurLevel;
    
public:
    void Log(string text, ELogLevel Level = ELogLevel::DEBUG, bool Print = 1);
    void SetLogLevel(ELogLevel newlevel) { CurLevel = newlevel; }
    string Json();
    bool NotEmpty() { return (EventLogQueue.size() > 0); }
    CEventLog() { CurLevel = LOG_LEVEL_DEFAULT; }
};

extern CEventLog EventLog;

extern CConfig Config;
extern float GetSlope(int chan);
extern CDataPoint ValuesArr[];
extern char HostName[20];
extern CDataPoint ValuesArr[MAX_SAMPLES];
extern time_t BootTime;
extern volatile int ValuesArrInd;
extern max31856_cfg_t max31856;
extern void Replace(string& source, string const& find, string const& replace);
extern std::queue<CDataPoint> SvrDataPointQ;
extern unsigned long long MACAddr;
extern volatile int ResetRequestSec;
extern bool PauseTemperatureThread;
int CaptivePortal(void);


/* The event group allows multiple bits for each event,
	but we only care about one event - are we connected
	to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

// These are defined to make message.h compatible.
#define ESPIDF
#define FLUSH fflush(stdout);

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS))
#define DelayMS(ms) (vTaskDelay(ms/portTICK_RATE_MS))

void Thread_SendDataToSvr(void *pvParameters);
void Thread_OncePerSecond(void *pvParameters);
void Thread_CPUTimeUpdate();
void Thread_WebServer();
void Thread_BatteryTest();

int Parse(char *buf, int buflen, string StrArr[], int StrMaxlen);
void ProcessMainPageRequest(struct netconn *conn);
void Thread_ReadTemperature(void *pvParameter);
void Thread_UpdateTime(void *pvParameter);
void Thread_SwitchPoller(void *pvParameters);
void PrintCurrentTime();

//static void obtain_time(void);
static void initialize_sntp(void);
//static int initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);
int TimeValid();
void TimeSetup(int ForceUpdate);
void getWifiInfo(SWifiInfo *info);
void NVS_Init();
esp_err_t Write(char *tag, char *value);
esp_err_t Read(char *tag, char *value, size_t len);
void http_server(void *pvParameters);
//static void ProcessHttpRequest(struct netconn *conn);
void Replace(string& source, string const& find, string const& replace);
bool SendMailOK(string ToStr, string FromStr, string SubjectStr, string MsgStr, string UserNameStr, string PwdStr);
int SMTPWaitReply(int WaitTimeMS, string &s);
string GetStatusTextStr();
void ProcessChartRequest(struct netconn *conn);
void ThermocoupleSelect(int n);
int GetSample(int offset, CDataPoint &Sample);
float GetSlope(int chan);
void ProcessSettingsPage(struct netconn *conn);
string ToJSON(string Name, float val, string &Str);
char *StrToJSON(string Name, string val, char *s);
void InitGPIO();
void LEDSetColor(int color);
void SaveDataPointToArray(CDataPoint DataPoint);
char *Trim(char *str);
static void start_dhcp_server();
void print_sta_info(void *pvParam);
void printStationList();
static void initialise_wifi_in_ap(void);
int SendHttpRequest(string Request, string &Response, int s);
void SetStatusLED(int status);
vector<string> split(string s, string delimiter);
char * strcat2(char *d, char *s, long n);
string GetJsonDataPoints(string &str);
void ChannelLED(int LEDNum, int Enable);
void FirmwareUpgradeTask(void * pvParameter);
void LEDPwmInit(void);
void SetLEDParms(int color, int intensity = 100, int onperiod = 100, int offperiod = 0);
    
string GetConsoleString(string &s);
void LEDColorTest();

std::string str_toupper(std::string s);

void ParsePOST(string &str, vector<string> &out);

// All C functions that are called from CPP must be declared here.
extern "C" {
    void app_main(void);
    max31856_cfg_t max31856_init();
    void thermocouple_set_type(max31856_cfg_t *max31856, max31856_thermocoupletype_t tc_type);
    max31856_thermocoupletype_t thermocouple_get_type(max31856_cfg_t *max31856);
    uint8_t thermocouple_read_fault(max31856_cfg_t *max31856, bool log_fault);
    float thermocouple_read_coldjunction(max31856_cfg_t *max31856);
    void thermocouple_read_temperature(int chan, max31856_cfg_t *max31856);
    void thermocouple_set_temperature_fault(max31856_cfg_t *max31856, float temp_low, float temp_high);
    
    mcp23017_err_t mcp23017_init(int i2c_sda, int i2c_scl);
    mcp23017_err_t MCP23017Write(mcp23017_gpio_t group, uint8_t v);
    mcp23017_err_t MCP23017WriteBoth(int v);
    
    void EventLogLog(char *s);
}

//************* GLOBAL VARS *************
static EventGroupHandle_t wifi_event_group;

const int CLIENT_CONNECTED_BIT = BIT0;
const int CLIENT_DISCONNECTED_BIT = BIT1;

extern int DeviceFlags;

extern CStatusLED StatusLED;

extern volatile int IdentifySecs;

extern int MCP23017CurVal;

extern volatile bool FirmwareUpgradeInProcess;

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

extern CWifiSTACredentials WifiSTACredentials;
