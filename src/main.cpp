#include "mbed.h"
//#include <iostream>
//#include <stdlib.h>
#include "TextLCD.h"
#include <stack>        //Biblioteca Pilha
#include "string.h"

#include "clp.h"

using namespace std;
int tamanho = 0; //Colocar la em cima
int PC;
unsigned int funcao_retorno;
Serial pc(USBTX, USBRX, 9600); // tx, rx
char buff[20];
TextLCD lcd(PTE20, PTE21, PTE22,PTE23, PTE29, PTE30); // rs, e, d4-d7
/*
  BusIn in(PTD0, PTD1, PTD2, PTD3, PTD4, PTD5, PTD6, PTD7);
  BusOut out(PTB0, PTB1, PTB2, PTB3, PTB4, PTB5, PTB6, PTB7);
*/
DigitalIn i0(PTA4);
DigitalIn i1(PTA5);
DigitalOut q0(LED1);
DigitalOut led(LED2);
DigitalOut ledFisico(PTD3);
//DigitalOut azul(LED3);
char acumulador;
char M[8],I[8],Q[8];
char PV //Present Value - Para Contador
char PT //Present Time - Para Temporizador
//char M,I,Q;
stack<int> pilha;
stack<int> pilha_logica;


typedef struct{
    char rotulo[17], operador[4], modificador[3], operando[17];
}instrucao;
instrucao *programa;

// Instrução LD ---------------------------------------------------------------
void inst_acum(int operador, int valor){
  switch(operador){
  case ANDv: acumulador = acumulador && valor; //AND
    break;
  case ORv: acumulador = acumulador || valor; //OR
    break;
  case XORv: acumulador = acumulador ^ valor; //XOR
    break;
  case GTv: acumulador = acumulador > valor; //GT - Maior que
    break;
  case GEv: acumulador = acumulador >= valor; //GE - Maior ou igual a
    break;
  case EQv: acumulador = acumulador == valor; //EQ - igual a
    break;
  case NEv: acumulador = acumulador != valor; //NE - Diferente de
    break;
  case LTv: acumulador = acumulador < valor;  //LT - Menor que
    break;
  case LEv: acumulador = acumulador <= valor; //LE - Menor ou igual a
    break;
  case LDv: acumulador = valor; //LD - LOAD
    break;
  }
}
// Instrução ST ---------------------------------------------------------------
void store_value(int operando, char value){
  if(operando&64){
    M[operando&7] = value;
  }
  if(operando&32){
    Q[operando&7] = value;
  }
  if(operando&16){
    I[operando&7] = value;
  }
}

int readVal(int operando){
  if(operando&64){
    return M[operando&7];
  }
  if(operando&32){
    //pc.printf("Lendo saida\n");
    return Q[operando&7];
  }
  if(operando&16){
    //pc.printf("Lendo entrada\n");
    return I[operando&7];
  }
}

// Instrucao OpA---------------------------------------------------------------
// Operacao adidada - Sempre que o caractere ')' for encontrado, esta instrucao
// deve ser chamada
void OpA (int operando, int mod){
  switch(pilha_logica.top()){
    case AND: acumulador=pilha.top() && acumulador;
      break;
    case OR: acumulador=pilha.top() || acumulador;
      break;
    case XOR: acumulador=pilha.top() ^ acumulador;
      break;
    case GT: acumulador=pilha.top() > acumulador;
      break;
    case GE: acumulador=pilha.top() >= acumulador;
      break;
    case EQ: acumulador=pilha.top() == acumulador;
      break;
    case NE: acumulador=pilha.top() != acumulador;
      break;
    case LT: acumulador=pilha.top() < acumulador;
      break;
    case LE: acumulador=pilha.top() <= acumulador;
      break;
    }
    pilha.pop();
    pilha_logica.pop();
  }
/*if(pilha_logica.top()  == ANDv)
    acumulador=pilha.pop() && acumulador;
  else if(pilha_logica.top() == ORv)
    acumulador=pilha.pop() || acumulador;
  else if(pilha_logica.top() == XORv)
    acumulador=pilha.pop() ^ acumulador;
  else if(pilha_logica.top() == GTv)
    acumulador=pilha.pop() > acumulador;
  else if(pilha_logica.top() == GEv)
    acumulador=pilha.pop() >= acumulador;
  else if(pilha_logica.top() == EQv)
    acumulador=pilha.pop() == acumulador;
  else if(pilha_logica.top() == NEv)
    acumulador=pilha.pop() != acumulador;
  else if(pilha_logica.top() == LTv)
    acumulador=pilha.pop() < acumulador;
  else if(pilha_logica.top() == LEv)
    acumulador=pilha.pop() <= acumulador;
    */
