import json
import os
import sys
from enum import Enum

benchmark = "tpch"
# benchmark = "job"

if benchmark == "job":
    lines = open('job_columns.dat').readlines()
else:
    lines = open('details.dat').readlines()

all_tpch_cols = [x.split(',')[0].strip() for x in lines]

class OpType(Enum):
    ALIAS = 0,
    PROJECTION = 1,
    SORT = 2,
    AGGREGATE = 3,
    JOINHASH = 4,
    VALIDATE = 5,
    PREDICATE = 6,
    STOREDTABLE = 7,
    DUMMY = 8,
    UNION = 9

def process_card_string(card):
    num_rows = int(card.split("/")[0].split(" ")[0].replace(",",""))
    num_chunks = int(card.split("/")[1].split(" ")[0])
    print(num_rows,num_chunks)
    return num_rows,num_chunks

def type_from_name(ops):
    #Assign OP type
    if ops == "Alias":
        return OpType.ALIAS
    elif ops == "Projection":
        return OpType.PROJECTION
    elif ops == "Sort":
        return OpType.SORT
    elif ops == "AggregateHash":
        return OpType.AGGREGATE
    elif "Join" in ops:
        return OpType.JOINHASH
    elif ops == "Validate":
        return OpType.VALIDATE
    elif ops == "TableScan":
        return OpType.PREDICATE
    elif ops == "GetTable":
        return OpType.STOREDTABLE
    elif ops == "Union":
        return OpType.UNION

    else:
        print(f"Unknown node type {ops}")
        exit(1)

def add_node_id(d, node_id_counter):
    """
    Adds a "nodeid" field to a nested dictionary and its children recursively,
    creating a copy of the dictionary to avoid modifying the original.

    Args:
        d: The nested dictionary to modify.
        node_id_counter: A counter to keep track of assigned node IDs.

    Returns:
        A new dictionary with the added "nodeid" fields.
    """

    new_dict = {k: v if isinstance(v, dict) else v for k, v in d.items()}  # Create a copy

    new_dict["nodeid"] = node_id_counter
    node_id_counter += 1

    if isinstance(new_dict.get("Left"), dict):
        new_dict["Left"] = add_node_id(new_dict["Left"], node_id_counter)  # Recursive call
    if isinstance(new_dict.get("Right"), dict):
        new_dict["Right"] = add_node_id(new_dict["Right"], node_id_counter)  # Recursive call

    return new_dict

class node:

    def __init__(self):
        self.nodeid = None
        self.ignore = False
        self.parent = None
        self.lchild = None
        self.rchild = None
        self.optype = None
        self.lcard  = 0
        self.rcard  = 0
        self.lchunk = 0
        self.rchunk = 0
        self.is_explored = False

    def populate_node(self,dict_node):
        #Set OpType
        if "name" not in dict_node.keys():
            self.optype = OpType.DUMMY
        else:
            self.optype = type_from_name(dict_node["name"])
        self.ignore = dict_node["ignore"]
        #Set Cardinality and chunk numbers
        if "lcard" in dict_node.keys():
            self.lcard = dict_node["lcard"]
        if "rcard" in dict_node.keys():
            self.rcard = dict_node["rcard"]
        #Set the operator specific data
        if self.optype == OpType.PROJECTION:
            self.forwarded_columns = dict_node["forward"]
            self.non_forwarded_columns = dict_node["noforward"]
        elif self.optype == OpType.JOINHASH:
            self.l_columns = dict_node["lcolumn"]
            self.r_columns = dict_node["rcolumn"]
        elif self.optype == OpType.PREDICATE:
            self.columns = dict_node["columns"]
        elif self.optype == OpType.AGGREGATE:
            self.groupby_nomod = dict_node["grpby_nomod"]
            self.groupby_mod = dict_node["grpby_mod"]
            self.aggregate_nomod = dict_node["aggr_nomod"]
            self.aggregate_mod = dict_node["aggr_mod"]
            if "aggr_onlyout" in dict_node.keys():
                self.aggregate_onlyout = dict_node["aggr_onlyout"] 
            else:
                self.aggregate_onlyout = []  
            if "aggr_onlyin" in dict_node.keys():
                self.aggregate_onlyin = dict_node["aggr_onlyin"] 
            else:
                self.aggregate_onlyin = []   
            # self.any_column = dict_node["any_column"]
        elif self.optype == OpType.SORT:
            self.nomod = dict_node["col_nomod"]
            self.mod = dict_node["col_mod"]

    def column_already_projected(self,column):
        '''
        Returns true if this column was already projected by a projection node or aggregate node sometime below in the tree
        Input 
        column: The column name we want to check
        '''
        print(f"Checking invoked by {self.nodeid} for {column}")
        nodes_to_explore = []
        if self.lchild != None:
            nodes_to_explore.append(self.lchild)
        if self.rchild != None:
            nodes_to_explore.append(self.rchild)

        while len(nodes_to_explore) > 0:
            n = nodes_to_explore.pop()
            # print(f"Traversing {n.nodeid}")
            if n.optype == OpType.PROJECTION:
                if column in n.non_forwarded_columns:
                    print(f"Rejected due to projection {column} {self.nodeid}")
                    return True
            
            if n.optype == OpType.AGGREGATE:
                print(f"{n.groupby_mod} {n.aggregate_mod} {column}")
                if column in n.aggregate_mod or column in n.groupby_mod:
                    print(f"Rejected due to aggregate {column} {self.nodeid}")
                    return True

            if n.lchild != None:
                nodes_to_explore.append(n.lchild)
            if n.rchild != None:
                nodes_to_explore.append(n.rchild)

        return False

