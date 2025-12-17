/****************************************************/
/* File: tm.c                                       */
/* The TM ("Tiny Machine") computer                 */
/* Source Code for the Compiler Construction        */
/* Text Book by Kenneth C. Louden                   */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/******* consts *******/
#define IADDR_SIZE 30000
#define DADDR_SIZE 30000
#define NO_REGS 8
#define PC_REG  7

#define LINESIZE 121
#define WORDSIZE 20

/******* type  *******/

typedef enum {
   opclRR,     /* reg operands r,s,t */
   opclRM,     /* reg r, mem d+s */
   opclRA      /* reg r, int d+s */
   } OPCLASS;

typedef enum {
   /* RR instructions */
   opHALT,    /* RR     halt, operands are ignored */
   opIN,      /* RR     read into reg(r); s and t are ignored */
   opOUT,     /* RR     write from reg(r), s and t are ignored */
   opADD,    /* RR     reg(r) = reg(s)+reg(t) */
   opSUB,    /* RR     reg(r) = reg(s)-reg(t) */
   opMUL,    /* RR     reg(r) = reg(s)*reg(t) */
   opDIV,    /* RR     reg(r) = reg(s)/reg(t) */
   opRRLim,   /* limit of RR opcodes */

   /* RM instructions */
   opLD,      /* RM     reg(r) = mem(d+reg(s)) */
   opST,      /* RM     mem(d+reg(s)) = reg(r) */
   opRMLim,   /* Limit of RM opcodes */

   /* RA instructions */
   opLDA,     /* RA     reg(r) = d+reg(s) */
   opLDC,     /* RA     reg(r) = d ; reg(s) is ignored */
   opJLT,     /* RA     if reg(r)<0 then reg(7) = d+reg(s) */
   opJLE,     /* RA     if reg(r)<=0 then reg(7) = d+reg(s) */
   opJGT,     /* RA     if reg(r)>0 then reg(7) = d+reg(s) */
   opJGE,     /* RA     if reg(r)>=0 then reg(7) = d+reg(s) */
   opJEQ,     /* RA     if reg(r)==0 then reg(7) = d+reg(s) */
   opJNE,     /* RA     if reg(r)!=0 then reg(7) = d+reg(s) */
   opRALim    /* Limit of RA opcodes */
   } OPCODE;

typedef enum {
   srOK,
   srIMEM_ERR,
   srDMEM_ERR,
   srZERODIVIDE
   } STEPRESULT;

typedef struct {
      int iop;
      int iarg1;
      int iarg2;
      int iarg3;
   } INSTRUCTION;

/******** vars ********/
int iloc = 0;
int dloc = 0;
int traceflag = FALSE;
int icountflag = FALSE;

INSTRUCTION iMem [IADDR_SIZE];
int dMem [DADDR_SIZE];
int reg [NO_REGS];

char * opCodeTab[] = {
   "HALT","IN","OUT","ADD","SUB","MUL","DIV","????",
   "LD","ST","????",
   "LDA","LDC","JLT","JLE","JGT","JGE","JEQ","JNE","????"
   };

char * stepResultTab[] = {
   "OK","IMEM_ERR","DMEM_ERR","ZERODIVIDE"
   };

char pgmName[20];
FILE *pgm;

char in_Line[LINESIZE];
int lineLen;
int inCol;
int num;
char word[WORDSIZE];
char ch;
int done;

/********************************************/
int opClass( int c ) {
   if ( c <= opRRLim ) return ( opclRR );
   else if ( c <= opRMLim ) return ( opclRM );
   else      return ( opclRA );
}

/********************************************/
void writeInstruction ( int loc ) {
   printf( "%5d: ", loc );
   if ( (loc >= 0) && (loc < IADDR_SIZE) ) {
      printf( "%6s%3d,", opCodeTab[iMem[loc].iop], iMem[loc].iarg1 );
      switch ( opClass(iMem[loc].iop) ) {
         case opclRR: printf( "%1d,%1d", iMem[loc].iarg2, iMem[loc].iarg3 );
                      break;
         case opclRM:
         case opclRA: printf( "%3d(%1d)", iMem[loc].iarg2, iMem[loc].iarg3 );
                      break;
         }
      printf( "\n" );
   }
}

/********************************************/
void getCh (void) {
   if ( ++inCol < lineLen )
      ch = in_Line[inCol];
   else ch = ' ';
}

