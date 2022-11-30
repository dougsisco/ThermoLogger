// Main.cpp

#include "tc_common.h"

// ****************************************************************************************************
// GLOBALS

char s200_g[200];

// Indicates that a reset has been requested in this number of seconds.
volatile int ResetRequestSec = 0;
CDataPoint ValuesArr[MAX_SAMPLES];
// Time at boot used to caculate up time. This is set in TimeSetup().
time_t BootTime;
volatile int ValuesArrInd = 0;
std::queue<CDataPoint> SvrDataPointQ;
max31856_cfg_t max31856;
CConfig Config;
//float temp1;
//int Count = 0;
char strftime_buf[64];

#define BUF_SIZE_SKT 300
#define BUF_SIZE (300)
//int sock;
int sockSMTP;
int WifiAuthIndex = 0;
int WifiStatus = 0;
unsigned long long MACAddr;
// Indicates that this unit is being identified.
int DeviceFlags = 0;
int SystemStatusFlag = 0;
int ThermocoupleCurrent = 0;

//E_BLINKTYPE BlinkType = BLINK_OFF;
// This is the low level for dimming the LEDs. When set the leds will dim instead of blink.
//int LEDDimLowLevel = 0;

CStatusLED StatusLED;

// Indicates that the unit is identifying for this number of seconds.
volatile int IdentifySecs = 0;

volatile int Sw1Pressed = 0;
volatile int Sw2Pressed = 0;

volatile bool AllThreadsStopRequest = 0;

// Number of seconds since we have successfully contacted the svr.
long TimeSvrConnectSec = 0;

// Current value of the MCP23017 ports.
int MCP23017CurVal = 0;

//char HostName[20];
long TimeSinceBootMS = 0;
//long TimeSinceLastMsgMS = 0;
// Indicates that the leds should show power on.
//bool PowerBlinkIndictor = 1;

CEventLog EventLog;

bool PauseTemperatureThread = 0;

// This holds the wifi authentication.
typedef struct {
	const char *SSID;
	const char *Pwd;
} s_WifiAuth;

CWifiSTACredentials WifiSTACredentials;

// Configure possible wireless networks here.
// Be sure to change this constant or we will crash.
// Note that this is zero-based, so with one entry use '0'. 
#define WIFIAUTH_LAST_INDEX 1

static void obtain_time(void) {
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	initialize_sntp();

	time_t now = 0;
	struct tm timeinfo = {0,0,0,0,0,1970,0,0,0};

	ESP_LOGI(TAG, "Setting system time..\n");
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	time(&now);
	localtime_r(&now, &timeinfo);
}

#include "lwip/apps/sntp.h"


static void initialize_sntp(void) {
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL); // 6-15-19 dgs $$$ THIS LINE CAUSES THE ASSERTION!
	sntp_setservername(0, (char*)"pool.ntp.org");
    
//    sntp_set_time_sync_notification_cb(TimeSyncNotification);
//    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH); // Added 2-10-20
	sntp_init();
}



static esp_err_t event_handler(void *ctx, system_event_t *event) {
    char s100[100];
    
	switch (event->event_id) {
		case SYSTEM_EVENT_AP_START:
			printf("Event:ESP32 is started in AP mode\n");
			break;
		
		case SYSTEM_EVENT_AP_STACONNECTED:
			xEventGroupSetBits(wifi_event_group, CLIENT_CONNECTED_BIT);
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			xEventGroupSetBits(wifi_event_group, CLIENT_DISCONNECTED_BIT);
			break;		

	case SYSTEM_EVENT_STA_START:
			//printf("event_handler() SYSTEM_EVENT_STA_START\n");
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			//printf("event_handler() SYSTEM_EVENT_STA_GOT_IP\n");
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);  	
			sprintf(s100,"My IP is " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));  	

			//        printf("netmask: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.netmask));
			//        printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
			//        printf("\n");
			fflush(stdout);
			WifiStatus = 1; // $$$ was 1
			break;
    	
		case SYSTEM_EVENT_STA_DISCONNECTED:
			// If wifi was never connected..
			if (WifiStatus == 0) {
				printf("event_handler() Wifi trying to connect to '%s' with pwd '%s'\n", WifiSTACredentials.SSID, WifiSTACredentials.Pwd);
				wifi_config_t sta_config;
				sta_config.sta.bssid_set = false;
    			strcpy((char*) sta_config.sta.ssid, WifiSTACredentials.SSID);
    			strcpy((char*) sta_config.sta.password, WifiSTACredentials.Pwd);

				esp_wifi_set_config(WIFI_IF_STA, &sta_config);
			}

			WifiStatus = 0;
			/* This is a workaround as ESP32 WiFi libs don't currently
				auto-reassociate. */
			//printf("event_handler() esp_wifi_connect\n");
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
    strcpy((char*) sta_config.sta.ssid, WifiSTACredentials.SSID);
    strcpy((char*) sta_config.sta.password, WifiSTACredentials.Pwd);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

	esp_wifi_stop();
	vTaskDelay(pdMS_TO_TICKS(1000));

	int r = esp_wifi_start();
	return r;
}

/**
 ** Determine if time has been obtained.
 ** 
 ** Return 0 if not valid, 1 if valid.
 ***/
int TimeValid() {
    time_t now;

    time(&now);
    
    if (now > 1570000000)
        return 1;
 
    return 0;
}

void TimeSetup(int ForceUpdate) {
	time_t now;
	struct tm timeinfo;

	time(&now);
	localtime_r(&now, &timeinfo);
	// Is time set?
	if(ForceUpdate || timeinfo.tm_year < (2016 - 1900)) {
//		ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
		obtain_time();
		// update 'now' variable with current time
		time(&now);
	}

	// Set timezone to Eastern Standard Time and print local time
	setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
	tzset();
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof (strftime_buf), "%c", &timeinfo);
    char s100[100];
