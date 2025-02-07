    • ESP32-S3 Project:- Firmware development for audio device  
    • When the device is turned on - it connected to cloud endpoint and fetched the latest config which has the mappings for RFID tag -> content  
    • If the content (mp3) was available in the SD card, content played.  
    • When the toy was placed on the device i.e. an RFID tag was read  
    • If content was not available locally on SD card it started downloading the content (server url where the content was stored was in the config mapping) and parallelly started streaming the content, from the cloud  
    • Sensors/ parts interfaced: INMP 441 microphone, SD Card, RC 522 RFID card reader, MAX 98357A I2S amplifier for speaker.