/********************************************/
int nonBlank (void) {
   while ( (inCol < lineLen) && (in_Line[inCol] == ' ') ) inCol++;
   if ( inCol < lineLen ) {
      ch = in_Line[inCol];
      return TRUE;
   } else return FALSE;
}

/********************************************/
int getWord (void) {
   int temp;
   if ( !nonBlank () ) return FALSE;
   temp = 0;
   while ( (inCol < lineLen) && (in_Line[inCol] != ' ')  && (in_Line[inCol] != '\t') && (in_Line[inCol] != '\n') && (in_Line[inCol] != '\r') ) {
      if ( temp < WORDSIZE ) word[temp++] = in_Line[inCol];
      inCol++;
   }
   word[temp] = '\0';
   return TRUE;
}

/********************************************/
int getNum (void) {
   int sign;
   int term;
   int temp = FALSE;
   num = 0;
   if ( nonBlank () ) {
      sign = 1;
      if ( ch == '-' ) {
         sign = -1;
         getCh();
      }
      term = 0;
      while ( isdigit(ch) ) {
         term = term * 10 + ( ch - '0' );
         getCh();
      }
      num = num + ( term * sign );
      temp = TRUE;
   }
   return temp;
}

/********************************************/
int skipCh ( char c ) {
   int temp = FALSE;
   if ( nonBlank() ) {
      if ( ch == c ) {
         getCh();
         temp = TRUE;
      }
   }
   return temp;
}

/********************************************/
int atEOL(void) {
   return ( !nonBlank () );
}

/********************************************/
int error( char * msg, int lineNo, int instNo) {
   printf( "Line %d", lineNo );
   if ( instNo >= 0 ) printf( " (Instruction %d)", instNo );
   printf( "   %s\n", msg );
   return FALSE;
}

/********************************************/
int readInstructions (void) {
   OPCODE op;
   int arg1, arg2, arg3;
   int loc, regNo, lineNo;
   int i;
   for ( i = 0 ; i < IADDR_SIZE ; i++ ) iMem[i].iop = opHALT;
   for ( i = 0 ; i < DADDR_SIZE ; i++ ) dMem[i] = 0;
   for ( i = 0 ; i < NO_REGS ; i++ ) reg[i] = 0;
   dMem[0] = DADDR_SIZE - 1;
   lineNo = 0;
   while ( ! feof( pgm ) ) {
      fgets( in_Line, LINESIZE-2, pgm );
      inCol = 0;
      lineNo++;
      lineLen = strlen(in_Line)-1;
      if ( in_Line[lineLen]=='\n' ) in_Line[lineLen] = '\0';
      else in_Line[++lineLen] = '\0';
      if ( (nonBlank()) && (in_Line[inCol] != '*') ) {
         if ( !getNum() ) return error( "Bad location", lineNo, -1);
         loc = num;
         if ( loc > IADDR_SIZE ) return error( "Location too large", lineNo, loc);
         if ( !skipCh(':') ) return error( "Missing colon", lineNo, loc);
         if ( !getWord() ) return error( "Missing opcode", lineNo, loc);
         op = opHALT;
         while ( (op < opRALim) && (strncmp(opCodeTab[op], word, 4) != 0) )
            op++;
         if ( strncmp(opCodeTab[op], word, 4) != 0 ) return error( "Unknown opcode", lineNo, loc);
         iMem[loc].iop = op;
         switch ( opClass(op) ) {
            case opclRR:
               if ( (!getNum()) || (num < 0) || (num >= NO_REGS) )
                  return error( "Bad first register", lineNo, loc);
               arg1 = num;
               if ( !skipCh(',') ) return error( "Missing comma", lineNo, loc);
               if ( (!getNum()) || (num < 0) || (num >= NO_REGS) )
                  return error( "Bad second register", lineNo, loc);
               arg2 = num;
               if ( !skipCh(',') ) return error( "Missing comma", lineNo, loc);
               if ( (!getNum()) || (num < 0) || (num >= NO_REGS) )
                  return error( "Bad third register", lineNo, loc);
               arg3 = num;
               break;

            case opclRM:
            case opclRA:
               if ( (!getNum()) || (num < 0) || (num >= NO_REGS) )
                  return error( "Bad first register", lineNo, loc);
               arg1 = num;
               if ( !skipCh(',') ) return error( "Missing comma", lineNo, loc);
               if ( !getNum() ) return error( "Bad displacement", lineNo, loc);
               arg2 = num;
               if ( !skipCh('(') ) return error( "Missing LParen", lineNo, loc);
               if ( (!getNum()) || (num < 0) || (num >= NO_REGS) )
                  return error( "Bad second register", lineNo, loc);
               arg3 = num;
               break;
            }
         iMem[loc].iarg1 = arg1;
         iMem[loc].iarg2 = arg2;
         iMem[loc].iarg3 = arg3;
      }
   }
   return TRUE;
}

