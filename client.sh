echo "Starting client"
NICK=$1
if [[ $1 -gt 0 ]]; then
	echo "Nickname is required!"
	exit 0
fi

echo "Listening to address"
ADDR=$(python3 broadcast/listen.py)
echo "ADDRESS: " $ADDR

IP=$(echo $ADDR | cut -d ":" -f 1)
PORT=$(echo $ADDR | cut -d ":" -f 2)
echo "IP: " $IP" PORT: " $PORT
./ChatBox $NICK $IP $PORT
