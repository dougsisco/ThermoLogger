/**
 ** captportal.cpp
 ** 
 ** 8-11-21 Changes by Doug Sisco.
 **
 ***/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42): Idea of Jeroen Domburg <jeroen@spritesmods.com>
 *
 * Cornelis wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */
// https://www.reddit.com/r/esp32/comments/9oqpnv/device_setup_with_captive_portal_in_espidf/

// Scanning came from https://github.com/espressif/esp-idf/issues/3252

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "string.h"
#include "cJSON.h"
#include "lwip/dns.h"
#include "captdns.h"

#include "../main/tc_common.h"

bool replace(std::string& str, const std::string& from, const std::string& to);
void ParsePOST(string &str, vector<string> &PostData);
    
static void wifi_scan(void);
static void test_wifi_scan_all();

void ParsePOST(string &str, vector<string> &out);

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS))

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_AP_init(void);

static void LED_Blink(void *pvParameters);

#define DelayMS(ms) (vTaskDelay(ms/portTICK_RATE_MS));

string urlDecode(string str);

//static EventGroupHandle_t wifi_event_group;

//const int CONNECTED_BIT = BIT0;

char* json_unformatted;

string AP_List_str;

const static char http_hdrs[] = "HTTP/1.1 200 OK\r\n"
    "Content-type: text/html\r\n"
    "enctype:text / plain\r\n" 
    "Accept-Encoding:*;q=0\r\n"
    "Connection: close\r\n\r\n";

const static char http_index2[] = "<!DOCTYPE html>"
      "<body>\n"
      "<h2>SiscoCircuits CloudLogger</h2>\n"
      "<h4>The unit will now reset and attempt to connect to the wireless.</h4>\n"
      "</body>\n"
      "</html>\n";

const static char http_head[] = "<!DOCTYPE html>"
      "<html>\n"
      "<head>\n"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
    "<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate' />\n"
    "<meta http-equiv='Pragma' content='no-cache' />\n"
    "<meta http-equiv='Expires' content='0' />\n"
      "  <style type=\"text/css\">\n"
      "    html, body, iframe { margin: 0; padding: 0; height: 100%; }\n"
      "    iframe { display: block; width: 100%; border: none; }\n"
      "    body { background-color: black; color:white; margin:5px; font-family:'Verdana' }\n"
            "h2 { color:white; font-family:'Verdana'; }"
            "h4 { color:white; font-family:'Verdana'; font-weight:normal; }"
"#pwd { font-size:20px}\n"
"#networklist input { margin-right:10px }\n"
"#submit { font-size:20px; margin-top:10px; color:white; background-color: green; border: solid 1px #00450d; border-radius: 2px; padding:3px 7px 0px 7px}\n"
"#networklist { font-size:23px; border-radius:5px; padding:15px 5px 0 15px; width:290px; color:white; background-color: #262626; margin-left:10px }\n"
      "  </style>\n"
      "<title>CloudLogger</title>\n"
      "</head>\n"; 

const static char http_index[] = "<!DOCTYPE html>"
      "<body>\n"
      "<h2>SiscoCircuits CloudLogger</h2>\n"
      "<h3>Wireless Network Setup</h3>\n"
      "<h4>Please select a wireless network, enter the wireless password and press 'SUBMIT'.</h4>\n"
      "<h4>After you press SUBMIT, the unit will reset and attempt to connect to the selected wifi network.</h4>"
        "<form action='/' method='post'>"
            "<div id='networklist'>\n"
                "$$$"
            "</div>\n"
            "<label>Wireless Password </label>\n"
            "<input type='text' id='pwd' name='pwd' autocorrect='off' autocapitalize='none'>\n"
            "<input type='submit' id='submit' value='Submit'>\n"
        "</form>\n"
      "</body>\n"
      "</html>\n";

    
static void test_wifi_scan_all() {
    uint16_t ap_count = 0;
    wifi_ap_record_t *ap_list;
    uint8_t i;
    char *authmode;
    string DefaultSelectionChecked(" checked");

    esp_wifi_scan_get_ap_num(&ap_count);	
//    printf("--------scan count of AP is %d-------\n", ap_count);
    
    // $$$ THIS SHOULD RESULT IN AN ERROR MESSAGE!
    if (ap_count <= 0)
        return; 
    
    ap_list = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));	

    for (i = 0; i < ap_count; i++) 
    {
        switch (ap_list[i].authmode) 
        {
        case WIFI_AUTH_OPEN:
            authmode = "WIFI_AUTH_OPEN";
            break;
        case WIFI_AUTH_WEP:
            authmode = "WIFI_AUTH_WEP";
            break;           
        case WIFI_AUTH_WPA_PSK:
            authmode = "WIFI_AUTH_WPA_PSK";
            break;           
        case WIFI_AUTH_WPA2_PSK:
            authmode = "WIFI_AUTH_WPA2_PSK";
            break;           
        case WIFI_AUTH_WPA_WPA2_PSK:
            authmode = "WIFI_AUTH_WPA_WPA2_PSK";
            break;
        default:
            authmode = "Unknown";
            break;
        }
        //printf("%26.26s    |    % 4d    |    %22.22s\n", ap_list[i].ssid, ap_list[i].rssi, authmode);
        
        // Create the radio buttons for the html request form.
        string ssid((char*)ap_list[i].ssid);
        AP_List_str += string("<input type='radio' class='container' name='ssid' value='") + ssid + string("'") + DefaultSelectionChecked + 
            string(">") + ssid + string("<br><br>\n");
        DefaultSelectionChecked = "";
    }

    free(ap_list);
}


