#! /usr/bin/python
import os
import pandas as pd
import numpy as np
import altair as alt

def get_size(start_path = '.'):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(start_path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            # skip if it is symbolic link
            if not os.path.islink(fp):
                total_size += os.path.getsize(fp)
    return total_size

def plot_compression_ratio():
    org_size = get_size(start_path='nocompressed_-1_0_0_0_gs_64.bp.dir')
    data = []
    for compressor in [1, 2, 3]:
        tmp = []
        for tol in ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1']:
            path = "compressed_-1_0_"+str(compressor)+"_"+tol+"_gs_64.bp.dir"
            size = get_size(start_path=path)
            #print size
            tmp.append(float(org_size)/size)
        data.append(tmp)

    df = pd.DataFrame(data,
                      index = ['MAGRD', 'SZ', 'ZFP'],
                      columns = ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1'])
    print df

    df = df.stack().reset_index()
    df.columns = ['Compressor', 'Error Tolerance', 'Compression Ratio']
    chart = alt.Chart(df).mark_bar().encode(
                                        x='Error Tolerance:N',
                                        y='Compression Ratio:Q',
                                        color='Compressor:N',
                                        column='Compressor:N'
                                    )
    chart = chart.properties(background='white')
    chart.save('chart.png')
plot_compression_ratio()

