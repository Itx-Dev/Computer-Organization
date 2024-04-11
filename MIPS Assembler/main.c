// Devin Reichenbach
// CMPE-220-02
// Dr. Lee
// 4 December 2023

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

void parseDirectives(FILE* filePTR);
void parseFile(FILE* filePTR, fpos_t textPosition);
void removeSpaces(char* str);
void printResult(char* instructionType, char* opcode, char* registersString, char* shamt, char* funct, char* currentLine);
void printSpacers();

char* findInstruction(char* textLine);
char* findOpCode(char* instruction);
char* findFunct(char* instruction);
char* findRTypeRegisters(char* textline);
char* findITypeRegisters(char* textline);
char* findBranchRegisters(char* textline);
char* decToBinString(int decimal, int bits);
char* immediateToBinString(char* immediateValue);

int binToLong(char* binString);
int immediateToInt(char* immediateValue);
int findInstructionType(char* instruction);

int PC = 0;
int dataAddress = 268500992;

int main()
{
    char* filename = calloc(32, sizeof(char));
    printf("Enter the name of your .asm with the file extension: ");
    scanf("%s", filename);

    // Open file
    FILE* filePTR = fopen(filename, "r");
    // if file is empty exception
    if (filePTR == NULL)
    {
        printf("Cannot open file \n");
        return -1;
    }

    // Read File
    parseDirectives(filePTR);



    // Close assembly file
    fclose(filePTR);
    return 0;
}

void printGlobalVariables(char* globalVariables) {
    printf("0x%08x | %s", dataAddress, globalVariables);
    dataAddress += 4;
}

void printSpacers() {
    for (int i = 0; i < 50; i++) {
        printf("-");
    }
    printf("\n");
}

void parseDirectives(FILE* filePTR) {
    // define pointer position
    fpos_t textPosition = 0;
    // Create char array to store directive text
    char directiveText[32];

    // While end of file is not reached
    while (feof(filePTR) == 0) {
        // Look for .data tag
        fgets(directiveText, 6, filePTR);

        // Find .data and move through section until .text is hit
        if (strncmp(directiveText, ".data", 5) == 0) {
            // Print formatted header
            printf("Data Segment\n");
            printSpacers();
            printf("Address    | Value\n");
            while (strncmp(directiveText, ".text", 5) != 0) {
                fgets(directiveText, 32, filePTR);
                removeSpaces(directiveText);
                // Find lines that have .word in them
                char* globalVariable = strstr(directiveText, ".word");
                // If .word is found
                if (globalVariable != NULL) {
                    char* globalVariableSubString = calloc(32, sizeof(char));
                    // Find the value of the global variable
                    for (int i = 5, substringIndex = 0; i < strlen(globalVariable); i++, substringIndex++) {
                        globalVariableSubString[substringIndex] = globalVariable[i];
                    }

                    printGlobalVariables(globalVariableSubString);
                }
            }
        }


        printf("\nText Segment\n");
        printSpacers();
        printf("Address   | Code                 | Source \n");

        // If .text is seen parse files
        if (strncmp(directiveText, ".text", 5) == 0) {
            // Get position in file where .text is
            fgetpos(filePTR, &textPosition);
            // Move file pointer 2 spaces then parse file
            parseFile(filePTR, textPosition);
        }
    }
}

/**
* Prints final formatted answer in machine code.
* @param instructionType
*/
void printResult(char* instructionType, char* opcode, char* registersString, char* shamt, char* function, char* currentLine) {
    PC += 4;
    char* binaryString = calloc(32, sizeof(char));
    sprintf(binaryString, "%s%s%s%s", opcode, registersString, shamt, function);
    long finalHexCode = binToLong(binaryString);
    printf("0x%07x | %s: 0x%08lx | %s" , PC - 4, instructionType, finalHexCode, currentLine);
}

int binToLong(char* binString) {
    removeSpaces(binString);
    long decimalValue = strtol(binString, NULL, 2);
    return decimalValue;
}