//	sprintf (s100,"The current time is: %s\n", strftime_buf);
//    EventLog.Log(s100, ELogLevel::INFO, 1);
    
    // Set the boot time.
    BootTime = now;
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


/**
 ** Save the current data point to the data array.
 ***/
void SaveDataPointToArray(CDataPoint DataPoint) {
	if (Config.EnableLogging) {
		//printf("Thread_SaveValuesToArray: %u:%u:%u t0=%.1f, t1=%.1f, t2=%.1f, t3=%.1f, t4=%.1f ValuesArrInd=%u\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
		//		  Config.ChannelsAr[0].ValueCurrent,Config.ChannelsAr[1].ValueCurrent,Config.ChannelsAr[2].ValueCurrent,Config.ChannelsAr[3].ValueCurrent,Config.ChannelsAr[4].ValueCurrent, ValuesArrInd);

			// Add the new value to the array.
		for (int i=0; i<NUM_CHANNELS; i++)
			ValuesArr[ValuesArrInd].val[i] = DataPoint.val[i];

		ValuesArr[ValuesArrInd].time = DataPoint.time;

		ValuesArrInd++;
		if (ValuesArrInd >= MAX_SAMPLES)
			ValuesArrInd = 0;	
	} else 
		printf("Logging disabled\n");
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

/**
 * Read the configuration from the VFS.
 * 
 * See https://github.com/espressif/esp-idf/tree/master/examples/storage/nvs_rw_blob
 * 
 * @return 0 on success.
 */
esp_err_t CWifiSTACredentials::Read() {
    printf("Restoring wifi credentials from FLASH..\n");
    esp_err_t err;
    size_t required_size;
    nvs_handle my_handle;
	
    // Open the NVS.
    err = nvs_open("wifi", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
        return 1;
    }
    else {
        required_size = sizeof(CWifiSTACredentials);
        //printf("required_size=%u\n", (int)required_size);
		
        if(required_size > 0) {
            err = nvs_get_blob(my_handle, "wifi", this, &required_size);
            if (err != ESP_OK) {
                printf("Error nvs_get_blob.\n");
            }
        }
    }	
    nvs_close(my_handle);
    
    if (err == 0)
        printf("Wifi credentials restored: SSID='%s', PWD='%s'\n", WifiSTACredentials.SSID, WifiSTACredentials.Pwd);
			
    	return err;
}

/**
 * Write the configuration to the VFS.
 * 
 * @return 0 on success.
 */
esp_err_t CWifiSTACredentials::Write() {
    esp_err_t err;
    size_t required_size;
    nvs_handle my_handle;

    printf("Saving wifi credentials to flash.\n");
	
    // Open the NVS.
    err = nvs_open("wifi", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return 1;
    } 
    
    // Write the config to the VFS.
    required_size = sizeof(CWifiSTACredentials);
    err = nvs_set_blob(my_handle, "wifi", this, required_size);

    if (err != ESP_OK) 
        printf("Error in nvs_set_blob.\n");

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
        printf("NVS commit error.\n");

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
 * Parse an html header string.
 * 
 * @param buf
 * @param buflen
 * @param StrArr
 * @param StrMaxlen
 */
///
int Parse(char *buf, int buflen, string StrArr[], int StrMaxlen) {
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
	return i;
}

// for string delimiter
vector<string> split(string s, string delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;
	vector<string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}



//void Thread_BatteryTest() {
//	adc1_config_width(ADC_WIDTH_9Bit);
//	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
//
//	// https://esp-idf.readthedocs.io/en/v2.0/api/peripherals/dac.html
//	// For curve fitting see https://mycurvefit.com/
//	while (1) {
//		float val = adc1_get_raw(ADC1_CHANNEL_0); // dgs: was adc1_get_voltage
////		ClientStats.BatteryVoltage = (0.00951 * val + 1.263)*10;
//		//printf("Battery=%d\n",ClientStats.BatteryVoltage);
//		//dac_out_voltage(DAC_CHANNEL_1, val/2);
//		delay(1000);
//	}
//}

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

void InitGPIO() {

#if !DEBUGGER
    // Init LED outputs
    LEDPwmInit();
//	gpio_pad_select_gpio(LEDR_GPIO_NUM);
//	gpio_set_direction(LEDR_GPIO_NUM, GPIO_MODE_OUTPUT);
//	gpio_set_level(LEDR_GPIO_NUM, 0);
//	
//	gpio_pad_select_gpio(LEDG_GPIO_NUM);
//	gpio_set_direction(LEDG_GPIO_NUM, GPIO_MODE_OUTPUT);
//	gpio_set_level(LEDG_GPIO_NUM, 1);
//	
//	gpio_pad_select_gpio(LEDB_GPIO_NUM);
//	gpio_set_direction(LEDB_GPIO_NUM, GPIO_MODE_OUTPUT);
//	gpio_set_level(LEDB_GPIO_NUM, 1);
#endif
    // Configure pushbutton switch I/O.
    // Note that GPIO pins 34-39 are input-only. These pins do not feature an output 
    //   driver or internal pull-up/pull-down circuitry.
    gpio_pad_select_gpio(SW1_GPIO_NUM);
    gpio_set_direction(SW1_GPIO_NUM, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SW1_GPIO_NUM, GPIO_PULLUP_ONLY);
    
    gpio_pad_select_gpio(SW2_GPIO_NUM);
    gpio_set_direction(SW2_GPIO_NUM, GPIO_MODE_INPUT); 
    gpio_set_pull_mode(SW2_GPIO_NUM, GPIO_PULLUP_ONLY);
}

const char* SYS_STATUS_WARNING_S = "WARNING";
const char* SYS_STATUS_ERROR_S = "ERROR";
const char* SYS_STATUS_OK_S = "OK";

void CStatusLED::SetStatus(int status, string DetailStr) {  
    char s200[200];
    const char *StatusStrPtr;
        
    if (status == SYS_STATUS_WARNING) {
        StatusStrPtr = SYS_STATUS_WARNING_S;
        SetColor(YELLOW);
        }
    else if (status == SYS_STATUS_ERR) {
        StatusStrPtr = SYS_STATUS_ERROR_S;
        SetColor(RED);
    }
	else {
    	StatusStrPtr = SYS_STATUS_OK_S;
    	SetColor(GREEN);
	}
	
    if (status != SystemStatusFlag) {
        if (DetailStr.length() > 0 && DetailStr.length() < 120)
            sprintf(s200, "System status changed to %s: %s", StatusStrPtr,DetailStr.c_str() );
        else
            sprintf(s200, "System status changed to %s", StatusStrPtr);
        EventLog.Log(s200);
        
	    SystemStatusFlag = status;
    }     
}

void CStatusLED::Blank() {
    SetLEDParms(0);
    Blanked = 1;  
}

void CStatusLED::Restore() {
    SetColor(LEDColor);
    Blanked = 0;
}

void CStatusLED::SetColor(int color) {
    if (LEDColor != color)
        // Prevent the next call to Invert() from blanking.
        Blanked = 1;  

    LEDColor = color;
    SetLEDParms(LEDColor);
}

void CStatusLED::Invert() {
    if (Blanked)
        Restore();
    else
        Blank();
}

#if 0
// 3-27-22 NOT SURE WHY THERE IS A DUPLICATE OF THESE TWO METHODS.
/**
 * Read the configuration from the VFS.
 * 
 * See https://github.com/espressif/esp-idf/tree/master/examples/storage/nvs_rw_blob
 * 
 * @return 0 on success.
 */
esp_err_t CWifiSTACredentials::Read() {
	printf("Restoring wifi credentials from FLASH..\n");
	esp_err_t err;
	size_t required_size;
	nvs_handle my_handle;
	
	// Open the NVS.
	err = nvs_open("wifi", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
        return 1;
    }
	else {
    	required_size = sizeof(CWifiSTACredentials);
		//printf("required_size=%u\n", (int)required_size);
		
		if (required_size > 0) {
    		err = nvs_get_blob(my_handle, "wifi", this, &required_size);
			if (err != ESP_OK) {
				printf("Error nvs_get_blob.\n");
			}
		}
	}	
	nvs_close(my_handle);
    
    if (err == 0)
        printf("Wifi credentials restored: SSID='%s', PWD='%s'\n", WifiSTACredentials.SSID, WifiSTACredentials.Pwd);
		
	// Test all config values for ranges.
//	if ( Config.CheckValues() )
//		err = ESP_FAIL;
	
	return err;
}

/**
 * Write the configuration to the VFS.
 * 
 * @return 0 on success.
 */
esp_err_t CWifiSTACredentials::Write() {
	esp_err_t err;
	size_t required_size;
	nvs_handle my_handle;

    printf("Saving wifi credentials to flash.\n");
	
	// Open the NVS.
	err = nvs_open("wifi", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return 1;
    } 
    
	// Write the config to the VFS.
	required_size = sizeof(CWifiSTACredentials);
    err = nvs_set_blob(my_handle, "wifi", this, required_size);

	if (err != ESP_OK) 
		printf("Error in nvs_set_blob.\n");

	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) 
		printf("NVS commit error.\n");

	nvs_close(my_handle);
	
	return err;
}
#endif

/**
 * Select a thermocouple input.
 * 
 * @param n
 */
void ThermocoupleSelect(int n) {
    MCP23017CurVal = (MCP23017CurVal & ~0xe000) | (n << 13);
    MCP23017WriteBoth(MCP23017CurVal);
}


/**
 * Light or extinguish the given channel LED.
 *
 */
void ChannelLED(int LEDNum, int Enable) {
    if (LEDNum > 11 || LEDNum < 0)
        return;
    
    if (Enable)
        MCP23017CurVal = (MCP23017CurVal & ~(1 << LEDNum));
    else
        MCP23017CurVal = (MCP23017CurVal |  (1 << LEDNum));
    
    MCP23017WriteBoth(MCP23017CurVal);
}

///
/*		
 * Get a sample given the offset from the current index.
 * 
 * @param offset
 * @param Sample
 * @return -1 on error.
 */
int GetSample(int offset,CDataPoint &Sample) {
	if (offset < 0 || offset > MAX_SAMPLES-1)
		return -1;
	
	int i = ValuesArrInd - offset - 1;
	
	if (i < 0)
		i += MAX_SAMPLES;
	
	Sample = ValuesArr[i];
	return 0;
}

	
char *Trim(char *str) {
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	if (*str == 0)  // All spaces?
	  return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}

// *******************************************************************************************
// THIS IS THE AP CODE
static void start_dhcp_server() {
    
	// initialize the tcp stack
	tcpip_adapter_init();
	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1); //ESP acts as router, so gw addr will be its own addr
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	// start the DHCP server   
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	printf("DHCP server started \n");
}
static void initialise_wifi_in_ap(void)
{
	esp_log_level_set("wifi", ESP_LOG_NONE);  // disable wifi driver logging
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	// configure the wifi connection and start the interface
	wifi_config_t ap_config;
	
	ap_config.ap.ssid_len = 0;
	ap_config.ap.channel = 0;
	ap_config.ap.authmode = AP_AUTHMODE;
	ap_config.ap.ssid_hidden = 0;
	ap_config.ap.max_connection = AP_MAX_CONNECTIONS;
	ap_config.ap.beacon_interval = AP_BEACON_INTERVAL;
	strcpy((char*)ap_config.ap.ssid, AP_SSID);
	strcpy((char*)ap_config.ap.password,AP_PASSPHARSE);
	
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("ESP WiFi started in AP mode \n");
}

