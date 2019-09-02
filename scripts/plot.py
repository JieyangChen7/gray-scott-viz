#! /usr/bin/python
import pandas as pd
import csv
import math
import altair as alt
import os

compressor_indices = [1, 2, 3] #, 4, 5]
compressor_names = ['MGARD', 'SZ-ABS', 'SZ-REL', 'SZ-PWR', 'ZFP']
eb_values = ['0.000001', '0.00001', '0.0001', '0.001', '0.01', '0.1']
iso_values = [0.1, 0.2, 0.3, 0.4, 0.5]
size = 64
gap = 10
compressor_u=-1

def load_original_surface_data():
    df = pd.DataFrame()
    for iso_value in iso_values:
        filename = 'nocompressed_{}_0_{}_0_gs_{}_iso_{}.csv'.format(compressor_u, 0, size, iso_value)
        prefix='./surface_area_results/'
        filename=prefix+filename

        tmp_df = pd.DataFrame()

        f = open(filename)
        r = csv.reader(f, delimiter=',')

        #iso_value_idx  = []
        timestep_idx = []
        surface      = []

        for row in r:
            #iso_value_idx.append(iso_value)
            timestep_idx.append(int(row[0]) * gap)
            surface.append(float(row[1]))


        
        
        tmp_df['Time Step']    = timestep_idx
        tmp_df['Surface Area'] = surface
        tmp_df.insert(0, 'Iso-value', iso_value)
        #print tmp_df
        df = pd.concat([df, tmp_df])
    return df


def load_compressed_surface_data():

    df = pd.DataFrame()
    for compressor_v in compressor_indices:
        for eb in eb_values:
            for iso_value in iso_values:
                tmp_df = pd.DataFrame()
 
                filename = 'decompressed_{}_{}_{}_{}_gs_{}_iso_{}.csv'.format(compressor_u, 0, compressor_v, eb, size, iso_value)
                
                prefix='./surface_area_results/'
                filename=prefix+filename

                f = open(filename)
                r = csv.reader(f, delimiter=',')
                
                compressor_idx = []
                eb_idx         = []
                iso_value_idx  = []
                timestep_idx   = []
                surface        = []
                for row in r:
                    compressor_idx.append(compressor_names[compressor_v-1])
                    eb_idx.append(eb)
                    iso_value_idx.append(iso_value)
                    timestep_idx.append(int(row[0]) * gap)
                    surface.append(float(row[1]))

                tmp_df['Compressor']   = compressor_idx
                tmp_df['Error Bound']  = eb_idx
                tmp_df['Iso-value']    = iso_value_idx
                tmp_df['Time Step']    = timestep_idx
                tmp_df['Surface Area'] = surface
                df = pd.concat([df, tmp_df])
                #df.set_index('Timestep', inplace=True)
    return df

def get_surface_diff_data():
    org_df = load_original_surface_data()
    
    for compressor_v in compressor_indices:
        for eb in eb_values:
            tmp_df = pd.DataFrame()
            


    for compressor in compressor_indices:
        tmp = []
        for tol in eb_values:
            df = load_surface_data(size, -1, 0, compressor, tol, iso_values)
            df = df - org_df
            df = df.abs()