/********************************************/
STEPRESULT stepTM (void) {
   INSTRUCTION currentinstruction  = iMem[ reg[PC_REG] ];
   int pc;
   int r,s,t,m;
   int ok;

   pc = reg[PC_REG];
   if ( (pc < 0) || (pc > IADDR_SIZE)  )
      return srIMEM_ERR;
   reg[PC_REG] = pc + 1;
   r = currentinstruction.iarg1;
   s = currentinstruction.iarg2;
   t = currentinstruction.iarg3;
   switch ( opClass(currentinstruction.iop) ) {
      case opclRR:
         switch ( currentinstruction.iop ) {
            case opHALT:
               printf("HALT: %d,%d,%d\n",r,s,t);
               return srIMEM_ERR;
            case opIN:
               do {
                  printf("Enter value for IN instruction: ");
                  ////fflush (stdin);
                  ////fflush (stdout);
                  if (fgets(in_Line, LINESIZE-2, stdin) == NULL) return srIMEM_ERR;
                  lineLen = strlen(in_Line);
                  inCol = 0;
               } while ( !getNum() );
               reg[r] = num;
               break;
            case opOUT:
               printf("OUT instruction prints: %d\n", reg[r] );
               break;
            case opADD: reg[r] = reg[s] + reg[t]; \
                        break;
            case opSUB: reg[r] = reg[s] - reg[t]; \
                        break;
            case opMUL: reg[r] = reg[s] * reg[t]; \
                        break;
            case opDIV: if ( reg[t] != 0 ) reg[r] = reg[s] / reg[t]; \
                        else return srZERODIVIDE; \
                        break;
            default: break;
         }
         break;

      case opclRM:
         m = currentinstruction.iarg2 + reg[currentinstruction.iarg3];
         if ( (m < 0) || (m > DADDR_SIZE) )
            return srDMEM_ERR;
         switch ( currentinstruction.iop ) {
            case opLD: reg[r] = dMem[m]; \
                       break;
            case opST: dMem[m] = reg[r]; \
                       break;
            default: break;
         }
         break;

      case opclRA:
         m = currentinstruction.iarg2 + reg[currentinstruction.iarg3];
         switch ( currentinstruction.iop ) {
            case opLDA: reg[r] = m; \
                        break;
            case opLDC: reg[r] = currentinstruction.iarg2; \
                        break;
            case opJLT: if ( reg[r] <  0 ) reg[PC_REG] = m; \
                        break;
            case opJLE: if ( reg[r] <= 0 ) reg[PC_REG] = m; \
                        break;
            case opJGT: if ( reg[r] >  0 ) reg[PC_REG] = m; \
                        break;
            case opJGE: if ( reg[r] >= 0 ) reg[PC_REG] = m; \
                        break;
            case opJEQ: if ( reg[r] == 0 ) reg[PC_REG] = m; \
                        break;
            case opJNE: if ( reg[r] != 0 ) reg[PC_REG] = m; \
                        break;
            default: break;
         }
         break;
   }
   return srOK;
}

