import pylab as plt
import messages_pb2
import pygmaps 
import pyqtgraph as pg



class LogData:
	def __init__(self, parent=None, filename=None):
		container = messages_pb2.LogContainer()
		f = open(filename, "r")
		d = f.read()
		f.close()
		container.ParseFromString(d)
		log_messages = container.log_data
		sensors = [y.sensors for y in container.log_data if y.HasField('sensors')]
		print "Logfile contains %i sensor logs" % len(sensors)
		actuators = [y.actuators for y in container.log_data if y.HasField('actuators') ]
		print "Logfile contains %i actuator logs" % len(actuators)
		self.gps_llh = [y.gps_llh for y in container.log_data if y.HasField('gps_llh')]
		print "Logfile contains %i GPS (LAT/LONG/HEIGHT) logs" % len(self.gps_llh)
		self.servos = [y.servos for y in container.log_data if y.HasField('servos')]
		print "Logfile contains %i servo logs" % len(self.servos)
		print "#################################################"
		self.imu = [y.imu for y in sensors if y.HasField('imu')]
		print "Logfile contains %i IMU logs" % len(self.imu)
		self.gyro = [y.gyro for y in self.imu if y.HasField('gyro')]
		print "Logfile contains %i GYRO logs" % len(self.gyro)
		self.accel = [y.accel for y in self.imu if y.HasField('accel')]
		print "Logfile contains %i ACCEL logs" % len(self.accel)
		self.mag = [y.mag for y in self.imu if y.HasField('mag')]
		print "Logfile contains %i MAG logs" % len(self.mag)
		self.gps_position = [y.gps_position for y in sensors if y.HasField('gps_position')]
		self.gps_velocity = [y.gps_velocity for y in sensors if y.HasField('gps_velocity')]
		print "Logfile contains %i GPS position logs" % len(self.gps_position)
		print "Logfile contains %i GPS velocity logs" % len(self.gps_velocity)
		self.airspeed = [y.airspeed for y in sensors if y.HasField('airspeed')]
		print "Logfile contains %i AIRSPEED logs" % len(self.airspeed)


	def getGoogleMap(self):
		latidute = [y.data.x for y in self.gps_llh]
		longitude = [y.data.y for y in self.gps_llh]
		height =  [y.data.z for y in self.gps_llh]
		mymap = pygmaps.maps(latidute[1],longitude[1],16)
		path = zip(latidute, longitude)
		mymap.addpath(path, "#0099FF")
		mymap.draw('./mymap.html')




	def calcStatisticsBB(self, data):
		stats = LogStatistics()
		times_s = [fromTimestamp(y.timestamp) for y in data]
		stats.calcFrequencies(times_s)
		return stats

	def calcStatisticsGPS(self, data):
		stats = LogStatistics()
		times_s = [y.timeOfWeek*1e-3 for y in data]
		stats.calcFrequencies(plt.sort(times_s))
		return stats


class LogStatistics:
	def __init__(self, parent=None):
		self.total_time = 0.0
		self.min_frequency = 0.0
		self.max_frequency = 0.0
		self.mean_frequency = 0.0
		self.min_period = 0.0
		self.max_period = 0.0
		self.mean_period = 0.0
		self.max_latency = 0.0
		self.min_latency = 0.0
		self.times = []

	def calcFrequencies(self, times):
		self.times = times
		diffs = plt.diff(times)
		self.total_time = times[-1] - times[0]
		self.mean_period =  plt.mean(diffs) 
		self.max_period = diffs.max() 
		self.min_period = diffs.min()
		self.max_frequency = 1/ self.min_period
		self.min_frequency = 1 / self.max_period
		self.mean_frequency = 1 / self.mean_period

	def getTimeHistogramm(self, color, name):
		diffs = plt.diff(self.times)
		## compute standard histogram
		y,x = plt.histogram(diffs, bins=plt.linspace(diffs.min(), diffs.max(), 500))

		## notice that len(x) == len(y)+1
		## We are required to use stepMode=True so that PlotCurveItem will interpret this data correctly.
		curve = pg.PlotCurveItem(x, y, stepMode=True, fillLevel=0, pen=color, brush=color, name=name)
		return curve




