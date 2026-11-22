from machine import Pin, SPI
from machine import SoftSPI
import utime
import os, sdcard

led = machine.Pin(25, machine.Pin.OUT)
# Assign chip select (CS) pin (and start it high)
cs = machine.Pin(22, machine.Pin.OUT)

#sd = sdcard.SDCard(spi, Pin(22))

#spi = machine.SPI(1,
#                  baudrate=1000000,
#                  polarity=0,
#                  phase=0,
#                  bits=8,
#                  firstbit=machine.SPI.MSB,
#                  sck=machine.Pin(5),
#                  mosi=machine.Pin(18),
#                  miso=machine.Pin(19))

spi =SoftSPI(baudrate=1000000,polarity=0,phase=0,sck=Pin(5),mosi=Pin(18),miso=Pin(19))

# Initialize SD card
sd = sdcard.SDCard(spi, cs)

os.mount(sd, '/sd')
os.chdir('sd')
time = 0

while True:
    temperature = 27 
    time = 5

    with open('log.txt', 'a') as f:
        print("Time: ", file=f)
        print(time, file=f)
        print("Temp: ", file=f)
        print(temperature, file=f)
        print(" ", file=f)
      
    led.value(1)
    utime.sleep(0.1)
    led.value(0)
    utime.sleep(0.1)
    led.value(1)
    utime.sleep(0.1)
    led.value(0)
    utime.sleep(4.7)