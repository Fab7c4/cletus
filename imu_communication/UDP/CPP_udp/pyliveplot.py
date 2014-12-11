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

z = pg.plot()
z.setWindowTitle('live plot of acceleration in z')
curveZ = z.plot()

#dataAccel = [[1]*1000]*3
dataAccel = [([1]*1000) for i in range(3)]
#windowAccel = [[1]*50]*3
windowAccel = [([1]*50) for i in range(3)]


def update():
    global curveX, curveY, curveZ, dataX, dataY, dataZ
    raw = open('accel.txt', 'rb')
    line = raw.readline()
    raw.close()
    line = line.split()
    (windowAccel[0]).append(int(line[1]))
    (windowAccel[1]).append(int(line[2]))
    (windowAccel[2]).append(int(line[3]))
    del windowAccel[0][0]
    del windowAccel[1][0]
    del windowAccel[2][0]
    dataAccel[0].append(sum(windowAccel[0])/len(windowAccel[0]))
    dataAccel[1].append(sum(windowAccel[1])/len(windowAccel[1]))
    dataAccel[2].append(sum(windowAccel[2])/len(windowAccel[2]))
    del dataAccel[0][0]
    del dataAccel[1][0]
    del dataAccel[2][0]
    xdataX = np.array(dataAccel[0], dtype='int')
    xdataY = np.array(dataAccel[1], dtype='int')
    xdataZ = np.array(dataAccel[2], dtype='int')
    curveX.setData(xdataX)
    curveY.setData(xdataY)
    curveZ.setData(xdataZ)
    app.processEvents()

timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(0)

if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
       QtGui.QApplication.instance().exec_()
