EESchema Schematic File Version 4
LIBS:Edifier_Remote-cache
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Device:Q_NPN_BCE Q1
U 1 1 5D48B44F
P 5400 3750
F 0 "Q1" H 5591 3796 50  0000 L CNN
F 1 "Q_NPN_BCE" H 5591 3705 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-220-3_Vertical" H 5600 3850 50  0001 C CNN
F 3 "~" H 5400 3750 50  0001 C CNN
	1    5400 3750
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D1
U 1 1 5D48E449
P 5500 3200
F 0 "D1" V 5539 3083 50  0000 R CNN
F 1 "LED" V 5448 3083 50  0000 R CNN
F 2 "LED_THT:LED_D5.0mm" H 5500 3200 50  0001 C CNN
F 3 "~" H 5500 3200 50  0001 C CNN
	1    5500 3200
	0    -1   -1   0   
$EndComp
$Comp
L Interface_Optical:TSOP43xx IR_Recv1
U 1 1 5D48EEA8
P 4875 4850
F 0 "IR_Recv1" H 4863 5275 50  0000 C CNN
F 1 "TSOP43xx" H 4863 5184 50  0000 C CNN
F 2 "OptoDevice:Vishay_MOLD-3Pin" H 4825 4475 50  0001 C CNN
F 3 "http://www.vishay.com/docs/82460/tsop45.pdf" H 5525 5150 50  0001 C CNN
	1    4875 4850
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0101
U 1 1 5D4946D9
P 6100 5375
F 0 "#PWR0101" H 6100 5125 50  0001 C CNN
F 1 "GND" H 6105 5202 50  0000 C CNN
F 2 "" H 6100 5375 50  0001 C CNN
F 3 "" H 6100 5375 50  0001 C CNN
	1    6100 5375
	1    0    0    -1  
$EndComp
Wire Wire Line
	5275 5050 6100 5050
Wire Wire Line
	6100 5050 6100 5375
$Comp
L power:GND #PWR0102
U 1 1 5D499519
P 5500 4150
F 0 "#PWR0102" H 5500 3900 50  0001 C CNN
F 1 "GND" H 5505 3977 50  0000 C CNN
F 2 "" H 5500 4150 50  0001 C CNN
F 3 "" H 5500 4150 50  0001 C CNN
	1    5500 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	5500 4150 5500 3950
Wire Wire Line
	5200 3750 4925 3750
Text Label 4925 3750 0    50   ~ 0
SEND
Wire Wire Line
	5500 3350 5500 3550
Wire Wire Line
	5500 3050 5500 2925
Text Label 5500 2925 0    50   ~ 0
5V
Wire Wire Line
	6700 4875 6500 4875
Text Label 6500 4875 0    50   ~ 0
5V
$Comp
L power:GND #PWR0103
U 1 1 5D49C889
P 6425 4500
F 0 "#PWR0103" H 6425 4250 50  0001 C CNN
F 1 "GND" H 6430 4327 50  0000 C CNN
F 2 "" H 6425 4500 50  0001 C CNN
F 3 "" H 6425 4500 50  0001 C CNN
	1    6425 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	6700 4375 6425 4375
Wire Wire Line
	6425 4375 6425 4500
Wire Wire Line
	6700 3075 6575 3075
Text Label 6575 3075 0    50   ~ 0
3V3
Wire Wire Line
	5275 4650 5500 4650
Text Label 5500 4650 0    50   ~ 0
3V3
Wire Wire Line
	5275 4850 5500 4850
Text Label 5500 4850 0    50   ~ 0
RECV
Wire Wire Line
	6700 4475 6550 4475
Text Label 6550 4475 0    50   ~ 0
RECV
Wire Wire Line
	6700 4275 6550 4275
Text Label 6550 4275 0    50   ~ 0
SEND
$Comp
L ESP32_Shield:ESP32_Devkit_DOIT_V1 ESP32?
U 1 1 5D4BEDDD
P 7425 4125
F 0 "ESP32?" H 7425 5087 60  0000 C CNN
F 1 "ESP32_Devkit_DOIT_V1" H 7425 4981 60  0000 C CNN
F 2 "ESP32:ESP32_Devkit_DOIT_V1" H 7425 4125 60  0001 C CNN
F 3 "" H 7425 4125 60  0001 C CNN
	1    7425 4125
	1    0    0    -1  
$EndComp
$EndSCHEMATC
