#!/usr/bin/env python3

##  ************************************************************
##  ************************************************************
##  ***                                                      ***
##  ***  BBSlink.net InterBBS Door Server Connection Script  ***
##  ***                                                      ***
##  ************************************************************
##  ************************************************************
##
##  PLEASE DO NOT DISTRIBUTE THIS FILE
##  ==================================
##
##  Version 0.3.1  5th October 2023
##
##  Kindly derived from the original BBSlink.net connection
##  original script by Sampsa Laine of B4BBS - http://sampsa.com/b4bbs
##  updated Python3 script by Mike Pedersen of https://www.arcadiabbs.com
##
##  (C)2015 Sampsa Laine for BBSlink.net.
##  Version 0.3.1 (C) 2023 Michael Pedersen for BBSlink.net.
##
##  Thanks go to haliphax for cryptography advice.
##
##
##  USAGE:
##  =====
##
##  1. Place this script in an appropriate directory that is 
##     accessible by your BBS software.
##
##  2. Configure the script with your system code, authorisation
##     code, and scheme code below.
##
##  3. Call the script with the following command:
##
##          python3 bbslink.py  
##
##     i.e., python3 bbslink.py lord 27
##
##  4. You can call different door games by changing the argument
##     in the command line. An up-to-date list of available doors
##     can always be found at http://www.bbslink.net/sysop
##
##  5. On Ubuntu you can install python-as-python3 to avoid the requirement
##     of reprogramming all your menu calls to include python3 in the command
##     run "apt install python-as-python3" and your door calls will remain
##     the same as they always were.
##
##  I hope you find BBSlink.net useful. Please guard your BBSlink login
##  credentials carefully, and should your system's authorisation code be
##  compromised, please contact me immediately for a new one.
##
##  All feedback is greatly appreciated!
##
##  Mark Mearns.

##  *********************************************************
##  *** Configure the following settings for your system: ***
##  *********************************************************

host = "games.bbslink.net" 	    # Server address, usually 'games.bbslink.net'
syscode = "yoursyscode"         # Your system code
authcode = "yourauthcode"       # Your system's authorisation code
schemecode = "yourschemecode"   # Scheme code


##  ***********************************
##  *** DO NOT EDIT BELOW THIS LINE ***
##  ***********************************

version = "0.3.1"

import hashlib
import random
import string
import http.client
import os
import sys
import time

def getMD5Hash(s):
    m = hashlib.md5()
    m.update(s.encode('utf-8'))
    rv = m.hexdigest()
    return rv

if not len(sys.argv) == 3:
    print("ERROR! Usage: bbslink.py  ")
    sys.exit(42)

os.system("clear")
print("  BBSlink.net Linux connection client " + version + "updated by," )
print(" -=- Mike Pedersen, Arcadia BBS (1:142/73), http://www.arcadiabbs.com -=- ")
print(" ")
print(" Based upon Linux connection client 0.2.2 by ")
print(" -=- Sampsa Laine, B4BBS (2:250/7), http://sampsa.com/b4bbs  -=- ")
print(" ")
print("  Connecting to BBSlink.net, please wait...")
print(" ")
time.sleep(2)

doorcode = sys.argv[1]
usernumber = sys.argv[2]
screenrows = 24
scripttype = "Python"
scriptver = version

xkey = ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(6)).lower()

h1 = http.client.HTTPConnection(host)
h1.request("GET", "/token.php?key=" + xkey)
r1 = h1.getresponse()
token = r1.read()
token = str(token).strip("b'")

headers = {
    "X-User": usernumber,
    "X-Token": token,
    "X-Version": scriptver,
    "X-Auth": getMD5Hash(authcode + token),
    "X-Key": xkey,
    "X-Rows": screenrows,
    "X-Door": doorcode,
    "X-Code": getMD5Hash(schemecode + token),
    "X-System": syscode,
    "X-Type": scripttype
}
h2 = http.client.HTTPConnection(host)
h2.request("GET", "/auth.php?key=" + xkey, "", headers)

r2 = h2.getresponse()
status = r2.read()
status = str(status).strip("b'")

if status == "complete":
    print (" Connecting ")
    os.system("telnet -E -K -8 " + host)
else:
    print("An error occurred when contacting the door server:")
    print("[" + status + "]")