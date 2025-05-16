import os
import base64, json, hmac, hashlib
from flask import Flask, Response
# ToDo add support for loop() to detect live system users && services

key = bytes([
    0x1f, 0x1d, 0x9d, 0x2b, 0x8d, 0xc2, 0x19, 0x6d,
    0x87, 0xa7, 0x58, 0x1b, 0xf9, 0xad, 0x3c, 0x6d,
    0x4b, 0x8f, 0x1f, 0xdd, 0x68, 0xe1, 0x0f, 0x70,
    0x84, 0x3d, 0xaf, 0xd7, 0x0c, 0xf9, 0xa9, 0x2d
])

app = Flask(__name__)

@app.route("/status")
def status():
    data = {
        "sys": 1,
        "ssh": 1,
        "user": 0,
        "tor": 1
    }
    plaintext = json.dumps(data).encode()
    h = hmac.new(key, plaintext, hashlib.sha256).digest()
    message = plaintext + h
    b64 = base64.b64encode(message).decode()
    return Response(b64, mimetype="text/plain")

if __name__ == "__main__":
    cert_file = 'cert.pem'
    key_file = 'key.pem'

    if not os.path.isfile(cert_file) or not os.path.isfile(key_file):
        raise FileNotFoundError("SSL certificate or key file not found.")

    # Start the Flask app with SSL (HTTPS)
    app.run(host="0.0.0.0", port=443, ssl_context=(cert_file, key_file))
