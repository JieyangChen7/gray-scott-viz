#! /usr/bin/python3
import adios2
import numpy as np
from findiff import FinDiff, coefficients, Coefficient
import sys
import csv

#print(len(sys.argv))
bp_file        = sys.argv[1]
output_csv     = sys.argv[2]
store_U        = int(sys.argv[3])
store_V        = int(sys.argv[4])
deriv_axis     = int(sys.argv[5])
order_of_deriv = int(sys.argv[6])
accuracy       = int(sys.argv[7])
store_deriv    = int(sys.argv[8])
store_lap      = int(sys.argv[9])
start_iter     = int(sys.argv[10])
end_iter       = int(sys.argv[11])

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

        if (store_U > -1):
            data = fstep.read("U")
            #print(data)
            dx = 1
            op = FinDiff(axis, dx, deriv, acc = accuracy)
            result = op(data)
            csv_row.append(result.sum())

            if (store_deriv == 1):
                fstep.write("U_deriv", result)

            op1 = FinDiff(0, dx, 2, acc = accuracy)
            op2 = FinDiff(1, dx, 2, acc = accuracy)
            op3 = FinDiff(2, dx, 2, acc = accuracy)

            result = op1(data) + op2(data) + op3(data)
            csv_row.append(result.sum())

            if (store_lap == 1):
                fstep.write("U_lap", result)

        if (store_V > -1):
            data = fstep.read("V")
            #print("Min: " + str(data.min()) + ", Max: " + str(data.max()))
            dx = 1
            op = FinDiff(deriv_axis, dx, order_of_deriv, acc = accuracy)
            result = op(data)
            csv_row.append(result.sum())

            if (store_deriv == 1):
                fstep.write("V_deriv", result)

            op1 = FinDiff(0, dx, 2, acc = accuracy)
            op2 = FinDiff(1, dx, 2, acc = accuracy)
            op3 = FinDiff(2, dx, 2, acc = accuracy)

            result = op1(data) + op2(data) + op3(data)
            csv_row.append(result.sum())

            if (store_lap == 1):
                fstep.write("V_lap", result)
    
        writer.writerow(csv_row)



