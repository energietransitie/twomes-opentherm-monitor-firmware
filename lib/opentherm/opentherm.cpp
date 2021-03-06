#include "opentherm.h"
#include <bitset>         // std::bitset

static xQueueHandle opentherm_gpio_evt_queue = NULL;
static const char *TAG = "Twomes OpenTherm Library ESP32";

//Gpio ISR handler for OpenTherm library: 
static void IRAM_ATTR opentherm_gpio_isr_handler(void *arg) {
	uint32_t gpio_num = (uint32_t)arg;
	xQueueSendFromISR(opentherm_gpio_evt_queue, &gpio_num, NULL);
} //opentherm_gpio_isr_handler

void openthermInterruptHandler(void *args) {
	uint32_t io_num;
	extern OpenTherm mOT; //TODO: check whether this is correct
	extern OpenTherm sOT; //TODO: check whether this is correct
	while (true) {
		if (xQueueReceive(opentherm_gpio_evt_queue, &io_num, portMAX_DELAY)) {
			//TODO: achieve similar effect as Arduino code in OpenTherm::begin calls on mOT and sOT
			// attachInterrupt(digitalPinToInterrupt(MASTER_IN_PIN_GPIO19), mOT.handleInterrupt, CHANGE);
			// attachInterrupt(digitalPinToInterrupt(SLAVE_IN_PIN_GPIO21), sOT.handleInterrupt, CHANGE);
			if (io_num == MASTER_IN_PIN_GPIO19) {
				mOT.handleInterrupt(NULL);
			}
			else if (io_num == SLAVE_IN_PIN_GPIO21) {
				sOT.handleInterrupt(NULL);
			}
			else if (io_num == WIFI_RESET_BUTTON_GPIO16_SW1) {
				vTaskDelay(100 / portTICK_PERIOD_MS); //Debounce delay: nothing happens if user presses button for less than 100 ms = 0.1s
				//INTERRUPT HANDLER WIFI_RESET_BUTTON_GPIO16_SW1
				uint8_t halfSeconds = 0;
				//Determine length of button press before taking action
				while (!gpio_get_level(WIFI_RESET_BUTTON_GPIO16_SW1)) {
					//Button WIFI_RESET_BUTTON_GPIO16_SW1 is (still) pressed
					vTaskDelay(500 / portTICK_PERIOD_MS); //Wait for 0.5s
					halfSeconds++;
					ESP_LOGI(TAG, "Button WIFI_RESET_BUTTON_GPIO16_SW1 has been pressed for %u half-seconds now", halfSeconds);
					if (halfSeconds >= LONG_BUTTON_PRESS_DURATION) {
						//Long press on WIFI_RESET_BUTTON_GPIO16_SW1 is for clearing Wi-Fi provisioning memory:
						ESP_LOGI("ISR", "Long-button press detected on button WIFI_RESET_BUTTON_GPIO16_SW1; resetting Wi-Fi provisioning and restarting device");
						char blinkArgs[2] = { 5, RED_LED_ERROR_GPIO22 };
						xTaskCreatePinnedToCore(blink, "blink_5_times_red", 768, (void *)blinkArgs, 10, NULL, 1);
						esp_wifi_restore();
						// also remove bearer to force re-activation
						delete_bearer();
						vTaskDelay(5 * (200 + 200) / portTICK_PERIOD_MS); //Wait for blink to finish
						esp_restart();                         //software restart, to enable linking to new Wi-Fi network.
						break;                                 //Exit loop (this should not be reached)
					}                                          //if (halfSeconds == 9)
					else if (gpio_get_level(WIFI_RESET_BUTTON_GPIO16_SW1)) {
						//Button WIFI_RESET_BUTTON_GPIO16_SW1 is released
						//Short press on WIFI_RESET_BUTTON_GPIO16_SW1 not used for anything currently
						ESP_LOGI("ISR", "Short button press detected on button WIFI_RESET_BUTTON_GPIO16_SW1");
					}
				} //while(!gpio_level)
			}
		}     //if(xQueueReceive)
	}  //while(true)
} // openthermInterruptHandler

