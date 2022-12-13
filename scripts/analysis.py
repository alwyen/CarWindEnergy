import os
import argparse
from pydoc import apropos
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import math

'''
TODO:   need to somehow match/synchronize the timing of the
        rocket logger to the (approximate) GPS speed data
'''

def plot_data(rocketlogger_csv):
    rlog_df = pd.read_csv(rocketlogger_csv)
    time_list = rlog_df['Time']
    voltage_arr = np.array(rlog_df['Voltage'])*-1
    current_arr = np.array(rlog_df['Current'])*-1
    # for i in range(len(time_list)):
    #     if not math.isnan(time_list[i]):
    #         print(f'Index: {i}; Time: {time_list[i]}')
    plt.subplot(3,1,1)
    plt.plot(voltage_arr)
    plt.title('Voltage (V)')
    plt.ylabel('V')
    plt.xlabel('Sample Number')

    plt.subplot(3,1,2)
    plt.plot(current_arr)
    plt.title('Current (I)')
    plt.ylabel('I')
    plt.xlabel('Sample Number')

    plt.subplot(3,1,3)
    plt.plot(voltage_arr*current_arr)
    plt.title('Power (W)')
    plt.ylabel('W')
    plt.xlabel('Sample Number')

    plt.tight_layout()
    plt.show()

    print(f'Voltage Max: {np.max(voltage_arr)}')
    print(f'Current Max: {np.max(current_arr)}')
    print(f'Power Max: {np.max(voltage_arr*current_arr)}')
        
def main(args):
    if args.file:
        if os.path.isfile(args.file):
            plot_data(args.file)

if __name__ == '__main__':
    scripts_dir = os.getcwd()
    data_dir = os.getcwd() + os.sep + '..' + os.sep + 'data'
    os.chdir(data_dir)
    data_dir = os.getcwd()

    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file', type=str, help='file path for csv file; just type in the file name')
    args = parser.parse_args()
    main(args)