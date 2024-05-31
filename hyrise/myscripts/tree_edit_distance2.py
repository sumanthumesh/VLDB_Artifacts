import zss
import sys
import json
from process_query_plan import all_tpch_cols, type_from_name,OpType
import time

NODE_TYPE_COST = 1
COLUMN_COST = 1

class TreeNode:
    def __init__(self, label):
        self.label = label
        self.children = list()

    def set_label(self,label):
        self.label = label

    def add_child(self,child):
        self.children.append(child)

    @staticmethod
    def get_children(node):
        return node.children

    @staticmethod
    def get_label(node):
        return node.label

    def addkid(self, node, before=False):
        if before:  self.my_children.insert(0, node)
        else:   self.my_children.append(node)
        return self

def add_column_to_node_label(access_counts,columns,cardinality):
    for col in columns:
        access_counts.update({col:True})

def dict_to_node(node):

    optype = type_from_name(node["name"])
    columns = []
    access_counts = dict()
    # if optype == OpType.ALIAS or optype == OpType.DUMMY or optype == OpType.VALIDATE or optype == OpType.UNION:
    if optype == OpType.PREDICATE:
        # columns += node["columns"]
        add_column_to_node_label(access_counts,node["columns"],node["lcard"])
    elif optype == OpType.JOINHASH:
        # columns += node["lcolumn"] + node["rcolumn"]
        add_column_to_node_label(access_counts,node["lcolumn"],node["lcard"])
        add_column_to_node_label(access_counts,node["rcolumn"],node["rcard"])
        # for col in node["lcolumn"]:
        #     access_counts.update({col:(int(node["lcard"]))})
        # for col in node["rcolumn"]:
        #     access_counts.update({col:(int(node["rcard"]))})
    elif optype == OpType.PROJECTION:
        # columns += node["noforward"]
        add_column_to_node_label(access_counts,node["noforward"],node["lcard"])
        # for col in node["noforward"]:
        #     access_counts.update({col:(int(node["lcard"]))})
    elif optype == OpType.AGGREGATE:
        # columns += node["aggr_nomod"] + node["grpby_nomod"]
        add_column_to_node_label(access_counts,node["aggr_nomod"],node["lcard"])
        add_column_to_node_label(access_counts,node["grpby_nomod"],node["lcard"])
        if "aggr_onlyout" in node.keys():
            add_column_to_node_label(access_counts,node["aggr_onlyout"],node["lcard"])
        if "aggr_onlyin" in node.keys():
            add_column_to_node_label(access_counts,node["aggr_onlyin"],node["lcard"])
        #     columns += node["aggr_onlyout"]
        #     columns += node["aggr_onlyin"]
    elif optype == OpType.SORT:
        # columns += node["col_nomod"]
        add_column_to_node_label(access_counts,node["col_nomod"],node["lcard"])
    elif optype == OpType.ALIAS or optype == OpType.UNION or optype == OpType.VALIDATE or optype == OpType.STOREDTABLE:
        columns = columns
    else:
        print(f"Unknown operator",node["name"])

    label = [optype]
    # column_vector = [True if col in all_tpch_cols else False for col in columns]

    label.append(access_counts)
    # print(label)

    n = TreeNode(label)
    if node["lchild"] != None:
        n.add_child(dict_to_node(node["lchild"]))
    if "rchild" in node.keys() and node["rchild"] != None:
        n.add_child(dict_to_node(node["rchild"]))

    # for child in node["children"]:
    #     n.add_child(dict_to_node(child))

    return n

def dict_to_tree(dictionary):
    return dict_to_node(dictionary)

def sum_up_dcit_accesses(dictionary):
    sum = 0
    for key,val in dictionary.items():
        sum += val
    return sum

def num_accessed_columns(dictionary):
    return len(dictionary)

def column_hamming_dist(x,y):
    # print(x,x == '')
    # print(y,y == '')
    dist = 0
    if x == '' and y != '':
        # dist += sum_up_dcit_accesses(y[1])
        dist += num_accessed_columns(y[1])
    elif x != '' and y == '':
        # dist += sum_up_dcit_accesses(x[1])
        dist += num_accessed_columns(x[1])
    elif x == '' and y == '':
        dist += 0
    else:
        x_cols = set(x[1].keys())
        y_cols = set(y[1].keys())
        union_cols = x_cols.union(y_cols)
        for col in union_cols:
            if col in x_cols and col in y_cols:
                dist += 0
            elif col in x_cols and col not in y_cols:
                dist += 1
            elif col not in x_cols and col in y_cols:
                dist += 1
    return dist

def node_dist(x,y):
    cost = 0
    if x == None and y == None:
        return 0
    elif (x == None and y != None):
        cost += (len(y)-1)*COLUMN_COST
    elif (x != None and y == None):
        cost += NODE_TYPE_COST + (len(x)-1)*COLUMN_COST
    elif x != y:
        cost += NODE_TYPE_COST + column_hamming_dist(x,y)

    return cost

def compare_json_trees(file1,file2):

    with open(file1) as file1:
        plan1 = json.load(file1)
    with open(file2) as file2:
        plan2 = json.load(file2)
    
    t1 = dict_to_tree(plan1)
    t2 = dict_to_tree(plan2)
    
    dist = zss.simple_distance(t1,t2,TreeNode.get_children,TreeNode.get_label,node_dist)

    return dist

if __name__ == '__main__':

    input_plan = sys.argv[1]

    cached_plans = sys.argv[2:]

    result = dict()

    clk_id = time.CLOCK_PROCESS_CPUTIME_ID 

    for cached_plan in cached_plans:
        start = time.clock_gettime_ns(clk_id)
        # start = time.time()
        dist = compare_json_trees(input_plan,cached_plan)
        end = time.clock_gettime_ns(clk_id)
        # end = time.time()
        # print(f"{end-start}")
        result.update({cached_plan:dist})

    sorted_dict = dict(sorted(result.items(), key=lambda item: item[1], reverse=False))


    # print(f"Checking similarity for {input_plan}")
    for key,val in sorted_dict.items():
        print(f"{input_plan},{key}:{val}")
        
    
    
'''
    with open(input) as file:
        plan = json.load(file)
    
    tree = dict_to_tree(plan)

    # tree1 = {
    #     "label": "1",
    #     "children": [
    #         {
    #             "label": "2",
    #             "children": []
    #         },
    #         {
    #             "label": "3",
    #             "children": [
    #                 {
    #                     "label": "4",
    #                     "children": []
    #                 }
    #             ]
    #         }
    #     ] 
    # }

    # tree2 = {
    #     "label": "1",
    #     "children": [
    #         {
    #             "label": "2",
    #             "children": [
    #                 {
    #                     "label": "4",
    #                     "children": []
    #                 }
    #             ]
    #         },
    #         {
    #             "label": "3",
    #             "children": []
    #         }
    #     ] 
    # }

    # print(tree1)

    t1 = dict_to_tree(plan)
    # t2 = dict_to_tree(tree2)

    # dist = zss.simple_distance(t1,t2,TreeNode.get_children,TreeNode.get_label,node_dist)
    
    # print(dist)

    stack = [t1]
    while len(stack) > 0:
        temp = stack.pop()
        print(f"{TreeNode.get_label(temp)} {TreeNode.get_children(temp)}")
        for child in temp.get_children(temp):
            stack.append(child)
'''