// print the list of connected stations
void printStationList() 
{
	printf(" Connected stations:\n");
	printf("--------------------------------------------------\n");
	
	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
   
	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));	
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));
	
	for (int i = 0; i < adapter_sta_list.num; i++) {
		
		tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
		printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n",
			i + 1,
			station.mac[0],
			station.mac[1],
			station.mac[2],
			station.mac[3],
			station.mac[4],
			station.mac[5],
			ip4addr_ntoa(&(station.ip)));
	}
	
	printf("\n");
}

void print_sta_info(void *pvParam) {
	printf("print_sta_info task started \n");
	while (1) {	
		EventBits_t staBits = xEventGroupWaitBits(wifi_event_group, CLIENT_CONNECTED_BIT | CLIENT_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		if ((staBits & CLIENT_CONNECTED_BIT) != 0) printf("New station connected\n\n");
		else printf("A station disconnected\n\n");
		printStationList();
	}
}

std::string str_toupper(std::string str) {
    std::string loc;
    for (std::string::size_type i = 0; i < str.length(); ++i)
        loc += std::toupper(str[i]);
    return loc;
}

/**
 ** Update the time once per hour.
 ***/
void Thread_UpdateTime(void *pvParameter) {
    while (1) {
        // Delay for 1 hr
        vTaskDelay(pdMS_TO_TICKS(60 * 60  * 1000));	
        TimeSetup(1);
    }
}

void PrintCurrentTime() {
    struct tm timeinfo;
    time_t now = time(0);
    
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    printf("%s\n", strftime_buf);    
}

/**
 ** If we have not been able to contact the server for 10 minutes then reboot.
 ** Once per day set the time.
 ***/
void Thread_OncePerSecond(void *pvParameters) {   
    int Seconds = 0;
    char s200[200];
    bool LoggedErrorState = 0;
    bool Sw1PressedLogged = 0;
    
    while (1) {      
        // If we are in an error state because of a WIFI error (or any other connection error)..
        //    Wait a little while and then reboot. hopefully this will resolve the problem.
        if(SystemStatusFlag == SYS_STATUS_ERR) {
            sprintf(s200,"Rebooting in %lu sec\n", ERROR_REBOOT_TIME_SEC-TimeSvrConnectSec);
            if (!LoggedErrorState) {
                LoggedErrorState = 1;
                EventLog.Log(s200,ELogLevel::CRITICAL, 1);
            }
            // If time to reboot..
            if (TimeSvrConnectSec++ > ERROR_REBOOT_TIME_SEC) {
                printf("\n\n**** REBOOTING!\n\n");
                EventLog.Log("REBOOTING NOW..", ELogLevel::CRITICAL, 1);
                esp_restart();
            }
        } else {
            if (TimeSvrConnectSec > 0) {
                TimeSvrConnectSec = 0;
                EventLog.Log("Reboot cancelled.");
            }
        }

        // Handle a reset request.
        if (ResetRequestSec > 0) {
            if (--ResetRequestSec == 0) {
                EventLog.Log("Rebooting now!");
                esp_restart();
            }
        }
        
        if (Seconds++ == 60) {
            Seconds = 0;
            
 //            PrintCurrentTime();
        }
        
        if (IdentifySecs > 0) {
            if (--IdentifySecs == 0) {
                EventLog.Log("Identify OFF");
                StatusLED.SetStatus(SYS_STATUS_OK);
            }                
        }
        
        if (Sw1Pressed) {
            // 5.6.2 Added to prevent multiple log entries.
            if (!Sw1PressedLogged) {
                Sw1PressedLogged = 1;
            
                EventLog.Log("Sw1 Pressed", ELogLevel::INFO, 1);
                Sw1Pressed = 0;          
            }
        }
        else {
            // If released..
            Sw1Pressed = 0;
        }
        
        if (Sw2Pressed) {
            Sw2Pressed = 0;          
            
            //EventLog.Log("Sw2 Pressed", ELogLevel::INFO);
        }            
        // Delay 1 second.
        vTaskDelay( pdMS_TO_TICKS(1000) );	
    }
}

/**
 ** This thread polls the switches.
 ** 
 ** NOTE that I tried to get the GPIO interrupt to work but I get spurious interrupts.
 ***/
void Thread_SwitchPoller(void *pvParameters) { 
    while (1) {
        if ( !gpio_get_level(SW1_GPIO_NUM) )
            Sw1Pressed = 1;
        if ( !gpio_get_level(SW2_GPIO_NUM) )
            Sw2Pressed = 1;
        
        // Delay 1 second.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 ** Get the Json string for this entry.
 ** 
*/
string CEventLogEntry::Json() {
    char s200[200];
    
    string JsonStr = "[";
    sprintf(s200, "%lu,%u,", (long)time, (int)level);
    JsonStr += s200;
    JsonStr += "\"" + text + "\"";
    JsonStr += "]";
    return JsonStr;
}

void CEventLog::Log(std::string text, ELogLevel Level, bool Print) {
    CEventLogEntry EventLogEntry;
    
    // Remove all \n chars.
    std::replace(text.begin(), text.end(), '\n', ' ');
    
    if (Level >= CurLevel) {
        EventLogEntry.text = text;
        time(&EventLogEntry.time);
        EventLogEntry.level = Level;
    
        // Push the data point to the server queue.
        if(EventLogQueue.size() < LOG_QUEUE_SIZE) {
            printf("CEventLog::Log() EventLogQueue.size=%u\n", EventLogQueue.size());
            EventLogQueue.push(EventLogEntry);
        }
        
        if (Print || PRINT_ALL_LOGS) {
            printf("**Log():%s\n", text.c_str());
            fflush(stdout);
        }
    }
}

/**
 ** Get the JSON string for all log entries.
 ** 
 ** Don't remove the entries from the queue; we will do this later!
 ***/
string CEventLog::Json() {
    string JsonStr;
    
    if (EventLogQueue.size() <= 0)
        return JsonStr;
    
    JsonStr += "\"log\":[";

    CEventLogEntry Event;
    
    // 5.6 Changed 'while' to 'if' so that we only get ONE log event. Apparently 
    //    there is a json encoding/decoding problem so that we can only send one event at a time.
    if (EventLogQueue.size() > 0) {
        // Get a data point from the queue.
        Event = EventLogQueue.front();
        
        EventLogQueue.pop();
        JsonStr += Event.Json() + ",";
    }
    // Remove the last comma from the string.
    JsonStr.pop_back();
    
    JsonStr += "]";
    
    string s = string("CEventLog::Json=") + JsonStr;
    printf(s.c_str());
    
    return JsonStr;
}

void EventLogLog(char *s) {
    EventLog.Log(s);    
}
    
void app_main(void) {
    char s200[200];    
    
    // Disable the annoying ESP-IDF messages. (or set to ESP_LOG_INFO)
    esp_log_level_set("*", ESP_LOG_NONE);
    
	    
    sprintf(s200, "**** Boot: Ver %s %s ****", VERSION, COPYRIGHT);
    EventLog.Log(s200, ELogLevel::INFO, 1);

    sprintf(s200, "Webserver:%s, WebURL:%s", WEB_SERVER, WEB_URL);
    EventLog.Log(s200, ELogLevel::INFO, 1);
    
    printf("\nESP-IDF ver: %s\n", esp_get_idf_version());
    printf("DEBUGGER:%u\n"
    	"PRINT_TEMPERATURE_VALUES:%u\n"
    	"PRINT_SENSOR_ERRORS:%u\n"
    	"PRINT_ONEWIRE_RESET_PULSE:%u\n"
    	"PRINT_CURRENT_DATAPOINT_TIME:%u\n"
        "PRINT_WEB_CONN_INFO:%u\n"
        "PRINT_HTML_HTTP_REQUEST:%u\n",
        DEBUGGER,
        PRINT_TEMPERATURE_VALUES,
        PRINT_SENSOR_ERRORS,
        PRINT_ONEWIRE_RESET_PULSE,
        PRINT_CURRENT_DATAPOINT_TIME,
        PRINT_WEB_CONN_INFO,
        PRINT_HTML_HTTP_REQUEST);

    // Initialize NVS and read the WIFI credentials.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        printf("Erasing NVS..\n");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Read the wifi SSID and password from NVM.
    WifiSTACredentials.Read();
    
// This is a patch to allow older versions to store the given wifi credentials to NVM.    
#ifdef WIFI_SSID_TEMP
    strcpy(WifiSTACredentials.SSID, WIFI_SSID_TEMP);
    strcpy(WifiSTACredentials.Pwd, WIFI_PWD_TEMP);
    WifiSTACredentials.Write();
#endif
    // Init the GPIO.
    InitGPIO();
    
    // If SW1 is pressed OR the SSID has not been initialized..
    // NOTE that the GPIO is different for the older board.
    if(!gpio_get_level(SW1_GPIO_NUM) || strcmp(WifiSTACredentials.SSID, SSID_NOT_INITIALIZED)==0 ) {
        printf("SW1 is pressed, waiting for release within 5 seconds..\n");
        StatusLED.SetColor(RED);
        
        for (int i=0; i<100; i++) {
            // If SW1 released..
            if(gpio_get_level(SW1_GPIO_NUM)) {
                printf("-->>Entering Captive Portal mode.\n");
                // Go into 'Captive Portal' mode to get the wifi credentials.
                // NOTE that this function performs a hardware reset so it does not return.
                CaptivePortal(); 
            }
            DelayMS(50);
            StatusLED.Invert();
        }
        // SW1 was not released within 5 seconds. This could mean that the pullup resistor is not installed.
        printf("SW1 was not released within 5 seconds.. continuing with normal operation.\n");
    }
    
    xTaskCreatePinnedToCore((TaskFunction_t) &Thread_SwitchPoller, "Thread_SwitchPoller", 2048, NULL, 1, NULL, 0);
     
//    LEDColorTest();
    
    // Set status led
	StatusLED.SetColor(YELLOW);
         
    // Init the MCP23017 I2C interface.
    esp_err_t ret = mcp23017_init(I2C_SDA, I2C_SCL);
    // Turn all LEDs off.
    MCP23017CurVal = 0xffff;
    MCP23017WriteBoth(MCP23017CurVal);
    
    // Disable brownout detector.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
	
    // Init the values array.
    for(int i = 0 ; i < MAX_SAMPLES ; i++)
    	ValuesArr[i].time = 0;
	
    //*******************************************************************
    // IMPORTANT: THE FOLLOWING NEEDS TO BE DONE AFTER Config.Init()!
    // Init the DS18B20 sensors.
    Config.ChannelsAr[0].SetChannel18b20((gpio_num_t)25);
    Config.ChannelsAr[1].SetChannel18b20((gpio_num_t)26);
    Config.ChannelsAr[2].SetChannel18b20((gpio_num_t)5);
    Config.ChannelsAr[3].SetChannel18b20((gpio_num_t)19);
    Config.ChannelsAr[4].SetChannelMAX31856(0);
    Config.ChannelsAr[5].SetChannelMAX31856(1);
    Config.ChannelsAr[6].SetChannelMAX31856(2);
    Config.ChannelsAr[7].SetChannelMAX31856(3);
    Config.ChannelsAr[8].SetChannelMAX31856(4);
    Config.ChannelsAr[9].SetChannel18b20((gpio_num_t)18);
    Config.ChannelsAr[10].SetChannel18b20((gpio_num_t)21);
    Config.ChannelsAr[11].SetChannel18b20((gpio_num_t)22);
    //*******************************************************************

    // LED test
    for(int i = 0 ; i < 12 ; i++) {
        ChannelLED(i, 1);
        vTaskDelay(pdMS_TO_TICKS(50));	
    } 
    // LED test
    for(int i = 0 ; i < 12 ; i++) {
        ChannelLED(i, 1);
        vTaskDelay(pdMS_TO_TICKS(50));	
    }  
    
    // Leave all LEDs on for 1 second.
    vTaskDelay(pdMS_TO_TICKS(1000));	
    
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
#if 0
    	int count = 0;
    while (1) {
        float v1 = Config.ChannelsAr[0].T18b20.ds18b20_get_temp();
        float v2 = Config.ChannelsAr[1].T18b20.ds18b20_get_temp();
        float v3 = Config.ChannelsAr[2].T18b20.ds18b20_get_temp();
        float v4 = Config.ChannelsAr[3].T18b20.ds18b20_get_temp();
        float v5 = Config.ChannelsAr[9].T18b20.ds18b20_get_temp();
        float v6 = Config.ChannelsAr[10].T18b20.ds18b20_get_temp();
        float v7 = Config.ChannelsAr[11].T18b20.ds18b20_get_temp();
		
        printf("%u %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n", count++, v1, v2, v3, v4, v5, v6, v7);
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
#endif	 
		
    // This is extra code which really does nothing except help to fix the 
    // intermittent wifi connection problem.
    // If wifi doesn't connect then either enable or disable this bit of code.
#ifdef FIX_WIFI_CONNECTION_PROBLEM
    	vTaskDelay(pdMS_TO_TICKS(200));
#endif

    // Start reading the temperature sensors. This thread has the highest priority 
    // since it uses precise delays to read the DS18B20 sensors. It was found that 
    // a low priority results in more checksum errors.
    // 4-25-21 Changed usStackDepth from 2048 to 4096 after stack overflow error (after I added some code).
    // 4-23-20 Lowered to 5 but then got 18B20 checksum errors so I raised it again to configMAX_PRIORITIES.
    // 5-3-19 I lowered this from 8 to 5 because the time was getting way behind.
    // 10-6-19 Moved this to BEFORE initialise_wifi() so the sensor LEDs will indicate valid sensors right away. 
    xTaskCreate(&Thread_ReadTemperature, "Thread_ReadTemperature", 4096, NULL, configMAX_PRIORITIES, NULL);

    // Init the wifi connection.
    if(initialise_wifi())
    	printf("ERROR INITING WIFI!!\n");
	
    // Read the MAC address.
    uint8_t mac[6];
    system_efuse_read_mac(mac);
	
//    sprintf(s200, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//    EventLog.Log(s200, ELogLevel::INFO, 1);

    MACAddr = ((long long)mac[0] << 40) + ((long long)mac[1] << 32) + ((long long)mac[2] << 24)
    	+ ((long long)mac[3] << 16) + ((long long)mac[4] << 8) + (long long)mac[5]; 
	
    // Set the host name.
    // Don't put a space in the hostname or it won't update!
    // If done properly the router will show the new hostname immediately.
    //	string s(Config.HostName);
    char s100[100], s120[120];
    sprintf(s100, "CloudLogger_%02X:%02X", mac[4], mac[5]);    
//    sprintf(s120, "My HostName is %s.\n", s100);
//    EventLog.Log(s120, ELogLevel::INFO, 1);
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, s100);

    // I'm not sure why, but without this we get watchdog timer errors.
    // Wait here until wifi is connected.
//    printf("Connecting to WIFI with SSID '%s' and pwd '%s'..\n", WifiSTACredentials.);
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    // Set low power wifi mode. This alternates between 50mA and 100mA.
    // **** MAYBE WAIT UNTIL WE ARE CONNECTED BEFORE DOING THIS??? ****
    // See https://github.com/espressif/esp-idf/blob/master/examples/wifi/power_save/main/power_save.c
    //	esp_wifi_set_ps(WIFI_PS_MODEM);

    // see http://esp32.info/docs/esp_idf/html/dd/d3c/group__xTaskCreate.html
    //	xTaskCreatePinnedToCore((TaskFunction_t) & Thread_BatteryTest, "Thread_BatteryTest", 2048, NULL, 1, NULL, 0);
    //	xTaskCreatePinnedToCore((TaskFunction_t) & Thread_Beep, "Thread_Beep", 2048, NULL, 1, NULL, 0);
    // xTaskCreate(&http_server, "http_server", 15000, NULL, 5, NULL);

    // Get the current time and set up the time server.
    printf("Configuring the time..\n");
    TimeSetup(1);

    // Set status led
    StatusLED.SetStatus(SYS_STATUS_OK);
	
    xTaskCreate(&Thread_OncePerSecond, "Thread_OncePerSecond", 4096, NULL, 1, NULL); // 7-18-21 Increased from 2048 to 4096.
    xTaskCreate(&Thread_SendDataToSvr, "Thread_SendDataToSvr", 4096, NULL, 5, NULL);
    xTaskCreate(&FirmwareUpgradeTask,  "FirmwareUpgradeTask",  8192, NULL, 5, NULL);

    // ******************************************************************************************************************

#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

void FirmwareUpgradeTask(void * pvParameter) {
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true,portMAX_DELAY);
    
    esp_http_client_config_t config;
    config.url = FIRMWARE_UPGRADE_URL;
    config.cert_pem = (char*)0;
    config.event_handler = _http_event_handler;
    config.buffer_size = 512; // dgs - added to fix bug: this was not init'd and caused the upgrade to fail.
    // config.port dgs - See note about FIRMWARE_UPGRADE_PORT in tc_common .h
    config.disable_auto_redirect = 1;

    #ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
    #endif
    
    while (1) {
        if (FirmwareUpgradeInProcess) {
            // NOTE: we must use s200_g here.. if we use a local var then we get a crash during the ota.
            sprintf(s200_g,"Upgrading firmware from %s", FIRMWARE_UPGRADE_URL);
            EventLog.Log(s200_g);
            
            printf("FirmwareUpgradeTask 2\n");
            
            // Pause the ReadTemperature thread.
            PauseTemperatureThread = 1;
            
            StatusLED.SetStatus(SYS_STATUS_WARNING, "Updating firmware");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            
            esp_err_t ret = esp_https_ota(&config);
            if (ret == ESP_OK) {
                ResetRequestSec = REBOOT_DELAY_TIME;
                while (1) {
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }
            else {
                StatusLED.SetStatus(SYS_STATUS_ERR, "Error upgrading firmware");
                EventLog.Log("Firmware upgrade failed.");
                FirmwareUpgradeInProcess = 0;
                PauseTemperatureThread = 0;
                }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
}

/**
 * Init the pulse-width-modulation timers for the LEDs.
 */
void LEDPwmInit(void) {
    //	gpio_pad_select_gpio(LEDR_GPIO_NUM);
    //	gpio_set_direction(LEDR_GPIO_NUM, GPIO_MODE_OUTPUT);
    //	gpio_set_level(LEDR_GPIO_NUM, 0);
    //	
    //	gpio_pad_select_gpio(LEDG_GPIO_NUM);
    //	gpio_set_direction(LEDG_GPIO_NUM, GPIO_MODE_OUTPUT);
    //	gpio_set_level(LEDG_GPIO_NUM, 1);
    //	
    //	gpio_pad_select_gpio(LEDB_GPIO_NUM);
    //	gpio_set_direction(LEDB_GPIO_NUM, GPIO_MODE_OUTPUT);
    //	gpio_set_level(LEDB_GPIO_NUM, 1);    
    ledc_channel_config_t ledc_channel_red = { 0 }, ledc_channel_green = { 0 }, ledc_channel_blue = { 0 };
    
    ledc_channel_red.gpio_num = LEDR_GPIO_NUM;
    ledc_channel_red.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_red.channel = LEDC_CHANNEL_1;
    ledc_channel_red.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_red.timer_sel = LEDC_TIMER_1;
    ledc_channel_red.duty = 0;

    ledc_channel_green.gpio_num = LEDG_GPIO_NUM;
    ledc_channel_green.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_green.channel = LEDC_CHANNEL_2;
    ledc_channel_green.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_green.timer_sel = LEDC_TIMER_1;
    ledc_channel_green.duty = 0;

    ledc_channel_blue.gpio_num = LEDB_GPIO_NUM;
    ledc_channel_blue.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_blue.channel = LEDC_CHANNEL_3;
    ledc_channel_blue.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_blue.timer_sel = LEDC_TIMER_1;
    ledc_channel_blue.duty = 0;

    ledc_timer_config_t ledc_timer = { (ledc_mode_t) 0 };
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.bit_num = LEDC_TIMER_10_BIT;
    ledc_timer.timer_num = LEDC_TIMER_1;
    ledc_timer.freq_hz = 25000;

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_blue));
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

/**
 * Set the LED color and intensity.
 * 
 * @param color 0xrrggbb, each is a %
 * @param intensity %
 * @param period %
 * @param blinkcount -1 for continuous
 * @param BlinkMode OFF, ON_OFF, ROTATE_CW, ROTATE_CCW, FADE
 * @param LLDimLevel, fade down to this level
 */
void SetLEDParms(int color, int intensity, int onperiod, int offperiod) {
//    printf("SetLEDParms: color=%d intensity=%d\n",color,intensity);
    
    //printf("SetLEDParms blinkcount=%u BlinkMode=%u onperiod=%u offperiod=%u\n", blinkcount, BlinkMode, onperiod, offperiod);

    intensity = intensity > 100 ? 100 : intensity;
    intensity = intensity < 0 ? 0 : intensity;
    intensity = (intensity * 2) / 5;
    
//    CurrentColor_g = color;

    int r = (((color >> 16) & 0xff) * intensity) / 100;
    int g = (((color >> 8) & 0xff) * intensity) / 100;
    int b = (((color) & 0xff) * intensity) / 100;
    r = r > 100 ? 100 : r;
    g = g > 100 ? 100 : g;
    b = b > 100 ? 100 : b;

    uint32_t max_duty = (1 << LEDC_TIMER_10_BIT) - 1;
    uint32_t RedDutyi = (r * (float) max_duty) / 100.0;
    uint32_t GreenDutyi = (g * (float) max_duty) / 100.0;
    uint32_t BlueDutyi = (b * (float) max_duty) / 100.0;

    // The duty cycle must be inverted because the LED outputs are common-VCC.
    RedDutyi = max_duty - RedDutyi;
    GreenDutyi = max_duty - GreenDutyi;
    BlueDutyi = max_duty - BlueDutyi;

    //printf("r=%d RedDutyi=%d max_duty=%d\n",r,RedDutyi,max_duty);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, RedDutyi));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, GreenDutyi));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, BlueDutyi));
	
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1));
	
    // Set other LED parms.
//    OnPeriod = onperiod;
//    OffPeriod = offperiod;
//    BlinkCount = blinkcount;
//    LEDDimLowLevel = LLDimLevel;
//    BlinkType = BlinkMode;
//    LED_SetAll(1);
}

//void Thread_LEDUpdate() {
//    int Period = 0;
//    bool LedIsOn = 1;
//    static int LedNum = 0;
//    static int LEDDimLevel = 0;
//    static bool LEDDimDirUp = 0;
//    static int LEDInc = 1;
//
//    while (1) {
//        //if (TimeSinceBootMS % 1000 == 0)
//        	//printf("LEDUpdate BlinkType=%u BlinkCount=%u OnPeriod=%u OffPeriod=%u\n", BlinkType, BlinkCount, OnPeriod, OffPeriod);
//
//        // Blink mechanism is active only if OnPeriod, OffPeriod and BlinkCount are set.
//        // BlinkCount < 0 -> blink forever
//        if(OnPeriod > 0 && OffPeriod > 0 && BlinkCount != 0) {
//            if (Period-- <= 0) {
//                if (LedIsOn)
//                    Period = OffPeriod;
//                else
//                    Period = OnPeriod;
//				
//                switch (BlinkType) {
//                    // Blink all LEDs.
//                case ON_OFF :
//                	if(LedIsOn) {
//                        BlinkCount--;
//                        // If blink is over then restore the color.
//                        if(BlinkCount == 0)
//                        	SetLEDParms(ColorBeforeBlink, 100);
//                        // If we should blink forever then continue 
//                        // blinking forever..
//                        if(BlinkCount < 0)
//                        	BlinkCount = -1;
//                        LedIsOn = 0;
//                        LED_SetAll(0);
//                        //printf("0");
//                    } else {
//                        LedIsOn = 1;
//                        LED_SetAll(1);
//                        //printf("1");
//                    }
//                    break;
//
//                    // Blink clockwise.
//                case ROTATE_CW :
//                case ROTATE_CCW :
//                		BlinkCount--;
//                    // All leds off.
//                    LED_SetAll(0);
//                    // Light the next led.
//                    gpio_set_level((gpio_num_t) LedGPIO[LedNum], 1);
//
//                    // If blink is over then restore the color.
//                    if(BlinkCount == 0) {
//                        SetLEDParms(ColorBeforeBlink, 100);
//                        LED_SetAll(1);
//                        LedNum = 0;
//                    }
//                    // If we should blink forever then continue 
//                    // blinking forever..
//                    if(BlinkCount < 0)
//                    	BlinkCount = -1;
//
//                    if (BlinkType == ROTATE_CW) {
//                        if (++LedNum > 7)
//                            LedNum = 0;
//                    }
//                    else {
//                        if (--LedNum < 0)
//                            LedNum = 7;
//                    }
//                    break;
//
//                    // Fade
//                case FADE :
//                	LEDInc = 1;
//                    if (LEDDimLevel > 20)
//                        LEDInc = 2;
//                    if (LEDDimLevel > 40)
//                        LEDInc = 4;
//                    if (LEDDimLevel > 60)
//                        LEDInc = 4;
//                    if (LEDDimLevel > 70)
//                        LEDInc = 2;
//                    if (LEDDimLevel > 90)
//                        LEDInc = 1;
//
//                    if (LEDDimDirUp) {
//                        LEDDimLevel += LEDInc;
//                        if (LEDDimLevel > 99) {
//                            LEDDimDirUp = 0;
//                        }
//                    }
//                    else {
//                        LEDDimLevel -= LEDInc;
//                        if (LEDDimLevel <= LEDDimLowLevel) {
//                            LEDDimDirUp = 1;
//                        }
//                    }
//                    SetLEDParms(CurrentColor_g, LEDDimLevel, OnPeriod, OffPeriod, -1, FADE, LEDDimLowLevel);
//                    break;
//
//                default:
//                    break;
//                }
//                ;
//            }
//
//            // If this is an off period then disable the LEDs.
//            if(!LedIsOn) {
//                LEDR_OFF;
//                LEDG_OFF;
//                LEDB_OFF;
//            }
//        } else {
//            LED_SetAll(1);
//            LedIsOn = 1;
//        }
//
//#if 0        
//        // If time to service the tricolor LED.
//        if(LedIsOn && LEDServiceDelay++ == 20) {
//            LEDServiceDelay = 0;
//
//            if (LED_R_Count0 > 0)
//                LEDR_ON;
//            if (LED_G_Count0 > 0)
//                LEDG_ON;
//            if (LED_B_Count0 > 0)
//                LEDB_ON;
//            LEDRCount = LED_R_Count0;
//            LEDGCount = LED_G_Count0;
//            LEDBCount = LED_B_Count0;
//        }
//
//        if (LEDRCount > 0)
//            LEDRCount--;
//        else
//            LEDR_OFF;
//
//        if (LEDGCount > 0)
//            LEDGCount--;
//        else
//            LEDG_OFF;
//
//        if (LEDBCount > 0)
//            LEDBCount--;
//        else
//            LEDB_OFF;
//#endif      
//        // 10ms delay
//        vTaskDelay(1);
//    }
//}

string GetConsoleString(string &s) {    
    char c;
    do {
        c = getchar();
        if (c == 0x7f || (c != 0xff && isprint(c))) {
            if (c != 0x7f)
                s.append(1, c);
            printf("%c", c);
        }
        if (c == 0x7f) 
            s.pop_back();
        vTaskDelay(pdMS_TO_TICKS(10));	
    } while (c != 10);
    printf("\n");
    
    return s;
}

void LEDColorTest() {
    string s;
    int color, intensity;
    while (1) {
        s.clear();
        GetConsoleString(s);
         
        sscanf(s.c_str(), "%x %u", &color, &intensity);
        printf("Color=0x%X, Intensity=%d\n", color, intensity);
        SetLEDParms(color, intensity, 100, 0);
    }
}
