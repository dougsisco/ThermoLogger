#include "tc_common.h"

void Thread_ReadTemperature(void *pvParameter) {
	CDataPoint DataPoint;
	// This is the total number of DS18B20 samples.
	static int SampleCount = 0;
	static int ErrorCount = 1;	
    char s100[100];
	
    printf(">>> Starting thread Thread_ReadTemperature()\n");
    
	// Initialize the thermocouple queue.
	QueueHandle_t thermocouple_queue = xQueueCreate(5, sizeof(struct max31856_cfg_t));

	if (thermocouple_queue == NULL) {
		ESP_LOGI("queue", "Not Ready: thermocouple_queue");
		return;
	}
	
	// Init the MAX31856.
	max31856_cfg_t max31856 = max31856_init(); 
    thermocouple_set_type(&max31856, MAX31856_TCTYPE_K);

    while (1) {
		DataPoint.Init();
		// Get the current time.
		time(&DataPoint.time);
		
		// Get the sensor values.
		for(int chan = 0 ; chan < NUM_CHANNELS ; chan++) {
			// If this is a thermocouple..
			if(Config.ChannelsAr[chan].SensorType == CChannel::SensorTypeE::TMAX31856) {				
    			// Select the TC by setting the MUX address.
				ThermocoupleSelect(Config.ChannelsAr[chan].MUXNum);

				// Delay while the MUX settles.
				// NOTE: if this delay is too short then the measurements will be off by a few degrees.
				vTaskDelay( pdMS_TO_TICKS(1) );	
				// Read the TC temperature.
				// Note that there is a 250ms delay in max31856.c/max31856_oneshot_temperature().
				thermocouple_read_temperature(chan, &max31856);
    			
    			// If temperature read error..
    			if(max31856.thermocouple_c == INVALID_TEMP) {
//        			Config.ChannelsAr[chan].ValueCurrent = INVALID_TEMP;
        			if (Config.ChannelsAr[chan].UpdateInvalidReadCountStatusChanged(1)) {
            			sprintf(s100, "Channel %u changed to OFFLINE", chan+1);
            			EventLog.Log(s100);
        			}
                }
				else {
    				if (Config.ChannelsAr[chan].UpdateInvalidReadCountStatusChanged(0)) {
        				sprintf(s100, "Channel %u changed to ONLINE", chan + 1);
        				EventLog.Log(s100);
    				}
    				
    				// Get the temperature.
					DataPoint.val[chan] = Config.ChannelsAr[chan].ValueCurrent = max31856.thermocouple_f;
    				
#if PRINT_TEMPERATURE_VALUES
				    printf("Ch %2u DS %2.2f MUX:%2u\n", chan + 1, max31856.thermocouple_f, Config.ChannelsAr[chan].MUXNum);
#endif
				}
			} else {
				// If this is a DS1820B and it is configured..
				if(Config.ChannelsAr[chan].T18b20.DS_GPIO > -1) {
    				float TempNow = Config.ChannelsAr[chan].T18b20.ds18b20_get_temp(chan+1);

					// Show that a DS18B20 value has been read.
					SampleCount++;
					
					// If the temp is valid then record it.
					if(TempNow != INVALID_TEMP) {
    					if (Config.ChannelsAr[chan].UpdateInvalidReadCountStatusChanged(0)) {
        					sprintf(s100, "Channel %u changed to ONLINE", chan + 1);
        					EventLog.Log(s100);
    					}
						
#if PRINT_TEMPERATURE_VALUES				
    					printf("Ch %2u DS %2.2f\n", chan + 1, TempNow);
#endif
						// Indicate that this sensor is connected.
						Config.ChannelsAr[chan].Connected = 1;
						
						// Save the value just read as the current value and the current datapoint value.
						DataPoint.val[chan] = Config.ChannelsAr[chan].ValueCurrent = TempNow;
					} else {
					    if (Config.ChannelsAr[chan].UpdateInvalidReadCountStatusChanged(1)) {
        					sprintf(s100, "Channel %u changed to OFFLINE", chan + 1);
        					EventLog.Log(s100);
    					}
#if PRINT_SENSOR_ERRORS	
						if (Config.ChannelsAr[chan].Connected) {
							printf("\n* SENSOR ERROR: Chan:%u Errors:%u, Samples:%u, Ratio:%.3f\n", chan, ErrorCount, SampleCount, (float)ErrorCount / (float)SampleCount); FLUSH; 
							ErrorCount++; 
						}
#endif												
						// Set the current datapoint value to the current valid value.
						DataPoint.val[chan] = Config.ChannelsAr[chan].ValueCurrent;
					}
				}
			}
    		
    	    // Update the channel LED.
    	    ChannelLED(chan, (Config.ChannelsAr[chan].InvalidReads > 0 ? 0 : 1));
		}
    	
    	// If the current time is valid (new 10-6-19)..	
    	if(TimeValid()) {
        	// Save the current data point to the array.
            SaveDataPointToArray(DataPoint);	
		
        	// Push the data point to the server queue.
        	if(SvrDataPointQ.size() < DATAPOINT_SVR_QUEUE_MAX_SIZE)
        		SvrDataPointQ.push(DataPoint);
        	else
        		printf("SvrDataPointQ is FULL!\n");
    	}

//		printf("!"); FLUSH;

		// Delay between temperature samples.
		// Note that there are also delays in each temperature measurement function.
    	// 4-27-20 Changed from 500 to 100.
		vTaskDelay(pdMS_TO_TICKS(100));	
    	
    	while (PauseTemperatureThread)
		    vTaskDelay(pdMS_TO_TICKS(500));	

#ifndef PRINT_TEMPERATURE_VALUES				
    	printf("V"); FLUSH;  // ************************ V ***********************
#endif
	}
}

/*
 * Update the invalid read count and set ValueCurrent to INVALID_TEMP
 *    if the invalid read count is higher than the max.
 * Return true if the 'invalid' status has changed.
 */
bool CChannel::UpdateInvalidReadCountStatusChanged(bool Invalid) {
    if (Invalid) {
        
        ValueCurrent = INVALID_TEMP;
        
        if (InvalidReads < MAX_SUCCESSIVE_INVALID_READS)
            InvalidReads++;
        else {     
            // The number of invalid reads has reached or is over the limit.
            // If this channel was in error then it hasn't changed..
            if (Error)
                return 0;
            else {
                Error = 1;
                return 1;
            }
        }
    } else {
        InvalidReads = 0; 
        
        // If 'invalid' status is changing..
        if (Error) {
            Error = 0;
            return 1;
        }
    }
    return 0;
}
