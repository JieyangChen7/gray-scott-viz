import sys
import csv
import os

input_data = sys.argv[1]
mode = sys.argv[2]
output_csv = sys.argv[3]
ts_start = int(sys.argv[4])
ts_end = int(sys.argv[5])


print "measuring: " + mode
print "output filename: " + output_csv
if os.path.exists(output_csv):
    os.remove(output_csv)
f = open(output_csv, 'w+')
writer = csv.writer(f)

OpenDatabase(input_data)
AddPlot("Pseudocolor", "V")

if (mode == "surface_simple"):
    AddOperator("Isosurface")
    iso_attr = IsosurfaceAttributes()
    iso_attr.contourNLevels=1
    #iso_attr.min=0.18
    #iso_attr.max=0.18
    SetOperatorOptions(iso_attr)

if (mode == "surface_complex"):
    AddOperator("Isosurface")
    iso_attr = IsosurfaceAttributes()
    iso_attr.contourNLevels=10
    SetOperatorOptions(iso_attr)

if (mode == "volume_simple"):
    AddOperator("Isovolume")
    iso_attr = IsovolumeAttributes()
    iso_attr.lbound = 0.18
    iso_attr.ubound = 0.2
    SetOperatorOptions(iso_attr)

if (mode == "volume_complex"):
    AddOperator("Isovolume")
    iso_attr = IsovolumeAttributes()
    SetOperatorOptions(iso_attr)



#Query("Volume")
#print "The float value is: %g" % GetQueryOutputValue()

print "Active time slider: %s" % GetActiveTimeSlider()
i = 0
for states in range(TimeSliderGetNStates()):
    SetTimeSliderState(states)
    if (i >= ts_start and i < ts_end):
    	DrawPlots()
    	if (mode == "surface_simple" or mode == "surface_complex"):
        	Query("3D surface area")
    	if (mode == "volume_simple" or mode == "volume_complex"):
        	Query("Volume")
    	v = GetQueryOutputValue()
    	writer.writerow([i, v])
    	f.flush()
    i += 1 
f.close()

exit()

