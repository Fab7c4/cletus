import sys
from PySide.QtCore import *
from PySide.QtGui import *
from PySide.QtWebKit import*
import pyqtgraph as pg
import pyqtgraph.opengl as gl
from logdata import *
import numpy as np




class Colors:
	Red = (255, 0, 0)
	Green = (0, 255, 0)
	Blue = (0, 0, 255)
	OrangeRed = (255, 69, 0)
	Orange = (255, 140, 0)
	DodgerBlue = (30, 144, 255)
	LawnGreen = (124, 252, 0)
	LimeGreen = (50, 205, 50)
	SkyBlue = (0, 191, 255)


def OpenFileHandler():
	dialog = QFileDialog()
	dialog.setFileMode(QFileDialog.AnyFile)
	dialog.setNameFilter("Cletus Log Files (*.bin)")
	if dialog.exec_():
		fileNames = dialog.selectedFiles()	
		LoadLogFile(fileNames[0])



def transparentColor(color, alpha):
	return color + (alpha, )

class MainWindow(QWidget):
	def __init__(self, parent=None):
		super(MainWindow,self).__init__(parent)
		self.setWindowTitle("Baby Betty LogViewer")

		layout = QVBoxLayout()

		header = QHBoxLayout()
		btn_open = QPushButton("&Open File")
		btn_open.clicked.connect(OpenFileHandler)
		header.addWidget(btn_open)
		layout.addItem(header)

		tabs = QTabWidget()
		self.tab_accel = PlotXYZ(labelX="timestamp", labelY="Acceleration", unitX="s", unitY="m/s^2")

		tabs.addTab(self.tab_accel, "Accel")
		self.tab_gyro = PlotXYZ(labelX="timestamp", labelY="Rotation", unitX="s", unitY="deg/s")
		tabs.addTab(self.tab_gyro, "Gyro")
		self.tab_mag = PlotXYZ(labelX="timestamp", labelY="Rotation", unitX="s", unitY="deg/s")
		tabs.addTab(self.tab_mag, "Mag")

		self.tab_gps_pos = GPSPosition()
		tabs.addTab(self.tab_gps_pos, "GPS Position")

		self.tab_gps_vel = PlotXYZ(labelX="timestamp", labelY="Velocity", unitX="s", unitY="m/s")
		tabs.addTab(self.tab_gps_vel, "GPS Velocity")

		self.tab_airspeed = PlotXYZ(labelX="timestamp", labelY="Airspeed",xTitle="Scaled",yTitle="Offset",zTitle="Raw", unitX="s", unitY="")
		tabs.addTab(self.tab_airspeed, "Airspeed")

		self.tab_stats = StatisticsWidget()
		tabs.addTab(self.tab_stats, "Stats")

		self.tab_map = QWebView()
		tabs.addTab(self.tab_map, "Map")
		layout.addWidget(tabs)
		self.setLayout(layout)


	def clear(self):
		print "Clear Window"
		self.tab_gps_pos.clear()
		self.tab_stats.clear()


		




class StatisticsWidget(QWidget):
	def __init__(self, parent=None):
		super(StatisticsWidget,self).__init__(parent)
		layout = QGridLayout()
		self.imu = StatisticsItem("IMU")
		self.gps_pos = StatisticsItem("GPS Baseline Position")
		self.gps_vel = StatisticsItem("GPS Baseline Velocity")
		self.airspeed = StatisticsItem("Airspeed")
		self.hist = pg.PlotWidget()
		self.hist.getPlotItem().addLegend()
		layout.addWidget(self.imu,1,1,1,1)
		layout.addWidget(self.gps_pos,2,1,1,1)
		layout.addWidget(self.gps_vel,3,1,1,1)
		layout.addWidget(self.airspeed,4,1,1,1)
		layout.addWidget(self.hist,1,2,3,1)
		self.setLayout(layout)
		pass

	def clear(self):
		self.gps_pos.clear()
		self.gps_vel.clear()
		self.imu.clear()
		print " clearing hist"
		self.hist.clear()
		pass

	def addHistogramm(self):
		pass



		


