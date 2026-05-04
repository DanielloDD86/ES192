The following code relies on 2 devices, a host device, eg a laptop, and the data lagger, in this case the Arduino with the resistor grid

To set the experiment up, one can either prepare their own python environment (recommended) or use the provided virtual environment.

To prepare own python environment:
- download and install python (3.11 or newer) and an IDE of choice (Instructions are for VSCode), ensure PATH setting is enabled when installing
- upon successful installation of both, restart the computer to allow PATH changes to take effect
- once restarted open the IDE and in the terminal type "pip install numpy" followed by "pip install scipy" then "pip install pyserial"
- once all 3 modules are installed the python environment is ready

to use the virtual environment:
- download and install an IDE of choice (Instructions use VSCode) 
- once installed, place the code and sensors_env together in a folder and open said folder in vs code
- select Voltage_Solver,py in the explorer on the left
- if prompted select the interpreter as sensors_env, else upon opening in the bottom right corner next to bell icon click the box and select sensors_env as the interpreter
- once selected click the run button once, the code should initially fail to execute but after about 20-30s VSCode should automatically type a command in the terminal and after that the code should execute fine. If this does not happen automatically, type the following in the terminal "(Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned) ; (& {path the folder, eg: c:\VSCode}\sensors_env\Scripts\Activate.ps1)"
- after this runs the python environment should be ready



Arduino:
upload the provided .ino file to the Arduino, the Arduino is pre loaded with it already so this is not necessary.#



Running:
- connect the Arduino via usb to the host device
- ensure COM port is correct in the .py file
- click run in the IDE of choice and hope for the best (I personally never had issues running it either raw on a laptop or via the sensors env  on my PC as the root python installation is so screwed up on it due to mingw64 that its incredible anything py based still runs)
- the solver will wait for a complete set of data and when it receives it, it will run the solver. 
- first list returned is seed values for stage 2, second list of resistors is the actual results
- the results usually improve by the second or third iteration of solver

Do note resistor switching is possible mid execution just ensure its done in the 3s gap between measurements as to not ruin the later measurements with erroneous seed data due to miss measured resistor array. it is recommended to kill the solver, swap resistors, then restart the solver

Good Luck, you are gonna need it