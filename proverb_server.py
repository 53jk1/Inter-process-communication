# $ python3 proverb_server.py

#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import time

# List of folk proverbs and sayings heard on the Internet.
proverbs = [
    "Using floats in a banking app is INFINITE fun.",
    "threads.synchronize your Remember to ",
    "C and C++ are great for parsing complex formats [a hacker on IRC]",
    ( # Phil Karlton + anonymous
      "There are only two hard things in Computer Science: cache ",
      "invalidation, naming things and off by one errors.")
    ]

# Create a named stream
try:
    os.mkfifo("/tmp/proverb", 0o644)
except OSError as e:
    # If the error is other than "file already exists" (17), pass the exception on.
    if e.errno != 17:
        raise

# Occupy the write end and wait for connections at the reading end.
print ("Pipe /tmp/proverb created. Waiting for clients.")
print ("Press CTRL+C to exit.")
proverb = 0
try:
    while True:
        fdw = open("/tmp/proverb", "w")
        print ("Client connected! Sending a proverb.")
        fdw.write(proverbs[proverb % len(proverbs)] + "\n")
        fdw.close()
        proverb += 1
        time.sleep(0.1) # Give the client a moment to disconnect.
except KeyboardInterrupt:
    pass
# Remove the named pipe from the file system.
print("\nCleaning up!")
os.unlink("/tmp/proverb")