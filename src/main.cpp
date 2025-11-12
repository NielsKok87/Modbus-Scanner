#include <Arduino.h>
#include <ModbusMaster.h>
#include <FastLED.h>

// WS2812 LED configuration
#define LED_PIN 10        // GPIO 10 for WS2812
#define NUM_LEDS 1        // Single LED
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// Modbus RTU configuration
#define MODBUS_RX_PIN 20  // GPIO 20 for RX (adjust for your wiring)
#define MODBUS_TX_PIN 21  // GPIO 21 for TX (adjust for your wiring)
#define MODBUS_DE_PIN 2   // GPIO 2 for DE/RE (Direction Enable - optional, set to -1 if not used)
#define MODBUS_BAUD 9600  // Common Modbus RTU baud rate

// Modbus slave configuration
#define SLAVE_ID 1        // Default slave ID, change as needed

// Create ModbusMaster object
ModbusMaster modbus;

// LED Status Colors and Functions
enum LEDStatus {
  LED_OFF,              // LED off
  LED_READY,            // System ready (blue)
  LED_SCANNING,         // Scanning for devices (purple pulse)
  LED_SUCCESS,          // Communication success (green)
  LED_ERROR,            // Communication error (red)
  LED_WARNING,          // Warning/timeout (orange)
  LED_WRITING,          // Writing data (yellow)
  LED_CONNECTING        // Trying to connect (cyan)
};

// Forward declarations
void printModbusError(uint8_t result);
void changeModbusSettings(uint32_t newBaud, uint8_t newSlaveId);
void readHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
void readInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
void readCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
void readDiscreteInputs(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
void writeSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value);
void writeSingleCoil(uint8_t slaveId, uint16_t address, bool value);
void scanModbusDevices();
bool autoDetectBaudRate(uint8_t slaveId, uint32_t* detectedBaud);
bool autoDetectSerialConfig(uint8_t slaveId, uint32_t baudRate);
void detectModbusDevice();
void testSpecificSlaveId();
void testDifferentBaudRates();
void readSpecificRegisters();
void writeToRegister();
void showCurrentConfiguration();
void changeSettingsInteractive();
void showHelp();
void showMainMenu();
void setLEDStatus(LEDStatus status, bool animate = true);
void ledStatusMessage(LEDStatus status, const char* message);

// Variables for periodic reading
unsigned long lastModbusRead = 0;
const unsigned long modbusInterval = 1000; // Read every 1 second

LEDStatus currentLEDStatus = LED_OFF;
unsigned long ledAnimationStart = 0;
bool ledAnimationActive = false;

// Function to control DE/RE pin (if used)
void preTransmission() {
  if (MODBUS_DE_PIN >= 0) {
    digitalWrite(MODBUS_DE_PIN, HIGH);
  }
}

void postTransmission() {
  if (MODBUS_DE_PIN >= 0) {
    digitalWrite(MODBUS_DE_PIN, LOW);
  }
}

// LED Control Functions
void initializeLED() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(100); // Set brightness (0-255)
  setLEDStatus(LED_READY);
  Serial.println("🔵 WS2812 LED initialized on GPIO 10");
}

void setLEDStatus(LEDStatus status, bool animate) {
  currentLEDStatus = status;
  ledAnimationActive = animate;
  ledAnimationStart = millis();
  
  switch (status) {
    case LED_OFF:
      leds[0] = CRGB::Black;
      break;
    case LED_READY:
      leds[0] = CRGB::Blue;
      break;
    case LED_SUCCESS:
      leds[0] = CRGB::Green;
      break;
    case LED_ERROR:
      leds[0] = CRGB::Red;
      break;
    case LED_WARNING:
      leds[0] = CRGB::Orange;
      break;
    case LED_WRITING:
      leds[0] = CRGB::Yellow;
      break;
    case LED_CONNECTING:
      leds[0] = CRGB::Cyan;
      break;
    case LED_SCANNING:
      leds[0] = CRGB::Purple;
      break;
  }
  
  FastLED.show();
}

