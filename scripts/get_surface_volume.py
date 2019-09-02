import sys
import csv
import os

input_data = sys.argv[1]
mode = sys.argv[2]
output_csv = sys.argv[3]
ts_start = int(sys.argv[4])
ts_end = int(sys.argv[5])
iso_value = float(sys.argv[6])


print "measuring: " + mode
print "iso value: " + str(iso_value)
print "output filename: " + output_csv
if os.path.exists(output_csv):
    os.remove(output_csv)
f = open(output_csv, 'w+')
writer = csv.writer(f)

OpenDatabase(input_data)
AddPlot("Pseudocolor", "V")

if (mode == "surface"):
    AddOperator("Isosurface")
    iso_attr = IsosurfaceAttributes()
    iso_attr.contourNLevels=1
    iso_attr.min=iso_value
    iso_attr.max=iso_value
    iso_attr.minFlag=1
    iso_attr.maxFlag=1
    iso_attr.contourValue=iso_value
    #iso_attr.contourMethod='Level'
    #iso_attr.scaling='Linear'
    print iso_attr
    SetOperatorOptions(iso_attr)

if (mode == "volume"):
    AddOperator("Isovolume")
    iso_attr = IsovolumeAttributes()
    iso_attr.lbound = 0.18
    iso_attr.ubound = 0.2
    SetOperatorOptions(iso_attr)


print "Active time slider: %s" % GetActiveTimeSlider()
i = 0
for states in range(TimeSliderGetNStates()):
    SetTimeSliderState(states)
    if (i >= ts_start and i < ts_end):
    	DrawPlots()
    	if (mode == "surface"):
        	Query("3D surface area")
    	if (mode == "volume_simple"):
        	Query("Volume")
    	v = GetQueryOutputValue()
        if v == ():
            v = 0.0
    	writer.writerow([i, v])
    	f.flush()
    i += 1 
f.close()

exit()

