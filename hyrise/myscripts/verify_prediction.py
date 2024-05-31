import sys

if __name__ == '__main__':
    dir = sys.argv[1]

    #Read the column wise access counts and compare with prediction
    with open(f"{dir}/column_wise_data.csv") as file:
        lines = file.readlines()
    

    column_wise_counts = dict()
    for line in lines:
        key = line.split(",")[0]
        val = int(line.split(",")[6])
        column_wise_counts.update({key:val})

    with open(f"{dir}/prediction.csv") as file:
        lines = file.readlines()

    prediction_counts = dict()
    for line in lines:
        key = line.split(",")[0]
        val = int(line.split(",")[1])
        prediction_counts.update({key:val})

    correctness_flag = True


    for key,val in column_wise_counts.items():
        if key not in prediction_counts.keys():
            if val !=0:
                print(f"Missing column {key} in prediction counts")
                correctness_flag = False
            else:
                print(f"Column {key} matches")
        elif val == prediction_counts[key]:
            print(f"Column {key} matches")
        else:
            print(f"Column {key} mismatch, {val} v/s {prediction_counts[key]}")
            correctness_flag = False

    if len(column_wise_counts.keys()) != len(prediction_counts.keys()):
        print(f"Column counts mismatch")
        correctness_flag = False

        for key in prediction_counts.keys():
            if key not in column_wise_counts.keys():
                print(f"Column {key} extra in prediction counts")

    if correctness_flag:
        print("All values match")
    else:
        print("Value mismatch detected")