void updateLEDAnimation() {
  if (!ledAnimationActive) return;
  
  unsigned long elapsed = millis() - ledAnimationStart;
  
  switch (currentLEDStatus) {
    case LED_SCANNING: {
      // Purple breathing effect for scanning
      int brightness = (sin(elapsed / 200.0) + 1) * 127;
      leds[0] = CHSV(192, 255, brightness); // Purple hue
      FastLED.show();
      break;
    }
    
    case LED_CONNECTING: {
      // Cyan pulse for connecting
      int brightness = (sin(elapsed / 300.0) + 1) * 127;
      leds[0] = CHSV(128, 255, brightness); // Cyan hue
      FastLED.show();
      break;
    }
    
    case LED_ERROR: {
      // Red blink for errors
      if ((elapsed / 250) % 2 == 0) {
        leds[0] = CRGB::Red;
      } else {
        leds[0] = CRGB::Black;
      }
      FastLED.show();
      
      // Stop blinking after 3 seconds
      if (elapsed > 3000) {
        ledAnimationActive = false;
        setLEDStatus(LED_READY, false);
      }
      break;
    }
    
    case LED_SUCCESS: {
      // Green flash and fade to ready
      if (elapsed < 500) {
        leds[0] = CRGB::Green;
      } else if (elapsed < 1000) {
        // Fade to blue
        int fade = map(elapsed - 500, 0, 500, 255, 0);
        leds[0] = CRGB(0, fade, 255 - fade);
      } else {
        ledAnimationActive = false;
        setLEDStatus(LED_READY, false);
      }
      FastLED.show();
      break;
    }
    
    case LED_WRITING: {
      // Yellow pulse for writing
      int brightness = (sin(elapsed / 150.0) + 1) * 127;
      leds[0] = CHSV(64, 255, brightness); // Yellow hue
      FastLED.show();
      break;
    }
    
    default:
      // For static colors, stop animation after a short time
      if (elapsed > 2000) {
        ledAnimationActive = false;
      }
      break;
  }
}

void ledStatusMessage(LEDStatus status, const char* message) {
  setLEDStatus(status);
  
  // Add LED status emoji to message
  String emoji;
  switch (status) {
    case LED_READY: emoji = "🔵"; break;
    case LED_SUCCESS: emoji = "✅"; break;
    case LED_ERROR: emoji = "🔴"; break;
    case LED_WARNING: emoji = "🟠"; break;
    case LED_WRITING: emoji = "🟡"; break;
    case LED_CONNECTING: emoji = "🔄"; break;
    case LED_SCANNING: emoji = "🟣"; break;
    default: emoji = "⚫"; break;
  }
  
  Serial.println(emoji + " " + String(message));
}

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for Serial to initialize
  
  // Initialize WS2812 LED
  initializeLED();
  
  Serial.println("\n" + String('=', 60));
  Serial.println("🔧 ESP32 C3 Modbus RTU Master - Interactive Setup");
  Serial.println(String('=', 60));
  
  ledStatusMessage(LED_READY, "System starting up...");
  
  // Setup DE/RE pin if used
  if (MODBUS_DE_PIN >= 0) {
    pinMode(MODBUS_DE_PIN, OUTPUT);
    digitalWrite(MODBUS_DE_PIN, LOW); // Start in receive mode
  }
  
  // Show initial configuration
  Serial.printf("📋 Current Configuration:\n");
  Serial.printf("   RX Pin: %d\n", MODBUS_RX_PIN);
  Serial.printf("   TX Pin: %d\n", MODBUS_TX_PIN);
  Serial.printf("   LED Pin: %d (WS2812)\n", LED_PIN);
  if (MODBUS_DE_PIN >= 0) {
    Serial.printf("   DE/RE Pin: %d\n", MODBUS_DE_PIN);
  } else {
    Serial.printf("   DE/RE Pin: Not used\n");
  }
  Serial.printf("   Default Baud: %d\n", MODBUS_BAUD);
  Serial.printf("   Default Slave ID: %d\n", SLAVE_ID);
  
  ledStatusMessage(LED_READY, "System ready! LED status indicators active.");
  
  // Interactive menu
  showMainMenu();
}

void showMainMenu() {
  Serial.println("\n📋 MAIN MENU - Choose an option:");
  Serial.println("1. Auto-detect device (recommended)");
  Serial.println("2. Manual device scan (all slave IDs)");
  Serial.println("3. Test specific slave ID");
  Serial.println("4. Test different baud rates");
  Serial.println("5. Read specific registers");
  Serial.println("6. Write to register");
  Serial.println("7. Show current configuration");
  Serial.println("8. Change settings");
  Serial.println("9. Help/Troubleshooting");
  Serial.println("\nType a number (1-9) and press Enter:");
}