class StatisticsItem(QGroupBox):
	def __init__(self,title=None, parent=None):
		super(StatisticsItem,self).__init__(title,parent)
		formLayout = QFormLayout()
		self.meanF = QLabel("---", self)
		self.maxF = QLabel("---", self)
		self.minF = QLabel("---", self)
		self.meanT = QLabel("---", self)
		self.maxT = QLabel("---", self)
		self.minT = QLabel("---", self)
		formLayout.addRow(self.tr("&Mean Freq [Hz]:"), self.meanF)
		formLayout.addRow(self.tr("&Min Freq [Hz]:"), self.minF)
		formLayout.addRow(self.tr("&Max Freq [Hz]:"), self.maxF)
		formLayout.addRow(self.tr("&Mean T [s]:"), self.meanT)
		formLayout.addRow(self.tr("&Min T [s]:"), self.minT)
		formLayout.addRow(self.tr("&Max T [s]:"), self.maxT)
		self.setLayout(formLayout)
		self.show()
		pass

	def clear(self):
		self.meanF.setText("---")
		self.maxF.setText("---")
		self.minF.setText("---")
		self.meanT.setText("---")
		self.maxT.setText("---")
		self.minT.setText("---")



	def setValues(self,stats):
		self.meanF.setText("{:.5f}".format(stats.mean_frequency))
		self.maxF.setText("{:.5f}".format(stats.max_frequency) )
		self.minF.setText("{:.5f}".format(stats.min_frequency) )
		self.meanT.setText("{:.5f}".format(stats.mean_period))
		self.maxT.setText("{:.5f}".format(stats.max_period) )
		self.minT.setText("{:.5f}".format(stats.min_period) )




class GPSPosition(QWidget):
	def __init__(self, parent=None):
		super(GPSPosition,self).__init__(parent)
		layout = QVBoxLayout()
		group1 = QGroupBox(title="Position")
		group1.setMinimumHeight(400)
		group2 = QGroupBox(title="Status")
		layout.addWidget(group1)
		layout.addWidget(group2)
		self.position_plot = Plot3D(labelX="X", labelY="Y", labelZ="Z", unit="m")
		vbox1 = QVBoxLayout()
		vbox1.addWidget(self.position_plot)
		group1.setLayout(vbox1)
		vbox2 = QVBoxLayout()
		self.status_plot = pg.PlotWidget(title="RTK Status", labels={'left':("Status", "") , 'bottom':("timestamp", "s")})
		vbox2.addWidget(self.status_plot)
		group2.setLayout(vbox2)
		self.setLayout(layout)
		self.show()
		pass

	def clear(self):
		self.position_plot.clear()




class PlotXYZ(pg.GraphicsLayoutWidget):
	def __init__(self, parent=None, xTitle="X", yTitle="Y",zTitle="Z", labelX=None, labelY=None,unitX=None, unitY=None):
		super(PlotXYZ, self).__init__(parent)
		#store Parameters for reinitialization
		self.xTitle = xTitle
		self.yTitle = yTitle
		self.zTitle = zTitle
		self.labelX = labelX
		self.labelY = labelY
		self.unitX = unitX
		self.unitY = unitY
		self.init()
  		self.addItem(self.plot_x)
  		self.nextRow()
  		self.addItem(self.plot_y)
  		self.nextRow()
  		self.addItem(self.plot_z)
  		pass

  	def clearPlots(self):
  		print "Clear Plot"
  		self.plot_x.clear()
  		self.plot_y.clear()
  		self.plot_z.clear()
  		self.init()
  		pass

  	def init(self):
  		print "Init plot"
  		self.plot_x = pg.PlotItem(title=self.xTitle, labels={'left':(self.labelY, self.unitY) , 'bottom':(self.labelX, self.unitX)})
		self.plot_y = pg.PlotItem(title=self.yTitle, labels={'left':(self.labelY, self.unitY) , 'bottom':(self.labelX, self.unitX)})
		self.plot_z = pg.PlotItem(title=self.zTitle, labels={'left':(self.labelY, self.unitY) , 'bottom':(self.labelX, self.unitX)})




class Plot3D(gl.GLViewWidget):
	def __init__(self, parent=None, Title="'pyqtgraph example: GLLinePlotItem'", labelX=None, labelY=None, labelZ=None, unit= None):
		super(Plot3D, self).__init__(parent)
		self.setWindowTitle(Title)
		#add Grids
		gx = gl.GLGridItem()
		gx.rotate(90, 0, 1, 0)
		gx.translate(-10, 0, 0)
		self.addItem(gx)
		gy = gl.GLGridItem()
		gy.rotate(90, 1, 0, 0)
		gy.translate(0, -10, 0)
		self.addItem(gy)
		gz = gl.GLGridItem()
		gz.translate(0, 0, -10)
		self.addItem(gz)
		self.show()


	def addLinePlot(self, x, y, z, color=pg.glColor(Colors.Red), width=2):
		self.removeItem
		pts = np.vstack([x,y,z]).transpose()
		self.plt = gl.GLLinePlotItem(pos=pts, color=color, width=width, antialias=True)
		self.addItem(self.plt)
		return 0

	def clear(self):
		if hasattr(self, 'plt'):
			self.removeItem(self.plt)
		pass








