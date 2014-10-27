#!/bin/sh

# Make sure server and client exist
if ! make; then
    exit 1
fi

# Run server in background
./bin/server > server.txt 2> server2.txt &

# Run client, providing some test input
./bin/client > client.txt 2> client2.txt

# Print output
echo "Server stdout:"
cat server.txt
echo
echo "Server stderr:"
cat server2.txt
echo
echo "Client stdout:"
cat client.txt
echo
echo "Client stderr:"
cat client2.txt

# Delete temp files
rm -rf server.txt server2.txt client.txt client2.txt

