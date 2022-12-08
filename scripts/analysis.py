import os
import argparse
from pydoc import apropos
import pandas as pd
import math

def plot_data(rocketlogger_csv):
    rlog_df = pd.read_csv(rocketlogger_csv)
    time_list = rlog_df['Time']
    for i in range(len(time_list)):
        if not math.isnan(time_list[i]):
            print(f'Index: {i}; Time: {time_list[i]}')
        

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