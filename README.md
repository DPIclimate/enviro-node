<h2 align="center">
    Enviro-Node
</h2>
<p align="center">
  <a href="#about">About</a> •
  <a href="#">Docs</a> •
  <a href="#Firmware">Firmware</a> •
  <a href="#license">License</a>
</p>



## About

TODO

## Firmware

This project uses PlatformIO as a build system. Instructions for building the project are below.

### Installation

Clone this project in your desired directory.

```bash
git clone https://github.com/DPIclimate/enviro-node
```

### Board definitions

This project uses a board definition as defined by Sparkfun Electronics via their MicroMod ecosystem. Easiest way to get this sorted is to use the PlatformIO CLI to download the `framework-arduino-mbed`: 

```bash
pio pkg install -g --tool "platformio/framework-arduino-mbed@^3.1.1"
```

This will create a folder in:

```bash
cd ~/.platformio/packages/framework-arduino-mbed@3.1.1/variants
# or when on windows
cd ~\.platformio\packages\framework-arduino-mbed@3.1.1\variants
```

This in this folder we want to add a custom board definition which can be downloaded here: 











### Install dependencies

#### cJSON

A JSON parsing and building library. See: [cJSON](https://github.com/DaveGamble/cJSON)

```bash
# In a directory of your choosing
git clone https://github.com/DaveGamble/cJSON
cd cJSON
mkdir build
cd build
cmake ..
make
```

### ENV Variables

This project uses several environmental variables. 
You need to provide these or the program will not run.

```bash
export UBI_TOKEN="<your_ubidots_token>"
export WW_TOKEN="<your_willy_weather_token>"
export IBM_TOKEN="<your_ibm_token>"
```

### Build & Run

Clone this repository from your command line:

```bash
git clone https://github.com/DPIclimate/ha-closure-analysis
cd ha-closure-analysis
mkdir build
cd build
cmake ..
make
./bin/program # Run
```

## License

This project is MIT licensed, as found in the LICENCE file.

> Contact [Harvey Bates](mailto:harvey.bates@dpi.nsw.gov.au)
