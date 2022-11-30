/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "tc_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "ds18b20.h"

portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;


// Sends one bit to bus
// Confirmed timing 8-24-19.
void C18b20::ds18b20_send(char bit) {
	gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(DS_GPIO, 0);
	ets_delay_us(5); // Spec is  > 1us
	
	if (bit == 1)
		gpio_set_level(DS_GPIO, 1);
	ets_delay_us(80); // Spec is 60 to 120us.
	
	gpio_set_level(DS_GPIO, 1);
}
// Reads one bit from bus
// Confirmed timing 8-24-19.
unsigned char C18b20::ds18b20_read(void) {
	unsigned char bit;
	
	gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(DS_GPIO, 0);
	ets_delay_us(2); // Spec is  > 1us
	
	// Actively pull up
	gpio_set_level(DS_GPIO, 1);
	ets_delay_us(1);
	
	// Set as input
	gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
	ets_delay_us(12); // Spec is 15 from first pull-down: 15-1-2 = 12
	
	if (gpio_get_level(DS_GPIO) == 1) 
		bit = 1;
	else 
		bit = 0;
	return (bit);
}
// Sends one byte to bus
// Confirmed timing 8-24-19.
void C18b20::ds18b20_send_byte(char data) {
	unsigned char i;
	unsigned char x;

    portENTER_CRITICAL(&mutex);
    for (i = 0; i < 8; i++) {
		x = data >> i;
		x &= 0x01;
		ds18b20_send(x);
	}
	ets_delay_us(100);
    portEXIT_CRITICAL(&mutex);
}
// Reads one byte from bus
unsigned char C18b20::ds18b20_read_byte(void) {
	unsigned char i;
	unsigned char data = 0;
	
    portENTER_CRITICAL(&mutex);
    for (i = 0; i < 8; i++)
	{
		if (ds18b20_read()) 
			data |= 0x01 << i;
		ets_delay_us(15);
	}
    portEXIT_CRITICAL(&mutex); 
	return (data);
}

// Send reset pulse and detect the presence.
unsigned char C18b20::ds18b20_RST_PULSE(void) {
	unsigned char PRESENCE;
	// Pull-down
	gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(DS_GPIO, 0);
	ets_delay_us(500);
	// Active pull up.
	gpio_set_level(DS_GPIO, 1);
	// Set as input
	gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
  
	// Wait for presence pulse.
	// Spec says delay is 15-60us (max is 60us)
	ets_delay_us(60);  // 8-24-19 dgs: this was 30 but I changed to 60.
	
	// Detect the presence pulse.
	if(gpio_get_level(DS_GPIO) == 0) 
		PRESENCE = 1; 
	else 
		PRESENCE = 0;
	// Spec says delay is 480us minimum from the last pull-up, sso 480-60 = 410
	ets_delay_us(410);  // 8-24-19 dgs: changed from 470 to 410
    
	return PRESENCE;
}

char CrcUpdate(char crc, char data) {
	crc = crc ^ data;
	
	for (int i = 0; i < 8; i++)
	{
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;
	}

	return crc;
}

char CrcUpdate(char crc, char data);
char Checksum(char *data, int len);

char Checksum(char *data, int len) {
	char crc = 0;
	for (int i = 0; i < len; i++) {
		crc = CrcUpdate(crc, data[i]);
	}
	return crc;
}

/**
 * Get temperature from DS18B20.
 * 
 * If error then return INVALID_TEMP.
 * 
 * 4-25-21 IMPORTANT: the 18B20P (parasitic-only) device will not work above 120F. 
 */
float C18b20::ds18b20_get_temp(int chan) {
	unsigned char check;
	short temp1 = 0;
	char data[8];
    char s100[100];
	    
#if PRINT_ONEWIRE_RESET_PULSE
	printf("-"); fflush(stdout);
#endif
	check = ds18b20_RST_PULSE();
		
	if (check == 1) {
		// Skip ROM
		ds18b20_send_byte(DS_CMD_SKIP_ROM);
		// Read scratchpad
		ds18b20_send_byte(DS_CMD_READ_SCRATCHPAD);
		data[0] = ds18b20_read_byte(); // Temperature LSB
		data[1] = ds18b20_read_byte(); // Temperature MSB
		data[2] = ds18b20_read_byte(); // TH/USER BYTE 1
		data[3] = ds18b20_read_byte(); // TH/USER BYTE 2
		data[4] = ds18b20_read_byte(); // CONFIG
		data[5] = ds18b20_read_byte(); // RESERVED
		data[6] = ds18b20_read_byte(); // RESERVED
		data[7] = ds18b20_read_byte(); // RESERVED
		// Read the checksum.
		int ChecksumRead = ds18b20_read_byte();
			
		check = ds18b20_RST_PULSE();
		// Skip ROM
		ds18b20_send_byte(DS_CMD_SKIP_ROM);
		// Start conversion: we do this at the end because the conversion
		// takes time and we can save time by letting the conversation occur while 
		// other sensors are being read.
		ds18b20_send_byte(DS_CMD_CONVERT);
    	
		float t1, t2;
		temp1 = (0xffff0000 | (data[1] << 8)) | data[0];
		t1 = (float)(temp1) / 16.0;
				  
		// Convert to F
		t2 = (float)(((9.0 *t1) / 5.0) + 32.0);
		  
		char ChecksumCalculated = Checksum(data, 8);
    	
		//printf("MSB=%X LSB=%X temp1=%X t1=%f config=%X\n",data[1],data[0],temp1,t1,data[4]);
		  		
    	// 4-25-21 IMPORTANT: the 18B20P (parasitic-only) device will not work above 120F, it returns checksum errors! 
		// If error..
		if(ChecksumRead != ChecksumCalculated) {
    		// Update the total number invalid reads.
    		sprintf(s100, "18B20 ERROR CH %d, COUNT=%d, t1=%f, t2=%f, csr=0x%X csc=0x%X", chan, ++InvalidReadsTotal, t1, t2, ChecksumRead, ChecksumCalculated);
    		EventLog.Log(s100);
    					    		
			return INVALID_TEMP;
		}
		    
		return t2;
	}
	// Return error.
	return INVALID_TEMP;
}
void C18b20::ds18b20_init() {
    unsigned char check;

    gpio_pad_select_gpio(DS_GPIO);
    InvalidReadsTotal = 0;

    // Check for presense pulse.
    check = ds18b20_RST_PULSE();
		
    // If this sensor is present..
    if (check == 1) {
        // Set the resolution to 9-bit. 
        // This is important because the default resolution is 12-bit and at this setting 
        //   conversions take 750ms. At 9-bit conversions take only 94ms.
        // Skip ROM
        ds18b20_send_byte(DS_CMD_SKIP_ROM);
        // Read scratchpad
        ds18b20_send_byte(DS_CMD_WRITE_SCRATCHPAD);
        ds18b20_send_byte(0);
        ds18b20_send_byte(0);
        // Configure resolution 
        //    0x1f = 9 bit  (93.75ms) 0.5C
        //    0x3f = 10 bit (187.5ms) 0.25C
        ds18b20_send_byte(0x1f);
        
        ds18b20_RST_PULSE();
        ds18b20_send_byte(DS_CMD_COPY_SCRATCHPAD);
   }
}

