import numpy as np # (NumPy, 2026)
from scipy.optimize import least_squares # (SciPy, 2025)
import serial # (pySerial, 2020)
import time

# ===== CONFIG =====
PORT = "COM3"      # change this to the COM port arduino is currentlly connected 
BAUD = 115200

# Mapping of mux order → node index
mux_to_node = [
    "source",  # C0
    0, 4, 8, 12, 13, 14, 15,
    11, 7, 3, 2, 1
]

# Solver algorithm mapping

NUM_RES = 24
NUM_NODES = 16
INTERIOR = [5, 6, 9, 10]

edges = [
    (0,1),(1,2),(2,3),
    (0,4),(1,5),(2,6),(3,7),
    (4,5),(5,6),(6,7),
    (4,8),(5,9),(6,10),(7,11),
    (8,9),(9,10),(10,11),
    (8,12),(9,13),(10,14),(11,15),
    (12,13),(13,14),(14,15)
]

average_resistor = 10000 # average resistor value in the circuit, solver will try to tend values towards it if they arent clearly different

# ========== Solver ==========

def solve_resistors(experiments, Rs=1000, V_supply=5.0,
                    fixed_index=None, fixed_value=None,
                    x0=None, stage=1, p_R=None):

    num_exp = len(experiments)

    suspects = []

    if stage == 2:
        suspects = detect_suspects(p_R)
        print(suspects)

    # prepare an array of intitial 0 values if none are provided (first iteration)
    if x0 is None:
        x0 = np.zeros(NUM_RES + num_exp * len(INTERIOR))

    def unpack(x):
        logR = x[:NUM_RES]
        R = np.exp(logR)

        Vint = []
        idx = NUM_RES
        for _ in range(num_exp):
            Vint.append(x[idx:idx+len(INTERIOR)])
            idx += len(INTERIOR)

        return R, logR, Vint

    def residuals(x):
        R, logR, Vint = unpack(x)
        res = []

        for exp_i, exp in enumerate(experiments):

            V = np.array(exp["voltages"], dtype=float)

            for k, node in enumerate(INTERIOR):
                V[node] = Vint[exp_i][k]

            source_node = exp["source"]

            for n in range(NUM_NODES):

                if n == exp["ground"]:
                    continue

                current_sum = 0.0

                for ei, (i, j) in enumerate(edges):
                    if i == n:
                        current_sum += (V[i] - V[j]) / R[ei]
                    elif j == n:
                        current_sum += (V[j] - V[i]) / R[ei]

                if n == source_node:
                    current_sum += (V[n] - V_supply) / Rs

                res.append(current_sum)

        # Only apply fixed resistor values if provided
        if fixed_index is not None and fixed_value is not None:
            res.append(logR[fixed_index] - np.log(fixed_value))

        # Regularisation, essentially adjusting the data from solver form to actual resistor values
        lambda_reg = 1e-4 # 1e-4 works best with just external nodes 5% error on them, will not work well with internal nodes up to a 100% error. 1e-5 works best with both changing, but results in an overall 15% error, these numbers are for single stage
        lambda_strong = 1e-3 # 2 stage solver is more reliable with 1e-4 as main regularisation as it avoids obscure initial pre suspect results
        lambda_weak = 1e-6
        for i, r in enumerate(R):
            if stage == 2 and len(suspects) < 7: # 2 stage drops average error from 15% to 4%, will not apply above 6 suspects as results will be terrible
                if i in suspects:
                    lambda_reg = lambda_weak
                else:
                    lambda_reg = lambda_strong
                    
            elif stage == 2 and len(suspects) >= 7:
                print("Warning - large number of deviating results, lower accuracy stage 2 is in use, expected error up to 15%")
                lambda_reg = 1e-5

            res.append(lambda_reg * (np.log(r) - np.log(average_resistor)))

        return res

    sol = least_squares(residuals, x0, verbose=0)

    return sol.x, unpack(sol.x)[0]

# ===== READ SERIAL =====
def read_experiments():
    ser = serial.Serial(PORT, BAUD, timeout=2)
    time.sleep(2)

    experiments = []
    current_exp = None

    while True:
        line = ser.readline().decode().strip()

        if not line:
            continue

        print(line)  # debug

        # if it's the start of new data then add old data into the list and start a new dictionary for results
        if line.startswith("EXP"):
            if current_exp != None:
                experiments.append(current_exp)

            exp_index = int(line.split()[1])

            # verification to ensure first calibration run has the full experiment package and not any malformed data
            if experiments == [] and exp_index != 0:
                pass
            
            else:
                current_exp = {
                    "voltages": [None]*16,
                    "source": 0,     # always that node, hard wired
                    "ground": None
                }

        # if it's the end of data then save the last data frame to list and exit the loop
        elif line == "DONE":
            if current_exp != None:
                experiments.append(current_exp)
            break
        
        # if the line doesn't meet previous conditions then it must be values so add them to the experiment data frame, as long as there in an experiment frame to add to
        else:
            if current_exp != None:
                values = list(map(float, line.split(",")))

                for i, v in enumerate(values):
                    node = mux_to_node[i]

                    if node == "source":
                        continue

                    current_exp["voltages"][node] = v

    ser.close()

    return experiments

# detect non standard results
def detect_suspects(R, nominal=average_resistor, threshold=0.10):
    suspects = []
    for i, r in enumerate(R):
        if abs(r - nominal) / nominal > threshold:
            suspects.append(i)
    return suspects

# Order of ground positions for assigning data to solver
ground_map = [8, 12, 13, 14, 15, 11, 7, 3, 2, 1]

def apply_ground_mapping(experiments):
    for i, exp in enumerate(experiments):
        exp["ground"] = ground_map[i]

prev_x = None
first_run = True

while True:
    print("\nWaiting for data...\n")

    experiments = read_experiments()
    if experiments != []:
        apply_ground_mapping(experiments)

        if first_run:
            print("Calibration run (fixed resistor applied)\n")

            prev_x, R = solve_resistors(
                experiments,
                Rs=1000,
                V_supply=5.0,
                fixed_index=0,
                fixed_value=10000.0,
                x0=prev_x
            )

            print(R)

            prev_x, R = solve_resistors(
                experiments,
                Rs=1000,
                V_supply=5.0,
                fixed_index=0,
                fixed_value=10000.0,
                x0=prev_x,
                stage=2,
                p_R=R
            )

            first_run = False

        else:
            print("Standard run (no fixed resistor value, all values can change)\n")

            prev_x, R = solve_resistors(
                experiments,
                Rs=1000,
                V_supply=5.0,
                fixed_index=None,
                fixed_value=None,
                x0=prev_x
            )

            print(R)

            prev_x, R = solve_resistors(
                experiments,
                Rs=1000,
                V_supply=5.0,
                fixed_index=None,
                fixed_value=None,
                x0=prev_x,
                stage=2,
                p_R=R
            )

        print("\nSolved resistances:\n", R)
