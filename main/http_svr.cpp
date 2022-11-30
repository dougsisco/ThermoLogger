#include "tc_common.h"

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

long UpdateCount = 0;

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
		if(TokenArr[0] == "GET") {
			// If this is a data request..
			if(TokenArr[3] == "?") {
#if 0
				// Print the token array.
				printf("*** TokenArr ***\n");
				for (int i = 0; TokenArr[i].length() > 0; i++)
					printf("TokenArr[%u]='%s'\n", i, TokenArr[i].c_str());
				printf("\n");
#endif
			
				// If request to set a var..
				if(TokenArr[5] == "=") {
					// ************************
					// Channel Options
					//
					// ?channel=X&axis=X
					// ^   ^   ^^^ ^  ^^
					// 3   4   567 8  90
					// ************************
					if(TokenArr[4] == "channel" && TokenArr[7] == "&" && TokenArr[9] == "=") {
						int SensorI = atoi(TokenArr[6].c_str());
						if (SensorI < 0 || SensorI > NUM_CHANNELS) {
							printf("ERROR: Sensor index '%u' out of range!\n", SensorI);
							return;
						}
						printf("Sensor Options, sensor %u\n", SensorI);
//						// Axis
//						if(TokenArr[8] == "axis") {
//							int AxisI = atoi(TokenArr[10].c_str());
//							if (AxisI == 0 || AxisI == 1) {
//								Config.ChannelsAr[SensorI].Axis = AxisI;
//								printf("Config.ChannelsAr[%u].Axis -> %u\n", SensorI, AxisI);
//							}
//						}
						// sensortype
//						if(TokenArr[8] == "sensortype") {
//							CChannel::SensorTypeE SensorType = (CChannel::SensorTypeE)(atoi(TokenArr[10].c_str()));
//							if (SensorType == CChannel::SensorTypeE::T18B20 || SensorType == CChannel::SensorTypeE::TMAX31856) {
//								Config.ChannelsAr[SensorI].SensorType = SensorType;
//								// If this is a DS18B20..
//								if(SensorType == CChannel::SensorTypeE::TMAX31856)
//									Config.ChannelsAr[SensorI].T18b20.ds18b20_init();
//
//								printf("Config.ChannelsAr[%u].SensorType -> %u\n", SensorI, (int)SensorType);
//							}
//							else {
//								printf("ERROR: SensorType out of range!\n");
//							}
//						}
					}
					// ************************
					// enablelogging
					//
					// ?enablelogging=X
					// ^   ^         ^^
					// 3   4         56
					// ************************
					if(TokenArr[4] == "enablelogging") {
						Config.EnableLogging = atoi(TokenArr[6].c_str());
						printf("Config.EnableLogging -> %u\n", Config.EnableLogging);
					}
					// ************************
					// hostname
					//
					// ?hostname=X
					// ^   ^    ^^
					// 3   4    56
					// ************************
					if(TokenArr[4] == "hostname") {
						if (TokenArr[6].length() > 1 && TokenArr[6].length() < 20)
							strncpy(Config.HostName, TokenArr[6].c_str(), 20);
						printf("Config.HostName -> %s\n", Config.HostName);
					}
					// ************************
					// channel
					// ************************
					// Handle a single channel data request.
					if(TokenArr[4] == "channel") {
						int n = atoi(TokenArr[6].c_str());
						printf("ChannelReq %u:%.1f\n", n, Config.ChannelsAr[n].ValueCurrent);
						sprintf(s100, "%.1f", Config.ChannelsAr[n].ValueCurrent);
						netconn_write(conn, HTML_HEADER_OK, strlen(HTML_HEADER_OK), NETCONN_NOCOPY);
						netconn_write(conn, s100, strlen(s100), NETCONN_NOCOPY);						
					} 
					// ************************
					// SamplePeriodTSec
					// ************************
					if(TokenArr[4] == "sampleperiod") {
						Config.SamplePeriodTSec = atoi(TokenArr[6].c_str());
						printf("Config.SamplePeriodTSec -> %u\n", Config.SamplePeriodTSec);
					}
					// ************************
					// WebRefreshPeriodTSec
					// ************************
					if(TokenArr[4] == "webrefreshperiod") {
						Config.WebRefreshPeriodTSec = atoi(TokenArr[6].c_str());
						printf("Config.WebRefreshPeriodTSec -> %u\n", Config.WebRefreshPeriodTSec);
					}
					// ************************
					// ChartSamples
					// ************************
					if(TokenArr[4] == "chartsamples") {
						Config.ChartSamples = atoi(TokenArr[6].c_str());
						printf("Config.ChartSamples -> %u\n", Config.ChartSamples);
					}
					// ************************
					// ZoomTo
					// ************************
					if(TokenArr[4] == "zoomto") {
						Config.ChartZoom = atoi(TokenArr[6].c_str());
						printf("Config.ChartZoom -> %u\n", Config.ChartZoom);
					}
					// ************************
					// ZoomIn
					// ************************
					if(TokenArr[4] == "zoomin") {
						int zlevel = Config.ChartZoom;
						if (zlevel < ZOOM_MAX)
							zlevel++;
						Config.ChartZoom = zlevel;
						printf("Config.ChartZoom -> %u\n", Config.ChartZoom);
					}
					// ************************
					// ZoomOut
					// ************************
					if(TokenArr[4] == "zoomout") {
						int zlevel = Config.ChartZoom;
						if (zlevel > 1)
							zlevel--;
						Config.ChartZoom = zlevel;
						printf("Config.ChartZoom -> %u\n", Config.ChartZoom);
					}
					// ************************
					// XOffset
					// ************************
					if(TokenArr[4] == "xoffset") {
						Config.XOffset = atoi(TokenArr[6].c_str());
						printf("Config.XOffset -> %u\n", Config.XOffset);
					}
					// ************************
					// ChartHighTemp1
					// ************************
					if(TokenArr[4] == "charthightemp1") {
						Config.ChartYAxis[0].HighTemp = atoi(TokenArr[6].c_str());
						printf("Config.ChartYAxis[0].HighTemp -> %u\n", Config.ChartYAxis[0].HighTemp);
						if (Config.ChartYAxis[0].LowTemp > Config.ChartYAxis[0].HighTemp - CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[0].LowTemp = Config.ChartYAxis[0].HighTemp - CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartLowTemp1
					// ************************
					if(TokenArr[4] == "chartlowtemp1") {
						Config.ChartYAxis[0].LowTemp = atoi(TokenArr[6].c_str());
						printf("Config.ChartYAxis[0].LowTemp -> %u\n", Config.ChartYAxis[0].LowTemp);
						if (Config.ChartYAxis[0].HighTemp < Config.ChartYAxis[0].LowTemp + CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[0].HighTemp = Config.ChartYAxis[0].LowTemp + CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartHighTemp2
					// ************************
					if(TokenArr[4] == "charthightemp2") {
						Config.ChartYAxis[1].HighTemp = atoi(TokenArr[6].c_str());
						printf("Config.ChartYAxis[1].HighTemp -> %u\n", Config.ChartYAxis[1].HighTemp);
						if (Config.ChartYAxis[1].LowTemp > Config.ChartYAxis[1].HighTemp - CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[1].LowTemp = Config.ChartYAxis[1].HighTemp - CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartLowTemp2
					// ************************
					if(TokenArr[4] == "chartlowtemp2") {
						Config.ChartYAxis[1].LowTemp = atoi(TokenArr[6].c_str());
						printf("Config.ChartYAxis[1].LowTemp -> %u\n", Config.ChartYAxis[1].LowTemp);
						if (Config.ChartYAxis[1].HighTemp < Config.ChartYAxis[1].LowTemp + CHART_TEMP_MIN_DIF)
							Config.ChartYAxis[1].HighTemp = Config.ChartYAxis[1].LowTemp + CHART_TEMP_MIN_DIF;
					}
					// ************************
					// ChartValueSelect
					// ************************
					if(TokenArr[4] == "chartvalueselect") {
						Config.ChartValueSelect = atoi(TokenArr[6].c_str());
						printf("Config.ChartValueSelect -> %u\n", Config.ChartValueSelect);
					}
					// ************************
					// SlopeTimeSec
					// ************************
					if(TokenArr[4] == "slopetimesec") {
						Config.SlopeTimeSec = atoi(TokenArr[6].c_str());
						if (Config.SlopeTimeSec < 2)
							Config.SlopeTimeSec = 10;
						printf("Config.SlopeTimeSec -> %u\n", Config.SlopeTimeSec);
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
						printf("*** Filling sample array with test data: MAX_SAMPLES=%u\n", MAX_SAMPLES);
						for (int i = 0; i < MAX_SAMPLES; i++) {
							ValuesArr[i].time = now + (time_t)i;
							ValuesArr[i].val[0] = ValuesArr[i].val[1] = (50.0 * sin(i / 10.0)) + 50.0;
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
				if(TokenArr[4] == "status") {
					printf("----------------------------------------\nProcessing status data request.\n");
					
					string ValStr = "{ ";
					// For each chart value index..
					for(int chartVi = 0 ; chartVi < NUM_CHART_VALS ; chartVi++) {
						// NOTE that the response must start with a \n or iphone browsers will not accept the response. This is 
						//  probably due to the lack of response header.

						int ChannelIndex = Config.ChartVal[chartVi].ChannelIndex;
						// If this chart value is valid..
						if (ChannelIndex > -1) {
							// Get this channel's slope.
							float slope = GetSlope(ChannelIndex);
						
							// If the value is valid..
							if(Config.ChannelsAr[ChannelIndex].ValueCurrent != INVALID_TEMP) {
								printf("chartVi=%d ChannelIndex=%d ValueCurrent=%f\n",chartVi,ChannelIndex, Config.ChannelsAr[ChannelIndex].ValueCurrent);
								// Json encode the current ChartVal. 
								sprintf(s200,
									"\"curval%u\":\"%.1f\",",
									chartVi, 
									Config.ChannelsAr[ChannelIndex].ValueCurrent);
								ValStr += s200;
							}
							// Json encode this channel's slope.
							sprintf(s200, "\"slope%u\":\"%.1f\",", ChannelIndex, slope);
							ValStr += s200;
						}						
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
				if(TokenArr[4] == "chart") {
					ProcessChartRequest(conn);
				}
				
				// Settings page request.
				if(TokenArr[4] == "settingspage") {
					ProcessSettingsPage(conn);
				}
			} else {
				// A '?' was not found.
				
				// If this is a page request..
				if(TokenArr[2] == "/" || TokenArr[2] == "/index.php") {
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
	int SampleInd = ValuesArrInd - 1;
	if (SampleInd < 0)
		SampleInd = MAX_SAMPLES - 1;
	if (SampleInd >= MAX_SAMPLES)
		SampleInd = 0;

	// List the values backward so that the graph is created with later values on the right.
	for(int count = 0 ; count < Config.ChartSamples ; SampleInd -= Config.ChartZoom, count++) {
		if (SampleInd <= 0)
			SampleInd = MAX_SAMPLES - 1 + SampleInd;

		localtime_r(&ValuesArr[SampleInd].time, &timeinfo);
		// If this is a valid date..
		if(ValuesArr[SampleInd].time > 0) {
			localtime_r(&ValuesArr[SampleInd].time, &timeinfo);

			// Start each JSON row with the date string.
			// {"c":[{"v":"Date(2000,0,02, 15,0,30)"},
			sprintf(s200,
				"{\"c\":[{\"v\":\"Date(%u,%u,%u, %u,%u,%u)\"}, ",
				timeinfo.tm_year+1900,
				timeinfo.tm_mon,
				timeinfo.tm_mday,
				timeinfo.tm_hour,
				timeinfo.tm_min,
				timeinfo.tm_sec);
			ValStr += s200;
			
			// Now add each value.
			// {"v":150},
			// If invalid:  {"v":null},
			
			// For each chart value index..
			for(int chartVi = 0 ; chartVi < NUM_CHART_VALS ; chartVi++) {
				int ChannelIndex = Config.ChartVal[chartVi].ChannelIndex;
				// If this chart value is valid..
				if(ChannelIndex > -1) {
					// $$$ max SampleInd???
					if(ValuesArr[SampleInd].val[ChannelIndex] != INVALID_TEMP)
						sprintf(s200, "{\"v\":%.1f},", ValuesArr[SampleInd].val[ChannelIndex]);
						else
							// If this is an invalid value..
							sprintf(s200, "{\"v\":null},");					
						ValStr += s200;
				}
			}
			// Remove the last ','.
			ValStr.erase(ValStr.length() - 1); // = ValStr.substr(0, ValStr.size()-1);
			ValStr += "]},\n";
		}
	}
	// Remove the last ','.
	ValStr.erase(ValStr.length() - 2);  //ValStr = ValStr.substr(0, ValStr.size()-2);
	
	ValStr += "\n]};";
	
	//printf("\n********************\nValStr=%s\n\n********************\n",ValStr.c_str());

	// Send the chart data.
	netconn_write(conn, HTML_HEADER_OK, strlen(HTML_HEADER_OK), NETCONN_NOCOPY);
	netconn_write(conn, ValStr.c_str(), ValStr.length() - 1, NETCONN_NOCOPY);	
	printf("END ProcessChartRequest()\n----------------------------------------\n");
}

void http_server(void *pvParameters) {
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
//		for (int AxisI=0; AxisI<2; AxisI++) {
//			SelectedStr = "";
//			if (Config.ChannelsAr[i].Axis == AxisI) {
//				SelectedStr = " selected";
//				//printf("Config.ChannelsAr[%u].Axis=%u, AxisI=%u\n",i,Config.ChannelsAr[i].Axis,AxisI);
//			}
//			sprintf(s200,"%u",AxisI);
//
//			sprintf(s100,"<option value=%u %s>%s</option>\n",AxisI,SelectedStr.c_str(),s200);
//			SelectOptionsStr += s100;
//		}
//		sprintf(s200,"$$$AXES_OPTIONS_%u",i);
//		Replace(SettingsPageStr,string(s200),SelectOptionsStr);
		
//		// Update each device menu..
//		SelectOptionsStr = "";
//		// For each sensor type menu option.
//		SelectedStr = "";
//		// If this sensor is a DS18B20..
//		if (Config.ChannelsAr[i].SensorType == CChannel::SensorTypeE::T18B20)
//			SelectedStr = " selected";
//
//			sprintf(s100,"<option value=0 %s>DS18B20</option>\n",SelectedStr.c_str());
//		SelectOptionsStr += s100;
//
//		SelectedStr = "";
//		// If this sensor is a TMAX31856..
//		if (Config.ChannelsAr[i].SensorType == CChannel::SensorTypeE::TMAX31856)
//			SelectedStr = " selected";
//
//		sprintf(s100,"<option value=1 %s>Thermocouple</option>\n",SelectedStr.c_str());
//		SelectOptionsStr += s100;
//		
//		sprintf(s200,"$$$SENSOR_TYPE_OPTIONS_%u",i);
//		Replace(SettingsPageStr,string(s200),SelectOptionsStr);
	}
		
	netconn_write(conn, SettingsPageStr.c_str(), strlen(SettingsPageStr.c_str()) - 1, NETCONN_NOCOPY);
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
	sprintf(s200, "%u", Config.ChartYAxis[0].LowTemp);
	Replace(OptionsStr, string("$$$VAXES_0_MIN"), string(s200));
	
	sprintf(s200, "%u", Config.ChartYAxis[0].HighTemp);
	Replace(OptionsStr, string("$$$VAXES_0_MAX"), string(s200));

	sprintf(s200, "%u", Config.ChartYAxis[1].LowTemp);
	Replace(OptionsStr, string("$$$VAXES_1_MIN"), string(s200));
	
	sprintf(s200, "%u", Config.ChartYAxis[1].HighTemp);
	Replace(OptionsStr, string("$$$VAXES_1_MAX"), string(s200));
	
	// Set the chart axis for each sensor.
	for(int i = 0 ; i < NUM_CHART_VALS ; i++) {
		sprintf(s100, "%u", Config.ChartVal[i].Axis);
		sprintf(s200, "$$$SENSOR_%u_AXIS", i);
		Replace(OptionsStr, string(s200), string(s100));
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
	for(int i = 0 ; i < NUM_CHANNELS ; i++) {
		if (Config.ChannelsAr[i].ValueCurrent != INVALID_TEMP)
			sprintf(s100, "%.1f", Config.ChannelsAr[i].ValueCurrent);
		else
			*s100 = 0;
		sprintf(s200, "$$$CURVAL%u", i);
		Replace(MainStr, string(s200), string(s100));
	}	

	// Insert the options into the main string.
	Replace(MainStr, "$$$CHART_OPTIONS", OptionsStr);
	
	sprintf(s200, "%u", 100*Config.WebRefreshPeriodTSec);
	Replace(MainStr, string("$$$REFRESH_PERIOD"), string(s200));

	// Boot time.
	localtime_r(&BootTime, &timeinfo);
	sprintf(s200, "%u-%u-%u %2u:%2u:%2u", timeinfo.tm_year + 1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	printf("BootTimeStr=%s\n", s200);
	Replace(MainStr, string("$$$BOOTTIME"), string(s200));

	// Initial status text.
	Replace(MainStr, string("$$$STATUS_TEXT"), GetStatusTextStr());
	
	// Send the HTML header
	netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);

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
	
	sprintf(s200,
		"<b>ChartSamples:</b>%u <b>Zoom:</b>%u <b>SampleP:</b>%.1f s <b>XOffset:</b>%u <b>ValuesArrInd:</b>%u "
			  "<b>ChartTime:</b>%u:%02u:%02u <b>Time:</b>%u:%02u:%02u <b>Ver</b> %s <br />"
			  "<b>Slope0:</b>%.1f <b>Slope1:</b>%.1f <b>Slope2:</b>%.1f<b>Slope3:</b>%.1f<b>Slope4:</b>%.1f<b>Slope5:</b>%.1f",
		Config.ChartSamples,
		Config.ChartZoom,
		(float)Config.SamplePeriodTSec/10,
		Config.XOffset,
		ValuesArrInd,
		timeinfo.tm_hour,
		timeinfo.tm_min,
		timeinfo.tm_sec,
		timeinfo_now.tm_hour,
		timeinfo_now.tm_min,
		timeinfo_now.tm_sec,
		VERSION,
		GetSlope(0),
		GetSlope(1),
		GetSlope(2),
		GetSlope(3),
		GetSlope(4),
		GetSlope(5));
	
	return (string)s200;
}