void intialize_opentherm_gpio() {
    ESP_LOGD(TAG, "starting initialize_opentherm_gpio();");

    gpio_config_t io_conf;
    //CONFIGURE OUTPUTS:
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = OPENTHERM_OUTPUT_BITMASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    //CONFIGURE INPUTS:
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = OPENTHERM_INPUT_BITMASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    ESP_LOGD(TAG, "exiting initialize_opentherm_gpio()");
}

void start_opentherm_interrupt_handling() {
	opentherm_gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	xTaskCreatePinnedToCore(openthermInterruptHandler, "handle_opentherm_interrupts", 2048, NULL, 10, NULL, 1);
	gpio_isr_handler_add(WIFI_RESET_BUTTON_GPIO16_SW1, opentherm_gpio_isr_handler, (void *)WIFI_RESET_BUTTON_GPIO16_SW1);
}

OpenTherm::OpenTherm(int inPin, int outPin, bool isSlave) : 
	status(OpenThermStatus::NOT_INITIALIZED),
	inPin(inPin),
	outPin(outPin),
	isSlave(isSlave),
	response(0),
	responseStatus(OpenThermResponseStatus::NONE),
	responseTimestamp(0),
	handleInterruptCallback(NULL),
	processResponseCallback(NULL) {}


void OpenTherm::begin(void (*handleInterruptCallback)(void *), void (*processResponseCallback)(unsigned long, OpenThermResponseStatus)) {
		//Arduino code:
		//pinMode(inPin, INPUT);
		//pinMode(outPin, OUTPUT);

		//ESP-IDF code:
		//set gpio configuration in 
	if (handleInterruptCallback != NULL) {
		this->handleInterruptCallback = handleInterruptCallback;
		//Arduino code:
		//attachInterrupt(digitalPinToInterrupt(inPin), handleInterruptCallback, CHANGE);

		//ESP-IDF code:
		gpio_isr_handler_add((gpio_num_t)inPin, opentherm_gpio_isr_handler, (void *)inPin);

	}
	status = OpenThermStatus::READY;
	this->processResponseCallback = processResponseCallback;
}

void OpenTherm::begin(void (*handleInterruptCallback)(void *)) {
	begin(handleInterruptCallback, NULL);
}

bool ICACHE_RAM_ATTR OpenTherm::isReady() {
	return status == OpenThermStatus::READY;
}

int ICACHE_RAM_ATTR OpenTherm::readState() {
	// Arduino code:
	// return digitalRead(inPin);

	// ESP-IDF code:
	return gpio_get_level((gpio_num_t)inPin);
}

void OpenTherm::setActiveState() {
	//Arduino code:
	//digitalWrite(outPin, LOW);

	//ESP-IDF code:
	esp_err_t err = gpio_set_level((gpio_num_t)outPin, LOW);
	if (err != ESP_OK) {
		ESP_LOGE("OpenTherm", "Error gpio: %s", esp_err_to_name(err));
	}
}

void OpenTherm::setIdleState() {
	//Arduino code:
	// digitalWrite(outPin, HIGH);

	//ESP-IDF code:
	esp_err_t err = gpio_set_level((gpio_num_t)outPin, HIGH);
	if (err != ESP_OK) {
		ESP_LOGE("OpenTherm", "Error gpio: %s", esp_err_to_name(err));
	}
}

void OpenTherm::activateBoiler() {
	setIdleState();
	//Arduino code:
	//delay(1000);

	//ESP-IDF code:
	vTaskDelay(1000);
}

void OpenTherm::sendBit(bool high) {
	if (high)
		setActiveState();
	else
		setIdleState();

	//Arduino code:
	//delayMicroseconds(500);

	//ESP-IDF code:
	ets_delay_us(500);

	if (high)
		setIdleState();
	else
		setActiveState();

	//Arduino code:
	//delayMicroseconds(500);

	//ESP-IDF code:
	ets_delay_us(500);
}

