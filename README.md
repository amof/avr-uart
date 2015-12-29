avr-uart
===================

### 1. Description

This library is based on the one made by Andy Gock ([here](https://github.com/andygock/avr-uart)).
The compatibility has been reduced with AtMega1284P and Atmega88PA-PU families.

### 2. Hardware supported

This library has been tested with :

 - Atmega1284P @1MHz -> 9600 bps
 - Atmega1284P @8MHz -> 9600 bps, 57600 bps
 - Atmega88PA-PU @8MHz -> 9600 bps, 57600 bps

### 3. Memory used

**Reference** : Atmega1284P blank solution 
Program Memory Usage 	:	164 bytes   0,1 % Full
Data Memory Usage 	:	0 bytes   	0,0 % Full

**Test1** : Atmega1284P 1 usart enable
Program Memory Usage 	:	370 bytes   0,3 % Full : + 206 bytes
Data Memory Usage 	:	133 bytes   0,8 % Full : 64*2 (buffer) + 5(TxHead+TxTail+Rx...+LastRxErro) = 133

**Test2** : Atmega1284P 2 usart enable
Program Memory Usage 	:	580 bytes   0,4 % Full : + 416 bytes
Data Memory Usage 	:	266 bytes   1,6 % Full : 64*4 + 10

**Test3** : Atmega88PA-PU
Program Memory Usage 	:	266 bytes   3,2 % Full
Data Memory Usage 	:	133 bytes   13,0 % Full

### 4. Roadmap

1. Reduce memory impact.
2. Add end function