def fromTimestamp(ts):
    return float(ts.tsec) + 1e-9*float(ts.tnsec)











# plt.figure(1)
# plt.subplot(311)
# plt.title('gyro with timestep')
# plt.plot([fromTimestamp(y.timestamp) for y in gyros], [y.data.x for y in gyros])
# plt.subplot(312)
# plt.plot([fromTimestamp(y.timestamp) for y in gyros], [y.data.y for y in gyros])
# plt.subplot(313)
# plt.plot([fromTimestamp(y.timestamp) for y in gyros], [y.data.z for y in gyros])

# plt.figure(2)
# plt.subplot(311)
# plt.title('gyro with count')
# plt.plot([k for k,y in enumerate(gyros)], [y.data.x for y in gyros])
# plt.subplot(312)
# plt.plot([k for k,y in enumerate(gyros)], [y.data.y for y in gyros])
# plt.subplot(313)
# plt.plot([k for k,y in enumerate(gyros)], [y.data.z for y in gyros])

# plt.figure(3)
# plt.subplot(311)
# plt.title('accel with timestep')
# plt.plot([fromTimestamp(y.timestamp) for y in accels], [y.data.x for y in accels])
# plt.subplot(312)
# plt.plot([fromTimestamp(y.timestamp) for y in accels], [y.data.y for y in accels])
# plt.subplot(313)
# plt.plot([fromTimestamp(y.timestamp) for y in accels], [y.data.z for y in accels])

# plt.figure(4)
# plt.subplot(311)
# plt.title('accel with count')
# plt.plot([k for k,y in enumerate(accels)], [y.data.x for y in accels])
# plt.subplot(312)
# plt.plot([k for k,y in enumerate(accels)], [y.data.y for y in accels])
# plt.subplot(313)
# plt.plot([k for k,y in enumerate(accels)], [y.data.z for y in accels])

# plt.figure(5)
# plt.subplot(311)
# plt.title('mag with timestep')
# plt.plot([fromTimestamp(y.timestamp) for y in mags], [y.data.x for y in mags])
# plt.subplot(312)
# plt.plot([fromTimestamp(y.timestamp) for y in mags], [y.data.y for y in mags])
# plt.subplot(313)
# plt.plot([fromTimestamp(y.timestamp) for y in mags], [y.data.z for y in mags])

# plt.figure(6)
# plt.subplot(311)
# plt.title('mag with count')
# plt.plot([k for k,y in enumerate(mags)], [y.data.x for y in mags])
# plt.subplot(312)
# plt.plot([k for k,y in enumerate(mags)], [y.data.y for y in mags])
# plt.subplot(313)
# plt.plot([k for k,y in enumerate(mags)], [y.data.z for y in mags])


# plt.figure(7)
# plt.subplot(311)
# plt.title('GPS velocity with count')
# plt.plot([k for k,y in enumerate(gpss)], [y.velocity.data.x for y in gpss])
# plt.subplot(312)
# plt.plot([k for k,y in enumerate(gpss)], [y.velocity.data.y for y in gpss])
# plt.subplot(313)
# plt.plot([k for k,y in enumerate(gpss)], [y.velocity.data.z for y in gpss])

# plt.figure(8)
# plt.subplot(311)
# plt.title('GPS velocity with timestamp')
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.velocity.data.x for y in gpss])
# plt.subplot(312)
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.velocity.data.y for y in gpss])
# plt.subplot(313)
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.velocity.data.z for y in gpss])


# plt.figure(9)
# plt.subplot(311)
# plt.title('GPS position with count')
# plt.plot([k for k,y in enumerate(gpss)], [y.position.data.x for y in gpss])
# plt.subplot(312)
# plt.plot([k for k,y in enumerate(gpss)], [y.position.data.y for y in gpss])
# plt.subplot(313)
# plt.plot([k for k,y in enumerate(gpss)], [y.position.data.z for y in gpss])

# plt.figure(10)
# plt.subplot(311)
# plt.title('GPS position with timestamp')
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.position.data.x for y in gpss])
# plt.subplot(312)
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.position.data.y for y in gpss])
# plt.subplot(313)
# plt.plot([fromTimestamp(y.timestamp) for y in gpss], [y.position.data.z for y in gpss])

# plt.show()