bool OpenTherm::sendRequestAync(unsigned long request) {
	//Serial.println("Request: " + String(request, HEX));
	//Arduino code:
	// noInterrupts();

	//ESP-IDF code:
	gpio_intr_disable((gpio_num_t)inPin);

	const bool ready = isReady();
	//Arduino code:
	// interrupts();

	//ESP-IDF code:
	gpio_intr_enable((gpio_num_t)inPin);

	if (!ready)
		return false;

	status = OpenThermStatus::REQUEST_SENDING;
	response = 0;
	responseStatus = OpenThermResponseStatus::NONE;

	sendBit(HIGH); //start bit
	std::bitset<32> request_bitset(request);
	for (int i = 31; i >= 0; i--) {
		sendBit(request_bitset[i]);
	}
	sendBit(HIGH); //stop bit
	setIdleState();

	status = OpenThermStatus::RESPONSE_WAITING;

	//Arduino code:
	// responseTimestamp = micros();

	//ESP-IDF code:
	responseTimestamp = esp_timer_get_time();
	return true;
}

unsigned long OpenTherm::sendRequest(unsigned long request) {
	if (!sendRequestAync(request))
		return 0;
	while (!isReady()) {
		process();
		vTaskDelay(10);
	}
	return response;
}

bool OpenTherm::sendResponse(unsigned long request) {
	status = OpenThermStatus::REQUEST_SENDING;
	response = 0;
	responseStatus = OpenThermResponseStatus::NONE;

	sendBit(HIGH); //start bit
	std::bitset<32> request_bitset(request);
	for (int i = 31; i >= 0; i--) {
		sendBit(request_bitset[i]);
	}
	sendBit(HIGH); //stop bit
	setIdleState();
	status = OpenThermStatus::READY;
	return true;
}

OpenThermResponseStatus OpenTherm::getLastResponseStatus() {
	return responseStatus;
}

void ICACHE_RAM_ATTR OpenTherm::handleInterrupt(void *) {
	esp_task_wdt_reset(); //added by Kevin for ESP-IDF code
	if (isReady()) {
		if (isSlave && readState() == HIGH) {
			status = OpenThermStatus::RESPONSE_WAITING;
		}
		else {
			return;
		}
	}

	unsigned long newTs = esp_timer_get_time();

	if (status == OpenThermStatus::RESPONSE_WAITING) {
		if (readState() == HIGH) {
			status = OpenThermStatus::RESPONSE_START_BIT;
			responseTimestamp = newTs;
	//		ESP_LOGD(TAG, "RESPONSE_STARTBIT");
		}
		else {
			status = OpenThermStatus::RESPONSE_INVALID;
			responseTimestamp = newTs;
		}
	}
	else if (status == OpenThermStatus::RESPONSE_START_BIT) {
		/*if (readState() == LOW) {
			float diff = float ((newTs - responseTimestamp))/1000;
			ESP_LOGI(TAG, "Time to response start bit: %.2f ms", diff);
		}*/
		if ((newTs - responseTimestamp < 750) && readState() == LOW) {
			status = OpenThermStatus::RESPONSE_RECEIVING;
			responseTimestamp = newTs;
			responseBitIndex = 0;
		}
		else {
			status = OpenThermStatus::RESPONSE_INVALID;
			responseTimestamp = newTs;
		}
	}
	else if (status == OpenThermStatus::RESPONSE_RECEIVING) {
		if ((newTs - responseTimestamp) > 750) {
			if (responseBitIndex < 32) {
				response = (response << 1) | !readState();
				responseTimestamp = newTs;
				responseBitIndex++;
	//			ESP_LOGD(TAG, "RESPONSE_RECEIVING: %lX", response);
			}
			else { //stop bit
				status = OpenThermStatus::RESPONSE_READY;
	//			ESP_LOGD(TAG, "RESPONSE_READY: %lX", response);
				responseTimestamp = newTs;
			}
		}
	}
	return; //Arduino code does NOT return
}

