import matplotlib.pyplot as plt
import re
import numpy as np
from latex_table import *

NUM_OF_TESTS = 10
resultsDIR = "./output/"
plotsDIR = "../PDF/plots/"
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
algosNames = ['APPROX', 'SM', 'MIP']
stats = ['res', 'time']
tests_idxs = [i for i in range(NUM_OF_TESTS)]
name_prefix = resultsDIR+"test"

params = [
    'STOCK_LENGTH',
    'NUM_OF_TYPES',
    'NUM_OF_ALL_OBJS',
    'LOWER_STOCK_NUM_BOUND',
]

def get_pattern_string(param_name):
    return f'{param_name}:\s(?P<{param_name}>\d+)\n'

params_dict = {param_name:[0 for _ in tests_idxs] for param_name in params}
params_patterns_dict = {param_name+'_pattern':get_pattern_string(param_name) for param_name in params}
combined_pattern = ''.join(val for (key, val) in params_patterns_dict.items())
combined_regx = re.compile(combined_pattern)

stocks_for_algos = [[0 for _ in tests_idxs] for _ in algosNames]
time_for_algos = [[0 for _ in tests_idxs] for _ in algosNames]
num_of_cols = [0 for _ in tests_idxs]
NUM_OF_COLS_regx = re.compile(r'(?<=NUMBER_OF_COLUMNS_\(CONFIGS\): )(\d+)')
USER_TIME_regx = re.compile(r'user\s+(\d+)m(\d+,\d+)s')
STOCKS_NEEDED_regx = re.compile(r'(?<=STOCKS_NEEDED: )(?P<STOCKS_NEEDED>\d+)')


def get_filename(test_idx, algo_name, stat):
    return name_prefix+str(test_idx)+'_'+algo_name+'_'+stat+'.txt'


def prepare_data():
    for stat in stats:
        if stat == 'res':
            for test_idx in tests_idxs:
                filename = get_filename(test_idx, 'SM', stat)
                with open(filename, 'r') as file:
                    text = file.read()
                    group_dict = combined_regx.search(text).groupdict()
                    for param_name in params:
                        params_dict[param_name][test_idx] = int(group_dict[param_name])

                    num_of_cols[test_idx] = int(NUM_OF_COLS_regx.search(text).group(0))

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
                        mins_usr, secs_usr = USER_TIME_regx.search(text).groups()
                        time_usr =  float(mins_usr) * 60 + float(secs_usr.replace(",", "."))
                        time_for_algos[algo_idx][test_idx] = time_usr
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
            plt.bar(br[len(br)-1], params_dict['LOWER_STOCK_NUM_BOUND'], color=colors[len(colors)-1], width=barWidth,
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

    new_lower_bound = [params_dict['LOWER_STOCK_NUM_BOUND'][permutation[i]] for i in range(len(params_dict['LOWER_STOCK_NUM_BOUND']))]

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
    cols = ['Numer testu', 'Długość belki', 'Liczba typów elementów', 'Liczba wszystkich elementów ($n$)', 'Liczba wygenerowanych konfiguracji']
    configs_table.begin(len(cols))
    configs_table.add_row(cols)

    for i, item in enumerate(num_of_cols):
        configs_table.add_row([i, params_dict['STOCK_LENGTH'][i],  params_dict['NUM_OF_TYPES'][i],  params_dict['NUM_OF_ALL_OBJS'][i], item])
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
    cols = ['Numer testu', *algosNames, 'Dolna granica']
    stocks_table.begin(len(cols))
    stocks_table.add_row(cols)
    
    for i in range(len(stocks_for_algos[0])):
        row = [i]
        for stocks_single_alg in stocks_for_algos:
            row.append(stocks_single_alg[i])
        row.append(params_dict['LOWER_STOCK_NUM_BOUND'][i])
        stocks_table.add_row(row)
    stocks_table.end()

    print(stocks_table.final_str)

if __name__ == "__main__":
    prepare_data()
    #make_basic_plots()
    #make_plots_from_configs()
    make_tables()

