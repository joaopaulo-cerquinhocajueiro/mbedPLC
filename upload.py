import serial
import time
porta = 'COM5'
ser = serial.Serial(porta,9600)
programa = open('src/programa.txt')
blocos = []
for line in programa:
	pedacos = line.split()
	if pedacos[0].endswith(':'):
		pedacos[0] = pedacos[0].strip(':')
	else:
		pedacos.insert(0,16*' ')
	pedacos[0] = pedacos[0].ljust(16,' ')
	pedacos.insert(2,' ')
	if pedacos[1].endswith('N'):
		pedacos[2]='N '
		pedacos[1] = pedacos[1].rstrip('N')
	if pedacos[1].endswith('C'):
		pedacos[2] = 'C '
		pedacos[1] = pedacos[1].rstrip('C')
	if pedacos[1].endswith('('):
		pedacos[2] = '( '
		pedacos[1] = pedacos[1].rstrip('(')
	pedacos[1] = pedacos[1].ljust(3,' ')
	if len(pedacos)==3:
		pedacos.append('  ')
	pedacos[3] = pedacos[3].ljust(16,' ')
	blocos.append(pedacos)

print(blocos)

ser.write(bytes(len(blocos)))
ser.write('\n')
time.sleep(1)
for bloco in blocos:
	for pedaco in bloco:
		pedaco = pedaco + '\n'
		ser.write(pedaco)
		print(pedaco)
		time.sleep(0.1)
	time.sleep(0.2)
import msvcrt
while True:
	if msvcrt.kbhit():
		break
	if ser.in_waiting:
		s = ser.readline()
		print(s)
#s = ser.read(100)
#print s
ser.close()
