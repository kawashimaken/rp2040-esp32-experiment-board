from machine import Pin,UART
import time
uart = UART(1, baudrate=9600, tx=Pin(8), rx=Pin(9))
uart.init(bits=8, parity=None, stop=1)
#
interrupt_flag=0
debounce_time=0
 
pressed_pin='5'
        
pin0 =Pin(0, Pin.IN)
pin1 =Pin(1, Pin.IN)
pin2 =Pin(2, Pin.IN)
pin3 =Pin(3, Pin.IN)
pin4 =Pin(4, Pin.IN)

def callback0(pin0):
    global interrupt_flag, debounce_time
    if (time.ticks_ms()-debounce_time) > 500:
        interrupt_flag= 1
        debounce_time=time.ticks_ms()
        print('0')
        uart.write('0')
        pressed_pin='0'

def callback1(pin1):
    global interrupt_flag, debounce_time
    if (time.ticks_ms()-debounce_time) > 500:
        interrupt_flag= 1
        debounce_time=time.ticks_ms()
        print('1')
        uart.write('1')
        pressed_pin='1'
        
def callback2(pin2):
    global interrupt_flag, debounce_time
    if (time.ticks_ms()-debounce_time) > 500:
        interrupt_flag= 1
        debounce_time=time.ticks_ms()
        print('2')
        uart.write('2')
        pressed_pin='2'

def callback3(pin3):
    global interrupt_flag, debounce_time
    if (time.ticks_ms()-debounce_time) > 500:
        interrupt_flag= 1
        debounce_time=time.ticks_ms()
        print('3')
        uart.write('3')
        pressed_pin='3'

def callback4(pin4):
    global interrupt_flag, debounce_time
    if (time.ticks_ms()-debounce_time) > 500:
        interrupt_flag= 1
        debounce_time=time.ticks_ms()
        print('4')
        uart.write('4')
        pressed_pin='4'
        



pin0.irq(trigger=Pin.IRQ_FALLING, handler=callback0)
pin1.irq(trigger=Pin.IRQ_FALLING, handler=callback1)
pin2.irq(trigger=Pin.IRQ_FALLING, handler=callback2)
pin3.irq(trigger=Pin.IRQ_FALLING, handler=callback3)
pin4.irq(trigger=Pin.IRQ_FALLING, handler=callback4)

while True:
    if interrupt_flag is 1:
        interrupt_flag=0
        print("Interrupt Detected")

    