void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() == 1 && input.charAt(0) >= '1' && input.charAt(0) <= '9') {
      int choice = input.toInt();
      
      switch (choice) {
        case 1:
          Serial.println("\n🔍 Starting auto-detection...");
          detectModbusDevice();
          break;
          
        case 2:
          Serial.println("\n🔍 Starting full device scan...");
          scanModbusDevices();
          break;
          
        case 3:
          testSpecificSlaveId();
          break;
          
        case 4:
          testDifferentBaudRates();
          break;
          
        case 5:
          readSpecificRegisters();
          break;
          
        case 6:
          writeToRegister();
          break;
          
        case 7:
          showCurrentConfiguration();
          break;
          
        case 8:
          changeSettingsInteractive();
          break;
          
        case 9:
          showHelp();
          break;
          
        default:
          Serial.println("❌ Invalid option. Please choose 1-9.");
          break;
      }
    } else {
      Serial.println("❌ Please enter a single digit (1-9).");
    }
    
    Serial.println("\n" + String('-', 40));
    showMainMenu();
  }
}

void testSpecificSlaveId() {
  Serial.println("\nEnter Slave ID to test (1-247):");
  
  while (!Serial.available()) delay(10);
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  int slaveId = input.toInt();
  if (slaveId < 1 || slaveId > 247) {
    Serial.println("❌ Invalid Slave ID. Must be between 1-247.");
    return;
  }
  
  Serial.printf("🔍 Testing Slave ID %d...\n", slaveId);
  
  // Initialize with current settings
  Serial1.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  modbus.begin(slaveId, Serial1);
  
  // Test basic communication
  uint8_t result = modbus.readHoldingRegisters(0, 1);
  
  if (result == modbus.ku8MBSuccess) {
    Serial.printf("✅ SUCCESS! Device found at Slave ID %d\n", slaveId);
    uint16_t value = modbus.getResponseBuffer(0);
    Serial.printf("   Register 0 value: %d (0x%04X)\n", value, value);
    
    // Try to read more registers
    Serial.println("📋 Reading first 5 holding registers:");
    readHoldingRegisters(slaveId, 0, 5);
  } else {
    Serial.printf("❌ No response from Slave ID %d\n", slaveId);
    printModbusError(result);
  }
}

void testDifferentBaudRates() {
  Serial.println("\nEnter Slave ID to test (1-247):");
  
  while (!Serial.available()) delay(10);
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  int slaveId = input.toInt();
  if (slaveId < 1 || slaveId > 247) {
    Serial.println("❌ Invalid Slave ID. Must be between 1-247.");
    return;
  }
  
  uint32_t detectedBaud;
  if (autoDetectBaudRate(slaveId, &detectedBaud)) {
    Serial.printf("✅ Device communicates at %d baud\n", detectedBaud);
  } else {
    Serial.println("❌ Could not detect baud rate for this device.");
  }
}

void readSpecificRegisters() {
  Serial.println("\nRegister Reading Setup:");
  
  Serial.println("Enter Slave ID (1-247):");
  while (!Serial.available()) delay(10);
  int slaveId = Serial.readStringUntil('\n').toInt();
  
  Serial.println("Enter register type (1=Holding, 2=Input, 3=Coils, 4=Discrete):");
  while (!Serial.available()) delay(10);
  int regType = Serial.readStringUntil('\n').toInt();
  
  Serial.println("Enter starting address:");
  while (!Serial.available()) delay(10);
  int startAddr = Serial.readStringUntil('\n').toInt();
  
  Serial.println("Enter number of registers to read:");
  while (!Serial.available()) delay(10);
  int quantity = Serial.readStringUntil('\n').toInt();
  
  Serial1.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  switch (regType) {
    case 1:
      readHoldingRegisters(slaveId, startAddr, quantity);
      break;
    case 2:
      readInputRegisters(slaveId, startAddr, quantity);
      break;
    case 3:
      readCoils(slaveId, startAddr, quantity);
      break;
    case 4:
      readDiscreteInputs(slaveId, startAddr, quantity);
      break;
    default:
      Serial.println("❌ Invalid register type.");
      break;
  }
}

void writeToRegister() {
  Serial.println("\nRegister Writing Setup:");
  
  Serial.println("Enter Slave ID (1-247):");
  while (!Serial.available()) delay(10);
  int slaveId = Serial.readStringUntil('\n').toInt();
  
  Serial.println("Enter register address:");
  while (!Serial.available()) delay(10);
  int address = Serial.readStringUntil('\n').toInt();
  
  Serial.println("Enter value to write:");
  while (!Serial.available()) delay(10);
  int value = Serial.readStringUntil('\n').toInt();
  
  Serial1.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  writeSingleRegister(slaveId, address, value);
}

