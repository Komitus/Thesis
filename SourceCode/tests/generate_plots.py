import matplotlib.pyplot as plt
import os
import math
import re
import numpy as np
import pandas as pd
from latex_table import *

NUM_OF_TESTS = 9
resultsDIR = "./output_tmp/"
plotsDIR = "./plots/"
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
algosNames = ['APPROX', 'SM', 'MIP']
stats = ['res', 'time']
tests_idxs = [i for i in range(NUM_OF_TESTS)]
name_prefix = resultsDIR+"test"

LOWER_BOUND_regx = re.compile(r'(?<=LOWER_STOCK_NUM_BOUND: )(\d+)')
NUM_OF_ALL_OBJS_regx = re.compile(r'(?<=NUM_OF_ALL_OBJS: )(\d+)')
STOCKS_NEEDED_regx = re.compile(r'(?<=STOCKS_NEEDED: )(\d+)')
NUM_OF_COLS_regx = re.compile(r'(?<=NUMBER_OF_COLUMNS_\(CONFIGS\): )(\d+)')
TIME_regx = re.compile(r'\s(\d+)m(\d+,\d+)s')
STOCK_regx = re.compile(r'(?<=STOCK_LENGTH: )(\d+)')
TYPES_regx = re.compile(r'(?<=NUM_OF_TYPES: )(\d+)')


stocks_for_algos = [[0 for _ in tests_idxs] for _ in algosNames]
time_for_algos = [[0 for _ in tests_idxs] for _ in algosNames]
lower_bound = [0 for _ in tests_idxs]
num_of_cols = [0 for _ in tests_idxs]
num_of_all_objs = [0 for _ in tests_idxs]
stocks_lengths = [0 for _ in tests_idxs]
num_of_types = [0 for _ in tests_idxs]

def get_filename(test_idx, algo_name, stat):
    return name_prefix+str(test_idx)+'_'+algo_name+'_'+stat+'.txt'


def prepare_data():
    for stat in stats:
        if stat == 'res':
            for test_idx in tests_idxs:
                filename = get_filename(test_idx, 'SM', stat)
                with open(filename, 'r') as file:
                    text = file.read()
                    num_of_types[test_idx] = int(TYPES_regx.search(text).group(0))
                    stocks_lengths[test_idx] = int(STOCK_regx.search(text).group(0))
                    lower_bound[test_idx] = int(LOWER_BOUND_regx.search(
                        text).group(0))
                    num_of_cols[test_idx] = int(NUM_OF_COLS_regx.search(
                        text).group(0))
                    num_of_all_objs[test_idx] = int(NUM_OF_ALL_OBJS_regx.search(
                        text).group(0))

                for algo_idx, algo_name in enumerate(algosNames):
                    filename = get_filename(test_idx, algo_name, stat)
                    with open(filename, 'r') as file:
                        text = file.read()
                        stocks_for_algos[algo_idx][test_idx] = int(STOCKS_NEEDED_regx.search(
                            text).group(0))

        elif stat == 'time':
            for test_idx in tests_idxs:
                for algo_idx, algo_name in enumerate(algosNames):
                    filename = get_filename(test_idx, algo_name, stat)
                    with open(filename, 'r') as file:
                        text = file.read()
                        #print(filename)
                        mins, secs = TIME_regx.search(text).groups()
                        time_for_algos[algo_idx][test_idx] = float(
                           mins) * 60 + float(secs.replace(",", "."))
                        #print(time_for_algos[algo_idx][test_idx])


