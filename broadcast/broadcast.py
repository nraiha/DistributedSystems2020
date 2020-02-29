import socket
import time
import sys

def broadcast(msg):
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	
	msg = bytes(msg, encoding='utf-8')
	s.settimeout(0.2)
	while True:
		s.sendto(msg, ("<broadcast>", 37020))
		time.sleep(1)

ip = sys.argv[1]
port = sys.argv[2]
arg = ip + ':' + port
broadcast(arg)
