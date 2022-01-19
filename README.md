# irSerialSenderESP
IR remote control record/sender for ESP8266 (nodeMCU)

This program can record and replay 38 kHz IR remote control signal using normal IR led and IR photodiode through serial port or Wifi UDP socket.

Command 'r' will switch device to recording mode and when the IR pulse train is received the IR pulse recording string [ON_Time,OFF_Time,ON_Time,OFF_Time...,x] will be send back via Serial terminal or Wifi UDP socket, copy and save it for each remote button.

Send these string back via Serial terminal or Wifi TCP socket to send an IR remote command.

A accompanying Android Remote Application and construction plan will follow shortly.

Based on code from : http://www.ladyada.net/learn/sensors/ir.html (public domain)

Note: Recent update has change the protocol from UDP to TCP due to frequence packet loss mostly occurred during record operation. Client software need to be update. Messaging sequence remain the same.
