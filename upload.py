# -*- coding: UTF-8 -*-

import serial # para comunicar com o kl25z
import time # para separa o envio dos blocos; talvez não seja necessário

# Inicializa a comunicação serial
porta = 'COM5'
ser = serial.Serial(porta,9600)

###
### Parsing
###
programa = open('src/programa.txt')
blocos = [] # Cada bloco vai ser um comando enviado ao CLP
for line in programa:
	pedacos = line.split()

	## Rótulo
	if pedacos[0].endswith(':'): # Se o primeiro pedaço termina em ':' é um rótulo
		pedacos[0] = pedacos[0].strip(':') # Salva sem o ':'
	else:
		pedacos.insert(0,16*' ') # Se não tiver rótulo, acrescenta um em branco
	pedacos[0] = pedacos[0].ljust(16,' ') # Ajusta o rótulo para ter 16 caracteres

	## Modificador
	# Como o modificador vem junto com o comando, é necessário identificá-lo e separá-lo
	pedacos.insert(2,'') # Insere um pedaço para o modificador
	# Testa se há operação adiada
	if pedacos[1].endswith('('):
		pedacos[2] = pedaços[2] + '('
		pedacos[1] = pedacos[1].rstrip('(')
	# Testa se há inversão
	if pedacos[1].endswith('N'):
		pedacos[2] = pedaços[2] + 'N'
		pedacos[1] = pedacos[1].rstrip('N')
	# Testa se há condicional
	if pedacos[1].endswith('C'):
		pedacos[2] = pedaços[2] + 'C'
		pedacos[1] = pedacos[1].rstrip('C')
	# Ajusta o tamanho do comando e do modificador
	pedacos[1] = pedacos[1].ljust(4,' ') # comando
	pedacos[2] = pedacos[2].ljust(2,' ') # modificador

	## Operando
	if len(pedacos)==3: # Se não houver operando
		pedacos.append('  ') # Acrescenta um vazio
	pedacos[3] = pedacos[3].ljust(16,' ') # ajusta  tamanho

	## Salva instrução completa
	blocos.append(pedacos)

#print(blocos)

###
### Programação
###

#Envia a quantidade de blocos que o programa tem
ser.write(bytes(len(blocos)))
ser.write('\n')
# Espera um tempo, por sanidade
time.sleep(0.1)
for bloco in blocos:
	for pedaco in bloco: # Para cada pedaço de cada bloco
		pedaco = pedaco + '\n' # Acrescenta uma quebra de linha
		ser.write(pedaco) # E envia pela serial
		print(pedaco) # E para o terminal, por simplicidade
		time.sleep(0.01)
	time.sleep(0.02)

###
### Comunicação
###
import msvcrt # Para detectar que uma tecla foi apertada
while not msvcrt.kbhit(): # Enquanto nenhuma tecla for apertada
	if ser.in_waiting: # Se chegar coisa na serial, imprime
		s = ser.readline()
		print(s)
#s = ser.read(100)
#print s
ser.close()
