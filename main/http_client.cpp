#include "tc_common.h"

volatile bool FirmwareUpgradeInProcess = 0;

//static const char *REQUEST1 = "GET " WEB_URL " HTTP/1.0\r\n"
//    "Host: " WEB_SERVER "\r\n"
//    "User-Agent: esp-idf/1.0 esp32\r\n"
//    "\r\n";		

static const char *REQUEST = "POST " WEB_URL " HTTP/1.0\r\n"
"Accept: application/x-www-form-urlencoded, text/javascript, */*; q=0.01\r\n"
"X-Requested-With: XMLHttpRequest\r\n"
"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
"Content-Length:$CONTENT-LENGTH$\r\n";		
	
string ToJSON(string Name, float val, string &Str) {
	char s200[200];
	sprintf(s200, "\"%s\":\"%.1f\"", Name.c_str(), val);
	Str = s200;
	return Str;
}		

char *StrToJSON(string Name, string val, char *s) {
	char s200[200];
	sprintf(s200, "\"%s\":\"%s\"", Name.c_str(), val.c_str());
	strcpy(s,s200);
	return s;
}		

/**
 ** Open and connect to a socket.
 ** 
 ** Return non-zero on error.
 ***/
int OpenSocket(int &s) {
	struct addrinfo *res;
	struct in_addr *addr;
    
#if PRINT_WEB_CONN_INFO
    printf("OpenSocket() start\n");
#endif
        
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

	// Resolve web server address.
#if PRINT_WEB_CONN_INFO
    	printf("OpenSocket(): getaddrinfo()\n");
#endif
	int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
#if PRINT_WEB_CONN_INFO
    printf("OpenSocket(): getaddrinfo() end\n");
#endif
    
	if (err != 0 || res == NULL) {
#if PRINT_WEB_CONN_INFO
    	printf("OpenSocket(): DNS lookup failed err=%d res=%p\n", err, res);
#endif
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		return err;
	}

	// Print the resolved IP.
	//   Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
#if PRINT_WEB_CONN_INFO
    printf("OpenSocket(): DNS lookup succeeded. IP=%s\n", inet_ntoa(*addr));
#endif

#if PRINT_WEB_CONN_INFO
    printf("OpenSocket(): socket()\n");
#endif
    // Allocate a socket.
	s = socket(res->ai_family, res->ai_socktype, 0);
#if PRINT_WEB_CONN_INFO
    printf("OpenSocket(): socket() end\n");
#endif
    
    if (s < 0) {
#if PRINT_WEB_CONN_INFO
    	printf("OpenSocket(): Failed to allocate socket.\n");
#endif
		freeaddrinfo(res);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		return -1;
	}

#if PRINT_WEB_CONN_INFO
    printf("OpenSocket(): connect()\n");
#endif
    // Connect to the socket.
	if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
#if PRINT_WEB_CONN_INFO
    	printf("OpenSocket(): socket connect failed errno=%d\n", errno);
#endif

    	close(s);
		freeaddrinfo(res);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
		return -1;
	}
    
#if PRINT_WEB_CONN_INFO
    	printf("OpenSocket(): connect() end\n");
#endif
	freeaddrinfo(res);
    
#if PRINT_WEB_CONN_INFO
    printf("OpenSocket() END\n");
#endif
	return 0;
}

/**
 ** Send an http request and receive the reply.
 ** 
 ** Return non-zero on error.
 ***/
int SendHttpRequest(string Request, string &Response, int s) {
    static int L = 0;
    
	char recv_buf[64];
//    printf("*** %u SendHttpRequest SendHttpRequest()\n",L);
//    printf("*\n");
	
	Response.clear();
	
#if PRINT_HTML_HTTP_REQUEST 
    printf("***********\nHTTP REQ=%s\n***********\n", Request.c_str() );
#endif
    
	// Write the http request.
//     printf("SendHttpRequest write1\n");
	if(write(s, Request.c_str(), Request.length()) < 0) {
		ESP_LOGE(TAG, "... socket send failed");
		close(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
		return -1;
	}

	// Set the rx timeout.
	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
   if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
		ESP_LOGE(TAG, "... failed to set socket receiving timeout");
		close(s);

		return -1;
	}
//    printf("SendHttpRequest setsockopt\n\n");

	// Read the http response.
	int r;
//    printf("SendHttpRequest read\n");
	do {
		// Clear the buffer.
		bzero(recv_buf, sizeof(recv_buf));
    	// Partially read the response.
		r = read(s, recv_buf, sizeof(recv_buf) - 1);
		
		if (r > 0)
			Response.append(recv_buf, r);
		
	} while (r > 0) ;

    //    printf("*** %u SendHttpRequest SendHttpRequest() DONE\n", L++);
    // Get the first line of the response.
    int EndPos = Response.find('\n');
    string FirstLine = Response.substr(0, EndPos);
    FirstLine = str_toupper(FirstLine);
    
    // If there was no response or the reponse was a server error..
    if(EndPos > -1 && FirstLine.find("SERVER ERROR") != string::npos) {
        // Set the status LED to indicate an error.
        StatusLED.SetStatus(SYS_STATUS_ERR, "http response error.");
        return -1;
    }
#if PRINT_HTML_SVR_RESPONSE
    printf("***********************\nSendHttpRequest() RESPONSE:\n%s\n***********************\n", FirstLine.c_str());
#endif    
    
	return 0;
}