def update_dict_counter(key,dictionary,incr_value):
    print(f"UPDATE: {key}, {incr_value}")
    if key not in all_tpch_cols:
        print(f"{key} not an actual column")
        exit(1)
    if key in dictionary.keys():
        dictionary[key] += incr_value
    else:
        dictionary[key] = incr_value

def access_counts_from_tree(tree):
    #Here tree is a dictionary with nodeid : node pairs

    #Start from a leaf node
    max_node_id = max(tree.keys())
    nodes_to_explore = [tree[max_node_id]]

    #Dictionary of column counters
    counters = dict()

    while(len(nodes_to_explore) > 0):
        n = nodes_to_explore.pop()

        if not n:
            continue

        if n.is_explored:
            continue

        if n.ignore:
            continue

        skip_flag = False

        if n.lchild != None and not n.lchild.is_explored:
            nodes_to_explore.append(n)
            nodes_to_explore.append(n.lchild)
            skip_flag = True
        if n.rchild != None and not n.rchild.is_explored:
            nodes_to_explore.append(n)
            nodes_to_explore.append(n.rchild)
            skip_flag = True
        # if (not n.lchild != None and not n.lchild.is_explored) or\
            # (not n.rchild != None and not n.rchild.is_explored):
        if skip_flag:    
            continue

        print(f"Exploring node {n.nodeid}")

        if n.optype == OpType.PREDICATE:
            for column in n.columns:
                update_dict_counter(column,counters,n.lcard)
        elif n.optype == OpType.JOINHASH:
            for col in n.l_columns:
                update_dict_counter(col,counters,n.lcard)
            for col in n.r_columns:
                update_dict_counter(col,counters,n.rcard)
                
        elif n.optype == OpType.PROJECTION:
            for column in n.non_forwarded_columns:
                # if not n.column_already_projected(column):
                    update_dict_counter(column,counters,n.lcard)
        elif n.optype == OpType.AGGREGATE:
            # if len(n.groupby_nomod) > 0:
            print(f"Aggregate nomod {n.aggregate_nomod}")
            for column in n.aggregate_nomod:
                # if not n.column_already_projected(column):
                    update_dict_counter(column,counters,n.lcard)
            # for column in n.any_column:
            #     if not n.column_already_projected(column):
            #         update_dict_counter(column,counters,n.parent.lcard)

            for column in n.groupby_nomod:
                update_dict_counter(column,counters,n.lcard)
                update_dict_counter(column,counters,n.parent.lcard)
            for column in n.aggregate_onlyout:
                update_dict_counter(column,counters,n.parent.lcard)
            for column in n.aggregate_onlyin:
                update_dict_counter(column,counters,n.lcard)

        elif n.optype == OpType.SORT:
            for col in n.nomod:
                update_dict_counter(col,counters,n.lcard)
        
        n.is_explored = True
        # if n.optype != OpType.AGGREGATE:
        #     nodes_to_explore.append(n.parent)
        nodes_to_explore.append(n.parent)
    
    return counters