// Instrução JMP (JUMP - Desvia para o rótulo Nome_do_Rótulo)
void JMP (int mod, char operando[17]){
  unsigned int i;
  funcao_retorno=PC; // Esta variavel salva onde o programa estava quando JMP foi chamado
  if(mod&2){
    if(acumulador != 0){
      for(i = 0; i < tamanho; i++){
        if(strcmp(programa[i].rotulo, operando) == 0){
          //pc.printf("Salto C\n");
          PC = i-1;
          return;
        }
      }
    }
  }
  else if(mod&1){
    if(acumulador == 0){
      for(i = 0; i < tamanho; i++){
        if(strcmp(programa[i].rotulo, operando) == 0){
          //pc.printf("Salto N\n");
          PC = i-1;
          return;
        }
      }
    }
  }
  else{
    for(i = 0; i < tamanho; i++){
      if(strcmp(programa[i].rotulo, operando) == 0){
        //pc.printf("Salto sem mod\n");
        PC = i-1;
        return;
      }
    }
  }
}
// Instrução RET (Retorna de uma função ou bloco de função)--------------------
void RET (int mod){
  if(mod&2){
    if(acumulador != 0)
      PC=funcao_retorno;
  }
  if (mod&1){
    if(acumulador == 0)
      PC=funcao_retorno;
  }
}
// Instrução CTU (Contador Crescente)------------------------------------------
// Para CTU - Entradas: CU, R, PV ; Saídas: CV, Q------------------------------
void CTU (int mod, char PV){
  char CU; //COUNT UP - Entrada de contagem crescente
  char R; //RESET - Reinicia o contador, faz CV=0
          //PV - PRESENT VALUE - Valor do Limite superior de contagem
  unsigned int CV; //COUTER VALUE - Contem o valor acumulado de contagem
  char Q; //QUIT - Saída energizada quando CV>=PV
  CU=I[0];
  R=I[1];
  Q=Q[0];
  CV=0;
  if ((mod&2) && (acumulador != 0)){
    while (Q == 0) {
      if (CU != 0)
        CV=CV+1;
      if (CV >= PV)
        Q=1;
      if (R != 0)
        CV=0;
    }
  }
  else if ((mod&1) && (acumulador == 0){
    while (Q == 0) {
        if (CU != 0)
          CV=CV+1;
        if (CV >= PV)
          Q=1;
        if (R != 0)
          CV=0;
    }
  }else{
    while (Q == 0) {
        if (CU != 0)
          CV=CV+1;
        if (CV >= PV)
          Q=1;
        if (R != 0)
          CV=0;
  }
}
// Instrução CTD (Contador Decrescente)----------------------------------------
// Para CTD - Entradas: CD, LD, PV ; Saídas: CV, Q-----------------------------
void CTD (char mod, char PV){
  char CD; //COUNT DOWN - Entrada de contagem decrescente
  char LD; //LOAD - Reinicia o contador, faz CV=PV
           //PV - PRESENT VALUE - Valor desejado de contagem
  unsigned int CV; //COUTER VALUE - Contem o valor acumulado de contagem
  char Q; //QUIT - Saída energizada quando CV<=0
  CD=I[0];
  LD=I[1];
  CV=PV;
  Q=Q[0];
  if ((mod&2) && (acumulador != 0)){
    while (Q == 0) {
      if (CD != 0)
        CV=CV-1;
      if (CV <= 0)
        Q=1;
      if (LD != 0)
        CV=PV;
    }
  }
  else if ((mod&1) && (acumulador == 0){
    while (Q == 0) {
      if (CD != 0)
        CV=CV-1;
      if (CV <= 0)
        Q=1;
      if (LD != 0)
        CV=PV;
    }
  }else{
  while (Q == 0) {
    if (CD != 0)
      CV=CV-1;
    if (CV <= 0)
      Q=1;
    if (LD != 0)
      CV=PV;
  }
}
// Instrução CTUD (Contador Bidirecional)--------------------------------------
// Para CTUD - Entradas: CU, CD, R, LD, PV ; Saídas: CV, QU, QD----------------
void CTUD (char mod, char PV){
  char CU; //COUNT UP - Entrada de contagem crescente
  char CD; //COUNT DOWN - Entrada de contagem decrescente
  char R; //RESET - Reinicia o contador, faz CV=0
  char LD; //LOAD - Reinicia o contador, faz CV=PV
           //PV - PRESENT VALUE - Valor desejado de contagem
  char QU; //QUIT - Saída limite superior energizada quando CV>=PV
  char QD; //QUIT - Saída limite inferior energizada quando CV<=0
  unsigned int CV; //COUTER VALUE - Contem o valor acumulado de contagem
  CU=I[0];
  CD=I[1];
  R=I[2];
  LD=I[3];
  CV=0;
  QU=Q[0];
  QD=Q[1];
  if ((mod&2) && (acumulador != 0)){
    while ((QU == 0)||(QD == 0)) {
      if (CU != 0)
        CV=CV+1;
      if (CD != 0)
        CV=CV-1;
      if (CV >= PV)
        QU=1;
      if (CV == 0)
        QD=1;
      if (LD =! 0)
        CV=0;
      if (R =! 0)
        CV=PV;
    }
  }
  else if ((mod&1) && (acumulador == 0){
    while ((QU == 0)||(QD == 0)) {
      if (CU != 0)
        CV=CV+1;
      if (CD != 0)
        CV=CV-1;
      if (CV >= PV)
        QU=1;
      if (CV == 0)
        QD=1;
      if (LD =! 0)
        CV=0;
      if (R =! 0)
        CV=PV;
    }
  }else{
    while ((QU == 0)||(QD == 0)) {
      if (CU != 0)
        CV=CV+1;
      if (CD != 0)
        CV=CV-1;
      if (CV >= PV)
        QU=1;
      if (CV == 0)
        QD=1;
      if (LD =! 0)
        CV=0;
      if (R =! 0)
        CV=PV;
    }
  }
}
// Intrução TP (Temporizador de pulso)-----------------------------------------
void TP (char PT){
  char IN
  IN=I[0];
  if (IN != 0){
    Q=1;
    wait(PT)
    Q=0;
  }
}
// Instrução TON (Temporizador com retardo para ligar)-------------------------
void TON (char PT){
  char IN
  IN=I[0];
  if (IN != 0){
    wait(PT)
    Q=1;
  }else{
    Q=0;
  }
}
// Instução TOF (Temporizador com retardo para desligar)-----------------------
void TOF (char PT){
  char IN
  IN=I[0];
  if (IN != 0){
    Q=1;
  }else{
    wait(PT)
    Q=0;
  }
}
// Converte o operador para valores numéricos
int val_operador(char operador[4]){
  if(strcmp(operador,"LD  ")==0)
    return LDv;
  if(strcmp(operador,"ST  ")==0)
    return STv;
  if(strcmp(operador,"S   ")==0)
    return Sv;
  if(strcmp(operador,"R   ")==0)
    return Rv;
  if(strcmp(operador,"AND ")==0)
    return ANDv;
  if(strcmp(operador,"OR  ")==0)
    return ORv;
  if(strcmp(operador,"XOR ")==0)
    return XORv;
  if(strcmp(operador,"GT  ")==0)
    return GTv;
  if(strcmp(operador,"GE  ")==0)
    return GEv;
  if(strcmp(operador,"EQ  ")==0)
    return EQv;
  if(strcmp(operador,"NE  ")==0)
    return NEv;
  if(strcmp(operador,"LT  ")==0)
    return LTv;
  if(strcmp(operador,"LE  ")==0)
    return LEv;
  if(strcmp(operador,")   ")==0)
    return OpAv;
  if(strcmp(operador,"JMP ")==0)
    return JMPv;
  if(strcmp(operador,"RET ")==0)
    return RETv;
  if(strcmp(operador,"CTU ")==0)
    return CTUv;
  if(strcmp(operador,"CTD ")==0)
    return CTDv;
  if(strcmp(operador,"CTUD")==0)
    return CTUDv;
  if(strcmp(operador,"TP  ")==0)
    return TPv;
  if(strcmp(operador,"TON ")==0)
    return TONv;
  if(strcmp(operador,"TOF ")==0)
    return TOFv;
  else
    return 0;
}

// Converte o operando para valores numéricos
int val_operando(char operando[3]){
  int offset = 0;
  switch(operando[0]){
    case 'I':
    case 'i':offset = 16;
            break;
    case 'Q':
    case 'q':offset = 32;
            break;
    case 'M':
    case 'm':offset = 64;
            break;
    default: offset = 0;
  }
  return offset + (int)operando[1] - 48; //48 é o código asciii para 0.
}

// Converte o modificador para valores numéricos
// bit 1 - N, bit 2 - C, bit 3 - (
int val_mod(char mod[3]){
  int valor = 0;
  for(int i= 0;i<3;i++){
    switch(mod[i]){
      case 'N':
        valor = valor | 1;
        break;
      case 'C':
        valor = valor | 2;
        break;
      case '(':
        valor = valor | 4;
        break;
    }
    return valor;
  }
}


void executa_instrucao(instrucao x){
  int operando = val_operando(x.operando);
  int mod = val_mod(x.modificador);
  int operador = val_operador(x.operador);
  // Obtem o valor a ser trabalhado
  int valor = readVal(operando);
  if(mod&1){ // se houver o modificador N
    valor = !valor; // inverte o valor
  }
  //pc.printf("%d,%d,%d,%d\n",operador,mod,operando,valor);
  //Checa se é uma instrução adiada
  if(mod&4){
    pilha.push(acumulador);
    pilha_logica.push(operador);
  } else {
    switch(operador){
    case ANDv:
    case ORv:
    case XORv:
    case GTv:
    case GEv:
    case EQv:
    case NEv:
    case LTv:
    case LEv:
    case LDv: inst_acum(operador, valor);
           break;
    case Sv: store_value(operando, (char)1);
          break;
    case Rv: store_value(operando, (char)0);
          break;
    case STv: valor = mod&1? !acumulador:acumulador;
              store_value(operando, valor);
              break;
    case OpAv: OpA(operando, mod);
           break;
    case JMPv: JMP(mod, x.operando);
           break;
    case RETv: RET(mod);
           break;
    case CTUv: CTU(mod, x.PV);
           break;
    case CTDv: CTD(mod, x.PV);
           break;
    case CTUDv: CTUD(mod, x.PV);
           break;
    case TPv: TP(x.PT);
           break;
    case TONv: TON(x.PT);
           break;
    case TOFv: TOF(x.PT);
          break;
    }
  }
}
void upload(){
  int  i,
  tempo_entre_campos = 1;
  led = 0;      // led vermelho (transferindo dados
  pc.scanf("%d", &tamanho);
  //lcd.printf("Tam : %d\n",tamanho);
  programa = (instrucao*)malloc(tamanho*sizeof(instrucao));
  //wait(1);
  //lcd.cls();
  for(i = 0; i < tamanho; i++){
    pc.scanf("%[^\n]", programa[i].rotulo);
    //lcd.cls();
    //lcd.printf("%s",programa[i].rotulo);
    pc.scanf("%[^\n]", programa[i].operador);
    //lcd.printf("%s",programa[i].operador);
    pc.scanf("%[^\n]", programa[i].modificador);
    //lcd.printf("%s",programa[i].modificador);
    pc.scanf("%[^\n]", programa[i].operando);
    //lcd.printf("%s",programa[i].operando);
  }
  //printf("Fim");
  wait(2);
  //for(i = 0; i < tamanho; i++){
  //  pc.printf("%s,%s,%s,%s\n",programa[i].rotulo,programa[i].operador,programa[i].modificador,programa[i].operando);
  //  wait(0.1);
  //}
  led = 1;    //Finalizando a transferência
  for(i = 0; i < tamanho; i++){
    pc.printf("%s,%s,%s,%s\n",programa[i].rotulo,programa[i].operador,programa[i].modificador,programa[i].operando);
    wait(0.1);
  }
}
//-----------------------------------------------------------------------------
int main(){
  pc.printf("Inicializacao\n");
  pc.attach(&upload);
  /*
  tamanho = 3;
  programa = (instrucao*)malloc(tamanho*sizeof(instrucao));
  strcpy(programa[0].rotulo,"                ");
  strcpy(programa[0].operador,"LD ");
  strcpy(programa[0].modificador,"  ");
  strcpy(programa[0].operando,"I0");
  strcpy(programa[1].rotulo,"                ");
  strcpy(programa[1].operador,"XOR");
  strcpy(programa[1].modificador,"  ");
  strcpy(programa[1].operando,"I1");
  strcpy(programa[2].rotulo,"                ");
  strcpy(programa[2].operador,"ST ");
  strcpy(programa[2].modificador,"N ");
  strcpy(programa[2].operando,"Q0");
  */
  Q[0] = 0;
  led = 0;
  while(1){

    // lendo as ENTRADAS
    I[0] = i0.read();
    I[1] = i1.read();
    //wait(0.2);
    ledFisico = 0;
    //Execução do programa
    if (tamanho>0){
      for(PC = 0; PC < tamanho; PC++){
        //pc.printf("%d|%s|%s|\n",PC,programa[PC].rotulo,programa[PC].operando);
        executa_instrucao(programa[PC]);
        //pc.printf("%d",val_operador(programa[i].operador));
      }
    }
    //Atualizando as SAIDAS
    //wait(0.2);
    ledFisico = 1;
    q0.write(Q[0]);
  }
  return 0;
}
