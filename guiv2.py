import matplotlib.pyplot as plt
import os
from tkinter import Tk, Button, BOTH, TOP, BOTTOM
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from time import sleep
from threading import Thread

def change_signal():
    global signal
    global driver

    print('estamos leyendo el sensor',signal)


    figure.clear()
    pulses.clear()
    x = []

    if signal == 0:
        signal = 1
        os.write(driver, b'1')
        print('cambiamos a sensor',signal)
    else:
        signal = 0
        os.write(driver, b'0')
        print('cambiamos a sensor',signal)

        # driver.flush()
        

def logic():
    global pulses
    global x
    while 1:

        pulse = os.read(driver, 1024)
        decoded = pulse.decode('utf-8')
        print(decoded)
        figure.clear()

        if signal == 0:
            # print("senal 0")
            if decoded == '2':
                # print("entrando..")
                if len(pulses) == 0:
                    pulses.append(1)
                    # print(pulses)
                else:
                    aux = pulses[len(pulses)-1]
                    aux += 1
                    pulses.append(aux)
                
            elif decoded == '1':
                print("entrando..")
                if len(pulses) == 0:
                    pulses.append(-1)
                    # print(pulses)
                else:
                    aux = pulses[len(pulses)-1]
                    aux -= 1
                    pulses.append(aux)
            # print(pulses)
            x = range(1, len(pulses) + 1)
            figure.add_subplot(111).plot(x,pulses)
        else:
            # print(decoded)
            if len(decoded) > 0:
                pulses.append(decoded)
                # print(decoded)
            
            x = range(1, len(pulses) + 1)
            figure.add_subplot(111).step(x,pulses)
            sleep(1)

        canvas.draw()
        sleep(0.001)


if __name__ == "__main__":

    driver = os.open('/dev/sensor_Driver',os.O_RDWR)

    counter = 0
    pulses = []
    x = []

    signal = 0

    root = Tk()

    global figure
    figure = Figure()

    if signal == 0:
        figure.add_subplot(111).plot([],[])
    else:
        figure.add_subplot(111).step([],[])

    canvas = FigureCanvasTkAgg(figure, master=root) 
    canvas.draw()
    canvas_widget = canvas.get_tk_widget()
    canvas_widget.pack(side=TOP, fill=BOTH, expand=1)

    button = Button(master=root, text="Change", command=change_signal)
    button.pack(side=BOTTOM)

    rep_thread = Thread(target=logic) 
    rep_thread.daemon = True
    rep_thread.start()


    root.mainloop()
