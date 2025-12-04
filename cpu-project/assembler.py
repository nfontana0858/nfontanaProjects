# Authors: Dylan McGrory and Nick Fontana
# Pledge: I pledge my honor that I have abided by the Stevens Honor System.

import os

os.chdir(os.path.dirname(os.path.abspath(__file__))) # Sets the current working directory to the directory where the program is located

# Stores the opcode values of each instruction
opcode = { 
    "ADD": "00",
    "SUB": "01",
    "LDR": "10",
    "STR": "11"
}

# Stores the text files relating to each header
header = {
    ".text": "instruction.txt",
    ".data": "data.txt"
}

# Stores all the info to be written to each text file
writeInfo = {
    "instruction.txt": [],
    "data.txt": []
}

writeFile = None # Default file to write to is none until a header is found

def encodeAssembly(line): # Returns the inputted line encoded into its hexadecimal representation 
    code = line.replace(", ", " ").replace(" ", ",").split(",") # Splits the line into [instruction, registers] 
    
    instruction = code[0] # Gets the instruction from the code
    registers = code[1:] # Gets the registers into a list [Rd, Rn, Rm] or [Rt, Rn]
    
    registers = [bin(int(register[1:]))[2:].zfill(2) for register in registers] # Looks weird, but reassigns each register to its binary representation (and ensures they're of length 2)
    
    op = opcode[instruction]
    binOutput = (f"{op}{'00'}{registers[1]}{registers[0]}" if op == "10" else f"{op}{registers[0]}{registers[1]}{'00'}") if len(registers) != 3 else f"{op}{registers[2]}{registers[1]}{registers[0]}" # Binary representation of each instruction

    return hex(int(binOutput, 2))[2:] # Converts the binary string into an integer and then into hexadecimal which is then returned

# Code below reads each line from the assembly file and stores all needed information
with open("assembly.txt", "r") as assem_file: # Prepares to read from the assembly text file
    for line in assem_file: # Loops through all lines in the inputted file
        line = line.strip() # Properly formats each line
        if line: # Runs if the current line has text
            tempFile = header.get(line) # Temporary file used to check for headers and to open files to write to
            
            if not tempFile: # Runs if the current line is not a header
                if writeFile: # Runs if we have a file to write to
                    if writeFile == "instruction.txt": # If the line is an instruction, it needs to be encoded before being added to the list
                        addInfo = encodeAssembly(line) # Used to temporarily store the encoded version of each line
                    else:
                        addInfo = line.split(" ")[1] # Gets the value for each data value
                    writeInfo[writeFile].append(addInfo) # Adds the info to each list to later be written to each respective file
            else:
                writeFile = tempFile # Sets writeFile to the name of the file to later write to based on the header
                # writeFile is now either "instruction.txt" or "data.txt"

# Code below writes the previously collected data to their respective files
for file, values in writeInfo.items(): # Loops through all values in each list from the writeInfo dictionary
    with open(file, "w") as txtFile: # Prepares to write to the file storing the encoded instructions
        txtFile.write(f"{' '.join(values)}") # Writes values to the file