void showCurrentConfiguration() {
  Serial.println("\n📋 CURRENT CONFIGURATION:");
  Serial.printf("   RX Pin: %d\n", MODBUS_RX_PIN);
  Serial.printf("   TX Pin: %d\n", MODBUS_TX_PIN);
  Serial.printf("   DE/RE Pin: %s\n", MODBUS_DE_PIN >= 0 ? String(MODBUS_DE_PIN).c_str() : "Not used");
  Serial.printf("   Baud Rate: %d\n", MODBUS_BAUD);
  Serial.printf("   Default Slave ID: %d\n", SLAVE_ID);
  Serial.printf("   Data Format: 8N1 (8 data bits, No parity, 1 stop bit)\n");
}

void changeSettingsInteractive() {
  Serial.println("\n⚙️ CHANGE SETTINGS:");
  Serial.println("Note: This only changes runtime settings, not permanent configuration.");
  
  Serial.println("\nEnter new baud rate (or press Enter to keep current):");
  while (!Serial.available()) delay(10);
  String baudInput = Serial.readStringUntil('\n');
  baudInput.trim();
  
  Serial.println("Enter new default Slave ID (or press Enter to keep current):");
  while (!Serial.available()) delay(10);
  String slaveInput = Serial.readStringUntil('\n');
  slaveInput.trim();
  
  uint32_t newBaud = baudInput.length() > 0 ? baudInput.toInt() : MODBUS_BAUD;
  uint8_t newSlaveId = slaveInput.length() > 0 ? slaveInput.toInt() : SLAVE_ID;
  
  if (newBaud > 0 && newSlaveId > 0 && newSlaveId <= 247) {
    changeModbusSettings(newBaud, newSlaveId);
  } else {
    Serial.println("❌ Invalid settings. No changes made.");
  }
}

void showHelp() {
  Serial.println("\n📚 TROUBLESHOOTING HELP:");
  Serial.println("\n🔌 Common Wiring Issues:");
  Serial.println("   • RX and TX pins swapped (RX→TX, TX→RX)");
  Serial.println("   • Missing ground connection");
  Serial.println("   • Wrong voltage levels (3.3V vs 5V)");
  Serial.println("   • Missing DE/RE control for RS485");
  
  Serial.println("\n⚙️ Communication Settings:");
  Serial.println("   • Wrong baud rate (try auto-detection)");
  Serial.println("   • Wrong parity settings (most use 8N1)");
  Serial.println("   • Wrong slave ID (try scanning)");
  
  Serial.println("\n📡 RS485 Specific:");
  Serial.println("   • Missing 120Ω termination resistors");
  Serial.println("   • Cable length too long (>1200m)");
  Serial.println("   • Poor quality cables (use twisted pair)");
  
  Serial.println("\n🔧 Testing Steps:");
  Serial.println("   1. Use option 1 (auto-detect) first");
  Serial.println("   2. If that fails, try option 2 (full scan)");
  Serial.println("   3. Check physical connections");
  Serial.println("   4. Verify device documentation");
}

void loop() {
  // Update LED animations
  updateLEDAnimation();
  
  // Handle interactive serial commands
  handleSerialInput();
  
  delay(50); // Small delay to prevent excessive CPU usage while allowing smooth LED animations
}

// Function to read Modbus holding registers
void readHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
  ledStatusMessage(LED_CONNECTING, "Reading holding registers...");
  Serial.printf("\n--- Reading %d holding registers from address %d (Slave ID: %d) ---\n", 
                quantity, startAddress, slaveId);
  
  uint8_t result = modbus.readHoldingRegisters(startAddress, quantity);
  
  if (result == modbus.ku8MBSuccess) {
    ledStatusMessage(LED_SUCCESS, "Holding registers read successfully!");
    
    for (uint16_t i = 0; i < quantity; i++) {
      uint16_t value = modbus.getResponseBuffer(i);
      Serial.printf("Register %d: 0x%04X (%d)\n", startAddress + i, value, value);
    }
  } else {
    ledStatusMessage(LED_ERROR, "Failed to read holding registers");
    printModbusError(result);
  }
}

