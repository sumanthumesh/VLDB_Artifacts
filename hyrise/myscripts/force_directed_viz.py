import pygraphviz as pv
import numpy as np
import sys
import multiprocessing
import subprocess
import os

# result = ""

def scale(x,old_min,old_max,new_min,new_max):
    scaled_x = x * (new_max-new_min)/(old_max-old_min) + new_min
    print(f"{x}:{scaled_x}")
    return scaled_x

def assign_node_name(name):
    if 'pqp' in name:
        return name.replace('pqp_','')
    if 'lqp' in name:
        return name.replace('lqp_','')
    else:
        print(f'Name doesnt match convention {name}')
        exit(2)

def draw_graph(node_list):
    #Check id shapes make sense
    # shape = m.shape()
    # if shape[0] != len(node_names) or shape[1] != len(node_names):
    #     print("Shape mismatch")
    #     exit(2)

    #Each node in nodelist is a 3 tuple (nodeA name, nodeB name, weight between them)

    all_lengths = [x[2] for x in node_list]
    xmin = min(all_lengths)
    xmax = max(all_lengths)

    #Create graph
    G = pv.AGraph()

    G.node_attr["shape"] = 'none'
    G.node_attr["width"] = 0.1
    G.node_attr["height"] = 0.1
    G.node_attr["style"] = 'filled'
    G.node_attr["fillcolor"] = '#00000000'
    G.node_attr["fontsize"] = 14
    G.node_attr["fontcolor"] = "black"
    G.node_attr["labeldistance"] = 5

    node_map = dict()
    for node in node_list:
        x = assign_node_name(node[0])
        y = assign_node_name(node[1])
        node_map.update({node[0]:x})
        node_map.update({node[1]:y})

    for node_id,node_name in node_map.items():
        if 'pqp' in node_id:
            G.add_node(node_name,label=node_name.split('_')[-1],shape='ellipse',fillcolor='lightblue')
        elif 'lqp' in node_id:
            G.add_node(node_name,label=node_name.split('_')[-1],shape='ellipse',fillcolor='lightgreen')
        # G.get_node(name).attr["label"]="Helooooooooo"
        # G.get_node(name).attr["labeldistance"]=1

    for node in node_list:
        G.add_edge(node_map[node[0]],node_map[node[1]],len=scale(node[2],xmin,xmax,0.3,5),penwidth=0.2)

    #Draw the graph
    G.layout('neato')
    G.write('force_directed.dot')
    G.draw('force_directed.png')
    G.draw('force_directed.pdf')


def run_command(command,result):
    result = subprocess.run(command,shell=True,text=True,capture_output=True)
    print(result.stdout)


def query_name_from_path(path):
    filename = os.path.basename(path)
    print(filename)
    if "pqp" in filename:
        query_name = filename.split('.')[0].replace('pqp_','')
    else:
        query_name = filename.split('.')[0].replace('lqp_','')
    return query_name


def text_to_nodes(textfile):
    nodes = []
    lines = open(textfile).readlines()
    for line in lines:
        input_plan = line.split(':')[0].split(',')[0]
        cached_plan = line.split(':')[0].split(',')[1]
        distance = float(line.split(':')[1])
        cached_query_name = os.path.basename(cached_plan).split('.')[0]
        input_query_name = os.path.basename(input_plan).split('.')[0]
        nodes.append((input_query_name,cached_query_name,distance))

    return nodes


if __name__ == '__main__':

    file_with_teds = sys.argv[1]

    # input_plan = sys.argv[1]
    # cached_plans = sys.argv[2:]

    tree_edit_script = f"/data1/sumanthu/hyrise/myscripts/tree_edit_distance2.py"
    # input_query_name = os.path.basename(input_plan).split('.')[0]

    #Run the tree edit distance
    # command = f"python {tree_edit_script} {input_plan} {' '.join(cached_plans)}"
    # print(command)

    # result=subprocess.run(command,shell=True,text=True,capture_output=True)

    # lines = result.stdout.splitlines()
    # print(lines)

    # print(lines)

    # #Process the output
    
    # nodes = []

    # for line in lines[1:]:
    #     fname = line.split(':')[0]
    #     distance = float(line.split(':')[1])
    #     cached_query_name = os.path.basename(fname).split('.')[0]
    #     nodes.append((input_query_name,cached_query_name,distance))

    # print(nodes)

    nodes = text_to_nodes(file_with_teds)

    for node in nodes:
        print(node)

    node_names = [nodes[0][0]]
    for node in nodes:
        node_names.append(node[1])
    
    node_names = set(node_names)

    print(node_names)

    draw_graph(nodes)

    # print(query_name_from_path('golden_templates/sf_100/lqp_5.json'))