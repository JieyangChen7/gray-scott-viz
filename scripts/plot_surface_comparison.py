
import csv
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as ticker
import math

def plot(complex_type):
	filename = "gs_64_0_0_0_0_" + complex_type + ".csv"
	f = open(filename)
	r = csv.reader(f, delimiter=',')
	org_avg = 0.0
	org = []
	ts = 0
	for row in r:
		org.append(float(row[1]))
		org_avg += float(row[1])
		ts += 1
	org_avg /= ts


	data = []
	data_min = []
	data_max = []
	data_sd = []
	for COMPRESSOR in [1, 2, 3]:
		data.append([])
		data_min.append([])
		data_max.append([])
		data_sd.append([])
		for TOL in ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1']:
			#data[-1].append([])
			filename = "decompressed_gs_64_" + str(COMPRESSOR) + "_2_" + TOL + "_" + TOL + "_" + complex_type + ".csv"
			f = open(filename)
			r = csv.reader(f, delimiter=',')
			avg = 0.0
			ts = 0
			minv = float("inf")
			maxv = 0.0

			for row in r:
				error = abs(float(row[1])-org[ts])/abs(org[ts])
				if (minv > error): 
					minv = error
				if (maxv < error):
					maxv = error
				avg += error
				ts += 1
			avg /= ts
			data[-1].append(avg)
			data_min[-1].append(minv)
			data_max[-1].append(maxv)

			tmp = 0.0
			for row in r:
				error = abs(float(row[1])-org[ts])/abs(org[ts])
				tmp += (error - avg) * (error - avg)
			sd = math.sqrt(tmp/ts)
			data_sd[-1].append(sd)


	x = ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1']
	x_pos = np.arange(len(x))
	#print x_pos
	print data[0]
	fig, ax = plt.subplots()

	#ax.errorbar(x_pos, data[0], yerr= data_sd, fmt='o')
	ax.plot(x_pos, data[0], 'r--', label='MGARD')
	ax.plot(x_pos, data[1], 'b--', label='SZ')
	ax.plot(x_pos, data[2], 'g--', label='ZFP')
	ax.legend(loc='upper left')
	ax.yaxis.set_major_formatter(ticker.PercentFormatter(xmax=5))

	#bar_width = 0.3
	#plt.bar(x_pos, data[0], bar_width, color = 'b', label='MGARD')
	#plt.bar(x_pos+bar_width, data[1], bar_width, color = 'g', label='SZ')
	#plt.bar(x_pos+bar_width+bar_width, data[2], bar_width, color = 'r', label='ZFP')

	

	plt.xticks(x_pos, x)
	plt.xlabel('Tolerance')
	plt.ylabel('Relative Surface Area Difference')


	plt.title('Complex case')
	plt.show()

#plot('simple')

#plot('medium')

plot('complex')
		