/* Initialize Wi-Fi as sta and set scan method */
static void wifi_scan(void) {
    tcpip_adapter_init();
    
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_scan_config_t scan_config; 
    
    scan_config.ssid = 0;
    scan_config.bssid = 0;
    scan_config.channel = 0; /* 0--all channel scan */
    scan_config.show_hidden = 0; // dgs changed to 0
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 120;
    scan_config.scan_time.active.max = 150;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    //ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    //printf("Event = %04X\n", event->event_id);
    
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        printf("got ip\n");
        printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
        printf("netmask: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.netmask));
        printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
        printf("\n");
        fflush(stdout);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
        
    case SYSTEM_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "SYSTEM_EVENT_SCAN_DONE");
        test_wifi_scan_all();
        break;
        
    default:
        break;
    }
    return ESP_OK;
}

/**
 ** Decode the given URL-coded string.
 **
 ***/
string urlDecode(string str) {
    string ret;
    char ch;
    int i, ii, len = str.length();

    for (i = 0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }
        else {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

/**
 ** Changes by Doug Sisco
 ** 
 ** This is a very simple HTTP server. 
 ** This server returns a page that prompts the user to select a wifi network and enter 
 ** a password.
 **
 ***/
static void http_server_netconn_serve(struct netconn *conn) {
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen, buf_total;
    err_t err;
    string SSID_s, Pwd_s;

 
    // Read the data from the port, blocking if nothing yet there.
    err = netconn_recv(conn, &inbuf);

    printf("--------NEW REQUEST-------\n");
    
    if (err == ERR_OK) {
        if (inbuf != NULL) {
            // Get the netbuf data.
            netbuf_data(inbuf, (void**)&buf, &buflen, &buf_total);
        }        
                             
        // If this is a POST..
        // The POST data is the chosen SSID and password.
        if (buflen >= 5 && strncmp(buf, "POST", 4) == 0) {  
            if (inbuf != NULL) {
                // Get the netbuf data.
                netbuf_data(inbuf, (void**)&buf, &buflen, &buf_total); 
                                 
                string s1;
                vector<string> PostArr;
                do {
                    if (s1.length() > 800) {
                        printf("s1 > 800\n");
                        break;
                    }
                      
                    buf[buflen] = 0;
                    s1 += buf;
                             
                    // Decode the URL-coded string.
                    s1 = urlDecode(s1);
                    
                    // Parse the POST data.
                    ParsePOST(s1, PostArr);
                    // If the POST data was found..
                    if (PostArr.size() > 1)
                        break;
                                
                    // Keep receiving data until there is none left.
                    // Note that netconn_recv() is blocking!
                    netconn_recv(conn, &inbuf);
                    
                    // Get the netbuf data.
                    if(buflen > 0)
                        netbuf_data(inbuf, (void**)&buf, &buflen, &buf_total); 
                } while (buflen > 0);
      
                //printf(">>>>>len=%u, s1=%s\n<<<<< EOD\n", buflen,s1.c_str());
                    
                if (PostArr.size() > 1) {
                    SSID_s = PostArr[1];
                    Pwd_s  = PostArr[3];
                    printf(">>>>SSID=%s, Pwd=%s\n", SSID_s.c_str(), Pwd_s.c_str() );
                }              
            }
            
            // $$$
            // Send the HTML final instructions and save SSID_g and pwd_g to flash and then restart.
            netconn_write(conn, http_hdrs, sizeof(http_hdrs) - 1, NETCONN_NOCOPY);
            netconn_write(conn, http_head, sizeof(http_head) - 1, NETCONN_NOCOPY);
            netconn_write(conn, http_index2, sizeof(http_index2) - 1, NETCONN_NOCOPY);
            /* Close the connection (server closes in HTTP) */
            netconn_close(conn);
            
            // Save the wifi credentials to flash.
            strcpy(WifiSTACredentials.SSID, SSID_s.c_str());
            strcpy(WifiSTACredentials.Pwd, Pwd_s.c_str());
            
            // If SSID is valid..
            if ( strlen(WifiSTACredentials.SSID) > 0)
                WifiSTACredentials.Write();
            
            // Restart.
            printf("\nRestarting.....\n");
            delay(2000);            
            esp_restart();
       }
            
        // If this is a GET..
        if (buflen >= 5 && strncmp(buf, "GET /", 5) == 0) {
            buf[buflen] = 0;

            string HTMLStr(http_index);
            replace(HTMLStr, string("$$$"), AP_List_str);
            
            // It doesn't matter what was requested, just send the web page.
            netconn_write(conn, http_hdrs, sizeof(http_hdrs) - 1, NETCONN_NOCOPY);
            netconn_write(conn, http_head, sizeof(http_head) - 1, NETCONN_NOCOPY);
            netconn_write(conn, HTMLStr.c_str(), HTMLStr.length(), NETCONN_NOCOPY);
        }
        /* Close the connection (server closes in HTTP) */
        netconn_close(conn);

        // Delete the buffer (netconn_recv gives us ownership, so we have to make sure to deallocate the buffer)
        netbuf_delete(inbuf);
    }
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

/**
 ** Parse the recieved POST data to a vector of strings.
 **
 **
 ***/
void ParsePOST(string &str, vector<string> &PostData)
{
    size_t CurPos, EqualPos;
    size_t end = 0;
    PostData.clear();
    
    // Find the first empty line, then advance to the first line after that.
    CurPos = str.find("\r\n\r\n");
    if (CurPos != string::npos)
        CurPos += 4;
    else {
        CurPos = str.find("\n\n");
        if (CurPos != string::npos) {
            CurPos += 2;
        } else {
            printf("No POST data!\n");
            return;
        }
    }    

    // ssid=full_quiver&pwd=samrizzo&fn=test
    // ^
    
    // whatever.com?ssid=full_quiver&pwd=samrizzo&fn=test
    //                  ^
    while((EqualPos = str.find('=', CurPos)) != string::npos) {
        PostData.push_back(str.substr(CurPos + 1, EqualPos - CurPos - 1));
        CurPos = str.find('&', EqualPos + 1);
        // whatever.com?ssid=full_quiver&pwd=samrizzo&fn=test
        //                              ^
        if(CurPos == string::npos) {
            PostData.push_back(str.substr(EqualPos + 1));
            break;
        }
        PostData.push_back(str.substr(EqualPos + 1, CurPos - EqualPos - 1));
        //printf("curpos=%u EqualPos=%u\n", CurPos, EqualPos);
    }
      
//    string s;
//    for (std::vector<string>::iterator i = PostData.begin(); i != PostData.end(); ++i) {
//        printf("str:'%s' ParsePOST:'%s', len=%u\n", str.c_str(),i->c_str(), i->length());
//    }
}

static void http_server_cp(void *pvParameters) {
    struct netconn *conn, *newconn;
    err_t err;
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    do {
        err = netconn_accept(conn, &newconn);
        if (err == ERR_OK) {
            printf("Connection was accepted.\n");
            http_server_netconn_serve(newconn);
            netconn_delete(newconn);
        }
    } while (err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
}

/**
 ** This is the entry point for the captive portal.
 ** It is called when a button is pressed.
 **
 ***/
int CaptivePortal(void) {
    WifiSTACredentials.Read();
    
    xTaskCreate(&LED_Blink, "led_blink", 1024, NULL, 5, NULL);
    
    // Update AP_List_str with the list of AP's.
    wifi_scan();
    //printf("%s\n", AP_List_str.c_str() );
  
    // Now create an AP and captive portal with the form to which the user will respond.
    wifi_AP_init();
    captdnsInit();
    xTaskCreate(&http_server_cp, "http_server_cp", 4096, NULL, 5, NULL); // 7-18-21 Increased from 2048.
    
    // Wait here while http_server_cp() resets the device.
    while (1) {
        vTaskDelay(100);   
    }
    return 0;
}

void wifi_AP_init(void) {
    nvs_flash_init();
    tcpip_adapter_init();

    // dgs: Removed since this has already been init'd.
//    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); 
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  // APSTA) );

    wifi_config_t apConfig;
    
    strncpy( (char*)apConfig.ap.ssid, CAPTIVE_PORTAL_SSID,32);
    strncpy( (char*)apConfig.ap.password, "notused",7);
    apConfig.ap.ssid_len = 0;
    apConfig.ap.channel = 6;
    apConfig.ap.authmode = WIFI_AUTH_OPEN;
    apConfig.ap.ssid_hidden = 0;
    apConfig.ap.max_connection = 4;
    apConfig.ap.beacon_interval = 100;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void LED_Blink(void *pvParameters) {
    static bool LedOn = 0;
    
    StatusLED.SetColor(GREEN);
    while (1) {
        DelayMS(100);   
        StatusLED.Invert();
    }    
}