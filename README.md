🛰 Secure Status Monitor (Flask + ESP8266 + OLED)

This project creates a secure status-monitoring system between a Flask server and an ESP8266 WROOM microcontroller with an OLED display. It uses HTTPS, Base64 encoding, and HMAC-SHA256 authentication to ensure integrity and authenticity of transmitted data.
📦 Project Structure

    Flask Server: Hosts an HTTPS endpoint /status that returns a signed, Base64-encoded JSON payload.

    ESP8266 Client: Connects securely to the server, verifies the HMAC signature, parses the JSON, and displays system status on an OLED.

🔐 Security Design

    Shared Secret Key: A 32-byte key hardcoded on both ends.

    HMAC-SHA256: Ensures that messages are authentic and untampered.

    Base64 Encoding: Encodes the binary message + HMAC for safe HTTP transmission.

    HTTPS: Secures communication over the network using SSL.

⚙ Flask Server Setup
🔧 Requirements

    Python 3

    Flask

📁 File Structure

server/
├── server.py       # Flask server code
├── cert.pem        # SSL certificate
└── key.pem         # SSL private key

🚀 Running the Server

    Generate a self-signed certificate (or use a real one):

openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out cert.pem

Start the server:

    python3 server.py

    The server will be available at https://<your-ip>:443/status.

🧪 Sample Output

Returns a Base64-encoded payload of:

{
  "sys": 1,
  "ssh": 1,
  "user": 0,
  "tor": 1
}

With HMAC-SHA256 signature appended and Base64-encoded.
📡 ESP8266 Client Setup
🛠 Hardware Required

    ESP8266 (e.g., NodeMCU or WROOM-02)

    OLED Display (128x64, I2C)

    USB cable for flashing

🔧 Libraries Needed

Install the following Arduino libraries:

    Adafruit_SSD1306

    Adafruit_GFX

    ArduinoJson

    Base64

    SHA256 (from CryptoLib or similar)

🧰 Configuration

In the ESP8266 sketch:

    Update your WiFi credentials:

const char* ssid = "YourSSID";
const char* password = "YourPassword";

Set the server IP:

    const char* host = "192.168.x.x";  // IP of Flask server

💡 OLED Wiring
OLED Pin	ESP8266 Pin
VCC	3.3V
GND	GND
SDA	D6 (GPIO14)
SCL	D5 (GPIO12)

You may modify OLED_SDA and OLED_SCL if using different pins.
🚀 Upload and Run

    Connect your ESP8266 via USB.

    Select the correct board (e.g., NodeMCU 1.0) and port.

    Upload the sketch.

    Monitor output via Serial Monitor (115200 baud).

🔍 How It Works

    Flask Server:

        Creates a JSON payload with system status.

        Signs it using HMAC-SHA256 with a shared secret key.

        Appends the HMAC to the message and encodes the whole thing in Base64.

    ESP8266:

        Connects to the server over HTTPS.

        Extracts and decodes the Base64 payload.

        Verifies the HMAC.

        Parses the JSON and displays the system status on the OLED.

🧪 Troubleshooting

    ❌ HMAC invalid: Shared key mismatch or tampering.

    ❌ Connection failed: Check SSL certs and IP.

    ❌ OLED init failed: Check wiring and I2C address (default is 0x3C).

    ❌ JSON error: Possible corrupt payload or deserialization issue.

🛡 Security Notes

    client.setInsecure() disables SSL cert verification on the ESP8266. For production, consider fingerprint verification or using a trusted cert.

    Keep the shared key secret. Never expose it in public repos.

    The HMAC ensures authenticity, but not confidentiality—anyone can read the message, but only a holder of the key can forge it.

📃 License

MIT License. Use freely, credit appreciated.
✍ Author

Made with ❤️ by aMiscreant