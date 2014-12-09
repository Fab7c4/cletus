#!/usr/bin/python
# -*- coding: utf-8 -*-

from pyqtgraph.Qt import QtGui, QtCore
import pylab as plt
import numpy as np
import pyqtgraph as pg
from pyqtgraph.ptime import time
import serial

app = QtGui.QApplication([])

p = pg.plot()
p.setWindowTitle('live plot from serial')
curve = p.plot()

#with open('accel.txt', 'rb') as npfile:
#    data = []
#    data = np.loadtxt(npfile, delimiter=',')
data = [1]*1000
window = [1]*50



def update():
    global curve, data
    raw = open('accel.txt', 'rb')
    line = raw.readline()
    raw.close()
    #print(line)
    window.append(int(line))
    del window[0]
    data.append(sum(window)/len(window))
    del data[0]
    xdata = np.array(data, dtype='int')
    curve.setData(xdata)
    app.processEvents()

timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(0)

if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
       QtGui.QApplication.instance().exec_()