/**
 * Read a single line of the file.
 * @param filePTR
 */
void parseFile(FILE* filePTR, fpos_t textPosition) {
    const int MAX_LENGTH = 32;
    char currentLine[MAX_LENGTH];
    // Move the pointer to the .text segment of assembly
    fsetpos(filePTR, &textPosition);

    // Read each line one by one
    while (fgets(currentLine, MAX_LENGTH, filePTR) != NULL) {
        removeSpaces(currentLine);
        // Find string of first instruction
        char* currentInstruction = findInstruction(currentLine);
        /*
         * If type = 0, instruction is R-Type
         * If type = 1, instruction is I-Type
         */
        int type = findInstructionType(currentInstruction);

        if (type == 0) {
            // Find Funct
            char* funct = findFunct(currentInstruction);
            char *RTypeRegisters = findRTypeRegisters(currentLine);
            char* shamt = "00000";
            // Print result
            printResult("R-Format", "000000", RTypeRegisters, shamt, funct, currentLine);
        } else if (type == 1) {
            // Find opcode
            char* opcode = findOpCode(currentInstruction);
            char* ITypeRegisters = findITypeRegisters(currentLine);
            // Print result
            printResult("I-Format", opcode, ITypeRegisters, " ", " ", currentLine);
        } else if (type == 2) {
            char* branchOpcode = findOpCode(currentInstruction);
            char* BTypeRegisters = findBranchRegisters(currentLine);
            printResult("I-Format", branchOpcode, BTypeRegisters, " ", " ", currentLine);
        }
    }

}


/**
 * Find the instruction of the MIPS line
 * @param textLine
 * @return isolated instruction string
 */
char* findInstruction(char* textLine) {
    int dollarSymbolsFound = 0;                                             // Sentinel for end of instruction string
    int count = 0;                                                          // Count how many characters are in instruction string
    char* instruction = calloc(6, sizeof(char));                                             // Store the instruction in string

    // Parse instruction line looking for
    for (int i = 0; textLine[i]; i++) {
        if (textLine[i] == '$' && dollarSymbolsFound < 1) {                // Look for first dollar sign indicating start of register and end of instruction
            while (count < i) {
                instruction[count] = tolower(textLine[count]);          // Capture only the instruction part of MIPS code making sure it is lowercase.
                count++;
            }
            dollarSymbolsFound++;                                           // If dollar sign is found, entire instruction character have been read and stored.
        }
    }

    return instruction;
}

/**
 * Find the instruction type of the given instruction.
 * @param instruction
 * @return R-Type = 0, I-Type = 1
 */
int findInstructionType(char* instruction) {
    int numOfRTypeInstructions = 27;
    int numOfITypeInstructions = 16;
    int numOfBTypeInstruction = 4;
    int type;
    int regFound = 0;
    // Define all R and I type instructions

    char* RTypeInstructions[] = {"add","addu","and","break","div","divu","jalr","jr","mfhi","mflo","mthi","mtlo","mult","multu","nor","or","sll","sllv","slt","sltu","sra","srav","srl","srlv", "sub","subu","syscall","xor"};

    char* ITypeInstructions[] = {"addi","addiu","andi","lb","lbu","lh","lhu","lui","lw","lwcl","ori","sb","slti","sltiu","sh","sw","swcl","xori"};
    char* BranchInstruction[] = {"bne","bgez","bgtz","blez","bltz","beq"};
    /*
     * Check for R-Type & I-Type instruction
     */
    for (int RTypeArrayIndex = 0; RTypeArrayIndex <= numOfRTypeInstructions && regFound < 1; RTypeArrayIndex++) {
        if (strcmp(instruction, RTypeInstructions[RTypeArrayIndex]) == 0) {
            type = 0;
            regFound++;
        }
    }

    for (int ITypeArrayIndex = 0; ITypeArrayIndex <= numOfITypeInstructions && regFound < 1; ITypeArrayIndex++) {
        if (strcmp(instruction, ITypeInstructions[ITypeArrayIndex]) == 0) {
            type = 1;
            regFound++;
        }
    }

    for (int BranchArrayIndex = 0; BranchArrayIndex <= numOfBTypeInstruction && regFound < 1; BranchArrayIndex++) {
        if (strcmp(instruction, BranchInstruction[BranchArrayIndex]) == 0) {
            type = 2;
            regFound++;
        }
    }

    return type;
}

