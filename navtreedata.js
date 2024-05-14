/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Wombat Environmental Node", "index.html", [
    [ "Wombat Firmware", "md_firmware_wombat_readme.html", [
      [ "About", "index.html#autotoc_md0", null ],
      [ "Hardware", "index.html#autotoc_md1", [
        [ "Microcontroller", "index.html#autotoc_md2", null ],
        [ "Debug and USB Interfaces", "index.html#autotoc_md3", null ],
        [ "Power Distribution", "index.html#autotoc_md4", [
          [ "Battery", "index.html#autotoc_md5", null ],
          [ "Solar", "index.html#autotoc_md6", null ],
          [ "3V3 Interface", "index.html#autotoc_md7", null ],
          [ "12V Interface for SDI-12 Devices", "index.html#autotoc_md8", null ]
        ] ],
        [ "Peripherals", "index.html#autotoc_md9", [
          [ "IO Expander", "index.html#autotoc_md10", null ],
          [ "Piezo Buzzer", "index.html#autotoc_md11", null ],
          [ "Programmable Button", "index.html#autotoc_md12", null ],
          [ "SD-Card", "index.html#autotoc_md13", null ],
          [ "SDI-12", "index.html#autotoc_md14", null ],
          [ "Digital", "index.html#autotoc_md15", null ]
        ] ],
        [ "CAT-M1", "index.html#autotoc_md16", null ]
      ] ],
      [ "Firmware", "index.html#autotoc_md17", [
        [ "Installing platformio and the Wombat board files", "index.html#autotoc_md18", null ]
      ] ],
      [ "License", "index.html#autotoc_md19", null ],
      [ "Overview", "md_firmware_wombat_readme.html#autotoc_md21", null ],
      [ "Command Line Interface", "md_firmware_wombat_readme.html#autotoc_md22", null ],
      [ "Access the CLI via the USB-C port", "md_firmware_wombat_readme.html#autotoc_md23", null ],
      [ "Access the CLI via the UART header", "md_firmware_wombat_readme.html#autotoc_md24", null ],
      [ "Send CLI commands via MQTT", "md_firmware_wombat_readme.html#autotoc_md25", [
        [ "Example scripts", "md_firmware_wombat_readme.html#autotoc_md26", null ]
      ] ],
      [ "Command Reference", "md_firmware_wombat_readme.html#autotoc_md27", [
        [ "config", "md_firmware_wombat_readme.html#autotoc_md28", [
          [ "config list", "md_firmware_wombat_readme.html#autotoc_md29", null ],
          [ "config save", "md_firmware_wombat_readme.html#autotoc_md30", null ],
          [ "config load", "md_firmware_wombat_readme.html#autotoc_md31", null ],
          [ "config dto <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md32", null ],
          [ "config eto <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md33", null ],
          [ "config ota", "md_firmware_wombat_readme.html#autotoc_md34", null ],
          [ "config sdi12defn", "md_firmware_wombat_readme.html#autotoc_md35", null ],
          [ "config reboot", "md_firmware_wombat_readme.html#autotoc_md36", null ]
        ] ],
        [ "interval", "md_firmware_wombat_readme.html#autotoc_md37", [
          [ "interval list", "md_firmware_wombat_readme.html#autotoc_md38", null ],
          [ "interval measure", "md_firmware_wombat_readme.html#autotoc_md39", null ],
          [ "interval uplink", "md_firmware_wombat_readme.html#autotoc_md40", null ],
          [ "interval clockmult", "md_firmware_wombat_readme.html#autotoc_md41", null ]
        ] ],
        [ "power", "md_firmware_wombat_readme.html#autotoc_md42", [
          [ "power show", "md_firmware_wombat_readme.html#autotoc_md43", null ]
        ] ],
        [ "sd - work with the SD card", "md_firmware_wombat_readme.html#autotoc_md44", [
          [ "sd rm", "md_firmware_wombat_readme.html#autotoc_md45", null ],
          [ "sd data <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md46", null ],
          [ "sd log <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md47", null ]
        ] ],
        [ "sdi12 - work with SDI-12 sensors", "md_firmware_wombat_readme.html#autotoc_md48", [
          [ "sdi12 scan", "md_firmware_wombat_readme.html#autotoc_md49", null ],
          [ "sdi12 m <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md50", null ],
          [ "sdi12 >>", "md_firmware_wombat_readme.html#autotoc_md51", null ],
          [ "sdi12 pt <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md52", null ]
        ] ],
        [ "c1 - work with the Cat-M1 modem", "md_firmware_wombat_readme.html#autotoc_md53", [
          [ "c1 pwr <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md54", null ],
          [ "c1 cti <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md55", null ],
          [ "c1 ls", "md_firmware_wombat_readme.html#autotoc_md56", null ],
          [ "c1 rm", "md_firmware_wombat_readme.html#autotoc_md57", null ],
          [ "c1 factory <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md58", null ],
          [ "c1 ntp", "md_firmware_wombat_readme.html#autotoc_md59", null ],
          [ "c1 ok", "md_firmware_wombat_readme.html#autotoc_md60", null ],
          [ "c1 pt <tt>[CLI only]</tt>", "md_firmware_wombat_readme.html#autotoc_md61", null ]
        ] ],
        [ "mqtt", "md_firmware_wombat_readme.html#autotoc_md62", [
          [ "mqtt list", "md_firmware_wombat_readme.html#autotoc_md63", null ],
          [ "mqtt host", "md_firmware_wombat_readme.html#autotoc_md64", null ],
          [ "mqtt port", "md_firmware_wombat_readme.html#autotoc_md65", null ],
          [ "mqtt user", "md_firmware_wombat_readme.html#autotoc_md66", null ],
          [ "mqtt password", "md_firmware_wombat_readme.html#autotoc_md67", null ],
          [ "mqtt topic", "md_firmware_wombat_readme.html#autotoc_md68", null ],
          [ "mqtt login", "md_firmware_wombat_readme.html#autotoc_md69", null ],
          [ "mqtt logout", "md_firmware_wombat_readme.html#autotoc_md70", null ],
          [ "mqtt publish", "md_firmware_wombat_readme.html#autotoc_md71", null ]
        ] ],
        [ "ftp", "md_firmware_wombat_readme.html#autotoc_md72", [
          [ "ftp list", "md_firmware_wombat_readme.html#autotoc_md73", null ],
          [ "ftp host", "md_firmware_wombat_readme.html#autotoc_md74", null ],
          [ "ftp user", "md_firmware_wombat_readme.html#autotoc_md75", null ],
          [ "ftp password", "md_firmware_wombat_readme.html#autotoc_md76", null ],
          [ "ftp login", "md_firmware_wombat_readme.html#autotoc_md77", null ],
          [ "ftp logout", "md_firmware_wombat_readme.html#autotoc_md78", null ],
          [ "ftp get", "md_firmware_wombat_readme.html#autotoc_md79", null ],
          [ "ftp upload", "md_firmware_wombat_readme.html#autotoc_md80", null ]
        ] ]
      ] ],
      [ "SDI-12 sensors", "md_firmware_wombat_readme.html#autotoc_md81", [
        [ "The SDI-12 Bus", "md_firmware_wombat_readme.html#autotoc_md82", null ],
        [ "SDI-12 Sensor Definitions", "md_firmware_wombat_readme.html#autotoc_md83", null ]
      ] ],
      [ "Firmware Updates", "md_firmware_wombat_readme.html#autotoc_md84", [
        [ "Uploading Firmware to the FTP Server", "md_firmware_wombat_readme.html#autotoc_md85", null ],
        [ "Requesting a Firmware Update", "md_firmware_wombat_readme.html#autotoc_md86", null ]
      ] ]
    ] ],
    [ "Todo List", "todo.html", null ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_c_a_t___m1_8cpp.html",
"class_t_c_a9534.html#a090994f5dc606d3676d2004c0b8be270",
"md_firmware_wombat_readme.html#autotoc_md52"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';