char * strcat2(char *d, char *s, long n) {
	char *d2 = d;
	
	while (n--)
		*d++ = *s++;
	
	return d2;
}

// Send queued values to the server.
//
void Thread_SendDataToSvr(void *pvParameters) {
	int SockHdl, r, err;
	char s100[100], s200[200];
	static long LoopCount = 0;
	SWifiInfo WIFIinfo;
    int LastPostResponseOK = 0;
    string s;
    static int DeviceFlagsLast = 0;
    int SocketErrorsCount = 0;
    int SendHttpRequestErrorsCount = 0;
	
    while (1) {
    	while (FirmwareUpgradeInProcess)
        	vTaskDelay(pdMS_TO_TICKS(500));	
    	
    	//		printf("---Thread_SendDataToSvr()\n");
		
		// Wait for the internet to be connected.
		// THIS CAUSES AN ABORT IN THE DEBUGGER!
		//		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

		// Get the WIFI Strength.
		getWifiInfo(&WIFIinfo);
		
		if(SvrDataPointQ.size() <= 0) {
			vTaskDelay(100 / portTICK_PERIOD_MS); // 4-27-20 Changed from 500 to 100.
			continue;
		}
    	
		// Open a socket to the server.
		// If error opening socket..
		if(OpenSocket(SockHdl)) {
    		EventLog.Log("Error opening socket");
    		if (SocketErrorsCount++ > MAX_SUCCESSIVE_SOCKET_ERRORS) {
        		// Set the status LED to indicate an error.
        		StatusLED.SetStatus(SYS_STATUS_ERR, "Error opening socket over limit");
        		continue;
    		}
		}
    	SocketErrorsCount = 0;
		
		// Send the MAC and then all queued samples as a JSON POST to the server.
		string ValuesStr = "data={";
		sprintf(s100, "\"mac\":%lld", MACAddr);
		ValuesStr.append(s100);
		ValuesStr.append(",");
		
        // Add the new data points to the JSON string.
        string str;
        ValuesStr.append(GetJsonDataPoints(str));
    	
    	long UpTime = (long)(time(NULL) - BootTime);
		
		// Add the rssi, channel value, version.
		sprintf(s100, "\"rssi\":%d, \"chan\":%u, \"uptime\":%lu, \"version\":\"%s\", \"deviceflags\":%u", 
        	WIFIinfo.rssi, WIFIinfo.channel, UpTime, VERSION, DeviceFlags);
		ValuesStr.append(s100);
    	
    	// If the svr connection is good and the event log is not empty then add all log entries.
       	if(LastPostResponseOK > 3 && EventLog.NotEmpty()) {
          	// Keep the temp s1 for debugging!
          	string s1 = EventLog.Json();
          	// $$$ WHY AREN'T THE QUEUED LOG ENTRIES SENT??
          	printf("Thread_SendDataToSvr: Sending EventLog=%s\n", s1.c_str());
        	ValuesStr += "," + s1;
        }
    	
    	ValuesStr += "}";
    	s = ValuesStr;

		sprintf(s200, "%u", ValuesStr.length());
		
		string Request = (string)REQUEST + "\r\n" + ValuesStr + "\n\r";
		Replace(Request,"$CONTENT-LENGTH$", string(s200));
		
#if PRINT_WEB_CONN_INFO
    	printf("REQUEST:\n\r%s\n\r*****\n\r", Request.c_str());
#endif

		// NOTE: I moved these declarations here because putting them at the start of the function
		// was causing stack corruption crashes, maybe because they were growing and not being deleted?
		// Maybe declaring them here causes a complete delete.
		string Response, ValStr;
		
		// If error sending request..
		if( SendHttpRequest(Request, Response, SockHdl) ) {
    		EventLog.Log("Thread_SendDataToSvr: ERROR SendHttpRequest");
    		if (SendHttpRequestErrorsCount++ > MAX_SUCCESSIVE_SENDHTTP_ERRORS) {
        		// Set the status LED to indicate an error.
                StatusLED.SetStatus(SYS_STATUS_ERR, "Thread_SendDataToSvr: ERROR SendHttpRequest over limit");
    		}
			continue;
		}
    	SendHttpRequestErrorsCount = 0;

		// Response data format is
		//   "{<Device Flags>
		
		// Find the start of the data which is marked by a '{'.
		int ContentPos = Response.find("{");
    	if (ContentPos >= 0) {
        	// If we are not in 'identify' mode..
        	if (IdentifySecs == 0)
        	    StatusLED.SetStatus(SYS_STATUS_OK);

        	LastPostResponseOK++;
    		
        	ValStr = Response.substr(ContentPos + 1);
    			
        	DeviceFlags = strtol(ValStr.c_str(), 0, 10);
        	
        	// If Device flags has changed..
        	if (DeviceFlags != DeviceFlagsLast) {
            	DeviceFlagsLast = DeviceFlags;
            	
             	// -------- RESET --------
                if(DeviceFlags & DEVICE_FLAG_RESET) {
                    // If reset has not yet been requested..
                    if (ResetRequestSec == 0) {
                        EventLog.Log("Reset requested..");
                        StatusLED.SetStatus(SYS_STATUS_WARNING);
                        ResetRequestSec = REBOOT_DELAY_TIME;
                    }
            	}
            	
            	// -------- IDENTIFY --------
        	    if(DeviceFlags & DEVICE_FLAG_IDENTIFY_ON) {
            	    IdentifySecs = IDENTIFY_TIME_SEC;
                	EventLog.Log("Identify ON");
            	    StatusLED.SetStatus(SYS_STATUS_WARNING);
            	}
        	
            	// -------- FIRMWARE UPGRADE --------
                if(DeviceFlags & DEVICE_FLAG_FIRMWARE_UPGRADE) {
                    StatusLED.SetStatus(SYS_STATUS_WARNING);
                    
                    DeviceFlags = DeviceFlags & ~DEVICE_FLAG_FIRMWARE_UPGRADE;
            	    if (FirmwareUpgradeInProcess)
                	    EventLog.Log("Firmware upgrade already in progress!");
            	    else
            	        EventLog.Log("Firmware upgrade requested");
            	    if (!FirmwareUpgradeInProcess) {
                	    FirmwareUpgradeInProcess = 1;
            	    }
            	    else {
                	    EventLog.Log("FirmwareUpgradeTask already running!");
            	    }
        	    }
        	}
        	
    	} else {
        	LastPostResponseOK = 0;
    	}

#if PRINT_CURRENT_DATAPOINT_TIME
		printf("%lu Time:%lu Identify:%d rssi:%d chan:%u\n", LoopCount++, (long int)DataPoint.time, DeviceFlags & DEVICE_FLAG_IDENTIFY,WIFIinfo.rssi, WIFIinfo.channel);
#endif		

		close(SockHdl);
    	//		printf("---END Thread_SendDataToSvr()\n");

    	// Blink the status LED.
        StatusLED.Invert();
		
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
/*
HTTP/1.1 200 OK
Date: Tue, 09 Apr 2019 21:26:30 GMT
Server: Apache/2.4.23 (Win32) PHP/5.5.38
X-Powered-By: PHP/5.5.38
Set-Cookie: PHPSESSID=e5sp91f9p64l1phvilcm924fv6; path=/
Expires: Thu, 19 Nov 1981 08:52:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Content-Length: 39
Connection: close
Content-Type: text/html

<pre>Array
(
    [doug] => 5.1
)
</pre>
 */

/*
 * Get the current data points as a JSON string.
 *
 */
string GetJsonDataPoints(string &str) {
	char s100[100];
	CDataPoint DataPoint;

	str.append("\"vals\":[");
		
	while (SvrDataPointQ.size() > 0) {
		// Get a data point from the queue.
		DataPoint = SvrDataPointQ.front();
		SvrDataPointQ.pop();
		
		sprintf(s100,
			"[%ld,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f],",
			DataPoint.time,
			DataPoint.val[0],
			DataPoint.val[1],
			DataPoint.val[2],
			DataPoint.val[3],
			DataPoint.val[4],
			DataPoint.val[5],
			DataPoint.val[6],
			DataPoint.val[7],
			DataPoint.val[8],
			DataPoint.val[9],
			DataPoint.val[10],
			DataPoint.val[11]);
		str.append(s100);
		
		// Get the current time.
//		struct tm * timeinfo;
//		timeinfo = localtime(&DataPoint.time);			
		//printf("** POST=%u:%02u:%02u\n**%s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,s100);
			
	}
	// Remove the last ','.
	str.erase(str.length() - 1); 
	str += "],";

	return str;
}
