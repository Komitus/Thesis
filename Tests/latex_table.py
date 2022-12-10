class LatexTable:
    def __init__(self):
        self.final_str=''

    def begin(self, num_of_columns):
        self.final_str='''
        \\begin{table}[H] 
        \\begin{center}
            \\begin{tabular}{'''

        tmp_str  = ''
        for _ in range(num_of_columns):
            tmp_str += '|p{3cm}' 

        self.final_str += tmp_str + '| } \\hline\n'
    
    def end(self):
        self.final_str += '''
            \\hline
        	\end{tabular}
            \caption{}
            \end{center}
            \end{table}
        '''

    def add_row(self, row : list[str]):
        tmp_str = ''

        for item in row[:-1]:
            tmp_str += (str(item) + ' & ')

        tmp_str += (str(row[len(row)-1]) + '\\\\ \n')

        self.final_str += tmp_str
        