// Function to read input registers
void readInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
  ledStatusMessage(LED_CONNECTING, "Reading input registers...");
  Serial.printf("\n--- Reading %d input registers from address %d (Slave ID: %d) ---\n", 
                quantity, startAddress, slaveId);
  
  uint8_t result = modbus.readInputRegisters(startAddress, quantity);
  
  if (result == modbus.ku8MBSuccess) {
    ledStatusMessage(LED_SUCCESS, "Input registers read successfully!");
    
    for (uint16_t i = 0; i < quantity; i++) {
      uint16_t value = modbus.getResponseBuffer(i);
      Serial.printf("Register %d: 0x%04X (%d)\n", startAddress + i, value, value);
    }
  } else {
    ledStatusMessage(LED_ERROR, "Failed to read input registers");
    printModbusError(result);
  }
}

// Function to read coils (discrete outputs)
void readCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
  ledStatusMessage(LED_CONNECTING, "Reading coils...");
  Serial.printf("\n--- Reading %d coils from address %d (Slave ID: %d) ---\n", 
                quantity, startAddress, slaveId);
  
  uint8_t result = modbus.readCoils(startAddress, quantity);
  
  if (result == modbus.ku8MBSuccess) {
    ledStatusMessage(LED_SUCCESS, "Coils read successfully!");
    
    for (uint16_t i = 0; i < quantity; i++) {
      bool value = modbus.getResponseBuffer(i / 16) & (1 << (i % 16));
      Serial.printf("Coil %d: %s\n", startAddress + i, value ? "ON" : "OFF");
    }
  } else {
    ledStatusMessage(LED_ERROR, "Failed to read coils");
    printModbusError(result);
  }
}

// Function to read discrete inputs
void readDiscreteInputs(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
  Serial.printf("\n--- Reading %d discrete inputs from address %d (Slave ID: %d) ---\n", 
                quantity, startAddress, slaveId);
  
  uint8_t result = modbus.readDiscreteInputs(startAddress, quantity);
  
  if (result == modbus.ku8MBSuccess) {
    Serial.println("✅ SUCCESS: Discrete inputs read successfully!");
    
    for (uint16_t i = 0; i < quantity; i++) {
      bool value = modbus.getResponseBuffer(i / 16) & (1 << (i % 16));
      Serial.printf("Input %d: %s\n", startAddress + i, value ? "HIGH" : "LOW");
    }
  } else {
    printModbusError(result);
  }
}

// Function to write a single holding register
void writeSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
  ledStatusMessage(LED_WRITING, "Writing to register...");
  Serial.printf("\n--- Writing value %d (0x%04X) to register %d (Slave ID: %d) ---\n", 
                value, value, address, slaveId);
  
  uint8_t result = modbus.writeSingleRegister(address, value);
  
  if (result == modbus.ku8MBSuccess) {
    ledStatusMessage(LED_SUCCESS, "Register written successfully!");
  } else {
    ledStatusMessage(LED_ERROR, "Failed to write register");
    printModbusError(result);
  }
}

// Function to write a single coil
void writeSingleCoil(uint8_t slaveId, uint16_t address, bool value) {
  ledStatusMessage(LED_WRITING, "Writing to coil...");
  Serial.printf("\n--- Writing %s to coil %d (Slave ID: %d) ---\n", 
                value ? "ON" : "OFF", address, slaveId);
  
  uint8_t result = modbus.writeSingleCoil(address, value);
  
  if (result == modbus.ku8MBSuccess) {
    ledStatusMessage(LED_SUCCESS, "Coil written successfully!");
  } else {
    ledStatusMessage(LED_ERROR, "Failed to write coil");
    printModbusError(result);
  }
}

// Main function to demonstrate Modbus communication
void readModbusData() {
  // Change slave ID if needed
  modbus.begin(SLAVE_ID, Serial1);
  
  // Example reads - customize these for your specific device
  // Uncomment the functions you want to test:
  
  // Read 4 holding registers starting from address 0
  readHoldingRegisters(SLAVE_ID, 0, 4);
  
  delay(100); // Small delay between requests
  
  // Read 2 input registers starting from address 0
  // readInputRegisters(SLAVE_ID, 0, 2);
  
  // delay(100);
  
  // Read 8 coils starting from address 0
  // readCoils(SLAVE_ID, 0, 8);
  
  // delay(100);
  
  // Read 8 discrete inputs starting from address 0
  // readDiscreteInputs(SLAVE_ID, 0, 8);
  
  // delay(100);
  
  // Example write operations (uncomment to test):
  // writeSingleRegister(SLAVE_ID, 0, 12345);
  // writeSingleCoil(SLAVE_ID, 0, true);
}

