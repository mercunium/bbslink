#! /usr/bin/env python

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
##  Version 0.2.2  19th December 2015
##
##  Kindly derived from the original BBSlink.net connection
##  script by Sampsa Laine of B4BBS - http://sampsa.com/b4bbs
##
##  (C)2015 Sampsa Laine for BBSlink.net.
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
##     code and scheme code below.
##
##  3. Call the script with the following command:
##
##          python bbslink.py  
##
##     i.e. python bbslink.py lord 27
##
##  4. You can call different door games by changing the argument
##     in the command line. An up-to-date list of available doors
##     can always be found at http://www.bbslink.net/sysop
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


host = "games.bbslink.net" # Server address, usually 'games.bbslink.net'

syscode = "" # Your system code
authcode = "" # Your system's authorisation code
schemecode = "" # Scheme code


##  ***********************************
##  *** DO NOT EDIT BELOW THIS LINE ***
##  ***********************************


version = "0.2.2"

import md5
import random
import string
import httplib
import os
import sys
import time

def getMD5Hash(s):
	m = md5.new()
	m.update(s)
	rv = m.hexdigest()
	return rv

if not len(sys.argv) == 3:
	print "ERROR! Usage: bbslink.py  "
	exit(42)

os.system("clear")
print "  BBSlink.net Linux connection client " + version + "\n  -=- Courtesy of Sampsa Laine, B4BBS (2:250/7), http://sampsa.com/b4bbs  -=- "
print " "
print "  Connecting to BBSlink.net, please wait..."
print " "
time.sleep(2)

doorcode = sys.argv[1]
usernumber = sys.argv[2]
screenrows = 24
scripttype = "Python"
scriptver = version

xkey = ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(6)).lower()

h1 = httplib.HTTPConnection(host)
h1.request("GET", "/token.php?key=" + xkey)
r1 = h1.getresponse()
token = r1.read()
token = str(token).strip()

headers = {
	"X-User": usernumber,
	"X-System": syscode,
	"X-Auth": getMD5Hash(authcode + token),
	"X-Code": getMD5Hash(schemecode + token),
	"X-Rows": screenrows,
	"X-Key": xkey,
	"X-Door": doorcode,
	"X-Token": token,
	"X-Type": scripttype,
	"X-Version": scriptver
}
h2 = httplib.HTTPConnection(host)
h2.request("GET","/auth.php?key=" + xkey,"",headers)
r2 = h2.getresponse()
status = r2.read()
status = str(status).strip()

if status == "complete":
	os.system("telnet -E -K -8 " + host)
else:
	print "An error occurred when contacting the door server:"
	print "[" + status + "]"