# 🔍 ESP32 C3 Modbus RTU Scanner & Diagnostics Tool

Een geavanceerde, interactieve Modbus RTU master implementatie voor ESP32 C3 microcontrollers met visuele status feedback via WS2812 LED en uitgebreide auto-detectie mogelijkheden.

## ✨ Functies

### 🔧 **Automatische Device Detectie**
- **Intelligente Auto-detectie**: Automatisch detecteren van Modbus apparaten met optimale instellingen
- **Baud Rate Detectie**: Test 8 verschillende baud rates (1200-115200 bps)
- **Serial Configuratie Detectie**: Automatische detectie van parity, data bits en stop bits
- **Multi-fase Scanning**: Gestructureerde aanpak voor maximale compatibiliteit

### 📡 **Uitgebreide Modbus Communicatie**
- **Volledige Function Code Support**:
  - 📊 Holding Registers (Read/Write)
  - 📈 Input Registers (Read)
  - 🔗 Coils (Read/Write)
  - 🔌 Discrete Inputs (Read)
- **Slave ID Scanning**: Scan alle mogelijke Slave IDs (1-247)
- **Realtime Register Monitoring**: Live uitlezen van register waarden
- **Error Diagnostics**: Gedetailleerde foutmeldingen met oplossingsrichtingen

### 🎨 **Visuele Status Indicatoren (WS2812 LED)**
- 🔵 **Blauw**: System Ready
- 🟣 **Paars (pulse)**: Scanning for devices
- 🔄 **Cyaan (pulse)**: Connecting/Reading
- ✅ **Groen**: Communication success
- 🔴 **Rood (blink)**: Communication error
- 🟠 **Oranje**: Warning/timeout
- 🟡 **Geel (pulse)**: Writing data

### 🖥️ **Interactief Menu Systeem**
1. **Auto-detect device** (Aanbevolen) - Volledige automatische detectie
2. **Manual device scan** - Scan alle Slave IDs handmatig
3. **Test specific slave ID** - Test individuele apparaten
4. **Test different baud rates** - Baud rate diagnostics
5. **Read specific registers** - Targeted register reading
6. **Write to register** - Register modificatie
7. **Show current configuration** - Systeemstatus weergave
8. **Change settings** - Runtime configuratie aanpassing
9. **Help/Troubleshooting** - Uitgebreide troubleshooting gids

## 🔌 Hardware Specificaties

### **ESP32 C3 Pin Configuratie**
```
GPIO 10  → WS2812 LED Data Pin
GPIO 20  → Modbus RX Pin
GPIO 21  → Modbus TX Pin
GPIO 2   → DE/RE Pin (optioneel voor RS485)
GND      → Common Ground
3.3V/5V  → Power Supply
```

### **Ondersteunde Boards**
- ✅ Adafruit QT Py ESP32-C3
- ✅ Andere ESP32 C3 development boards
- 🔧 Aanpasbare pin configuratie

## 📋 Technische Specificaties

| **Feature** | **Specificatie** |
|-------------|------------------|
| **Platform** | ESP32 C3 (Adafruit QT Py) |
| **Framework** | Arduino |
| **Modbus Library** | ModbusMaster v2.0.1 |
| **LED Library** | FastLED v3.6.0 |
| **Baud Rates** | 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 |
| **Serial Formats** | 8N1, 8E1, 8O1, 8N2, 7E1, 7O1 |
| **Slave ID Range** | 1-247 |
| **Monitor Speed** | 115200 bps |

## 🚀 Installatie & Setup

### **1. PlatformIO Projectstructuur**
```
Modbus-Scanner/
├── src/
│   └── main.cpp           # Hoofdprogramma
├── include/              # Header bestanden
├── lib/                  # Project libraries
├── platformio.ini        # Project configuratie
└── README.md             # Deze documentatie
```

### **2. Dependencies**
Het project gebruikt de volgende libraries (automatisch geïnstalleerd):
```ini
lib_deps = 
    4-20ma/ModbusMaster@^2.0.1    # Modbus RTU communicatie
    fastled/FastLED@^3.6.0        # WS2812 LED controle
```

### **3. Compileren & Uploaden**
```bash
# Project bouwen
platformio run

# Upload naar ESP32
platformio run --target upload

# Monitor serial output
platformio device monitor
```

## 🔧 Configuratie

### **Hardware Aanpassingen**
Pas de volgende defines aan in `main.cpp` voor jouw hardware setup:

```cpp
// WS2812 LED configuratie
#define LED_PIN 10        // GPIO pin voor WS2812
#define NUM_LEDS 1        // Aantal LEDs

// Modbus RTU configuratie
#define MODBUS_RX_PIN 20  // RX pin
#define MODBUS_TX_PIN 21  // TX pin
#define MODBUS_DE_PIN 2   // DE/RE pin (of -1 indien niet gebruikt)
#define MODBUS_BAUD 9600  // Default baud rate

// Default slave configuratie
#define SLAVE_ID 1        // Default slave ID
```