def make_basic_plots():

    # set width of bar
    barWidth = 0.15
    # Set position of bar on X axis
    br = [[] for _ in range(len(algosNames)+1)]
    br[0] = np.arange(NUM_OF_TESTS)
    for i in range(1, len(algosNames)+1):
        br[i] = [x + barWidth for x in br[i-1]]

    y_label_str = ''
    for stat in stats:
        fig, ax = plt.subplots()
        if stat == 'res':
            y_label_str = 'Liczba belek'
            y_arr = stocks_for_algos
            plt.bar(br[len(br)-1], lower_bound, color=colors[len(colors)-1], width=barWidth,
                     label='dolna granica')

        elif stat == 'time':
            y_label_str = 'Czas wykonania programu (s)'
            y_arr = time_for_algos
        # Make the plot
        for algo_idx, algo_name in enumerate(algosNames):
            plt.bar(br[algo_idx], y_arr[algo_idx], color=colors[algo_idx], width=barWidth,
                label=algo_name)

        # Adding Xticks
        plt.xlabel('Numer testu', fontsize=15)
        plt.ylabel(y_label_str, fontsize=15)
        plt.xticks([r + barWidth for r in range(len(br[0]))], tests_idxs)
        if stat == 'res':
            plt.title('Liczba belek w rozwiązaniu dla poszczególnych algorytmów \n\
                (i ich dolna granica) w zależności od numeru testu')
        else:
            plt.title('Czas wykonania programu dla poszczególnych algorytmów \n\
                w zależności od numeru testu')
        plt.legend()

        png_name = plotsDIR+stat+'.png'
        fig.savefig(png_name, bbox_inches="tight", dpi=1000)
        plt.close()


def make_plots_from_configs():
    L = [ (num_of_cols[i],i) for i in range(len(num_of_cols)) ]
    L.sort()
    n,permutation = zip(*L)

    new_lower_bound = [lower_bound[permutation[i]] for i in range(len(lower_bound))]

    new_stocks_for_algos = [[algo_test[permutation[i]] \
        for i in range(len(algo_test))] for algo_test in stocks_for_algos]

    new_time_for_algos = [[algo_test[permutation[i]] \
        for i in range(len(algo_test))] for algo_test in time_for_algos]

    for stat in stats:
        
        fig, ax = plt.subplots()
        
        if stat == 'res':
            title = 'Liczba belek w rozwiązaniu dla poszczególnych algorytmów \n\
                (i ich dolna granica) w zależności od liczby wygenerowanych konfiguracji'
            y_label_str = 'Liczba belek'
            y_arr = new_stocks_for_algos
            ax.plot(n, new_lower_bound, color=colors[len(colors)-1], label='dolna granica')

        elif stat == 'time':
            title = 'Czas wykonania programu dla poszczególnych algorytmów \n\
                w zależności od liczby wygenerowanych konfiguracji'
            y_label_str = 'Czas wykonania programu (s)'
            y_arr = new_time_for_algos

        # Make the plot
        for algo_idx, algo_name in enumerate(algosNames):
            ax.plot(n, y_arr[algo_idx], color=colors[algo_idx], label=algo_name)
	
        ax.set_title(title)
        ax.set_xlabel('Liczba wygenerowanych konfiguracji')
        ax.set_ylabel(y_label_str)
        ax.legend()

        png_name = plotsDIR+stat+'_configs.png'
        fig.savefig(png_name, bbox_inches="tight", dpi=1000)
        plt.close()

def make_tables():
    configs_table = LatexTable()
    cols = ['Numer testu', 'Liczba wygenerowanych konfiguracji', 'Długość belki', 'Liczba typów elementów', 'Liczba wszystkich elementów ($n$)']
    configs_table.begin(len(cols))
    configs_table.add_row(cols)

    for i, item in enumerate(num_of_cols):
        configs_table.add_row([i, item, stocks_lengths[i], num_of_types[i], num_of_all_objs[i]])
    configs_table.end()

    print(configs_table.final_str, '\n\n')

    time_table = LatexTable()
    cols = ['Numer testu', *algosNames]
    time_table.begin(len(cols))
    time_table.add_row(cols)
    
    for i in range(len(time_for_algos[0])):
        row = [i]
        for time_single_alg in time_for_algos:
            tmp_str = f'{time_single_alg[i]:.3f}'
            row.append(tmp_str)
        time_table.add_row(row)
    time_table.end()

    print(time_table.final_str)

    stocks_table = LatexTable()
    cols = ['Numer testu', *algosNames, 'dolna granica']
    stocks_table.begin(len(cols))
    stocks_table.add_row(cols)
    
    for i in range(len(stocks_for_algos[0])):
        row = [i]
        for stocks_single_alg in stocks_for_algos:
            row.append(stocks_single_alg[i])
        row.append(lower_bound[i])
        stocks_table.add_row(row)
    stocks_table.end()

    print(stocks_table.final_str)

if __name__ == "__main__":
    prepare_data()
    make_basic_plots()
    make_plots_from_configs()
    make_tables()

