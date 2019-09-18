#! /usr/bin/python3
import adios2
import numpy as np
import sys
import csv
import math

#print(len(sys.argv))
bp_file     = sys.argv[1]
output_csv  = sys.argv[2]
store_U     = int(sys.argv[3])
store_V     = int(sys.argv[4])
num_of_bins = int(sys.argv[5])
start_iter  = int(sys.argv[6])
end_iter    = int(sys.argv[7])

low  = 0
high = 1

bin_size = (high - low) / num_of_bins

f = open(output_csv, 'w+')
writer = csv.writer(f)
fh = adios2.open(bp_file, "r")
for fstep in fh:

    csv_row = []

    step = fstep.current_step()
    csv_row.append(step)
    #print(step)
    if (step >= start_iter and step < end_iter):
    
     #   step_vars = fstep.available_variables()
     #   for name, info in step_vars.items():
     #        print("variable_name: " + name)
     #        for key, value in info.items():
     #           print("\t" + key + ": " + value)
     #        print("\n")

        data = []

        if (store_U > -1):
            data = fstep.read("U")
        if (store_V > -1):
            data = fstep.read("V")

        #print(data)
        counter = []
        for i in range(num_of_bins):
            counter.append(0)

        t = int(data.shape[0]/2)
        for i in range(t, t+1):
            for j in range(data.shape[1]):
                for k in range(data.shape[2]):
                    #print(str(i) + " " + str(j) + " " + str(k))
                    bin_id = int((data[i][j][k] - low)/bin_size)              
                    if bin_id >= 0 and bin_id < num_of_bins:
                        counter[bin_id] += 1
        
        entropy = 0.0
        #total_elem = data.shape[0] * data.shape[1] * data.shape[2]
        total_elem = 1 * data.shape[1] * data.shape[2]
        for i in range(num_of_bins):
            p = float(counter[i]/total_elem)
            if p > 0:
                entropy += -1 * p * math.log(p)
        csv_row.append(entropy)
    
        writer.writerow(csv_row)



