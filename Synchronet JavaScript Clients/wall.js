//  **************************
//  **************************
//  ***                    ***
//  ***  BBSlink.net Wall  ***
//  ***                    ***
//  **************************
//  **************************
//  
//  PLEASE DO NOT DISTRIBUTE THIS FILE
//  ==================================
//  
//  Version 0.9.beta  23rd July 2015
//  (C)2015 Mark Mearns. All Rights Reserved.
//
//  Thanks go to haliphax for cryptography advice.
//
//
//  USAGE:
//  =====
//  
//  1. Place this script in your Synchronet mods directory,
//     e.g. C:\SBBS\MODS (Windows) or /sbbs/mods/ (Linux).
//  2. Configure this script with the codes given for your system
//  3. Create an external program as normal in SCFG to call this
//     script, usually ?wall
//
//
//     +--------------------------------------------------------+
//     ?                      BBSlink Wall                      ?
//     +--------------------------------------------------------?
//     ? Name                       BBSlink Wall                ?
//     ? Internal Code              BBSWALL                     ?
//     ? Start-up Directory         ../mods                     ?
//     ? Command Line               ?wall                       ?
//     ? Clean-up Command Line                                  ?
//     ? Execution Cost             None                        ?
//     ? Access Requirements                                    ?
//     ? Execution Requirements                                 ?
//     ? Multiple Concurrent Users  Yes                         ?
//     ? Intercept I/O              No                          ?
//     ? Native Executable          No                          ?
//     ? Use Shell to Execute       No                          ?
//     ? Modify User Data           No                          ?
//     ? Execute on Event           No                          ?
//     ? Pause After Execution      No                          ?
//     ? BBS Drop File Type         None                        ?
//     ? Place Drop File In         Node Directory              ?
//     ? Time Options...                                        ?
//     +--------------------------------------------------------+
//
//     Note that this screen is from Synchronet for Linux, Windows versions
//     may vary, but the commands are all the same.
//
//  All feedback is greatly appreciated!
//  
//  Mark Mearns.


//  Insert your system's BBSlink.net log in credentials between the "" below:

var host = "games.bbslink.net"; // Server address, usually 'games.bbslink.net'

var syscode = ""; // Your system code
var authcode = ""; // Your system's authorisation code
var schemecode = ""; // Scheme code


// ***********************************
// *** DO NOT EDIT BELOW THIS LINE ***
// ***********************************

load("sbbsdefs.js");
load("http.js");

// Show the wall!
ShowWall();

// Ask user if they want to write to the wall themselves
var writetowall = console.noyes("Write on the wall");

// If they answered no, quit.
if (writetowall==true) {
    exit();
}

// Find out if user already has a registered user name
console.clear();
console.print("  \1yLooking for your pen - it's around here somewhere...\1w");
console.crlf();
var username = SendToServer("username");

// Check for variable authentication error and display as appropriate
if (username.substring(0, 3) == "*xx") {
    console.crlf();
    console.writeln("  Sorry, I looked everywhere.");
    console.writeln("  Can't find it.");
    console.crlf();
    console.writeln("  Oh, and this happened:");
    console.print("    \1m" + username.substring(3));
    console.crlf();
    console.crlf();
    console.crlf();
    console.pause();
    exit();
}

if (username == "*notexist") {
    // Not registered
    var i = 0;
    console.crlf();
    console.crlf();
    console.print("  So you're new here! Choose a name \1b(max 12 characters)\1w:");
    console.crlf();

    start: while(true) {

        console.crlf();
        var getnewusername = console.getstr("", maxlen=12);
        console.crlf();
        var nuresult = SendToServer("newuser", getnewusername);

        if (nuresult == "*created") {
            console.print("  \1yWelcome to the wall, \1w" + getnewusername);
            console.crlf();
            i = 5000;
        } else if (nuresult == "*inuse") {
            console.print("  That user name is already in use, choose another!");
            console.crlf();
        } else if (nuresult == "*inval") {
            console.print("  That user name is invalid, choose another!");
            console.crlf();
        }

        if (i < 3000) continue start;
        break;
    }
}