def string2rows(s,lqp=False):
    if lqp:
        return int(float(s.strip().split(' ')[0].replace(',','')))
    return int(s.strip().split(' ')[0].replace(',',''))

def preprocess_lqp_node(node):
    if not node:
        return None
    description = node["description"] 
    # print(description)       
    temp_node = dict()
    temp_node["ignore"] = False
    if description.startswith("[StoredTable]"):
        temp_node["name"] = "GetTable"
        #Has no children
        temp_node["lchild"] = None
        temp_node["rchild"] = None
    elif description.startswith("[Validate]"):
        temp_node["name"] = "Validate"
        #Has one lchild
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        temp_node["rchild"] = None
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
    elif description.startswith("[UnionNode]"):
        temp_node["name"] = "Union"
        #Has one lchild
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        temp_node["rchild"] = preprocess_lqp_node(node["Rightchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
        temp_node["rcard"] = string2rows(node["RightCard"],lqp=True) 
    elif description.startswith("[Alias]"):
        temp_node["name"] = "Alias"
        #One child
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True)
    elif description.startswith("[Predicate]"):
        temp_node["name"] = "TableScan"
        #Has one child
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        temp_node["rchild"] = None
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],True) 

        temp_node["columns"] = []
        for col in all_tpch_cols:
            if col in description:
                temp_node["columns"].append(col)
    elif description.startswith("[Join]"):
        temp_node["name"] = "JoinHash"
        #Has two children
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        temp_node["rchild"] = preprocess_lqp_node(node["Rightchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
        temp_node["rcard"] = string2rows(node["RightCard"],lqp=True) 
        #One predicate case
        temp_node["lcolumn"] = []
        temp_node["rcolumn"] = []

        print(description)
        # Left column
        first_split = description.split(']')[1:]
        print(first_split)
        second_split = [s.split("[")[1].strip() if "[" in s else '' for s in first_split]
        left_cols = [s.split('=')[0].strip() if "=" in s else '' for s in second_split]
        right_cols = [s.split('=')[1].strip() if "=" in s else '' for s in second_split]
        
        for col in left_cols:
            if col in all_tpch_cols:
                temp_node["lcolumn"].append(col)
        for col in right_cols:
            if col in all_tpch_cols:
                temp_node["rcolumn"].append(col)
    elif description.startswith("[Aggregate]"):
        #Name
        temp_node["name"] = "AggregateHash"
        #One child
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
        #Groupbys
        group_by_string = description.split('[')[2].split(']')[0]
        temp_node["grpby_nomod"] = []
        temp_node["grpby_mod"] = []
        temp_node["grpby_expr"] = []
        for expr in group_by_string.split(','):
            expr = expr.strip()
            if '+' in expr or '-' in expr or '*' in expr or '/' in expr:
                temp_node["grpby_expr"].append(expr)
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["grpby_mod"]:
                        temp_node["grpby_mod"].append(col)
            else:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["grpby_nomod"]:
                        temp_node["grpby_nomod"].append(col)
        #Aggregate
        aggregate_string = description.split('[')[3].split(']')[0]
        temp_node["aggr_nomod"] = []
        temp_node["aggr_mod"] = []
        temp_node["aggr_expr"] = []
        for expr in aggregate_string.split(','):
            expr = expr.strip()
            if '+' in expr or '-' in expr or '*' in expr or '/' in expr:
                temp_node["aggr_expr"].append(expr)
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["aggr_mod"]:
                        temp_node["aggr_mod"].append(col)
            else:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["aggr_nomod"]:
                        temp_node["aggr_nomod"].append(col)
    elif description.startswith("[Projection]"):
        temp_node["name"] = "Projection"
        #One child
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
        temp_node["forward"] = []
        temp_node["noforward"] = []
        
        for expr in description.replace("[Projection]","").strip():
            if '+' in expr or '-' in expr or '*' in expr or '/' in expr:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["noforward"]:
                        temp_node["noforward"].append(col)
            else:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["forward"]:
                        temp_node["forward"].append(col)
    elif description.startswith("[Sort]"):
        temp_node["name"] = "Sort"
        #One child
        temp_node["lchild"] = preprocess_lqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 

        sort_string = description.replace("[Sort]","").strip()
        temp_node["col_mod"] = []
        temp_node["col_nomod"] = []
        for expr in sort_string.split(','):
            temp = expr.split("(")[0].strip()
            if '+' in temp or '-' in temp or '*' in temp or '/' in temp:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["col_mod"]:
                        temp_node["col_mod"].append(col)
            else:    
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["col_nomod"]:
                        temp_node["col_nomod"].append(col)
    else:
        print(f"Unknown Operator {description}")
        exit(1)
    return temp_node

def preprocess_pqp_node(node):
    if not node:
        return None
    description = node["description"] 
    temp_node = dict()
    temp_node["ignore"] = False
    if description.startswith("GetTable"):
        temp_node["name"] = "GetTable"
        #Has no children
        temp_node["lchild"] = None
        temp_node["rchild"] = None
    elif description.startswith("Validate"):
        temp_node["name"] = "Validate"
        #Has one lchild
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        temp_node["rchild"] = None
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 
    elif description.startswith("UnionPositions") or description.startswith("UnionAll"):
        temp_node["name"] = "Union"
        #Has one lchild
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        temp_node["rchild"] = preprocess_pqp_node(node["Rightchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"],lqp=True) 
        temp_node["rcard"] = string2rows(node["RightCard"],lqp=True)
    elif description.startswith("Alias"):
        temp_node["name"] = "Alias"
        #One child
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"])
    elif description.startswith("TableScan"):
        temp_node["name"] = "TableScan"
        #Has one child
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        temp_node["rchild"] = None
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 

        temp_node["columns"] = []
        for col in all_tpch_cols:
            if col in description:
                temp_node["columns"].append(col)
    elif description.startswith("JoinHash"):
        temp_node["name"] = "JoinHash"
        #Has two children
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        temp_node["rchild"] = preprocess_pqp_node(node["Rightchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 
        temp_node["rcard"] = string2rows(node["RightCard"].split(' ')[0]) 
        #One predicate case
        temp_node["lcolumn"] = []
        temp_node["rcolumn"] = []

        # Left column
        col = description.split("FromLeft")[0].strip().split(' ')[-1]
        if col in all_tpch_cols:
            temp_node["lcolumn"].append(col)
        # Right column
        col = description.split("=")[1].split("FromRight")[0].strip()
        if col in all_tpch_cols:
            temp_node["rcolumn"].append(col)
        if "AND" in description and "Secondary" in description:
            secondary_string = description.split("AND")[1].strip()
            col = secondary_string.split("FromLeft")[0].strip().split(' ')[-1]
            if col in all_tpch_cols:
                temp_node["lcolumn"].append(col)
            col = description.split("=")[1].split("FromRight")[0].strip()
            if col in all_tpch_cols:
                temp_node["rcolumn"].append(col)

        # temp_node["lcolumn"].append(description.split("FromLeft")[0].strip().split(' ')[-1])
        # temp_node["rcolumn"].append(description.split("=")[1].split("FromRight")[0].strip())
    elif description.startswith("AggregateHash"):
        #Name
        temp_node["name"] = "AggregateHash"
        #One child
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 
        #Groupbys
        group_by_string = description.split('{')[1].split('}')[0]
        temp_node["grpby_nomod"] = []
        temp_node["grpby_mod"] = []
        temp_node["grpby_expr"] = []
        for expr in group_by_string.split(','):
            if '+' in expr or '-' in expr or '*' in expr or '/' in expr:
                temp_node["grpby_expr"].append(expr)
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["grpby_mod"]:
                        temp_node["grpby_mod"].append(col)
            else:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["grpby_nomod"]:
                        temp_node["grpby_nomod"].append(col)
        #Aggregate
        aggregate_string = description.split('}')[1].strip()
        temp_node["aggr_nomod"] = []
        temp_node["aggr_mod"] = []
        temp_node["aggr_expr"] = []
        for expr in aggregate_string.split(','):
            if '+' in expr or '-' in expr or '*' in expr or '/' in expr:
                temp_node["aggr_expr"].append(expr)
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["aggr_mod"]:
                        temp_node["aggr_mod"].append(col)
            else:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["aggr_nomod"]:
                        temp_node["aggr_nomod"].append(col)
    elif description.startswith("Projection"):
        temp_node["name"] = "Projection"
        #One child
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 
        temp_node["forward"] = []
        temp_node["noforward"] = []
        forward_string = description.split('}')[0].split('{')[1].strip()
        for expr in forward_string.split(','):
            if expr == '':
                continue
            temp_node["forward"].append(expr)
        noforward_string = description.split('}')[1].split('{')[1].strip()
        for expr in noforward_string.split(','):
            if expr == '':
                continue
            for col in all_tpch_cols:
                if col in expr and col not in temp_node["noforward"]:
                    temp_node["noforward"].append(col)
    elif description.startswith("[Sort]"):
        temp_node["name"] = "Sort"
        #One child
        temp_node["lchild"] = preprocess_pqp_node(node["Leftchildren"])
        #Cardinality
        temp_node["lcard"] = string2rows(node["LeftCard"].split(' ')[0]) 

        sort_string = description.replace("[Sort]","").strip()
        temp_node["col_mod"] = []
        temp_node["col_nomod"] = []
        for expr in sort_string.split(','):
            # temp = expr.split("(")[0].strip()
            temp = expr
            if "(Descending)" in temp:
                temp.replace("(Descending)","")
            if "(Ascending)" in temp:
                temp.replace("(Ascending)","")
            if '+' in temp or '-' in temp or '*' in temp or '/' in temp:
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["col_mod"]:
                        temp_node["col_mod"].append(col)
            else:    
                for col in all_tpch_cols:
                    if col in expr and col not in temp_node["col_nomod"]:
                        temp_node["col_nomod"].append(col)
    else:
        print(f"Unknown Operator :{description}")
        exit(1)
    return temp_node

def process_plan(path_to_json):
    
    with open(path_to_json) as file:
        new_dict = json.load(file)

    #A dictionary where nodes are mapped by their node id
    nodes = dict()

    nodes_to_explore = [new_dict]

    #Keep a track of who is whose parent and stuff
    #Each elemment here is a tree edge. A,B where A is parent and B is child
    edges = []

    #Do a first pass, in this each node only comes to know who its parent is
    #In the next pass we'll assign children
    while(len(nodes_to_explore) != 0):
        #pop a new dict node from dictionary
        dict_node = nodes_to_explore.pop()
        #Create an empty tree node
        n = node()
        #Set the node id
        n.nodeid = dict_node["nodeid"]
        #Capture details
        n.populate_node(dict_node)
        #Add the children of dict_node to explore list
        if dict_node["lchild"]:
            nodes_to_explore.append(dict_node["lchild"])
            edges.append((n.nodeid,dict_node["lchild"]["nodeid"]))
        if "rchild" in dict_node.keys() and dict_node["rchild"]:
            nodes_to_explore.append(dict_node["rchild"])
            edges.append((n.nodeid,dict_node["rchild"]["nodeid"]))
        nodes.update({n.nodeid:n})

    print("Here")
    for key,val in nodes.items():
        print(f"{key}:{val.optype}")

    #Go through all the edges we recorded and set the parent and child values
    #Keep in mind that always we append left child's edge to egdes list first
        
    for edge in edges:
        #Set children
        if nodes[edge[0]].lchild == None:
            nodes[edge[0]].lchild = nodes[edge[1]]
        elif nodes[edge[0]].rchild == None:
            nodes[edge[0]].rchild = nodes[edge[1]]
        else: 
            print("Trying to add third edge")
            exit(1)
        #Set parents
        nodes[edge[1]].parent = nodes[edge[0]]

    print(edges)

    with open("plan2.json","w") as file:
        json.dump(new_dict,file,indent=4)

    counters = access_counts_from_tree(nodes)
    print(counters)

    return counters

if __name__ == '__main__':
    path_to_jsons = sys.argv[1:]

    counters = dict()

    for path_to_json in path_to_jsons:
        temp_ctr = process_plan(path_to_json)
        for key in temp_ctr:
            if key in counters.keys():
                counters[key] += temp_ctr[key]
            else:
                counters.update({key:temp_ctr[key]})

    with open(f"{os.path.abspath(os.path.dirname(path_to_json))}/prediction.csv","w") as file:
        for key,val in counters.items():
            file.write(f"{key},{val}\n")

    # with open(path_to_json) as file:
    #     dictionary = json.load(file)

    # #Preprocess the file
    # new_dict = preprocess_pqp_node(dictionary)

    # #Write dictionary to file
    # with open("modified_pqp.json","w") as file:
    #     json.dump(new_dict,file, indent=4)
        

    

    # node_id_counter = 0
    # # Got through the dictionary and add node_ids
    # stack = [new_dict]
    # while(len(stack)>0):
    #     temp = stack.pop()
    #     if not temp:
    #         continue
    #     temp.update({"nodeid":node_id_counter})
    #     if "lchild" in temp.keys():
    #         stack.append(temp["lchild"])
    #     if "rchild" in temp.keys():
    #         stack.append(temp["rchild"])
    #     node_id_counter += 1

    # # add_node_id(new_dict,node_id_counter)

    # # print(new_dict)

    # with open("plan2.json","w") as file:
    #     json.dump(new_dict,file,indent=4)
'''
    with open(path_to_json) as file:
        new_dict = json.load(file)

    #A dictionary where nodes are mapped by their node id
    nodes = dict()

    nodes_to_explore = [new_dict]

    #Keep a track of who is whose parent and stuff
    #Each elemment here is a tree edge. A,B where A is parent and B is child
    edges = []

    #Do a first pass, in this each node only comes to know who its parent is
    #In the next pass we'll assign children
    while(len(nodes_to_explore) != 0):
        #pop a new dict node from dictionary
        dict_node = nodes_to_explore.pop()
        #Create an empty tree node
        n = node()
        #Set the node id
        n.nodeid = dict_node["nodeid"]
        #Capture details
        n.populate_node(dict_node)
        #Add the children of dict_node to explore list
        if dict_node["lchild"]:
            nodes_to_explore.append(dict_node["lchild"])
            edges.append((n.nodeid,dict_node["lchild"]["nodeid"]))
        if "rchild" in dict_node.keys() and dict_node["rchild"]:
            nodes_to_explore.append(dict_node["rchild"])
            edges.append((n.nodeid,dict_node["rchild"]["nodeid"]))
        nodes.update({n.nodeid:n})

    print("Here")
    for key,val in nodes.items():
        print(f"{key}:{val.optype}")

    #Go through all the edges we recorded and set the parent and child values
    #Keep in mind that always we append left child's edge to egdes list first
        
    for edge in edges:
        #Set children
        if nodes[edge[0]].lchild == None:
            nodes[edge[0]].lchild = nodes[edge[1]]
        elif nodes[edge[0]].rchild == None:
            nodes[edge[0]].rchild = nodes[edge[1]]
        else: 
            print("Trying to add third edge")
            exit(1)
        #Set parents
        nodes[edge[1]].parent = nodes[edge[0]]

    print(edges)

    with open("plan2.json","w") as file:
        json.dump(new_dict,file,indent=4)

    counters = access_counts_from_tree(nodes)
    print(counters)

    with open(f"{os.path.abspath(os.path.dirname(path_to_json))}/prediction.csv","w") as file:
        for key,val in counters.items():
            file.write(f"{key},{val}\n")
'''
