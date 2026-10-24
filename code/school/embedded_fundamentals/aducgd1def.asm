
;REV.  1.1    31 October 2002
;ADuC831   Apps, Analog Devices Inc.
p0       data  080h  ;PORT 0	
sp       data  081h  ;STACK POINTER	
dpl      data  082h  ;DATA POINTER - LOW BYTE
dph      data  083h  ;DATA POINTER - HIGH BYTE
dpp      data  084h  ;DATA POINTER - PAGE BYTE
pcon     data  087h  ;POWER CONTROL
tcon     data  088h  ;TIMER CONTROL
tmod     data  089h  ;TIMER MODE
tl0      data  08ah  ;TIMER 0 - LOW BYTE
tl1      data  08bh  ;TIMER 1 - LOW BYTE
th0      data  08ch  ;TIMER 0 - HIGH BYTE
th1      data  08dh  ;TIMER 1 - HIGH BYTE
p1       data  090h  ;PORT 1
scon     data  098h  ;SERIAL PORT CONTROL
sbuf     data  099h  ;SERIAL PORT BUFFER
i2cdat   data  09ah  ;I2C DATA REGISTER
i2cadd   data  09bh  ;I2C ADDRESS REGISTER
t3con    data  09eh  ;TIMER 3 CONTROL
t3fd     data  09dh  ;TIMER 3 FRACTIONAL DIVIDER
p2       data  0a0h  ;PORT 2
timecon  data  0a1h  ;TIC CONTROL
hthsec   data  0a2h  ;TIC - HTHSEC DATA
sec      data  0a3h  ;TIC - SEC DATA
min      data  0a4h  ;TIC - MIN DATA
hour     data  0a5h  ;TIC - HOUR DATA
intval   data  0a6h  ;TIC INTERVAL REGISTER
dpcon    data  0a7h  ;DUAL DATA POINTER CONTROL REGISTER
ie       data  0a8h  ;INTERRUPT ENABLE 1
ieip2    data  0a9h  ;INTERRUPT ENABLE 2
pwmcon   data  0aeh  ;PWM CONTROL REGISTER
cfg832   data  0afh  ;GENERAL FLASH/PWM CONTROL REGISTER
p3       data  0b0h  ;PORT 3
pwm0l    data  0b1h  ;PWM DATA REGISTER
pwm0h    data  0b2h  ;PWM DATA REGISTER
pwm1l    data  0b3h  ;PWM DATA REGISTER
pwm1h    data  0b4h  ;PWM DATA REGISTER
sph      data  0b7h  ;EXTENDED STACK POINTER REGISTER
ip       data  0b8h  ;INTERRUPT PRIORITY
econ     data  0b9h  ;FLASH EEPROM CONTROL
edata1   data  0bch  ;FLASH EEPROM DATA1 
edata2   data  0bdh  ;FLASH EEPROM DATA2 
edata3   data  0beh  ;FLASH EEPROM DATA3 
edata4   data  0bfh  ;FLASH EEPROM DATA4 
wdcon    data  0c0h  ;WATCHDOG TIMER CONTROL
chipid   data  0c2h  ;CHIPID REGISTER
eadrl    data  0c6h  ;FLASH EEPROM PAGE ADDRESS - LOW BYTE
eadrh    data  0c7h  ;FLASH EEPROM PAGE ADDRESS - LOW BYTE
t2con    data  0c8h  ;TIMER 2 CONTROL
rcap2l   data  0cah  ;TIMER 2 CAPTURE REGISTER - LOW BYTE
rcap2h   data  0cbh  ;TIMER 2 CAPTURE REGISTER - HIGH BYTE
tl2      data  0cch  ;TIMER 2 - LOW BYTE
th2      data  0cdh  ;TIMER 2 - HIGH BYTE
psw      data  0d0h  ;PROGRAM STATUS WORD
dmal     data  0d2h  ;DMA ADDRESS LOW BYTE
dmah     data  0d3h  ;DMA ADDRESS HIGH BYTE
dmap     data  0d4h  ;DMA ADDRESS PAGE BYTE
pllcon   data  0d7h  ;PLL CONTROL REGISTER
adccon2  data  0d8h  ;ADC CONTROL
adcdatal data  0d9h  ;ADC DATA LOW BYTE
adcdatah data  0dah  ;ADC DATA HIGH BYTE
psmcon   data  0dfh  ;POWER SUPPLY MONITOR
acc      data  0e0h  ;ACCUMULATOR
dcon     data  0e8h  ;D1 AND D0 CONTROL
i2ccon   data  0e8h  ;I2C CONTROL
adccon1  data  0efh  ;ADC CONTROL
b        data  0f0h  ;MULTIPLICATION REGISTER
adcofsl  data  0f1h  ;ADC OFFSET LOW BYTE
adcofsh  data  0f2h  ;ADC OFFSET HIGH BYTE
adcgainl data  0f3h  ;ADC GAIN LOW BYTE
adcgainh data  0f4h  ;ADC GAIN HIGH BYTE
adccon3  data  0f5h  ;ADC CONTROL
spidat   data  0f7h  ;SPI DATA REGISTER
spicon   data  0f8h  ;SPI CONTROL REGISTER
dac0l    data  0f9h  ;DAC0 LOW BYTE
dac0h    data  0fah  ;DAC0 HIGH BYTE
dac1l    data  0fbh  ;DAC1 LOW BYTE
dac1h    data  0fch  ;DAC1 HIGH BYTE
daccon   data  0fdh  ;DAC CONTROL REGISTER
it0      bit   088h  ;TCON.0 - EXT. INTERRUPT 0 TYPE
ie0      bit   089h  ;TCON.1 - EXT. INTERRUPT 0 EDGE FLAG
it1      bit   08ah  ;TCON.2 - EXT. INTERRUPT 1 TYPE
ie1      bit   08bh  ;TCON.3 - EXT. INTERRUPT 1 EDGE FLAG
tr0      bit   08ch  ;TCON.4 - TIMER 0 ON/OFF CONTROL
tf0      bit   08dh  ;TCON.5 - TIMER 0 OVERFLOW FLAG
tr1      bit   08eh  ;TCON.6 - TIMER 1 ON/OFF CONTROL
tf1      bit   08fh  ;TCON.7 - TIMER 1 OVERFLOW FLAG
t2       bit   090h  ;P1.0 - TIMER 2 TRIGGER INPUT
t2ex     bit   091h  ;P1.1 - TIMER 2 COUNT INPUT
ri       bit   098h  ;SCON.0 - RECEIVE INTERRUPT FLAG
ti       bit   099h  ;SCON.1 - TRANSMIT INTERRUPT FLAG
rb8      bit   09ah  ;SCON.2 - RECEIVE BIT 8
tb8      bit   09bh  ;SCON.3 - TRANSMIT BIT 8
ren      bit   09ch  ;SCON.4 - RECEIVE ENABLE
sm2      bit   09dh  ;SCON.5 - SERIAL MODE CONTROL BIT 2
sm1      bit   09eh  ;SCON.6 - SERIAL MODE CONTROL BIT 1
sm0      bit   09fh  ;SCON.7 - SERIAL MODE CONTROL BIT 0
ex0      bit   0a8h  ;IE.0 - EXTERNAL INTERRUPT 0 ENABLE
et0      bit   0a9h  ;IE.1 - TIMER 0 INTERRUPT ENABLE
ex1      bit   0aah  ;IE.2 - EXTERNAL INTERRUPT 1 ENABLE
et1      bit   0abh  ;IE.3 - TIMER 1 INTERRUPT ENABLE
es       bit   0ach  ;IE.4 - SERIAL PORT INTERRUPT ENABLE
et2      bit   0adh  ;IE.5 - TIMER 2 INTERRUPT ENABLE
eadc     bit   0aeh  ;IE.6 - ENABLE ADC INTURRUPT
ea       bit   0afh  ;IE.7 - GLOBAL INTERRUPT ENABLE
rxd      bit   0b0h  ;P3.0 - SERIAL PORT RECEIVE INPUT
txd      bit   0b1h  ;P3.1 - SERIAL PORT TRANSMIT OUTPUT
int0     bit   0b2h  ;P3.2 - EXTERNAL INTERRUPT 0 INPUT
int1     bit   0b3h  ;P3.3 - EXTERNAL INTERRUPT 1 INPUT
t0       bit   0b4h  ;P3.4 - TIMER 0 COUNT INPUT
t1       bit   0b5h  ;P3.5 - TIMER 1 COUNT INPUT
wr       bit   0b6h  ;P3.6 - WRITE CONTROL FOR EXT. MEMORY
rd       bit   0b7h  ;P3.7 - READ CONTROL FOR EXT. MEMORY
px0      bit   0b8h  ;IP.0 - EXTERNAL INTERRUPT 0 PRIORITY
pt0      bit   0b9h  ;IP.1 - TIMER 0 PRIORITY
px1      bit   0bah  ;IP.2 - EXTERNAL INTERRUPT 1 PRIORITY
pt1      bit   0bbh  ;IP.3 - TIMER 1 PRIORITY
ps       bit   0bch  ;IP.4 - SERIAL PORT PRIORITY
pt2      bit   0bdh  ;IP.5 - TIMER 2 PRIORITY
padc     bit   0beh  ;IP.6 - ADC PRIORITY
psi      bit   0bfh  ;IP.7 - SPI OR 2-WIRE SERIAL INTERFACE PRIORITY
wdwr     bit   0c0h  ;WDCON.0 - WATCHDOG WRITE ENABLE
wde      bit   0c1h  ;WDCON.1 - WATCHDOG ENABLE
wds      bit   0c2h  ;WDCON.2 - WATCHDOG STATUS BIT
wdir     bit   0c3h  ;WDCON.3 - WATCHDOG INTERRUPT RESPONSE ENABLE BIT
pre0     bit   0c4h  ;WDCON.4 - WATCHDOG TIMEOUT SELECTION BIT0
pre1     bit   0c5h  ;WDCON.5 - WATCHDOG TIMEOUT SELECTION BIT1
pre2     bit   0c6h  ;WDCON.6 - WATCHDOG TIMEOUT SELECTION BIT2
pre3     bit   0c7h  ;WDCON.7 - WATCHDOG TIMEOUT SELECTION BIT3
cap2     bit   0c8h  ;T2CON.0 - CAPTURE OR RELOAD SELECT
cnt2     bit   0c9h  ;T2CON.1 - TIMER OR COUNTER SELECT
tr2      bit   0cah  ;T2CON.2 - TIMER 2 ON/OFF CONTROL
exen2    bit   0cbh  ;T2CON.3 - TIMER 2 EXTERNAL ENABLE FLAG
tclk     bit   0cch  ;T2CON.4 - TRANSMIT CLOCK SELECT
rclk     bit   0cdh  ;T2CON.5 - RECEIVE CLOCK SELECTT
exf2     bit   0ceh  ;T2CON.6 - EXTERNAL TRANSITION FLAG
tf2      bit   0cfh  ;T2CON.7 - TIMER 2 OVERFLOW FLAG
p        bit   0d0h  ;PSW.0 - ACCUMULATOR PARITY FLAG
f1       bit   0d1h  ;PSW.1 - FLAG 0
ov       bit   0d2h  ;PSW.2 - OVERFLOW FLAG
rs0      bit   0d3h  ;PSW.3 - REGISTER BANK SELECT 0
rs1      bit   0d4h  ;PSW.4 - REGISTER BANK SELECT 1
f0       bit   0d5h  ;PSW.5 - FLAG 0
ac       bit   0d6h  ;PSW.6 - AUXILIARY CARRY FLAG
cy       bit   0d7h  ;PSW.7 - CARRY FLAG
cs0      bit   0d8h  ;ADCCON2.0 - ADC INPUT CHANNEL SELECT BIT0
cs1      bit   0d9h  ;ADCCON2.1 - ADC INPUT CHANNEL SELECT BIT1
cs2      bit   0dah  ;ADCCON2.2 - ADC INPUT CHANNEL SELECT BIT2
cs3      bit   0dbh  ;ADCCON2.3 - ADC INPUT CHANNEL SELECT BIT3
sconv    bit   0dch  ;ADCCON2.4 - SINGLE CONVERSION ENABLE
cconv    bit   0ddh  ;ADCCON2.5 - CONTINUOUS CONVERSION ENABLE
dma      bit   0deh  ;ADCCON2.6 - DMA MODE ENABLE
adci     bit   0dfh  ;ADCCON2.7 - ADC INTURRUPT FLAG
d0en     bit   0ebh  ;DCON.3 - D0 ENABLE BIT
d0       bit   0edh  ;DCON.5 - D0 DATA BIT
d1en     bit   0eeh  ;DCON.6 - D1 ENABLE BIT
d1       bit   0efh  ;DCON.7 - D1 DATA BIT
i2ci     bit   0e8h  ;I2CCON.0 - I2C INTURRUPT FLAG
i2ctx    bit   0e9h  ;I2CCON.1 - I2C TRANSMIT SELECT
i2crs    bit   0eah  ;I2CCON.2 - I2C RESET
i2cm     bit   0ebh  ;I2CCON.3 - I2C MASTER MODE SELECT
mdi      bit   0ech  ;I2CCON.4 - I2C MASTER MODE SDATA INPUT
mco      bit   0edh  ;I2CCON.5 - I2C MASTER MODE SCLOCK OUTPUT
mde      bit   0eeh  ;I2CCON.6 - I2C MASTER MODE SDATA ENABLE
mdo      bit   0efh  ;I2CCON.7 - I2C MASTER MODE SDATA OUTPUT
spr0     bit   0f8h  ;SPICON.0 - SPI BITRATE SELECT BIT0
spr1     bit   0f9h  ;SPICON.1 - SPI BITRATE SELECT BIT1
cpha     bit   0fah  ;SPICON.2 - SPI CLOCK PHASE SELECT
cpol     bit   0fbh  ;SPICON.3 - SPI CLOCK POLARITY SELECT
spim     bit   0fch  ;SPICON.4 - SPI MASTER/SLAVE MODE SELECT
spe      bit   0fdh  ;SPICON.5 - SPI INTERFACE ENABLE
wcol     bit   0feh  ;SPICON.6 - SPI WRITE COLLISION ERROR FLAG
ispi     bit   0ffh  ;SPICON.7 - SPI END OF TRANSFER FLAG
