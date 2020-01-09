#include "arduino_secrets.h" 

// WiFi Connection Variables
#include <WiFi101.h>
#include <SPI.h>

char ssid[] = SECRET_SSID;      //  your network SSID (name)
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

WiFiClient WiFi_Client;

// MQTT variables
#include <MQTTClient.h>
#include <MQTT.h>

MQTTClient MQTT_CLIENT(1024);


// Microphone sampling variables
#include <avdweb_AnalogReadFast.h>
#define SAMPLE_FREQUENCY 4400

#define PAYLOAD_CHUNKS 64

int MICROPHONE_IN_PIN = A1;
float TIME_SINCE_LAST_SAMPLE = micros();
float TIME_SINCE_SAMPLING_STARTED = micros();

int READS_IN_A_SECOND = 0;

int READ_OFFSET = 0;
int SAMPLES[ SAMPLE_FREQUENCY ];

int SAMPLE_DELAY = 1000000 / SAMPLE_FREQUENCY;

void connectToWiFi(){
	while ( status != WL_CONNECTED) {
		Serial.print("Attempting to connect to WiFi network named: ");
		Serial.println(ssid);
		// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
		status = WiFi.begin(ssid, pass);
		// wait 10 seconds for connection:
		delay(10000);
	}

	Serial.print("Connected to WiFi network: ");
	Serial.println(ssid);
}

void connectToMQTTBroker(){
	MQTT_CLIENT.begin(MQTT_BROKER, WiFi_Client);
	
	while (!MQTT_CLIENT.connect("ping-pong-o-tron", MQTT_USER, MQTT_PASS)) {
		Serial.print(".");
		delay(1000);
	}
	
	Serial.println("Connected to MQTT Broker");
	
}

void setup() {

	Serial.begin(115200);

	// check for the presence of the shield:
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield not present");
		// don't continue:
		while (true);
	} else {
		Serial.println("WiFi Shield present");
	}

	connectToWiFi();

	connectToMQTTBroker();  

}

void loop() {
	
	long loopTime = micros();

	if(WiFi.status() != WL_CONNECTED){
		Serial.println("Lost connection to network. Attempting to re-establish...");
		connectToWiFi();
	}
	
	if(!MQTT_CLIENT.connected()){
		Serial.println("Lost connection to broker. Attempting to re-establish...");
		connectToMQTTBroker();
	}
	
	if(READ_OFFSET == 0){
		TIME_SINCE_SAMPLING_STARTED = micros();
	}

	if(loopTime - TIME_SINCE_LAST_SAMPLE < 0){
		TIME_SINCE_LAST_SAMPLE = 0;
		READ_OFFSET = 0;
	}
	
	if(loopTime - TIME_SINCE_LAST_SAMPLE > SAMPLE_DELAY){
		SAMPLES[READ_OFFSET] = analogReadFast(MICROPHONE_IN_PIN);
		READ_OFFSET++;
		TIME_SINCE_LAST_SAMPLE = loopTime;
	}

	if(READ_OFFSET > SAMPLE_FREQUENCY){
		 Serial.print(String(READ_OFFSET) + " ");
		 Serial.println(micros() - TIME_SINCE_SAMPLING_STARTED);

		String payload = "";

		for(int i = 0; i < SAMPLE_FREQUENCY; i++){
			
			payload += String(SAMPLES[i]) + ",";
			
			if(i % PAYLOAD_CHUNKS == 0 && i != 0){
				MQTT_CLIENT.publish("ping-pong-o-tron/sample/" + String(loopTime), payload);
				payload = "";
			}
			
		}
		
		READ_OFFSET = 0;
		
	}
	
}