/********************************************/
int doCommand (void) {
   char cmd;
   int stepcnt=0, i;
   int printcnt;
   int stepResult;
   int loc;
   do {
      printf( "Enter command: " );
      ////fflush (stdin);
      ////fflush (stdout);
      
      if ( fgets(in_Line, LINESIZE-2, stdin) == NULL ) 
          return FALSE;
      
      lineLen = strlen(in_Line);
      inCol = 0;
      
      if ( !nonBlank() ) continue;
      
      cmd = ch; /* <--- CORREÇÃO CRÍTICA: Salva o comando digitado */
      
      switch ( ch ) {
         case 'h':
         case '?':
            printf("Commands are:\n");
            printf("   s(tep <n>      Execute n (default 1) TM instructions\n");
            printf("   g(o            Execute TM instructions until HALT\n");
            printf("   r(egs          Print the contents of the registers\n");
            printf("   i(Mem <b <n>>  Print n iMem locations starting at b\n");
            printf("   d(Mem <b <n>>  Print n dMem locations starting at b\n");
            printf("   t(race         Toggle instruction trace\n");
            printf("   p(rint         Toggle print of total instructions executed ('go' only)\n");
            printf("   c(lear         Reset simulator for new execution of program\n");
            printf("   h(elp          Cause this list of commands to be printed\n");
            printf("   q(uit          Terminate the simulation\n");
            break;
         case 't':
            traceflag = ! traceflag;
            printf("Tracing now ");
            if ( traceflag ) printf("on.\n"); else printf("off.\n");
            break;
         case 'p':
            icountflag = ! icountflag;
            printf("Printing instruction count now ");
            if ( icountflag ) printf("on.\n"); else printf("off.\n");
            break;
         case 's':
            if ( atEOL() ) stepcnt = 1;
            else if ( getNum() )  stepcnt = abs(num);
            else printf( "Step count?\n" );
            break;
         case 'g': stepcnt = 1; break;
         case 'r':
            for (i=0;i<NO_REGS;i++) {
               printf("%1d: %4d    ",i,reg[i]);
               if ( (i%4) == 3 ) printf("\n");
            }
            break;
         case 'i':
            printcnt = 1;
            if ( getNum() ) {
               loc = num;
               if ( getNum() ) printcnt = num;
            } else printf( "Instruction Locations?\n" );
            if ( (loc >= 0) && (loc < IADDR_SIZE) ) {
               int end = loc + printcnt;
               if ( end > IADDR_SIZE ) end = IADDR_SIZE;
               for ( i = loc; i < end; i++ )
                  writeInstruction(i);
            } else printf( "Bad Location\n" );
            break;
         case 'd':
            printcnt = 1;
            if ( getNum() ) {
               loc = num;
               if ( getNum() ) printcnt = num;
            } else printf( "Data Locations?\n" );
            if ( (loc >= 0) && (loc < DADDR_SIZE) ) {
               int end = loc + printcnt;
               if ( end > DADDR_SIZE ) end = DADDR_SIZE;
               for ( i = loc; i < end; i++ ) {
                  printf("%5d: %5d  ",i,dMem[i]);
                  if ( (i%4) == 3 ) printf("\n");
               }
            } else printf( "Bad Location\n" );
            break;
         case 'c':
            iloc = 0;
            dloc = 0;
            stepcnt = 0;
            for ( i = 0 ; i < IADDR_SIZE ; i++ ) iMem[i].iop = opHALT;
            for ( i = 0 ; i < DADDR_SIZE ; i++ ) dMem[i] = 0;
            for ( i = 0 ; i < NO_REGS ; i++ ) reg[i] = 0;
            dMem[0] = DADDR_SIZE - 1;
            printf( "Simulator reset.\n" );
            break;
         case 'q': return FALSE;
         default: printf( "Command %c unknown.\n", ch ); break;
      }
      stepResult = srOK;
      if ( stepcnt > 0 ) {
         if ( cmd == 'g' ) {
            stepcnt = 0;
            while ( stepResult == srOK ) {
               iloc = reg[PC_REG] ;
               if ( traceflag ) writeInstruction( iloc ) ;
               stepResult = stepTM () ;
               stepcnt++ ;
            }
            if ( icountflag )
               printf( "Number of instructions executed = %d\n",stepcnt );
         } else {
            while ( (stepcnt > 0) && (stepResult == srOK) ) {
               iloc = reg[PC_REG] ;
               if ( traceflag ) writeInstruction( iloc ) ;
               stepResult = stepTM () ;
               stepcnt-- ;
            }
         }
         printf( "%s\n",stepResultTab[stepResult] );
      }
   } while ( TRUE );
}

int main( int argc, char * argv[] )
{
   if ( argc != 2 ) {
      printf("usage: %s <filename>\n",argv[0]);
      exit(1);
   }
   strcpy(pgmName,argv[1]) ;
   if ( strchr (pgmName, '.') == NULL )
      strcat(pgmName,".tm");
   pgm = fopen(pgmName,"r");
   if ( pgm == NULL ) {
      printf("file '%s' not found\n",pgmName);
      exit(1);
   }

   if ( ! readInstructions() )
      exit(1);

   printf( "TM simulation (enter h for help)...\n" );
   doCommand();
   return 0;
}