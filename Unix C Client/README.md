# bbslink_client

## NAME

`bbslink_client` - BBSlink door game launcher for UNIX-like systems

## SYNOPSIS

```
bbslink_client -c file [-r rows] door user-number
bbslink_client -h
```

## DESCRIPTION

`bbslink_client` authenticates a caller with the BBSlink service and starts
an interactive Telnet session with the game server.

The calling BBS supplies the door code and its local numeric user identifier.
The program does not read BBS databases or drop files. It does not depend on
a particular BBS implementation.

Authentication is performed using libcurl and OpenSSL. The program then uses
`execvp(3)` to start the system's `telnet(1)` program. Standard input, output,
error and the caller's terminal environment are inherited by Telnet.

## OPTIONS

`-c file`
: Read authentication credentials from `file`. Required.

`-r rows`
: Number of terminal rows reported to BBSlink (10-200). By default,
  the program uses the current terminal size if available, or 24 rows.

`-h`
: Print usage information and exit.

`door`
: BBSlink door code, e.g. `menu`. Letters, digits, underscores and hyphens
  are accepted.

`user-number`
: Numeric identifier of the current caller, supplied by the BBS.

## REQUIREMENTS

- C compiler and `make`
- libcurl development files
- OpenSSL libcrypto development files
- `pkg-config`
- Telnet client available in `PATH`

The program has no library or header dependencies on BBS software.

## BUILD

```
make
make install PREFIX=/usr/local
```

Use `DESTDIR` for staged installation. For example:

```
make install PREFIX=/usr/local DESTDIR=/tmp/pkgroot
```

## CONFIGURATION

Create a credentials file containing the following three entries:

```
syscode=YOUR_SYSCODE
authcode=YOUR_AUTHCODE
schemecode=YOUR_SCHEMECODE
```

The codes are assigned when a BBS is registered with BBSlink.

The file must be a regular file with no group or other permissions. Modes
`0600` and `0400` are accepted. It must be readable by the account executing
the client. Symlinks are rejected on systems supporting `O_NOFOLLOW`.

For example:

```
umask 077
cp bbslink_credentials.example /path/to/bbslink_credentials
chmod 600 /path/to/bbslink_credentials
```

Edit the copied file and replace the placeholder codes. Do not commit it to
version control.

## EXAMPLES

Start the remote door menu for caller 42:

```
bbslink_client -c /path/to/bbslink_credentials menu 42
```

Specify a 40-row terminal and a door code:

```
bbslink_client -c /path/to/bbslink_credentials -r 40 doorcode 42
```

The BBS should launch the client with a connected interactive terminal.
No particular calling convention is imposed beyond the arguments described
above.

## EXIT STATUS

`0`
: Usage information was displayed (`-h`), or the executed Telnet client
  exited successfully.

`1`
: Credential, network, authentication or program-execution failure.
  Following successful authentication, the exit status is that of the
  executed Telnet client.

`2`
: Invalid command-line arguments.

## SECURITY

The BBSlink legacy authentication exchange uses plaintext HTTP and MD5-based
signatures. The resulting game session uses unencrypted Telnet. Traffic and
session contents are not protected against observation or modification on
the network. These protocol choices are dictated by the remote service.

Do not run this program setuid or setgid. Do not put real credentials on the
command line or in the source tree. Use a restricted credentials file.

## LIMITATIONS

The program delegates Telnet negotiation, terminal emulation and 8-bit
handling to the installed Telnet client. Behavior may differ between Telnet
implementations and BBS terminal setups. It does not handle drop files or
perform BBS-specific user lookups.

The program assumes the legacy BBSlink endpoints `token.php`, `auth.php`
and Telnet port 23. Compatibility must be checked against the running
BBSlink service.

## FILES

`bbslink_client.c`
: Program source.

`bbslink_credentials.example`
: Credentials file template. Contains no usable credentials.

`Makefile`
: Build and installation rules.

## SEE ALSO

- [BBSlink](https://www.bbslink.net/)
- [BBSlink Developer Guide](https://github.com/mercunium/bbslink/blob/main/bbslinkdevguide.pdf)
- `telnet(1)`, `execvp(3)`

## LICENSE

GNU General Public License version 3. See `COPYING`.