// Function to print detailed Modbus error information
void printModbusError(uint8_t result) {
  switch (result) {
    case modbus.ku8MBSuccess:
      Serial.println("✅ SUCCESS");
      break;
    case modbus.ku8MBIllegalFunction:
      Serial.println("❌ ERROR: Illegal Function (0x01) - The function code is not supported");
      break;
    case modbus.ku8MBIllegalDataAddress:
      Serial.println("❌ ERROR: Illegal Data Address (0x02) - The data address is not valid");
      break;
    case modbus.ku8MBIllegalDataValue:
      Serial.println("❌ ERROR: Illegal Data Value (0x03) - The data value is not valid");
      break;
    case modbus.ku8MBSlaveDeviceFailure:
      Serial.println("❌ ERROR: Slave Device Failure (0x04) - The slave device failed to perform");
      break;
    case modbus.ku8MBInvalidSlaveID:
      Serial.println("❌ ERROR: Invalid Slave ID - No response from slave device");
      break;
    case modbus.ku8MBInvalidFunction:
      Serial.println("❌ ERROR: Invalid Function - Function code not supported by library");
      break;
    case modbus.ku8MBResponseTimedOut:
      Serial.println("❌ ERROR: Response Timed Out - Slave did not respond within timeout period");
      break;
    case modbus.ku8MBInvalidCRC:
      Serial.println("❌ ERROR: Invalid CRC - Data corruption detected");
      break;
    default:
      Serial.printf("❌ ERROR: Unknown error code: 0x%02X\n", result);
      break;
  }
}

// Function to scan for Modbus devices (useful for debugging)
void scanModbusDevices() {
  ledStatusMessage(LED_SCANNING, "Scanning for Modbus devices...");
  Serial.println("\n🔍 Scanning for Modbus devices (IDs 1-247)...");
  Serial.println("This may take a while...\n");
  
  int devicesFound = 0;
  
  for (uint8_t id = 1; id <= 247; id++) {
    modbus.begin(id, Serial1);
    
    // Try to read one holding register
    uint8_t result = modbus.readHoldingRegisters(0, 1);
    
    if (result == modbus.ku8MBSuccess) {
      setLEDStatus(LED_SUCCESS, false); // Brief green flash
      Serial.printf("✅ Device found at ID: %d\n", id);
      devicesFound++;
      delay(100); // Show success briefly
      setLEDStatus(LED_SCANNING); // Back to scanning
    }
    else if (result != modbus.ku8MBResponseTimedOut && result != modbus.ku8MBInvalidSlaveID) {
      setLEDStatus(LED_WARNING, false); // Brief orange flash
      Serial.printf("⚠️  Device at ID %d responded with error: ", id);
      printModbusError(result);
      delay(100);
      setLEDStatus(LED_SCANNING); // Back to scanning
    }
    
    delay(50); // Small delay between attempts
    
    // Print progress every 50 devices
    if (id % 50 == 0) {
      Serial.printf("Progress: %d/247 devices checked\n", id);
    }
  }
  
  if (devicesFound > 0) {
    ledStatusMessage(LED_SUCCESS, "Scan complete - devices found!");
  } else {
    ledStatusMessage(LED_WARNING, "Scan complete - no devices found");
  }
  
  Serial.printf("\n🎯 Scan complete! Found %d device(s)\n", devicesFound);
  if (devicesFound == 0) {
    Serial.println("💡 Tips:");
    Serial.println("   - Check wiring connections (RX, TX, GND)");
    Serial.println("   - Verify baud rate matches your device");
    Serial.println("   - Check if DE/RE pin is needed and properly connected");
    Serial.println("   - Ensure correct voltage levels (3.3V vs 5V)");
  }
}

