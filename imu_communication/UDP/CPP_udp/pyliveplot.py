#!/usr/bin/python
# -*- coding: utf-8 -*-

from pyqtgraph.Qt import QtGui, QtCore
import pylab as plt
import numpy as np
import pyqtgraph as pg
from pyqtgraph.ptime import time
import serial

app = QtGui.QApplication([])

x = pg.plot()
x.setWindowTitle('live plot of acceleration in x')
curveX = x.plot()

y = pg.plot()
y.setWindowTitle('live plot of acceleration in y')
curveY = y.plot()

dataX = [1]*1000
windowX = [1]*50
dataY = [1]*1000
windowY = [1]*50



def update():
    global curveX, curveY, dataX, dataY
    raw = open('values.txt', 'rb')
    line = raw.readline()
    raw.close()
    line = line.split()
    #print(line)
    windowX.append(int(line[1]))
    windowY.append(int(line[2]))
    del windowX[0]
    del windowY[0]
    dataX.append(sum(windowX)/len(windowX))
    dataY.append(sum(windowY)/len(windowY))
    del dataX[0]
    del dataY[0]
    xdataX = np.array(dataX, dtype='int')
    xdataY = np.array(dataY, dtype='int')
    curveX.setData(xdataX)
    curveY.setData(xdataY)
    app.processEvents()

timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(0)

if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
       QtGui.QApplication.instance().exec_()