### **RS485 Configuratie**
Voor RS485 communicatie:
- Verbind **DE/RE pin** (GPIO 2) met de RS485 transceiver
- Gebruik **120Ω terminatie resistors** aan beide uiteinden van de bus
- Zorg voor correcte **A/B polariteit**

## 📖 Gebruikshandleiding

### **Quick Start Guide**

1. **Power On**: ESP32 start met blauwe LED (ready)
2. **Open Serial Monitor**: 115200 baud
3. **Selecteer Optie 1**: "Auto-detect device" (aanbevolen)
4. **Volg LED Status**:
   - 🟣 Scanning in progress
   - ✅ Device found
   - 🔴 No device found

### **Auto-Detectie Proces**
```
Phase 1: Quick ID scan (IDs 1-10)
Phase 2: Baud rate detection
Phase 3: Serial configuration detection
Phase 4: Device information gathering
```

### **Handmatige Configuratie**
```
Menu Option 8: Change Settings
→ Baud Rate: 9600, 19200, 38400...
→ Slave ID: 1-247
→ Runtime configuratie (niet permanent)
```

## 🔍 Troubleshooting

### **Veel Voorkomende Problemen**

| **Probleem** | **Mogelijke Oorzaken** | **Oplossing** |
|--------------|----------------------|---------------|
| **Geen apparaten gevonden** | Verkeerde bedrading | Check RX↔TX, TX↔RX verbindingen |
| **Timeout errors** | Verkeerde baud rate | Gebruik auto-detectie of probeer andere baud rates |
| **CRC errors** | Slechte kabelkwaliteit | Gebruik twisted pair kabels, check lengte |
| **LED knippert rood** | Communicatie fout | Check voeding, GND verbinding |

### **Bedrading Checklist**
- ✅ **RX/TX**: Cross-connected (RX→TX, TX→RX)
- ✅ **GND**: Common ground verbinding
- ✅ **Power**: Correcte spanning (3.3V/5V)
- ✅ **DE/RE**: Voor RS485 transceivers
- ✅ **Terminatie**: 120Ω resistors op bus uiteinden

### **LED Status Diagnostics**
- 🔵 **Blauw constant**: Systeem operationeel
- 🟣 **Paars pulsend**: Bezig met scannen
- 🔴 **Rood knipperend**: Communicatie probleem
- 🟠 **Oranje**: Timeout/waarschuwing
- ✅ **Groen flash**: Succesvolle communicatie

## 🛠️ Advanced Features

### **Custom Register Maps**
```cpp
// Voorbeeld: Lees specifieke registers
readHoldingRegisters(slaveId, 100, 10);    // Registers 100-109
readInputRegisters(slaveId, 0, 5);         // Input registers 0-4
readCoils(slaveId, 0, 16);                 // Coils 0-15
```

### **Batch Operations**
```cpp
// Meerdere registers schrijven
writeSingleRegister(slaveId, 100, 12345);
writeSingleRegister(slaveId, 101, 67890);
```

### **Error Handling**
Het systeem biedt gedetailleerde error codes:
- `0x01` - Illegal Function
- `0x02` - Illegal Data Address  
- `0x03` - Illegal Data Value
- `0x04` - Slave Device Failure
- `Timeout` - No response

## 📊 Performance Specificaties

| **Metric** | **Waarde** |
|------------|------------|
| **Scan Snelheid** | ~50 IDs per seconde |
| **Response Time** | <100ms typisch |
| **Max Bus Length** | 1200m (RS485) |
| **Max Devices** | 247 slaves |
| **LED Refresh Rate** | 60Hz smooth animations |
| **Memory Usage** | <50KB RAM |

## 🔄 Firmware Updates

### **Version History**
- **v1.0.0**: Initial release met basic Modbus support
- **v1.1.0**: WS2812 LED status indicators toegevoegd
- **v1.2.0**: Auto-detectie algoritmes geïmplementeerd
- **v1.3.0**: Uitgebreide error handling en diagnostics

### **Updating**
```bash
# Fetch latest code
git pull origin main

# Clean build
platformio run --target clean
platformio run

# Upload updated firmware
platformio run --target upload
```

## 🤝 Contributing

### **Development Setup**
1. Fork het repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

### **Bug Reports**
Gebruik GitHub Issues met:
- ESP32 board info
- PlatformIO versie
- Detailed error description
- Serial monitor output

## 📄 License

Dit project is gelicenseerd onder de MIT License - zie het [LICENSE](LICENSE) bestand voor details.

## 🙏 Credits

- **ModbusMaster Library**: [4-20ma/ModbusMaster](https://github.com/4-20ma/ModbusMaster)
- **FastLED Library**: [FastLED/FastLED](https://github.com/FastLED/FastLED)
- **ESP32 Arduino Core**: [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)

---

**Made with ❤️ for the Industrial IoT Community**

> *"Modbus communication made simple and reliable"*