/**
 * Remove the spaces between characters
 * @param textLine
 */
void removeSpaces(char *textLine) {
    // To keep track of non-space character count
    int count = 0;
    // Traverse the provided string. If the current character is not a space,
    for (int i = 0; textLine[i]; i++)
        if (textLine[i] != ' ') {
            textLine[count++] = textLine[i];        // count is incremented here
        }
    textLine[count] = '\0';                  // null terminator added to end of string
    }

char* findITypeRegisters(char* textline) {
    int regIndexStart = 0;
    int regFound = 0;
    char *reg1 = calloc(3, sizeof(char));
    char *reg2 = calloc(3, sizeof(char));
    char *immediateValue = calloc(8, sizeof(char));

    // Find where the registers start
    while (textline[regIndexStart] != '$') {
        regIndexStart++;
    }
    // create space for 11 characters
    char *regSubstring = calloc(26, sizeof(char));
    strncpy(regSubstring, textline + regIndexStart, 26);

    // Parse string by comma.
    char *parseRegister = strtok(regSubstring, ",");

    char* immediateValueWithoutNewLine = calloc(16, sizeof(char));
    // Keep printing tokens while one of the
    // delimiters present in str[].
    while (parseRegister != NULL) {
        if (regFound == 0) {
            reg2 = parseRegister;
            regFound++;
        } else if (regFound == 1) {
            reg1 = parseRegister;
            regFound++;
        } else if (regFound > 1) {
            immediateValue = parseRegister;
            long maxLength = strcspn(immediateValue, "\n");
            strncpy(immediateValueWithoutNewLine, immediateValue, maxLength);

            regFound++;
        }

        parseRegister = strtok(NULL, ",");
    }

    regSubstring = NULL; free(regSubstring);
    parseRegister = NULL; free(parseRegister);

    // Register array with indexing corresponding to decimal value of register.
    char *regArray[32] = {"$zero", "$at", "$v0", "$v1",
                          "$a0", "$a1", "$a2", "$a3",
                          "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
                          "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9",
                          "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

    // Allocate space for two register Hex strings
    char *reg1BinString = calloc(5, sizeof(char));
    char *reg2BinString = calloc(5, sizeof(char));

    // Search table for matching registers and set corresponding string to hex value.
    for (int sysRegisters = 0; sysRegisters < 31; sysRegisters++) {
        if (strcmp(reg1, regArray[sysRegisters]) == 0) {
            reg1BinString = decToBinString(sysRegisters, 6);
        }
        if (strcmp(reg2, regArray[sysRegisters]) == 0) {
            reg2BinString = decToBinString(sysRegisters, 6);
        }
    }

    // Free allocated space.
    reg1 = NULL; free(reg1);
    reg2 = NULL; free(reg2);

    int immediateIntValue = immediateToInt(immediateValueWithoutNewLine);
    char* immediateValueString = decToBinString(immediateIntValue, 17);



    char *resultString = strcat(reg1BinString, reg2BinString);
    resultString = strcat(resultString, immediateValueString);

    reg1BinString = NULL; free(reg1BinString);
    reg2BinString = NULL; free(reg2BinString);
    immediateValue = NULL; free(immediateValue);

    return resultString;
}

char* findBranchRegisters(char* textline) {
    int regIndexStart = 0;
    int regFound = 0;
    // Allocate memory for two register values.
    char *reg1 = calloc(3, sizeof(char));
    char *reg2 = calloc(3, sizeof(char));

    // Find where the registers start
    while (textline[regIndexStart] != '$') {
        regIndexStart++;
    }
    // create space for 11 characters
    char *regSubstring = calloc(26, sizeof(char));
    strncpy(regSubstring, textline + regIndexStart, 26);

    // Parse string by comma.
    char *parseRegister = strtok(regSubstring, ",");

    // Keep printing tokens while one of the
    // delimiters present in str[].
    while (parseRegister != NULL) {
        if (regFound == 0) {
            reg1 = parseRegister;
            regFound++;
        } else if (regFound == 1) {
            reg2 = parseRegister;
            regFound++;
        }

        parseRegister = strtok(NULL, ",");
    }
    // Register array with indexing corresponding to decimal value of register.
    char *regArray[32] = {"$zero", "$at", "$v0", "$v1",
                          "$a0", "$a1", "$a2", "$a3",
                          "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
                          "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7","$t8", "$t9",
                          "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

    // Allocate memory for two register bin values
    char *reg1BinString = calloc(5, sizeof(char));
    char *reg2BinString = calloc(5, sizeof(char));

    // Search table for matching registers and set corresponding string to hex value.
    for (int sysRegisters = 0; sysRegisters < 31; sysRegisters++) {
        if (strcmp(reg1, regArray[sysRegisters]) == 0) {
            reg1BinString = decToBinString(sysRegisters, 6);
        }
        if (strcmp(reg2, regArray[sysRegisters]) == 0) {
            reg2BinString = decToBinString(sysRegisters, 6);
        }
    }

    reg1 = NULL; free(reg1);
    reg2 = NULL; free(reg2);

    // Calculate target address
    char* labelAddressString = calloc(17, sizeof(char));
    int labelAddress = (PC + 4) * 2 - 32;
    labelAddressString = decToBinString(labelAddress, 17);

    // Combine registers into one string.
    char* resultString = strcat(reg1BinString, reg2BinString);
    resultString = strcat(resultString, labelAddressString);

    reg1BinString = NULL; free(reg1BinString);
    reg2BinString = NULL; free(reg2BinString);
    labelAddressString = NULL; free(labelAddressString);

    return resultString;
}

/**
 * Find the first two registers of instruction.
 * @param textline
 * @return
 */
char* findRTypeRegisters(char* textline) {
    int regIndexStart = 0;
    int regFound = 0;
    // Allocate space for 3 registers to be parsed.
    char *reg1 = calloc(3, sizeof(char));
    char *reg2 = calloc(3, sizeof(char));
    char *reg3 = calloc(3, sizeof(char));

    // Find where the registers start
    while (textline[regIndexStart] != '$') {
        regIndexStart++;
    }
    // create space for 11 characters
    char *regSubstring = calloc(11, sizeof(char));
    strncpy(regSubstring, textline + regIndexStart, 11);

    // Parse string by comma.
    char *parseRegister = strtok(regSubstring, ",");

    // Keep printing tokens while one of the
    // delimiters present in str[].
    while (parseRegister != NULL) {
        if (regFound == 0) {
            reg3 = parseRegister;
            regFound++;
        } else if (regFound == 1) {
            reg1 = parseRegister;
            regFound++;
        } else if (regFound == 2) {
            reg2 = parseRegister;
            regFound++;
        }
        parseRegister = strtok(NULL, ",");
    }
    // Register array with indexing corresponding to decimal value of register.
    char *regArray[32] = {"$zero", "$at", "$v0", "$v1",
                          "$a0", "$a1", "$a2", "$a3",
                          "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
                          "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
                          "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

    // Allocate space for strings to hold hex values
    char *reg1BinString = calloc(5, sizeof(char));
    char *reg2BinString = calloc(5, sizeof(char));
    char *reg3BinString = calloc(5, sizeof(char));

    // Search for matching registers in table and set corresponding hex string to hex value of register.
    for (int sysRegisters = 0; sysRegisters < 32; sysRegisters++) {
        if (strcmp(reg1, regArray[sysRegisters]) == 0) {
            reg1BinString = decToBinString(sysRegisters, 6);
        }
        if (strcmp(reg2, regArray[sysRegisters]) == 0) {
            reg2BinString = decToBinString(sysRegisters, 6);
        }
        if (strncmp(reg3, regArray[sysRegisters], 3) == 0) {
            reg3BinString = decToBinString(sysRegisters, 6);
        }
    }

    reg1 = NULL; free(reg1);
    reg2 = NULL; free(reg2);
    reg3 = NULL; free(reg3);

    char* resultString = calloc(32, sizeof(char));
    // Combine strings into one
    resultString = strcat(reg1BinString, reg2BinString);
    resultString = strcat(resultString, reg3BinString);

    reg1BinString = NULL; free(reg1BinString);
    reg2BinString = NULL; free(reg2BinString);
    reg3BinString = NULL; free(reg3BinString);

    return resultString;
}

/**
 * Convert immediateValue into an integer and return.
 * @param immediateValue
 * @return
 */
int immediateToInt(char* immediateValue) {
    int sum = 0;
    // Convert string into int by multiplying each digit by 10 to the power of the place value and adding the number to the sum
    for (int i = 0, place = strlen(immediateValue); i < strlen(immediateValue); i++, place--) {
        int digit = immediateValue[i] - '0';
        sum += pow(10, place - 1) * digit;
    }
    return sum;
}

/**
 * Convert given decimal to a binary string
 * @param decimal
 * @return 4 byte size max
 */
char* decToBinString(int decimal, int bits) {
    // If number given is zero
    if (decimal == 0) {
        return "0";
    }
    // Stores binary representation of number.
    int binaryNum[bits - 1]; // Assuming 32 bit integer.
    char charBinaryNum[bits - 1];

    // Initalize array to all zeros
    for (int i = 0; i < bits - 1; i++) {
        binaryNum[i] = 0;
    }

    // Convert decimal to binary and store in int array.
    for (int i = 0; decimal > 0; i++) {
        binaryNum[i] = decimal % 2;
        decimal = decimal / 2;
    }

    // Convert Binary into a char array of binary digits
    for (int reverseIndex = bits - 2,  charIndex = 0; reverseIndex >= 0; reverseIndex--, charIndex++) {
        if (binaryNum[reverseIndex] == 1) {
            charBinaryNum[charIndex] = '1';
        } else {
            charBinaryNum[charIndex] = '0';
        }
    }
    // Allocate memory for return string
    char* returnString = calloc(bits + 1, sizeof(char));

    // Copy char array into return string
    strncpy(returnString, charBinaryNum, bits);
    // Add space to end of string
    returnString[bits - 1] = ' ';
    // Add null terminator to end of string
    returnString[bits] = '\0';

    return returnString;
}

/**
 * Find machine code for funct of R-type
 * @param instruction
 * @return machine code string for funct.
 */
char* findFunct(char *instruction) {
    // Convert instruction pointer to char array
    char instructionCopy[6];
    strcpy(instructionCopy, instruction);

// Search table for opcode of I-type format
    if (strcmp(instructionCopy, "add") == 0) {
        return "100000";
    }
    if (strcmp(instructionCopy, "addu") == 0) {
        return "100001";
    }
    if (strcmp(instructionCopy, "and") == 0) {
        return "100100";
    }
    if (strcmp(instructionCopy, "break") == 0) {
        return "001101";
    }
    if (strcmp(instructionCopy, "div") == 0) {
        return "011010";
    }
    if (strcmp(instructionCopy, "divu") == 0) {
        return "011011";
    }
    if (strcmp(instructionCopy, "jalr") == 0) {
        return "001001";
    }
    if (strcmp(instructionCopy, "jr") == 0) {
        return "001000";
    }
    if (strcmp(instructionCopy, "mfhi") == 0) {
        return "010000";
    }
    if (strcmp(instructionCopy, "mflo") == 0) {
        return "010010";
    }
    if (strcmp(instructionCopy, "mthi") == 0) {
        return "010001";
    }
    if (strcmp(instructionCopy, "mtlo") == 0) {
        return "010011";
    }
    if (strcmp(instructionCopy, "mult") == 0) {
        return "011000";
    }
    if (strcmp(instructionCopy, "multu") == 0) {
        return "011001";
    }
    if (strcmp(instructionCopy, "nor") == 0) {
        return "100111";
    }
    if (strcmp(instructionCopy, "or") == 0) {
        return "100101";
    }
    if (strcmp(instructionCopy, "sll") == 0) {
        return "000000";
    }
    if (strcmp(instructionCopy, "sllv") == 0) {
        return "000100";
    }
    if (strcmp(instructionCopy, "slt") == 0) {
        return "101010";
    }
    if (strcmp(instructionCopy, "sltu") == 0) {
        return "101011";
    }
    if (strcmp(instructionCopy, "sra") == 0) {
        return "000011";
    }
    if (strcmp(instructionCopy, "srav") == 0) {
        return "000111";
    }
    if (strcmp(instructionCopy, "srl") == 0) {
        return "000010";
    }
    if (strcmp(instructionCopy, "srlv") == 0) {
        return "000110";
    }
    if (strcmp(instructionCopy, "sub") == 0) {
        return "100010";
    }
    if (strcmp(instructionCopy, "subu") == 0) {
        return "100011";
    }
    if (strcmp(instructionCopy, "syscall") == 0) {
        return "001100";
    }
    if (strcmp(instructionCopy, "xor") == 0) {
        return "100110";
    }

}

/**
 * Find machine code for opcode of I-Type
 * @param instructionCopy
 * @param type
 * @return machine code strigng
 */
char* findOpCode(char* instruction) {
    // Convert instruction pointer to char array
    char instructionCopy[6];
    strcpy(instructionCopy, instruction);

    // Search table for opcode of I-type format
    if (strcmp(instructionCopy, "addi") == 0) {
        return "001000";
    }
    if (strcmp(instructionCopy, "addiu") == 0) {
        return "001001";
    }
    if (strcmp(instructionCopy, "andi") == 0) {
        return "001100";
    }
    if (strcmp(instructionCopy, "beq") == 0) {
        return "000100";
    }
    if (strcmp(instructionCopy, "bgez") == 0) {
        return "000001";
    }
    if (strcmp(instructionCopy, "bgtz") == 0) {
        return "000111";
    }
    if (strcmp(instructionCopy, "blez") == 0) {
        return "000110";
    }
    if (strcmp(instructionCopy, "bltz") == 0) {
        return "000001";
    }
    if (strcmp(instructionCopy, "bne") == 0) {
        return "000101";
    }
    if (strcmp(instructionCopy, "lb") == 0) {
        return "100000";
    }
    if (strcmp(instructionCopy, "lbu") == 0) {
        return "100100";
    }
    if (strcmp(instructionCopy, "lh") == 0) {
        return "100001";
    }
    if (strcmp(instructionCopy, "lhu") == 0) {
        return "100101";
    }
    if (strcmp(instructionCopy, "lui") == 0) {
        return "001111";
    }
    if (strcmp(instructionCopy, "lw") == 0) {
        return "100011";
    }
    if (strcmp(instructionCopy, "lwcl") == 0) {
        return "110001";
    }
    if (strcmp(instructionCopy, "ori") == 0) {
        return "001101";
    }
    if (strcmp(instructionCopy, "sb") == 0) {
        return "101000";
    }
    if (strcmp(instructionCopy, "slti") == 0) {
        return "001010";
    }
    if (strcmp(instructionCopy, "sh") == 0) {
        return "101001";
    }
    if (strcmp(instructionCopy, "sw") == 0) {
        return "101011";
    }
    if (strcmp(instructionCopy, "swcl") == 0) {
        return "111001";
    }
    if (strcmp(instructionCopy, "xori") == 0) {
        return "001110";
    }
}