// Function to change Modbus settings at runtime
void changeModbusSettings(uint32_t newBaud, uint8_t newSlaveId) {
  Serial.printf("🔧 Changing Modbus settings: Baud=%d, Slave ID=%d\n", newBaud, newSlaveId);
  
  // Reinitialize serial with new baud rate
  Serial1.end();
  delay(100);
  Serial1.begin(newBaud, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  // Update ModbusMaster with new slave ID
  modbus.begin(newSlaveId, Serial1);
  
  Serial.println("✅ Settings updated successfully!");
}

// Auto-detect baud rate function
bool autoDetectBaudRate(uint8_t slaveId, uint32_t* detectedBaud) {
  // Common Modbus RTU baud rates to test
  uint32_t baudRates[] = {
    9600,   // Most common
    19200,  // Common
    38400,  // Common
    57600,  // Less common
    115200, // High speed
    4800,   // Older devices
    2400,   // Very old devices
    1200    // Very rare
  };
  
  int numBaudRates = sizeof(baudRates) / sizeof(baudRates[0]);
  
  ledStatusMessage(LED_SCANNING, "Auto-detecting baud rate...");
  Serial.println("\n🔍 AUTO-DETECTING BAUD RATE...");
  Serial.printf("Testing %d different baud rates with Slave ID %d\n\n", numBaudRates, slaveId);
  
  for (int i = 0; i < numBaudRates; i++) {
    uint32_t testBaud = baudRates[i];
    Serial.printf("Testing %d baud... ", testBaud);
    
    // Reinitialize serial with test baud rate
    Serial1.end();
    delay(100);
    Serial1.begin(testBaud, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
    modbus.begin(slaveId, Serial1);
    
    // Try to read a holding register (most devices support this)
    uint8_t result = modbus.readHoldingRegisters(0, 1);
    
    if (result == modbus.ku8MBSuccess) {
      Serial.println("✅ FOUND!");
      ledStatusMessage(LED_SUCCESS, "Baud rate detected!");
      *detectedBaud = testBaud;
      return true;
    } else if (result == modbus.ku8MBIllegalDataAddress) {
      // Device responded but register doesn't exist - still good!
      Serial.println("✅ FOUND! (but register 0 doesn't exist)");
      ledStatusMessage(LED_SUCCESS, "Baud rate detected!");
      *detectedBaud = testBaud;
      return true;
    } else {
      Serial.println("❌ No response");
    }
    
    delay(100); // Small delay between tests
  }
  
  ledStatusMessage(LED_ERROR, "Baud rate detection failed");
  Serial.println("\n❌ No baud rate detected. Device may not be connected or responding.");
  return false;
}

// Auto-detect serial configuration (parity, data bits, stop bits)
bool autoDetectSerialConfig(uint8_t slaveId, uint32_t baudRate) {
  Serial.println("\n🔧 AUTO-DETECTING SERIAL CONFIGURATION...");
  Serial.printf("Testing different configurations at %d baud with Slave ID %d\n\n", baudRate, slaveId);
  
  // Test different serial configurations
  struct {
    uint32_t config;
    const char* name;
  } configs[] = {
    {SERIAL_8N1, "8N1 (8 data, No parity, 1 stop)"},
    {SERIAL_8E1, "8E1 (8 data, Even parity, 1 stop)"},
    {SERIAL_8O1, "8O1 (8 data, Odd parity, 1 stop)"},
    {SERIAL_8N2, "8N2 (8 data, No parity, 2 stop)"},
    {SERIAL_7E1, "7E1 (7 data, Even parity, 1 stop)"},
    {SERIAL_7O1, "7O1 (7 data, Odd parity, 1 stop)"}
  };
  
  int numConfigs = sizeof(configs) / sizeof(configs[0]);
  
  for (int i = 0; i < numConfigs; i++) {
    Serial.printf("Testing %s... ", configs[i].name);
    
    // Reinitialize serial with test configuration
    Serial1.end();
    delay(100);
    Serial1.begin(baudRate, configs[i].config, MODBUS_RX_PIN, MODBUS_TX_PIN);
    modbus.begin(slaveId, Serial1);
    
    // Try to read a holding register
    uint8_t result = modbus.readHoldingRegisters(0, 1);
    
    if (result == modbus.ku8MBSuccess || result == modbus.ku8MBIllegalDataAddress) {
      Serial.println("✅ WORKS!");
      Serial.printf("🎯 Detected configuration: %s\n", configs[i].name);
      return true;
    } else {
      Serial.println("❌ Failed");
    }
    
    delay(100);
  }
  
  Serial.println("\n⚠️  No configuration detected. Using default 8N1.");
  // Reset to default
  Serial1.end();
  delay(100);
  Serial1.begin(baudRate, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  modbus.begin(slaveId, Serial1);
  return false;
}

// Comprehensive device detection and configuration
void detectModbusDevice() {
  ledStatusMessage(LED_SCANNING, "Starting comprehensive device detection...");
  Serial.println("\n" + String('=', 60));
  Serial.println("🔍 COMPREHENSIVE MODBUS DEVICE DETECTION");
  Serial.println(String('=', 60));
  
  // First, try to find devices at different slave IDs with default settings
  Serial.println("\n📡 Phase 1: Scanning for device IDs (using default 9600 baud, 8N1)...");
  
  Serial1.begin(9600, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  uint8_t foundSlaveIds[10]; // Store up to 10 found slave IDs
  int foundCount = 0;
  
  for (uint8_t id = 1; id <= 10 && foundCount < 10; id++) { // Quick scan first 10 IDs
    modbus.begin(id, Serial1);
    uint8_t result = modbus.readHoldingRegisters(0, 1);
    
    if (result == modbus.ku8MBSuccess || result == modbus.ku8MBIllegalDataAddress) {
      Serial.printf("✅ Device found at Slave ID: %d\n", id);
      setLEDStatus(LED_SUCCESS, false); // Brief success flash
      foundSlaveIds[foundCount++] = id;
      delay(100);
      setLEDStatus(LED_SCANNING); // Back to scanning
    }
    delay(50);
  }
  
  if (foundCount == 0) {
    Serial.println("❌ No devices found with default settings. Trying auto-detection...");
    
    // Phase 2: Auto-detect baud rate for common slave IDs
    Serial.println("\n📡 Phase 2: Auto-detecting baud rate...");
    
    uint8_t commonIds[] = {1, 2, 3, 247}; // Common slave IDs to test
    bool detected = false;
    uint32_t detectedBaud;
    uint8_t detectedId;
    
    for (int i = 0; i < 4 && !detected; i++) {
      Serial.printf("\n--- Trying Slave ID %d ---\n", commonIds[i]);
      if (autoDetectBaudRate(commonIds[i], &detectedBaud)) {
        detected = true;
        detectedId = commonIds[i];
        foundSlaveIds[0] = detectedId;
        foundCount = 1;
        
        // Phase 3: Auto-detect serial configuration
        Serial.println("\n📡 Phase 3: Auto-detecting serial configuration...");
        autoDetectSerialConfig(detectedId, detectedBaud);
      }
    }
    
    if (!detected) {
      ledStatusMessage(LED_ERROR, "Auto-detection failed");
      Serial.println("\n❌ Could not auto-detect any devices.");
      Serial.println("💡 Manual troubleshooting suggestions:");
      Serial.println("   1. Check physical connections (RX ↔ TX, TX ↔ RX, GND ↔ GND)");
      Serial.println("   2. Verify power supply to the device");
      Serial.println("   3. Check if DE/RE control is needed for RS485");
      Serial.println("   4. Try different slave IDs (some devices use non-standard IDs)");
      Serial.println("   5. Check device documentation for communication settings");
      return;
    }
  }
  
  // Phase 4: Get device information
  Serial.println("\n📊 Phase 4: Reading device information...");
  ledStatusMessage(LED_CONNECTING, "Reading device information...");
  
  for (int i = 0; i < foundCount; i++) {
    uint8_t slaveId = foundSlaveIds[i];
    Serial.printf("\n--- Device Information (Slave ID: %d) ---\n", slaveId);
    modbus.begin(slaveId, Serial1);
    
    // Try to read common registers
    Serial.println("📋 Attempting to read common registers:");
    
    // Try holding registers 0-9
    for (int reg = 0; reg < 10; reg++) {
      uint8_t result = modbus.readHoldingRegisters(reg, 1);
      if (result == modbus.ku8MBSuccess) {
        uint16_t value = modbus.getResponseBuffer(0);
        Serial.printf("  Holding Register %d: %d (0x%04X)\n", reg, value, value);
      }
      delay(50);
    }
    
    // Try input registers 0-4
    Serial.println("📈 Input Registers:");
    for (int reg = 0; reg < 5; reg++) {
      uint8_t result = modbus.readInputRegisters(reg, 1);
      if (result == modbus.ku8MBSuccess) {
        uint16_t value = modbus.getResponseBuffer(0);
        Serial.printf("  Input Register %d: %d (0x%04X)\n", reg, value, value);
      }
      delay(50);
    }
  }
  
  ledStatusMessage(LED_SUCCESS, "Device detection complete!");
  Serial.println("\n" + String('=', 60));
  Serial.println("✅ DETECTION COMPLETE!");
  Serial.printf("Found %d device(s). Check output above for details.\n", foundCount);
  Serial.println(String('=', 60));
}
