#include "mips-small-pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************************************/
int main(int argc, char *argv[])
{
  short i;
  char line[MAXLINELENGTH];
  state_t state;
  FILE *filePtr;

  if (argc != 2)
  {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    return 1;
  }

  memset(&state, 0, sizeof(state_t));

  state.pc = state.cycles = 0;
  state.IFID.instr = state.IDEX.instr = state.EXMEM.instr = state.MEMWB.instr =
      state.WBEND.instr = NOPINSTRUCTION; /* nop */

  /* read machine-code file into instruction/data memory (starting at address 0)
   */

  filePtr = fopen(argv[1], "r");
  if (filePtr == NULL)
  {
    printf("error: can't open file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }

  for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
       state.numMemory++)
  {
    if (sscanf(line, "%x", &state.dataMem[state.numMemory]) != 1)
    {
      printf("error in reading address %d\n", state.numMemory);
      exit(1);
    }
    state.instrMem[state.numMemory] = state.dataMem[state.numMemory];
    printf("memory[%d]=%x\n", state.numMemory, state.dataMem[state.numMemory]);
  }

  printf("%d memory words\n", state.numMemory);

  printf("\tinstruction memory:\n");
  for (i = 0; i < state.numMemory; i++)
  {
    printf("\t\tinstrMem[ %d ] = ", i);
    printInstruction(state.instrMem[i]);
  }

  run(&state);

  return 0;
}
/************************************************************/

/************************************************************/
void run(Pstate state)
{
  state_t new;
  int p1;
  int instr_counter = 4;
  int op3;
  int s1;
  int isr;
  int s2;
  int p3;
  int op2;
  int i;
  int directive = 0;
  int readRegA, readRegB;
  int op1;
  int off;
  int p2;
  int funct;
  int tracker = 0;
  int count = 0;
  int set_to_zero = 0;
  int flag = 1;
  int flag2 = 1;
  int currentOP = 0;
  int err_code = 3939393;
  int opctemp;
  int fieldtemp3;
  int fieldtemp2;
  int operationtemp;
  int flag3 = 1;
  int flag4 = 1;

  memset(&new, 0, sizeof(state_t));

  while (1)
  {

    printState(state);

    memcpy(&new, state, sizeof(state_t));

    new.cycles++;

    /* --------------------- IF stage --------------------- */
    new.IFID.instr = new.instrMem[new.pc / instr_counter];
    new.pc += instr_counter;
    isr = new.IFID.instr;
    opctemp = (opcode(new.IFID.instr));

    if (opctemp != BEQZ_OP)
    {
      new.IFID.pcPlus1 = new.pc;
    }
    else
    {
      if (1)
      {
        off = offset(isr);
        if (off <= 0)
        {
          new.pc += off;
          new.IFID.pcPlus1 = new.pc - off;
        }
        else
        {
          new.IFID.pcPlus1 = new.pc;
        }
      }
      else
      {
        for (i = 0; i < instr_counter; i++)
        {
          tracker += i * directive;
          count++;
        }
      }
    }

    /* --------------------- ID stage --------------------- */
    new.IDEX.instr = state->IFID.instr;
    isr = new.IDEX.instr;

    tracker++;
    new.IDEX.readRegB = new.reg[field_r2(isr)];
    new.IDEX.offset = offset(isr);
    new.IDEX.readRegA = new.reg[field_r1(isr)];
    new.IDEX.pcPlus1 = state->IFID.pcPlus1;
    opctemp = opcode(state->IDEX.instr);
    if (flag3)
    {
      if (opctemp == LW_OP)
      {
        if ((opcode(isr) != HALT_OP))
        {
          if (field_r1(isr) == field_r2(state->IDEX.instr))
          {
            new.IDEX.readRegB = set_to_zero;
            new.IDEX.instr = NOPINSTRUCTION;
            new.IDEX.pcPlus1 = set_to_zero;
            new.IDEX.readRegA = set_to_zero;
            new.IDEX.offset = offset(NOPINSTRUCTION);
            new.IFID.instr = state->IFID.instr;
            isr = new.IFID.instr;
            new.pc -= instr_counter;
            new.IFID.pcPlus1 = new.pc;
          }
          else
          {
            count++;
          }
        }
        if ((opcode(isr) == REG_REG_OP))
        {
          if ((field_r2(isr) == field_r2(state->IDEX.instr)))
          {
            new.IDEX.readRegA = set_to_zero;
            new.IDEX.offset = offset(NOPINSTRUCTION);
            new.IDEX.readRegB = set_to_zero;
            new.IDEX.pcPlus1 = set_to_zero;
            new.IDEX.instr = NOPINSTRUCTION;
            new.IFID.instr = state->IFID.instr;
            isr = state->IFID.instr;
            new.pc -= instr_counter;
            new.IFID.pcPlus1 = new.pc;
          }
          else if ((field_r1(isr) == field_r2(state->IDEX.instr)))
          {
            new.IDEX.readRegA = set_to_zero;
            new.IDEX.readRegB = set_to_zero;
            new.IDEX.pcPlus1 = set_to_zero;
            new.IDEX.offset = offset(NOPINSTRUCTION);
            new.IFID.instr = state->IFID.instr;
            new.IDEX.instr = NOPINSTRUCTION;

            isr = state->IFID.instr;
            new.pc -= instr_counter;
            new.IFID.pcPlus1 = new.pc;
          }
          else
          {
            tracker++;
          }
        }
      }
      else
      {
        for (i = 0; i < instr_counter; i++)
        {
          tracker -= i * directive;
          count += instr_counter;
        }
        /*
        printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
         offset(instr));
         */
      }
    }

    /* --------------------- EX stage --------------------- */

    new.EXMEM.instr = state->IDEX.instr;
    isr = new.EXMEM.instr;
    funct = func(isr);
    readRegB = new.reg[field_r2(isr)];
    opctemp = (opcode(new.IFID.instr));
    readRegA = new.reg[field_r1(isr)];
    p3 = state->WBEND.instr;
    p2 = state->MEMWB.instr;
    p1 = state->EXMEM.instr;
    op3 = opcode(p3);
    op2 = opcode(p2);
    op1 = opcode(p1);

    s1 = field_r1(isr);
    s2 = field_r2(isr);
    if (op3 != opcode(NOPINSTRUCTION))
    {
      fieldtemp3 = field_r3(p3);
      fieldtemp2 = field_r2(p3);
      /*
      printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
       offset(instr));
       */
      if (op3 == BEQZ_OP)
      {
        if (s1 != 0 && s1 == fieldtemp2)
        {
          readRegA = state->WBEND.writeData;
          count++;
        }
        if (s2 != 0 && s2 == fieldtemp2)
        {
          readRegB = state->WBEND.writeData;
          count--;
        }
      }
      else if (op3 == LW_OP)
      {
        if (s1 == fieldtemp2 && s1 != 0)
        {
          readRegA = state->WBEND.writeData;
          tracker++;
        }
        if (s2 == fieldtemp2 && s2 != 0)
        {
          readRegB = state->WBEND.writeData;
          tracker--;
        }
      }
      else if ((op3 == ADDI_OP))
      {
        if (s1 == fieldtemp2 && s1 != 0)
        {
          readRegA = state->WBEND.writeData;
        }
        if (s2 == fieldtemp2 && s2 != 0)
        {
          readRegB = state->WBEND.writeData;
        }
      }
      else if (op3 == REG_REG_OP)
      {

        if (s1 == fieldtemp3 && s1 != 0)
        {
          readRegA = state->WBEND.writeData;
        }
        if (s2 == fieldtemp3 && s2 != 0)
        {
          readRegB = state->WBEND.writeData;
        }
      }

      else if (op3 == SW_OP)
      {
        if (s2 != 0 && s2 == fieldtemp2)
        {
          readRegB = state->EXMEM.aluResult;
        }
      }
      else
      {
        tracker++;
        /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
      }
    }
    if (op2 != opcode(NOPINSTRUCTION))
    {
      fieldtemp3 = field_r3(p2);
      fieldtemp2 = field_r2(p2);

      if (op2 == ADDI_OP && 1)
      {

        if (s1 == fieldtemp2 && s1 != 0)
        {
          readRegA = state->MEMWB.writeData;
        }
        if (s2 == fieldtemp2 && s2 != 0)
        {
          readRegB = state->MEMWB.writeData;
        }
        else
        {
          count--;
        }
      }
      else if (op2 == LW_OP)
      {
        if (s1 == fieldtemp2 && s1 != 0)
        {
          readRegA = state->MEMWB.writeData;
          count = err_code;
        }
        if (s2 == fieldtemp2 && s2 != 0)
        {
          readRegB = state->MEMWB.writeData;
        }
      }
      else if (op2 == REG_REG_OP)
      {
        if (s1 == fieldtemp3 && s1 != 0)
        {
          readRegA = state->MEMWB.writeData;
          count++;
        }
        if (s2 == fieldtemp3 && s2 != 0)
        {
          readRegB = state->MEMWB.writeData;
          count--;
        }
      }
      else if (op2 == BEQZ_OP)
      {
        if (s1 == fieldtemp2 && s1 != 0)
        {
          readRegA = state->MEMWB.writeData;
        }
        if (s2 == fieldtemp2 && s2 != 0)
        {
          readRegB = state->MEMWB.writeData;
        }
      }
      else
      {
        count++;
        /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
      }
    }
    if (op1 == REG_REG_OP)
    {

      if (s1 == field_r3(p1) && s1 != 0)
      {
        readRegA = state->EXMEM.aluResult;
      }
      if (s2 == field_r3(p1) && s2 != 0)
      {
        readRegB = state->EXMEM.aluResult;
      }
    }
    else if (op1 != opcode(NOPINSTRUCTION))
    {
      if (op1 == ADDI_OP)
      {
        if (s1 == field_r2(p1) && s1 != 0)
        {
          readRegA = state->EXMEM.aluResult;
          count--;
        }
        if (s2 == field_r2(p1) && s2 != 0 && opcode(isr) != op1)
        {
          readRegB = state->EXMEM.aluResult;
        }
      }
      else if (op1 == LW_OP)
      {
        if (s1 == field_r2(p1) && s1 != 0)
        {
          readRegA = state->EXMEM.aluResult;
        }
        if (s2 == field_r2(p1) && s2 != 0 && opcode(isr) != op1)
        {
          readRegB = state->EXMEM.aluResult;
        }
      }
      else if (op1 == BEQZ_OP)
      {
        if (s1 == field_r2(p1) && s1 != 0)
        {
          readRegA = state->EXMEM.aluResult;
        }
        if (s2 == field_r2(p1) && s2 != 0 && opcode(isr) != op1)
        {
          readRegB = state->EXMEM.aluResult;
        }
      }
      else
      {
        tracker++;
        /* come back here later */
        /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
      }
    }
    else
    {
      count++;
    }
    if (flag4)
    {
      if (opcode(isr) == REG_REG_OP)
      {
        if (isr == NOPINSTRUCTION)
        {
          new.EXMEM.readRegB = 0;
          new.EXMEM.aluResult = 0;
        }
        else if (funct == OR_FUNC)
        {
          operationtemp = readRegA | readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
        else if (funct == ADD_FUNC)
        {
          operationtemp = readRegA + readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
        else if (funct == SRL_FUNC)
        {
          operationtemp = ((unsigned int)readRegA) >> readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
        else if (funct == AND_FUNC)
        {
          operationtemp = readRegA & readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
        else if (funct == SLL_FUNC && 1)
        {
          operationtemp = readRegA << readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
        else if (funct == SUB_FUNC)
        {
          operationtemp = readRegA - readRegB;
          new.EXMEM.aluResult = operationtemp;
          new.EXMEM.readRegB = readRegB;
        }
      }
      else if (opcode(isr) == ADDI_OP)
      {
        operationtemp = readRegA + offset(isr);
        new.EXMEM.aluResult = operationtemp;
        new.EXMEM.readRegB = state->IDEX.readRegB;
      }
      else if (opcode(isr) == LW_OP)
      {
        operationtemp = readRegA + field_imm(isr);
        new.EXMEM.aluResult = operationtemp;
        new.EXMEM.readRegB = new.reg[field_r2(isr)];
      }
      else if (opcode(isr) == SW_OP)
      {
        operationtemp = readRegA + field_imm(isr);
        new.EXMEM.readRegB = readRegB;
        new.EXMEM.aluResult = operationtemp;
        tracker = 32;
      }
      else if (opcode(isr) == BEQZ_OP)
      {
        if (flag)
        {
        }
        if ((state->IDEX.offset > 0 && readRegA == 0))

        {
          new.IDEX.instr = NOPINSTRUCTION;
          new.IFID.instr = NOPINSTRUCTION;
          new.pc = state->IDEX.offset + state->IDEX.pcPlus1;
          new.IFID.pcPlus1 = 0;
          new.IDEX.readRegA = 0;
          new.IDEX.readRegB = 0;
          new.IDEX.pcPlus1 = 0;
          tracker--;
          new.IDEX.offset = 32;
          new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(isr);
          new.EXMEM.readRegB = new.reg[field_r2(isr)];
        }
        else if (state->IDEX.offset < 0 && readRegA != 0)
        {
          new.IDEX.instr = NOPINSTRUCTION;
          new.IFID.instr = NOPINSTRUCTION;
          new.pc = state->IDEX.offset + state->IDEX.pcPlus1;
          new.IFID.pcPlus1 = 0;
          new.IDEX.readRegA = 0;
          new.IDEX.readRegB = 0;
          new.IDEX.pcPlus1 = 0;
          tracker++;
          new.IDEX.offset = 32;
          new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(isr);
          new.EXMEM.readRegB = new.reg[field_r2(isr)];
        }
        else
        {
          new.EXMEM.readRegB = new.reg[field_r2(isr)];
          new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(isr);
        }
      }
      else
      {
        new.EXMEM.readRegB = 0;
        new.EXMEM.aluResult = 0;
      }
    }

    /* --------------------- MEM stage --------------------- */

    new.MEMWB.instr = state->EXMEM.instr;
    isr = new.MEMWB.instr;
    currentOP = opcode(isr);
    if (flag)
    {
      if (currentOP == LW_OP)
      {
        new.MEMWB.writeData = state->dataMem[state->EXMEM.aluResult / instr_counter];
      }
      else if (currentOP == REG_REG_OP || currentOP == ADDI_OP || currentOP == BEQZ_OP)
      {
        new.MEMWB.writeData = state->EXMEM.aluResult;
      }
      else if (currentOP == SW_OP && 1)
      {
        count--;
        new.MEMWB.writeData = state->EXMEM.readRegB;
        new.dataMem[(field_imm(isr) + new.reg[field_r1(isr)]) / instr_counter] = new.MEMWB.writeData;
      }
      else if (currentOP == HALT_OP)
      {
        tracker++;
        new.MEMWB.writeData = 0;
        tracker = 0;
      }
      else if (currentOP == err_code)
      {
        new.MEMWB.writeData = state->EXMEM.readRegB;
        new.dataMem[(field_imm(isr) + new.reg[field_r1(isr)]) / instr_counter] = new.MEMWB.writeData;
        tracker = 0;
        printf("HELLO HERE!!!!");
      }
      else
      {
        tracker++;
        /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
      }
    }

    /* --------------------- WB stage --------------------- */

    new.WBEND.instr = state->MEMWB.instr;
    isr = new.WBEND.instr;
    currentOP = opcode(isr);

    if (flag2)
    {
      if (currentOP == BEQZ_OP || currentOP == SW_OP)
      {
        new.WBEND.writeData = state->MEMWB.writeData;
      }
      else if (currentOP == ADDI_OP || currentOP == LW_OP)
      {
        new.reg[field_r2(isr)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;
      }
      else if (currentOP == REG_REG_OP)
      {
        new.reg[field_r3(isr)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;
      }
      else if (currentOP == HALT_OP)
      {
        printf("machine halted\n");
        printf("total of %d cycles executed\n", state->cycles);
        exit(0);
      }
      else if (currentOP == err_code)
      {
        new.reg[field_r3(isr)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;
        printf("HELLO HERE!!!!");
      }
    }
    else
    {
      tracker++;
      /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
    }

    /* --------------------- end stage --------------------- */
    readRegB = 0;
    count++;
    instr_counter = 4;
    readRegA = 0;
    /* transfer new state into current state */
    /*
printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
 offset(instr));
 */
    memcpy(state, &new, sizeof(state_t));
  }
}
/************************************************************/

/************************************************************/
int opcode(int instruction) { return (instruction >> OP_SHIFT) & OP_MASK; }
/************************************************************/

/************************************************************/
int func(int instruction) { return (instruction & FUNC_MASK); }
/************************************************************/

/************************************************************/
int field_r1(int instruction) { return (instruction >> R1_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r2(int instruction) { return (instruction >> R2_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r3(int instruction) { return (instruction >> R3_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_imm(int instruction) { return (instruction & IMMEDIATE_MASK); }
/************************************************************/

/************************************************************/
int offset(int instruction)
{
  /* only used for lw, sw, beqz */
  return convertNum(field_imm(instruction));
}
/************************************************************/

/************************************************************/
int convertNum(int num)
{
  /* convert a 16 bit number into a 32-bit Sun number */
  if (num & 0x8000)
  {
    num -= 65536;
  }
  return (num);
}
/************************************************************/

/************************************************************/
void printState(Pstate state)
{
  short i;
  printf("@@@\nstate before cycle %d starts\n", state->cycles);
  printf("\tpc %d\n", state->pc);

  printf("\tdata memory:\n");
  for (i = 0; i < state->numMemory; i++)
  {
    printf("\t\tdataMem[ %d ] %d\n", i, state->dataMem[i]);
  }
  printf("\tregisters:\n");
  for (i = 0; i < NUMREGS; i++)
  {
    printf("\t\treg[ %d ] %d\n", i, state->reg[i]);
  }
  printf("\tIFID:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IFID.instr);
  printf("\t\tpcPlus1 %d\n", state->IFID.pcPlus1);
  printf("\tIDEX:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IDEX.instr);
  printf("\t\tpcPlus1 %d\n", state->IDEX.pcPlus1);
  printf("\t\treadRegA %d\n", state->IDEX.readRegA);
  printf("\t\treadRegB %d\n", state->IDEX.readRegB);
  printf("\t\toffset %d\n", state->IDEX.offset);
  printf("\tEXMEM:\n");
  printf("\t\tinstruction ");
  printInstruction(state->EXMEM.instr);
  printf("\t\taluResult %d\n", state->EXMEM.aluResult);
  printf("\t\treadRegB %d\n", state->EXMEM.readRegB);
  printf("\tMEMWB:\n");
  printf("\t\tinstruction ");
  printInstruction(state->MEMWB.instr);
  printf("\t\twriteData %d\n", state->MEMWB.writeData);
  printf("\tWBEND:\n");
  printf("\t\tinstruction ");
  printInstruction(state->WBEND.instr);
  printf("\t\twriteData %d\n", state->WBEND.writeData);
}
/************************************************************/

/************************************************************/
void printInstruction(int instr)
{

  if (opcode(instr) == REG_REG_OP)
  {

    if (func(instr) == ADD_FUNC)
    {
      print_rtype(instr, "add");
    }
    else if (func(instr) == SLL_FUNC)
    {
      print_rtype(instr, "sll");
    }
    else if (func(instr) == SRL_FUNC)
    {
      print_rtype(instr, "srl");
    }
    else if (func(instr) == SUB_FUNC)
    {
      print_rtype(instr, "sub");
    }
    else if (func(instr) == AND_FUNC)
    {
      print_rtype(instr, "and");
    }
    else if (func(instr) == OR_FUNC)
    {
      print_rtype(instr, "or");
    }
    else
    {
      printf("data: %d\n", instr);
    }
  }
  else if (opcode(instr) == ADDI_OP)
  {
    print_itype(instr, "addi");
  }
  else if (opcode(instr) == LW_OP)
  {
    print_itype(instr, "lw");
  }
  else if (opcode(instr) == SW_OP)
  {
    print_itype(instr, "sw");
  }
  else if (opcode(instr) == BEQZ_OP)
  {
    print_itype(instr, "beqz");
  }
  else if (opcode(instr) == HALT_OP)
  {
    printf("halt\n");
  }
  else
  {
    printf("data: %d\n", instr);
  }
}
/************************************************************/

/************************************************************/
void print_rtype(int instr, const char *name)
{
  printf("%s %d %d %d\n", name, field_r3(instr), field_r1(instr),
         field_r2(instr));
}
/************************************************************/

/************************************************************/
void print_itype(int instr, const char *name)
{
  printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
         offset(instr));
}
/************************************************************/