def LoadLogFile(filename=None):
	if filename != None:

		window.clear()
		data = LogData(filename=filename)
		print "Plotting"
		window.tab_accel.plot_x.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.x for y in data.accel],clear=True,pen=pg.mkPen(Colors.DodgerBlue))
		window.tab_accel.plot_y.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.y for y in data.accel],clear=True,pen=pg.mkPen(Colors.LawnGreen))
		window.tab_accel.plot_z.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.z for y in data.accel],clear=True,pen=pg.mkPen(Colors.OrangeRed))

		window.tab_gyro.plot_x.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.x for y in data.gyro],clear=True, pen=pg.mkPen(Colors.DodgerBlue))
		window.tab_gyro.plot_y.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.y for y in data.gyro],clear=True, pen=pg.mkPen(Colors.LawnGreen))
		window.tab_gyro.plot_z.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.z for y in data.gyro],clear=True, pen=pg.mkPen(Colors.OrangeRed))

		window.tab_mag.plot_x.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.x for y in data.mag],clear=True, pen=pg.mkPen(Colors.DodgerBlue))
		window.tab_mag.plot_y.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.y for y in data.mag],clear=True, pen=pg.mkPen(Colors.LawnGreen))
		window.tab_mag.plot_z.plot([fromTimestamp(y.timestamp) for y in data.imu], [y.z for y in data.mag],clear=True, pen=pg.mkPen(Colors.OrangeRed))

		window.tab_airspeed.plot_x.plot([fromTimestamp(y.timestamp) for y in data.airspeed], [y.scaled for y in data.airspeed],clear=True, pen=pg.mkPen(Colors.DodgerBlue))
		window.tab_airspeed.plot_y.plot([fromTimestamp(y.timestamp) for y in data.airspeed], [y.offset for y in data.airspeed],clear=True, pen=pg.mkPen(Colors.LawnGreen))
		window.tab_airspeed.plot_z.plot([fromTimestamp(y.timestamp) for y in data.airspeed], [y.raw for y in data.airspeed],clear=True, pen=pg.mkPen(Colors.OrangeRed))

		# window.tab_gps_pos.plot_x.plot([y.timeOfWeek for y in data.gps_position], [y.data.x for y in data.gps_position])
		# window.tab_gps_pos.plot_y.plot([y.timeOfWeek for y in data.gps_position], [y.data.y for y in data.gps_position])
		# window.tab_gps_pos.plot_z.plot([y.timeOfWeek for y in data.gps_position], [y.data.z for y in data.gps_position])
		window.tab_gps_pos.position_plot.addLinePlot(x=[y.data.x for y in data.gps_position], y=[y.data.y for y in data.gps_position], z=[y.data.z for y in data.gps_position])
		window.tab_gps_pos.status_plot.plot([y.timeOfWeek*1e-3 for y in data.gps_position], [y.fixedRTK for y in data.gps_position],clear=True)
		window.tab_gps_pos.status_plot.plot([y.timeOfWeek*1e-3 for y in data.gps_velocity], [y.fixedRTK for y in data.gps_velocity], pen=pg.mkPen(Colors.DodgerBlue))
		window.tab_gps_vel.plot_x.plot([y.timeOfWeek*1e-3 for y in data.gps_velocity], [y.data.x for y in data.gps_velocity],clear=True)
		window.tab_gps_vel.plot_y.plot([y.timeOfWeek*1e-3 for y in data.gps_velocity], [y.data.y for y in data.gps_velocity],clear=True)
		window.tab_gps_vel.plot_z.plot([y.timeOfWeek*1e-3 for y in data.gps_velocity], [y.data.z for y in data.gps_velocity],clear=True)
		data.getGoogleMap()

		window.tab_map.load(QUrl("./mymap.html"))
		window.tab_map.show()

		statsIMU = data.calcStatisticsBB(data.imu)
		window.tab_stats.imu.setValues(statsIMU)

		statsAirspeed = data.calcStatisticsBB(data.airspeed)
		window.tab_stats.airspeed.setValues(statsAirspeed)


		statsGPS_vel = data.calcStatisticsGPS(data.gps_velocity)
		window.tab_stats.gps_vel.setValues(statsGPS_vel)          

		statsGPS_pos = data.calcStatisticsGPS(data.gps_position)
		window.tab_stats.gps_pos.setValues(statsGPS_pos)    

		window.tab_stats.hist.addItem(statsAirspeed.getTimeHistogramm(color=transparentColor(Colors.OrangeRed, 180), name="Airspeed"))
		window.tab_stats.hist.addItem(statsIMU.getTimeHistogramm(color=transparentColor(Colors.SkyBlue, 180),name="IMU"))
		window.tab_stats.hist.addItem(statsGPS_vel.getTimeHistogramm(color=transparentColor(Colors.LimeGreen, 180),name="GPS Velocity"))
		window.tab_stats.hist.addItem(statsGPS_pos.getTimeHistogramm(color=transparentColor(Colors.Orange, 180),name="GPS Position"))









# Main script
app = QApplication(sys.argv)
window = MainWindow()
window.show()
app.exec_()