def get_size(start_path = '.'):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(start_path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            # skip if it is symbolic link
            if not os.path.islink(fp):
                total_size += os.path.getsize(fp)
    return total_size

def load_compression_ratio(size, compressor_u, tol_u, compressor_v, tol_v):
    prefix = './gs_data/'
    # filename = prefix + 'nocompressed_-1_0_0_0_gs_64.bp.dir'
    # org_size = get_size(start_path=filename)
    filename = 'compressed_{}_{}_{}_{}_gs_{}.csv'.format(compressor_u, tol_u, compressor_v, tol_v, size)
    filename=prefix+filename
    f = open(filename)
    r = csv.reader(f, delimiter=',')
    df = pd.DataFrame()
    timestep = []
    cr = []
    for row in r:
        timestep.append(int(row[0]))
        cr.append(float(row[2]))
    df['Timestep'] = timestep
    df['Compression Ratio'] = cr
    df.set_index('Timestep', inplace=True)
    return df

def load_psnr_data(size, compressor_u, tol_u, compressor_v, tol_v):
    prefix = './gs_data/'
    filename = prefix + 'analysis_{}_{}_{}_{}_gs_{}.csv'.format(compressor_u, tol_u, compressor_v, tol_v, size)
    f = open(filename)
    r = csv.reader(f, delimiter=',')
    timestep = []
    psnr = []
    df = pd.DataFrame()
    for row in r:
        timestep.append(int(row[0]))
        psnr.append(float(row[1]))
    df['Time Step'] = timestep
    df['PSNR'] = psnr
    return df


def plot_psnr_ts():
    size=64
    for compressor in compressor_indices:
        df = pd.DataFrame()
        tmp = []
        for tol in eb_values:
            tdf = load_psnr_data(64, -1, 0, compressor, tol)
            df['Time Step'] = tdf['Time Step']
            df[tol] = tdf['PSNR']

        df.set_index('Time Step', inplace=True)
        df=df.stack().reset_index()
        df.columns = ['Time Step', 'Error Bound', "PSNR"]
        print df
        chart = alt.Chart(df).mark_line(clip=True).encode(
                x='Time Step',
                y=alt.Y('PSNR', scale=alt.Scale(domain=(0, 160.0))),
                color='Error Bound'
        )
        chart = chart.properties(background='white')
        if compressor == 1:
            chart.save('psnr-mgard.png', scale_factor=2.0)
        if compressor == 2:
            chart.save('psnr-sz-abs.png', scale_factor=2.0)
        if compressor == 3:
            chart.save('psnr-sz-rel.png', scale_factor=2.0)
        if compressor == 4:
            chart.save('psnr-sz-pwr.png', scale_factor=2.0)
        if compressor == 3:
            chart.save('psnr-zfp.png', scale_factor=2.0)

def plot_psnr_cr():
    cr_df = get_compression_ratio()
    cr_df = cr_df.stack().reset_index()
    cr_df.columns = ['Compressor', 'Error Bound', 'Compression Ratio']


    df = pd.DataFrame()
    data = []
    for compressor in compressor_indices:
        tmp = []
        for tol in eb_values:
            tdf = load_psnr_data(64, -1, 0, compressor, tol)
            tdf=tdf.mean(axis = 0)
            avg_psnr = tdf['PSNR']
            tmp.append(avg_psnr)
        data.append(tmp)
    df = pd.DataFrame(data,
                      index = compressor_names,
                      columns = eb_values)
    df = df.stack().reset_index()
    df['Compression Ratio'] = cr_df['Compression Ratio']
    df.columns = ["Compressor", "Error Bound", "PSNR", "Compression Ratio"]
    print df
    chart = alt.Chart(df).mark_circle(size=60).encode(
                x='PSNR',
                y='Compression Ratio',
                color='Compressor')
    chart = chart.properties(background='white')
    chart.save('psnr-cr.png', scale_factor=2.0)

def plot_psnr_eb():
    cr_df = get_compression_ratio()
    cr_df = cr_df.stack().reset_index()
    cr_df.columns = ['Compressor', 'Error Bound', 'Compression Ratio']


    df = pd.DataFrame()
    data = []
    for compressor in compressor_indices:
        tmp = []
        for tol in eb_values:
            tdf = load_psnr_data(64, -1, 0, compressor, tol)
            tdf=tdf.mean(axis = 0)
            avg_psnr = tdf['PSNR']
            tmp.append(avg_psnr)
        data.append(tmp)
    df = pd.DataFrame(data,
                      index = compressor_names,
                      columns = eb_values)
    df = df.stack().reset_index()
    df['Compression Ratio'] = cr_df['Compression Ratio']
    df.columns = ["Compressor", "Error Bound", "PSNR", "Compression Ratio"]
    print df
    chart = alt.Chart(df).mark_line().encode(
                x='Error Bound',
                y='PSNR',
                color='Compressor'
                )
    chart = chart.properties(background='white')
    chart.save('psnr-eb.png', scale_factor=2.0)


def plot_surface_ts():
    size = 64
    compressor_u=-1
    tol_u=0
    compressor_v=0
    tol_v=0

    df=load_surface_data(size, compressor_u, tol_u, compressor_v, tol_v, iso_values)
    df=df.stack().reset_index()
    df.columns = ['Time Step', 'Iso Value', "Surface Area"]
    print df

    chart = alt.Chart(df).mark_line().encode(
    x='Time Step',
    y='Surface Area',
    color='Iso Value'
    )

    chart = chart.properties(background='white')
    chart.save('chart.png', scale_factor=2.0)

df = load_original_surface_data()
print df
#plot_surface_ts()
#get_compression_ratio()
#plot_psnr_ts()
#plot_psnr_cr()
#plot_psnr_eb()
#get_surface_diff_data()