void OpenTherm::process() {
	// noInterrupts();
	gpio_intr_disable((gpio_num_t)inPin);
	OpenThermStatus st = status;
	unsigned long ts = responseTimestamp;
	gpio_intr_enable((gpio_num_t)inPin);
	// interrupts();

	if (st == OpenThermStatus::READY)
		return;
	unsigned long newTs = esp_timer_get_time();
	if (st != OpenThermStatus::NOT_INITIALIZED && (newTs - ts) > 1000000) {
		// ESP_LOGD(TAG, "RESPONSE_TIMEOUT: %lX", response);
		status = OpenThermStatus::READY;
		responseStatus = OpenThermResponseStatus::TIMEOUT;
		if (processResponseCallback != NULL) {
			processResponseCallback(response, responseStatus);
		}
	}
	else if (st == OpenThermStatus::RESPONSE_INVALID) {
		// ESP_LOGD(TAG, "RESPONSE_INVALID: %lX", response);
		status = OpenThermStatus::DELAY;
		responseStatus = OpenThermResponseStatus::INVALID;
		if (processResponseCallback != NULL) {
			processResponseCallback(response, responseStatus);
		}
	}
	else if (st == OpenThermStatus::RESPONSE_READY) {
		// ESP_LOGD(TAG, "RESPONSE_READY: %lX", response);
		status = OpenThermStatus::DELAY;
		responseStatus = (isSlave ? isValidRequest(response) : isValidResponse(response)) ? OpenThermResponseStatus::SUCCESS : OpenThermResponseStatus::INVALID;
		if (processResponseCallback != NULL) {
			processResponseCallback(response, responseStatus);
		}
	}
	else if (st == OpenThermStatus::DELAY) {
		// ESP_LOGD(TAG, "RESPONSE_DELAY: %lX", response);
		if ((newTs - ts) > 100000) {
			status = OpenThermStatus::READY;
			// ESP_LOGD(TAG, "RESPONSE_READY: %lX", response);
		}
	}
}

bool OpenTherm::parity(unsigned long frame) //odd parity
{
	uint8_t p = 0;
	while (frame > 0) {
		if (frame & 1)
			p++;
		frame = frame >> 1;
	}
	return (p & 1);
}

OpenThermMessageType OpenTherm::getMessageType(unsigned long message) {
	OpenThermMessageType msg_type = static_cast<OpenThermMessageType>((message >> 28) & 7);
	return msg_type;
}

OpenThermMessageID OpenTherm::getDataID(unsigned long frame) {
	return (OpenThermMessageID)((frame >> 16) & 0xFF);
}

unsigned long OpenTherm::buildRequest(OpenThermMessageType type, OpenThermMessageID id, unsigned int data) {
	unsigned long request = data;
	if (type == OpenThermMessageType::WRITE_DATA) {
		request |= 1ul << 28;
	}
	request |= ((unsigned long)id) << 16;
	if (parity(request))
		request |= (1ul << 31);
	return request;
}

unsigned long OpenTherm::buildResponse(OpenThermMessageType type, OpenThermMessageID id, unsigned int data) {
	unsigned long response = data;
	response |= type << 28;
	response |= ((unsigned long)id) << 16;
	if (parity(response))
		response |= (1ul << 31);
	return response;
}

bool OpenTherm::isValidResponse(unsigned long response) {
	if (parity(response))
		return false;
	uint8_t msgType = (response << 1) >> 29;
	return msgType == READ_ACK || msgType == WRITE_ACK;
}

bool OpenTherm::isValidRequest(unsigned long request) {
	if (parity(request))
		return false;
	uint8_t msgType = (request << 1) >> 29;
	return msgType == READ_DATA || msgType == WRITE_DATA;
}

void OpenTherm::end() {
	if (this->handleInterruptCallback != NULL) {
		// Arduino code:
		// detachInterrupt(digitalPinToInterrupt(inPin));

		// ESP-IDF code:
		gpio_isr_handler_remove((gpio_num_t)inPin);
		gpio_intr_disable((gpio_num_t)inPin);
	}
}

const char *OpenTherm::statusToString(OpenThermResponseStatus status) {
	switch (status) {
		case NONE:
			return "NONE";
		case SUCCESS:
			return "SUCCESS";
		case INVALID:
			return "INVALID";
		case TIMEOUT:
			return "TIMEOUT";
		default:
			return "UNKNOWN";
	}
}

const char *OpenTherm::messageTypeToString(OpenThermMessageType message_type) {
	switch (message_type) {
		case READ_DATA:
			return "READ_DATA";
		case WRITE_DATA:
			return "WRITE_DATA";
		case INVALID_DATA:
			return "INVALID_DATA";
		case RESERVED:
			return "RESERVED";
		case READ_ACK:
			return "READ_ACK";
		case WRITE_ACK:
			return "WRITE_ACK";
		case DATA_INVALID:
			return "DATA_INVALID";
		case UNKNOWN_DATA_ID:
			return "UNKNOWN_DATA_ID";
		default:
			return "UNKNOWN";
	}
}

