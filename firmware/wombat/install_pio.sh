cd $(dirname $0)
# Install platformio core.
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py

# This needs to be in a login script too.
PATH="~/.platformio/penv/bin:$PATH"

# This command will fail due to the lack of a dpiclimate_esp_32_wombat.json file. Copying the file before running
# the command does not work - the directory must get cleared or something.
pio pkg install -p platformio/espressif32

# Now copy the missing file
cp pio/platforms/espressif32/boards/dpiclimate_esp32_wombat.json ~/.platformio/platforms/espressif32/boards

# Run the command again and it will complete.
pio pkg install -p platformio/espressif32

# Copy the variant pins file over to complete the ESP32 Wombat installation.
mkdir -p ~/.platformio/packages/framework-arduinoespressif32/variants/dpiclimate_esp32_wombat
cp pio/packages/framework-arduinoespressif32/variants/dpiclimate_esp32_wombat/pins_arduino.h ~/.platformio/packages/framework-arduinoespressif32/variants/dpiclimate_esp32_wombat

# Now pio run should work.
