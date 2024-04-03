# Fingerprint-based-Attendance
A cloud fingerprint-based attendance system where detected users can time-in or time-out and the data will be sent to Google sheets.

The system runs using:
-Two respective buttons for the time-in/time-out. 
-A 16x2 IC2 LCD is used to display current date and time with accuracy with the help of; 
-DS3231 RTC module. 
-Arduino Nano and ESP32-CAM are used for the overall function where:
-Nano is used for the peripherals (Fingerprint sensor, LCD, and RTC module). The communication from the fingerprint sensor to the arduino is utilized here and the data will be read by the ESP32 CAM.
-ESP32-CAM is used for the connection to the internet and receives the detected data along with the time and date from the Arduino Nano. The data will then be transmitted to the google sheets to be displayed.
-Google Sheets API for ESP was used for the programming to the cloud.
