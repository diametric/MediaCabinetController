import lightblue

sock = lightblue.socket()
sock.bind(("", 0))
sock.listen(1)

lightblue.advertise("EchoServer", sock, lightblue.RFCOMM)
print "Advertised and listening on channel %d..." % sock.getsockname()[1]

conn, addr = sock.accept()
print "Connected by", addr

data = conn.recv(1024)
print "Data: ", data
conn.send(data)

import time
time.sleep(1)

conn.close()
sock.close()

