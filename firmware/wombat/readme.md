# Wombat Firmware

## Overview

The Wombat is a simple data logger whose basic runtime sequence is:

1. Connect to the internet and use NTP to set the time.
2. Read the solar and battery bus voltages.
3. Read all attached SDI-12 sensors with addresses in the range of 0 - 9.
4. Read and reset the pulse count from the `Digital` input.
5. Create a JSON message with these values, plus miscellaneous node information such as the SIM card CCID, basic mobile signal strength, serial number, firmware version etc.
6. Append the message to the `data.json` file on the SD card if the card is present.
7. Write the message to a file on the ESP-32 SPIFFS filesystem.
8. If this is an uplink cycle, send all messages found on the SPIFFS filesystem to an MQTT broker.
9. If a configuration script was received from the MQTT broker, run it.
10. Enter deep sleep until the next measurement cycle.

The firmware is written using the [ESP-32 Arduino platform](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html)
because Arduino libraries are used. [PlatformIO](https://platformio.org/) is used for building.

The Wombat currently has a 1024 byte limit on the length of the telemetry message. This is sufficient for current
deployments.

## Command Line Interface

Wombats are configured using a command line interface (CLI). This is available via a UART or by publishing a script to
an MQTT topic.

After the output from most commands, the Wombat will print `OK` on success or `ERROR: [optional message]` if a command
fails. The OK/ERROR string is printed on a new line delimited by CRLF, for example: `\r\nOK\r\n`.

## Access the CLI via the USB-C port

To use the command line via the UART header or USB-C port on the Wombat, short the `External Button` header while
rebooting.

The USB-C port does not have both sets of data pins connected, so try the USB-C cable in both orientations.

## Access the CLI via the UART header

The UART header (located just under the ESP32 module) can be used to print console messages from the Wombat. Log
messages are transmitted via the TX.

To enable the RX pin so commands can be entered, break the solder jumper `JP1` on the back of the PCB.

> WARNING: While this jumper is broken the USB-C port will not accept commands, but it will still emit log messages.

## Send CLI commands via MQTT

A Wombat checks for a configuration scrip every time it connects to the MQTT server, using its serial number as the
in the topic name. For example, if the boot messages show:

```text
[    39][I][main.cpp:135] timeout_task(): [wombat] Starting
[  1307][I][DeviceConfig.cpp:66] reset(): [config] Resetting values to defaults
[  1308][I][DeviceConfig.cpp:72] reset(): [config] Node Id: B8D61A017074
[  1310][I][main.cpp:194] setup(): [wombat] Wake up time: 1970-01-01T00:00:18Z
```

then the `Node Id` is `B8D61A017074` and the topic the Wombat will read commands from is `wombat/B8D61A017074`.

If a configuration script is found, the Wombat downloads it and publishes an empty message to the topic to clear the
script.

Configuration scripts must the published as a retained message, meaning the message will persist in the topic until a
new retained message is written to the topic. To clear the configuration script from the topic, an empty message is
published by the Wombat after receiving the configuration script.

> Some commands should not be used in configuration scripts. These commands are marked with `[CLI only]` in the command
> reference below.

Below is an excerpt of the logging messages showing the downloading and clearing of a configuration script from the
MQTT server.

```text
AT+UMQTTC=1

OK
[1884670][I][mqtt_stack.cpp:20] mqttCmdCallback(): [mqtt_stack] cmd: 1, result: 1
[1884697][I][Utils.h:73] hasURC(): [CommandURCVector] valid = 1, command = 1, result = 1, err1 = 0, err2 = 0
[1884715][I][Utils.h:101] waitForURC(): [CommandURCVector] command = 1, result = 1, found_urc = 1, retries = 59
[1884751][I][mqtt_stack.cpp:86] mqtt_login(): [mqtt_stack] Connected to MQTT
AT+UMQTTC=4,1,"wombat/B8D61A017074"

OK
[1884759][I][mqtt_stack.cpp:105] mqtt_login(): [mqtt_stack] Checking for a config script, waiting for subscribe URC
[1885466][I][mqtt_stack.cpp:20] mqttCmdCallback(): [mqtt_stack] cmd: 4, result: 1
[1885485][I][mqtt_stack.cpp:20] mqttCmdCallback(): [mqtt_stack] cmd: 6, result: 1
[1885503][I][Utils.h:73] hasURC(): [CommandURCVector] valid = 1, command = 4, result = 1, err1 = 0, err2 = 0
[1885521][I][Utils.h:101] waitForURC(): [CommandURCVector] command = 4, result = 1, found_urc = 1, retries = 23
[1885539][I][mqtt_stack.cpp:109] mqtt_login(): [mqtt_stack] out of sub urc loop, checking values: result = 1
[1885561][I][Utils.h:73] hasURC(): [CommandURCVector] valid = 1, command = 6, result = 1, err1 = 0, err2 = 0
[1885580][I][Utils.h:101] waitForURC(): [CommandURCVector] command = 6, result = 1, found_urc = 1, retries = 20
[1885598][I][mqtt_stack.cpp:118] mqtt_login(): [mqtt_stack] out of read urc loop, result = 1
[1885616][I][mqtt_stack.cpp:122] mqtt_login(): [mqtt_stack] Config download waiting.
AT+UMQTTC=6,1

+UMQTTC: 6,1,59,19,"wombat/B8D61A017074",40,"ftp login
ftp upload log.txt
ftp logout
"

OK
AT+UMQTTC=2,0,1,0,"wombat/B8D61A017074",""
15:23:26.063 >
OK
```

### Example scripts

A script to upload the data and log files, and then clear the log file:

```
ftp login
ftp upload data.json
ftp upload log.txt
ftp logout
sd rm log.txt
```

A script unconditionally update the firmware and reboot to quickly get a message with the new firmware version
identifier:

```text
config ota 1
config reboot
```

## Command Reference

### config

#### config list

The `config list` command lists various configuration settings as a set of configuration commands that can be pasted
into another Wombats CLI to copy the configuration.

```text
$ config list
interval measure 900
interval uplink 900
interval clockmult 1.0
mqtt host mqtt_server.example.com
mqtt port 1883
mqtt user mqtt_username
mqtt password mqtt_password_in_cleartext
mqtt topic wombat
ftp host ftp_server.example.com
ftp user ftp
ftp password ftp_password_in_cleartext

OK
$
```

#### config save

Saves the configuration to non-volatile storage.

#### config load

Loads the Wombat configuration from non-volatile storage. The configuration must have been saved previously.

#### config dto `[CLI only]`

Disables the 60-minute timeout that will reboot the Wombat if it is still running. This timeout is enabled by default.

#### config eto `[CLI only]`

Enables the 60-minute timeout that will reboot the Wombat if it is still running. This timeout is enabled by default.

#### config ota

Triggers an over-the-air update. By default, this command checks the version on FTP the server and only downloads it if
it is newer than what is running on the Wombat. This can be over-ridden by supplying an argument of '1'.

Example: `config ota 1` force an update of the firmware.

#### config sdi12defn

Downloads the [sdi12defn.json](data/sdi12defn.json) file from the FTP server. No version checking is done, the file is always downloaded and
replaces what is currently on the Wombat.

> See [SDI-12 Sensor Definitions](#sdi-12-sensor-definitions)

#### config reboot

Performs a clean shutdown and reboots the Wombat.

### interval

#### interval list

Lists the interval settings as a set of configuration commands that can be pasted
into another Wombats CLI to copy the configuration.

```text
interval measure 900
interval uplink 900
interval clockmult 1.0
```

#### interval measure

Sets the interval, in seconds, that the Wombat will wake up and record data from the sensors.

Example: `interval measure 900` takes measurements every 15 minutes.

#### interval uplink

Sets the interval, in seconds, that the Wombat will upload data to the MQTT broker. This must be a multiple of
the measurement interval.

Example: `interval uplink 3600` uploads data every hour.

#### interval clockmult

Sets the coefficient used to adjust the sleep time if clock drift is a problem. The time to sleep is calculated in
milliseconds, converted to micro-seconds, and this coefficient is applied to the microsecond value.

Example: `interval clockmult 1.006`

### power

#### power show

Prints the solar and battery voltages.

### sd - work with the SD card

#### sd rm

Deletes a file from the SD card. All files are in the root directory and need no prefix on the name. Only two files
exist on the SD card, `data.json` and `log.txt`. No quotes are required around the filename. Only a single filename may
be supplied.

Example: `sd rm log.txt` will delete the log file.

#### sd data `[CLI only]`

Prints the `data.json` file to the console. The contents are surrounded by `[` and `]` to make it a JSON array.

#### sd log `[CLI only]`

Prints the `log.txt` file to the console.

### sdi12 - work with SDI-12 sensors

#### sdi12 scan

Scans the SDI-12 bus, prints the sensors found and populates the SDI-12 sensor data structure.

#### sdi12 m `[CLI only]`

Performs a measurement on an SDI-12 sensor with the supplied address. If the type of the sensor is in the
`sdi12defns.json` file then the commands and field mask in that file are used, otherwise a measure command is used and
all values are printed.

Example: `sdi12 m 1` reads SDI-12 sensor with address 1 and prints the values.

#### sdi12 >>

Sends a command to the SDI-12 bus and prints the response. The whole argument to this command is sent as the SDI-12
command.

Example: `sdi12 >> 0A8!` changes the address of sensor 0 to 8.

#### sdi12 pt `[CLI only]`

Enters SDI-12 passthrough mode. In this mode all characters typed are sent directly to the SDI-12 bus and all
characters received from the bus are printed to the console.

Exit passthrough mode with ctrl-D.

### c1 - work with the Cat-M1 modem

#### c1 pwr `[CLI only]`

Switches power to the modem on or off. The modem is switched off by default and must be powered before it can be used.
When testing the Wombat from the command line and wanting to use the modem, issue `c1 pwr 1` to supply power to the
modem. `c1 pwr 0` will remove power from the modem, and will issue a power off AT command if a previous `c1 cti`
command was successful.

Example: `c1 pwr 1` switches on the modem power bus.

#### c1 cti `[CLI only]`

Attempts to connect the Wombat to the internet. Will also get the time via NTP. This command will enable power to the
modem the first time it is run.

#### c1 ls

Lists files on the modem filesystem.

#### c1 rm

Deletes a file from the modem filesystem. No quotes are required around the filename. Only a single filename may be
supplied.

Example: `c1 rm wombat.sha1` removes the `wombat.sha1` file from the modem filesystem.

#### c1 factory `[CLI only]`

Attempts to reset the modem to factory defaults using the `AT+UFACTORY=2,2` command.

#### c1 ntp

Issues an NTP request to set the RTC of the Wombat.

#### c1 ok

Issues an `ATI` command and prints the response as a basic modem UART test.

#### c1 pt `[CLI only]`

Enters passthrough mode on the modem UART. In this mode all characters typed are sent directly to the modem and all
characters received from the modem are printed to the console.

Exit passthrough mode with ctrl-D.

### mqtt

#### mqtt list

Lists the MQTT settings as a set of configuration commands that can be pasted
into another Wombats CLI to copy the configuration.

#### mqtt host

Sets the MQTT hostname. A hostname or IPv4 address can be used.

Example: `mqtt hostname mqtt.example.com`

#### mqtt port

Sets the MQTT port number. Must be an integer.

Example: `mqtt port 1883`

#### mqtt user

Sets the MQTT username.

Example: `mqtt user example_user`

#### mqtt password

Sets the MQTT password.

Example: `mqtt password an_example_password`

#### mqtt topic

Sets the name of the topic that test messages sent via `mqtt publish` are published to. Note this setting has no
impact on messages sent from the usual running of the Wombat.

Example: `mqtt topic test_topic`

#### mqtt login

Attempt to log in to a MQTT server using the current configuration settings.

#### mqtt logout

Log out of the MQTT server.

#### mqtt publish

Publish a test message to the MQTT server. The message content is 'ABCDEF' and is published to the topic set with
`mqtt topic`.

### ftp

#### ftp list

Lists the FTP settings as a set of configuration commands that can be pasted
into another Wombats CLI to copy the configuration.

#### ftp host

Sets the FTP hostname. A hostname or IPv4 address can be used.

Example: `ftp hostname ftp.example.com`

#### ftp user

Sets the FTP username.

Example: `ftp user example_user`

#### ftp password

Sets the FTP password.

Example: `ftp password an_example_password`

#### ftp login

Attempt to log in to a FTP server using the current configuration settings.

#### ftp logout

Log out of the FTP server.

#### ftp get

Get a file from the FTP server. The file must be in the root directory of the FTP server.

No quotes are necessary around the filename, and only a single filename is accepted. No whitespace is allowed in the
filename.

The file is written to the modem filesystem.

Example: `ftp get wombat.sha1`

#### ftp upload

Uploads a file from the SD card to the FTP server. The file is written to a directory named `uploads/node_SERIALNO`
where SERIALNO is the Wombat serial number.

No quotes are necessary around the filename, and only a single filename is accepted. No whitespace is allowed in the
filename.

Example: `ftp upload log.txt`


## SDI-12 sensors

### The SDI-12 Bus

Up to 10 SDI-12 sensors may be connected to the Wombat using addresses 0 - 9.

When the Wombat boots it scans the SDI-12 bus for sensors on these addresses and builds a list of the sensors found.
Every sensor on the list is read during the measurement cycle.

SDI-12 Sensors are connected to the `OUTPUT` header and may be powered via the `3V3` line or the `12 V` line. The `3V3`
is powered as long as the batteries are connected via the `Power Button` header, including when the Wombat is asleep.
All SDI-12 sensor data wires must be connected to the `SDI-12` line.

The always-on `3V3` line is useful for sensors such as automatic weather stations which must be always on for recording
rain and wind events.

The `12 V` line is only powered while the Wombat is awake.


### SDI-12 Sensor Definitions

If a file named [`sdi12defns.json`](data/sdi12defn.json) exists on the ESP-32 SPIFFS filesystem, it will be used to
determine which commands to use when reading data from SDI-12 sensors, what labels to give the values, and to
filter unwanted values.

The general layout of the file is:

```json
{
  "vendor": {
    "model": {
      "read_cmds": [ cmd1, ... ],
      "value_mask": "10110",
      "labels": [ "Field_1", ... ],
      "ignore_sr": true
    }
  }
}
```
where:

| Field      | Description                                                                                                                                                                                                                    |
|------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| vendor     | The vendor identifier returned by the sensor in the response to an identify command.                                                                                                                                           |
| model      | The sensor model identifier returned by the sensor in the response to an identify command.                                                                                                                                     |
| read_cmds  | A list of commands to send to read values. The command must start with `a`, a placeholder for the sensor address,  and end with `!`. For example: `"aC!"` or `"aC2!"`.                                                         |
| value_mask | Optional - a string of `0` and `1` which can be used to mask out unwanted values. Use `0` to drop a value. For example: `"110"` drops the 3rd value read from the sensor.                                                      |
| labels     | A list of names to give the values in the telemetry message. The names are prefixed with the sensor address, so a name of `"Temperature"` will appear in the message as `"1_Temperature"` if it is read from sensor address 1. |
| ignore_sr | Optional - a boolean that can be used to tell the Wombat to always wait for the full length of time the sensor claimed a measurement will take. The Wombat will read and ignore the service request from the sensor.           |


> If a sensor definition cannot be found for a sensor, the Wombat issues a basic measure command `aM!`, and labels the
> values `"a_Vn"` where `a` is the sensor address and `n` is the number of the value, from 0 to *n-1* where *n* is the
> number of values read back from the sensor. For example, a temperature and humidity sensor at address 5 would have
> values labelled `5_V0` and `5_V1`.

A new version of the `sdi12defns.json` file can be downloaded from the root directory of the FTP server using the
`config sdi12dfns` command.


## Firmware Updates

The firmware can be updated over-the-air via the `config ota` command.

A binary image named `wombat.bin` and an information file named `wombat.sha1` must must be present in the root
directory of the FTP server.

The information file contains a single line of text with the following fields:

```text
version length firmware-image-sha1-sig git-commit-id
```

for example:

```text
1.0.7 546064 4efb177a5d132ed22e9f045f03e61b9a1d85cacd 7b5b8c08f67312f4c587a83316fbbdc911a5df6d
```

This file is generated by the [split_firmware.sh](split_firmware.sh) script, which is run by [scp_firmware.sh](scp_firmware.sh).

### Uploading Firmware to the FTP Server

The [scp_firmware.sh](scp_firmware.sh) script is used to generate the information file and upload both it and the
firmware image to an FTP server. Note the script uses SSH to copy the files, not FTP.

Two environment variables are used by the script:

| Environment variable | Description                                                                                                                                                                                 |
|----------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| SSH_HOST             | This variable is expected to contain an SSH host identifier from the ~/.ssh/config file. It may work to set it to something like `-i ~/.ssh/my_key user@host` but this has not been tested. |
| DEST_DIR             | The directory on the host to copy the files to. This must equate to the FTP server root directory, for example `/srv/ftp`.                                                                    |

> The script must be run from the `src/firmware/wombat` directory.

### Requesting a Firmware Update

Issuing a `config ota` command will cause the Wombat to log in to the FTP server, download the `wombat.sha1` file and
use this to check if the software on the server is newer than what is running on the Wombat. If so, the Wombat
downloads and installs the new firmware image and switches over on the next boot.

To force a firmware update issue the `config ota 1` command. This is useful during development when the version number
of the firmware is not changing, or perhaps to downgrade the firmware.
