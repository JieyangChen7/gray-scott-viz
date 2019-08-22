#! /usr/bin/python
import pandas as pd
import numpy as np
import altair as alt
import csv

def load_data(device, complex_type):
    print('processing {}-{}'.format(device, complex_type))
    dfs = []


    
    
    org_compress_time = 0.0
    org_write_io_time = 0.0
    org_tranfer_time = 0.0
    org_read_io_time = 0.0
    org_decompress_time = 0.0
    #org_io_time = 0.0
    org_mc_time = 0.0
    org_rd_time = 0.0


    filename = "nocompressed_-1_0_0_0_gs_64_" + complex_type + "_" + device + ".csv"
    f = open(filename)
    r = csv.reader(f, delimiter=',')
    i = 0
    for row in r:
        #org_io_time += float(row[1])
        org_mc_time += float(row[2])
        org_rd_time += float(row[3])
        i += 1
    #org_io_time /= i
    org_mc_time /= i
    org_rd_time /= i

    filename = "profile_nocompressed_-1_0_0_0_gs_64.csv"
    f = open(filename)
    r = csv.reader(f, delimiter=',')
    tmp = []
    for row in r:
        tmp.append(float(row[0]))
    org_write_io_time = tmp[0]
    org_read_io_time = tmp[1]
    
    




    
    for compressor in [1, 2, 3]:
        data = []
        data.append([
                    #org_compress_time, 
                    #org_write_io_time, 
                    #org_tranfer_time, 
                    org_read_io_time,
                    org_decompress_time,
                    org_mc_time, 
                    org_rd_time
                    ])
        for tol in ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1']:
            # load rendering results
            filename = "decompressed_-1_0_"+str(compressor)+"_"+tol+"_gs_64_" + complex_type + "_" + device + ".csv"
            f = open(filename)
            r = csv.reader(f, delimiter=',')
            
            compress_time = 0.0
            write_io_time = 0.0
            tranfer_time = 0.0
            read_io_time = 0.0
            decompress_time = 0.0
            #io_time = 0.0
            mc_time = 0.0
            rd_time = 0.0
            i = 0
            for row in r:
                #io_time += float(row[1])
                mc_time += float(row[2])
                rd_time += float(row[3])
                i += 1
            #io_time /= i
            mc_time /= i
            rd_time /= i

            # load compression results
            filename = "compressed_-1_0_"+str(compressor)+"_"+tol+"_gs_64.csv"
            f = open(filename)
            r = csv.reader(f, delimiter=',')
            n = 0
            tmp = []
            for row in r:
                tmp.append(float(row[0]))
                n += 1
            for i in range(n-1):
                compress_time += tmp[i]
            write_io_time = tmp[n-1]-compress_time

            # load compression results
            filename = "decompressed_-1_0_"+str(compressor)+"_"+tol+"_gs_64.csv"
            f = open(filename)
            r = csv.reader(f, delimiter=',')
            n = 0
            tmp = []
            for row in r:
                tmp.append(float(row[0]))
                n += 1
            for i in range(n-1):
                decompress_time += tmp[i]
            read_io_time = tmp[n-1]-decompress_time



            data.append([
                        #compress_time, 
                        #write_io_time, 
                        #tranfer_time, 
                        read_io_time,
                        decompress_time,
                        org_mc_time, 
                        org_rd_time
                        ])
        df = pd.DataFrame(data, 
                          index = ['No Compression', '0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1'],
                          columns = [
                                    #'1. Compression time', 
                                    #'2. Write compressed data time',
                                    #'3. Network transfer time',
                                    '1. Read compressed data time',
                                    '2. Decompression time',
                                    '3. Marching Cubes time', 
                                    '4. Rendering time'
                                    ])
        #print df        
        df = df.stack().reset_index()
        df.columns = ['Error Tolerance', 'Time Breakdown', 'values']
        #print df
        dfs.append(df)
    dfs[0]['Compressor'] = 'MGARD'
    dfs[1]['Compressor'] = 'SZ'
    dfs[2]['Compressor'] = 'ZFP'

    final_df = pd.concat([dfs[0], dfs[1], dfs[2]])
    #print final_df
    return final_df
    

def plot(device, complex_type):

    final_df = load_data(device, complex_type)
    #print final_df


#f1=pd.DataFrame(10*np.random.rand(4,3),index=["A","B","C","D"],columns=["I","J","K"])
#df2=pd.DataFrame(10*np.random.rand(4,3),index=["A","B","C","D"],columns=["I","J","K"])
#df3=pd.DataFrame(10*np.random.rand(4,3),index=["A","B","C","D"],columns=["I","J","K"])

#def prep_df(df, name):
#    df = df.stack().reset_index()
#    df.columns = ['c1', 'c2', 'values']
#    df['DF'] = name
#    return df

#df1 = prep_df(df1, 'DF1')
#df2 = prep_df(df2, 'DF2')
#df3 = prep_df(df3, 'DF3')

#df = pd.concat([df1, df2, df3])
#print df

    chart = alt.Chart(final_df).mark_bar().encode(

        # tell Altair which field to group columns on
        x='Error Tolerance:N',
        # tell Altair which field to use as Y values and how to calculate
        y=alt.Y('sum(values):Q', scale=alt.Scale(domain=(0, 12.0)), axis=alt.Axis(title='Time (seconds)')),

        # tell Altair which field to use to use as the set of columns to be  represented in each group
        column='Compressor:N',

        # tell Altair which field to use for color segmentation 
        color=alt.Color('Time Breakdown:N',
                #scale=alt.Scale(
                    # make it look pretty with an enjoyable color pallet
                #    range=['#96ceb4', '#ffcc5c','#ff6f69'],
                #),
            ))
    chart = chart.properties(background='white')
        
        #.configure_facet_cell(
        ## remove grid lines around column clusters
        #    strokeWidth=0.0)
    chart.save('chart-{}-{}.png'.format(device, complex_type), scale_factor=2.0)

#plot('serial', 'simple')
#plot('openmp', 'simple')
#plot('cuda', 'simple')

#plot('serial', 'complex')
#plot('openmp', 'complex')
#plot('cuda', 'complex')

plot('serial', 'all')
plot('openmp', 'all')
plot('cuda', 'all')