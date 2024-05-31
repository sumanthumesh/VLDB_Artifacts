from process_query_plan import *
import sys
import json
import os

if __name__ == "__main__":

    json_path = sys.argv[1]
    if len(sys.argv) == 3:
        output_path = sys.argv[2]
    else:
        output_path = f"{os.path.abspath(os.path.dirname(json_path))}/pqp_template.json"
        
    with open(json_path) as file:
        raw_dictionary = json.load(file)

    #Preprocess the file
    new_dict = preprocess_pqp_node(raw_dictionary)

    #Write dictionary to file
    with open("modified_pqp.json","w") as file:
        json.dump(new_dict,file, indent=4)
        
    node_id_counter = 0
    # Got through the dictionary and add node_ids
    stack = [new_dict]
    while(len(stack)>0):
        temp = stack.pop()
        if not temp:
            continue
        temp.update({"nodeid":node_id_counter})
        if "lchild" in temp.keys():
            stack.append(temp["lchild"])
        if "rchild" in temp.keys():
            stack.append(temp["rchild"])
        node_id_counter += 1

    with open(f"{output_path}","w") as file:
        json.dump(new_dict,file,indent=4)