// Even if they didn't before, they should have a user name now. If they still
// don't for some reason, show an error and quit. This will also catch
// authentication errors.
var username = SendToServer("username");
if (username.charAt(0) == '*') {
    console.crlf();
    console.writeln("  Sorry, I can't find it :-(");
    console.crlf();
    console.writeln("  (An error occurred [" + username + "])");
    console.crlf();
    console.crlf();
    console.pause();
    exit();
}

// Get the user's thoughts - in 64 characters or less!
console.clear();
console.crlf();
console.crlf();
console.print("  \1yWhat's on your mind, \1w" + username + "\1y?  \1b(max 64 characters)");
console.crlf();
console.crlf();
console.print('  \1b������������������������������������������������������������������Ŀ');
console.crlf();
console.print('  \1b�                                                                  �');
console.crlf();
console.print('  \1b��������������������������������������������������������������������');
console.crlf();
console.crlf();
console.crlf();
console.crlf();
console.print('\1w');
console.gotoxy(5,6);

// Take the wall post
var getwallpost = console.getstr("", maxlen=64);
console.gotoxy(1,9);

// If input is more than 2 characters, submit it
if (console.strlen(getwallpost) > 2) {

    var postresult = SendToServer("post", getwallpost);
    
    // Check result of post attempt
    if (postresult == "*post") {
        // Successful
        console.print("  \1yPost successful!");
    } else if (postresult == "*int") {
        // Last post < 10 minutes ago
        console.print("  \1ySorry, you have to wait 10 minutes between posts.");
    } else if (postresult == "*inval") {
        // Post contained > 64 characters
        console.print("  \1yYour post contained too many characters (max length 64 chars).");
    } else {
        // Post failed, unknown reason
        console.print("  \1rPost failed :-(");
    }

} else {

    console.print("  \1yYour post was too short!");

}

console.crlf();
console.crlf();
console.crlf();
console.pause();

// Show the wall!
ShowWall();

console.pause();


// X-key generator
function randomString(len, charSet) {
    charSet = charSet || 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    var randomString = '';
    for (var i = 0; i < len; i++) {
        var randomPoz = Math.floor(Math.random() * charSet.length);
        randomString += charSet.substring(randomPoz,randomPoz+1);
    }
    return randomString;
}

// Show the wall!
function ShowWall() {

    // Show splash
    console.writeln("Reading the wall...");
    console.clear();

    // Get ANSI text from BBSlink server
    getwall = new HTTPRequest();
        getwall.SetupGet("http://" + host + "/wall.php?action=show");
        getwall.SendRequest();
        getwall.ReadResponse();
        var wall = getwall.body;

    // Display the wall

    console.clear();
    console.writeln(wall);

}

// Send data to the server
function SendToServer(action, data) {
    var xkey = randomString(6);
    var scripttype = "JS";
    var scriptver = "0.1.beta";

    gettkn = new HTTPRequest();
        gettkn.SetupGet("http://" + host + "/token.php?key=" + xkey);
        gettkn.SendRequest();
        gettkn.ReadResponse();
    var token = gettkn.body;

    req = new HTTPRequest();
        req.SetupGet("http://" + host + "/wall.php?action=" + action + "&key=" + xkey);
        req.request_headers.push("X-User: " + user.number);
        req.request_headers.push("X-System: " + syscode);
        req.request_headers.push("X-Auth: " + md5_calc(authcode + token, hex=true));
        req.request_headers.push("X-Code: " + md5_calc(schemecode + token, hex=true));
        req.request_headers.push("X-Key: " + xkey);
        req.request_headers.push("X-Token: " + token);
        req.request_headers.push("X-Type: " + scripttype);
        req.request_headers.push("X-Version: " + scriptver);
        req.request_headers.push("X-Data: " + data);
        req.SendRequest();
        req.ReadResponse();

    return req.body;
}