//building requests

unsigned long OpenTherm::buildSetBoilerStatusRequest(bool enableCentralHeating, bool enableHotWater, bool enableCooling, bool enableOutsideTemperatureCompensation, bool enableCentralHeating2) {
	unsigned int data = enableCentralHeating | (enableHotWater << 1) | (enableCooling << 2) | (enableOutsideTemperatureCompensation << 3) | (enableCentralHeating2 << 4);
	data <<= 8;
	return buildRequest(OpenThermMessageType::READ_DATA, OpenThermMessageID::Status, data);
}

unsigned long OpenTherm::buildSetBoilerTemperatureRequest(float temperature) {
	unsigned int data = temperatureToData(temperature);
	return buildRequest(OpenThermMessageType::WRITE_DATA, OpenThermMessageID::TSet, data);
}

unsigned long OpenTherm::buildGetBoilerTemperatureRequest() {
	return buildRequest(OpenThermMessageType::READ_DATA, OpenThermMessageID::Tboiler, 0);
}

//parsing responses
bool OpenTherm::isFault(unsigned long response) {
	return response & 0x1;
}

bool OpenTherm::isCentralHeatingActive(unsigned long response) {
	return response & 0x2;
}

bool OpenTherm::isHotWaterActive(unsigned long response) {
	return response & 0x4;
}

bool OpenTherm::isFlameOn(unsigned long response) {
	return response & 0x8;
}

bool OpenTherm::isCoolingActive(unsigned long response) {
	return response & 0x10;
}

bool OpenTherm::isDiagnostic(unsigned long response) {
	return response & 0x40;
}

uint16_t OpenTherm::getUInt(const unsigned long response) const {
	const uint16_t u88 = response & 0xffff;
	return u88;
}

float OpenTherm::getFloat(const unsigned long response) const {
	const uint16_t u88 = getUInt(response);
	const float f = (u88 & 0x8000) ? -(0x10000L - u88) / 256.0f : u88 / 256.0f;
	return f;
}

unsigned int OpenTherm::temperatureToData(float temperature) {
	if (temperature < 0)
		temperature = 0;
	if (temperature > 100)
		temperature = 100;
	unsigned int data = (unsigned int)(temperature * 256);
	return data;
}

//basic requests

unsigned long OpenTherm::setBoilerStatus(bool enableCentralHeating, bool enableHotWater, bool enableCooling, bool enableOutsideTemperatureCompensation, bool enableCentralHeating2) {
	return sendRequest(buildSetBoilerStatusRequest(enableCentralHeating, enableHotWater, enableCooling, enableOutsideTemperatureCompensation, enableCentralHeating2));
}

bool OpenTherm::setBoilerTemperature(float temperature) {
	unsigned long response = sendRequest(buildSetBoilerTemperatureRequest(temperature));
	return isValidResponse(response);
}

float OpenTherm::getBoilerTemperature() {
	unsigned long response = sendRequest(buildGetBoilerTemperatureRequest());
	return isValidResponse(response) ? getFloat(response) : 0;
}

float OpenTherm::getReturnTemperature() {
	unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0));
	return isValidResponse(response) ? getFloat(response) : 0;
}

bool OpenTherm::setDHWSetpoint(float temperature) {
	unsigned int data = temperatureToData(temperature);
	unsigned long response = sendRequest(buildRequest(OpenThermMessageType::WRITE_DATA, OpenThermMessageID::TdhwSet, data));
	return isValidResponse(response);
}

float OpenTherm::getDHWTemperature() {
	unsigned long response = sendRequest(buildRequest(OpenThermMessageType::READ_DATA, OpenThermMessageID::Tdhw, 0));
	return isValidResponse(response) ? getFloat(response) : 0;
}

float OpenTherm::getModulation() {
	unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0));
	return isValidResponse(response) ? getFloat(response) : 0;
}

float OpenTherm::getPressure() {
	unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0));
	return isValidResponse(response) ? getFloat(response) : 0;
}

unsigned char OpenTherm::getFault() {
	return ((sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::ASFflags, 0)) >> 8) & 0xff);
}