import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def read_collision_data_complete(filename):
    
    with open(filename, 'r') as file:
        lines = file.readlines()

    sections = {}
    data_rows = []
    similarity_values = []
    snp_rate = []

    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        if line.startswith(':1:'):
            sections['test_name'] = line[3:].strip()
        elif line.startswith(':2:'):
            sections['column_headers'] = line[3:].strip().split(',')
        elif line.startswith(':3:'):
            line_content = line[3:].strip()
            line_parts = line_content.split(',')
            sections['Hashname'] = line_parts[0].strip()
            sections['SequenceLength'] = int(line_parts[1].strip())
            sections['TokenLength'] = int(line_parts[2].strip())
            sections['Distance_Metric'] = int(line_parts[3].strip())
        elif line.startswith(':3.1:'):
            line_content = line[5:].strip()
            line_parts = line_content.split(',')
            sections['SubseqHash_k'] = line_parts[0].strip()
            sections['SubseqHash_d'] = int(line_parts[1].strip())
        elif line.startswith(':4:'):
            line_content = line[3:].strip()
            similarity_values = [float(x.strip()) for x in line_content.split(',')]
            sections['similarity_values'] = similarity_values
        elif line.startswith(':5:'):
            line_content = line[3:].strip()
            snp_rate = [float(x.strip()) for x in line_content.split(',')]
            sections['snp_rate'] = snp_rate
        elif line.startswith(':6:'):
            line_content = line[3:].strip()
            collision_rates = [float(x.strip()) for x in line_content.split(',')]
            sections['collision_rates'] = collision_rates
            
            # Create data row
            row_data = {
                'test_name': sections.get('test_name', ''),
                'hashname': sections.get('Hashname', ''),
                'sequencelength': sections.get('SequenceLength', 0),
                'tokenlength': sections.get('TokenLength', 0),
                'similarity_values': similarity_values,
                'snp_rate': snp_rate,
                'collision_rates': collision_rates,
                'SubseqHash_k': sections.get('SubseqHash_k', 0),
                'SubseqHash_d': sections.get('SubseqHash_d', 0)

            }
            data_rows.append(row_data)
    
    return sections, pd.DataFrame(data_rows)




sections,df =  read_collision_data_complete("/home/dynamics/bikram/BioHasher/results/collisionResults_SubseqHash-64.csv")


