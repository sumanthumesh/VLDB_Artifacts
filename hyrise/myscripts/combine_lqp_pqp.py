import json
import threading
import re
import sys
from pathlib import Path

all_columns = []

def is_operated(col):
    return '*' in col or '+' in col or '-' in col or '/' in col or 'EXTRACT' in col and col != "COUNT(*)"

def traverse_lqp_helper(tree, result):
    if tree is None or "description" not in tree:
        return
    result["ops"] = re.search(r'\[(.*?)\]', tree["description"]).group(1)
    if result["ops"] == "Projection":
        columns = tree["description"].split()
        columns = ' '.join(columns[1:])
        columns = columns.split(',')
        result["columns"] = [x.strip() for x in columns]
        result["forward"] = []
        result["nonforward"] = []
        for col in columns:
            if is_operated(col):
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', col, re.IGNORECASE):
                        if ref not in result["nonforward"]:
                            result["nonforward"].append(ref)
            else:
                if re.sub(r'\s+', '', col) not in result["forward"]:
                    result["forward"].append(re.sub(r'\s+', '', col))
    if result["ops"] == "Aggregate":
        result["groupby_modified"] = []
        result["groupby_unmodified"] = []
        result["aggregate_modified"] = []
        result["aggregate_unmodified"] = []
        result["any_column"] = []
        groupby_string = tree["description"].split("[")[2].split("]")[0]
        aggregate_string = tree["description"].split("[")[3].split("]")[0]

        aggr_cols = aggregate_string.split(",")
        result["aggr_columns"] = [x.strip() for x in aggr_cols]
        grby_cols = groupby_string.split(",")
        result["grby_columns"] = [x.strip() for x in grby_cols]

        print(aggr_cols)
        print(grby_cols)

        for str in grby_cols:
            if is_operated(str):
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', str, re.IGNORECASE):
                        if ref not in result["groupby_modified"]:
                            result["groupby_modified"].append(ref)
            else:
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', str, re.IGNORECASE):
                        if ref not in result["groupby_unmodified"]:
                            result["groupby_unmodified"].append(ref)
        for str in aggr_cols:
            if is_operated(str):
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', str, re.IGNORECASE):
                        if ref not in result["aggregate_modified"]:
                            result["aggregate_modified"].append(ref)
            elif 'ANY' in str:
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', str, re.IGNORECASE):
                        if ref not in result["any_column"]:
                            result["any_column"].append(ref)
            else:
                for ref in all_columns:
                    if re.search(r'\b'+ref+r'\b', str, re.IGNORECASE):
                        if ref not in result["aggregate_unmodified"]:
                            result["aggregate_unmodified"].append(ref)
            
    if result["ops"] == "Predicate":
        result["ops"] = "Table Scan"
        result["scan column"] = []
        for ref in all_columns:
            if re.search(r'\b'+ref+r'\b', tree["description"], re.IGNORECASE):
            # if re.search(ref, tree["description"], re.IGNORECASE):
                result["scan column"].append(ref)
    if "Leftchildren" in tree:
        result["Left"] = {}
        traverse_lqp_helper(tree["Leftchildren"], result["Left"])
    if "Rightchildren" in tree:
        result["Right"] = {}
        traverse_lqp_helper(tree["Rightchildren"], result["Right"])

def traverse_pqp_helper(tree, result):
    if tree is None or "description" not in tree:
        return
    if re.search("join", tree["description"], re.IGNORECASE):
        match = re.search(r'^(\S+)\s(\((.*?)\))', tree["description"])
        result["join_type"] = match.group(1) + ' ' + match.group(2)
        result["left columns"] = re.findall(r'(\w+)\s+FromLeft', tree["description"])
        result["right columns"] = re.findall(r'(\w+)\s+FromRight', tree["description"])
    if "LeftCard" in tree:
        result["left_card"] = tree["LeftCard"]
    if "RightCard" in tree:
        result["right_card"] = tree["RightCard"]
    if "Leftchildren" in tree:
        traverse_pqp_helper(tree["Leftchildren"], result["Left"])
    if "Rightchildren" in tree:
        traverse_pqp_helper(tree["Rightchildren"], result["Right"])

def traverse_lqp(lqp, result):
    for id in lqp:
        traverse_lqp_helper(lqp[id], result[id])
    return

def traverse_pqp(pqp, result):
    for id in pqp:
        traverse_pqp_helper(pqp[id], result[id])
    return

def enumerate_nodes(tree):
    node_id_counter = 0
    #Got through the dictionary and add node_ids
    stack = [tree]
    while(len(stack)>0):
        temp = stack.pop()
        temp.update({"nodeid":node_id_counter})
        if "Left" in temp.keys():
            stack.append(temp["Left"])
        if "Right" in temp.keys():
            stack.append(temp["Right"])
        node_id_counter += 1

if __name__ == '__main__':

    dir = Path(sys.argv[1])
    result = dict()
    lqp = dict()
    pqp = dict()

    with open("all_tpch_columns.dat", 'r') as f:
        for line in f:
            all_columns.append(line.strip())

    for file in dir.iterdir():
        if file.is_file() and file.suffix == '.json' and not re.search("pp", file.name, re.IGNORECASE):
            with open(file, 'r') as f:
                plan_type, id = file.name.split('_')
                id = id[:-5]
                tree = json.load(f)
                if plan_type == "lqp":
                    lqp[int(id)] = tree
                else:
                    pqp[int(id)] = tree
                result[int(id)] = {}
    # load done

    traverse_lqp(lqp, result)
    traverse_pqp(pqp, result)

    for id in result:
        with open(f'{dir}/pp_{id}.json', 'w') as f:
            enumerate_nodes(result[id])
            json.dump(result[id],f, indent=4)
            print("Dumping")            
            
