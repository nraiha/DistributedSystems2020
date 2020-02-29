echo "Starting server"
LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH

PORT=420
IP=$(ip addr show eth0 | grep "inet " | awk '{print $2}' | cut -d/ -f1)

echo "Starting broadcast"
echo "Broadcast message: IP: " $IP

python3 broadcast/broadcast.py $IP $PORT &

echo "Starting server with port: " $PORT

FLAG=0
./ChatBoxServer $PORT
while [ $? -gt 0 ]
do

	echo "Error happened, restarting server..."
	tail -n 2 messages.txt > messages_backup.txt
	./ChatBoxServer $PORT messages_backup.txt
	FLAG+=1
	if [[ $FLAG -gt 5 ]]; then
		echo "5 restarts happened"
		kill $(ps | grep python3 |awk '{print $1}')
		exit 1
	fi
done

echo "Performing cleanup"
# Cleanup
kill $(ps | grep python3 | awk '{print $1}')
