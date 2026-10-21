# BBSlink – InterBBS Door Game Clients

**Bring classic BBS door games to your bulletin board.**

[BBSlink](https://www.bbslink.net/) is a free InterBBS door game service that lets sysops offer remotely hosted door games to their callers without installing and maintaining each game locally. Players launch games from their own BBS, while BBSlink handles the game server side.

This repository brings together connection clients and scripts for different BBS platforms. Whether you want to connect an existing board or develop a client for another system, this is a place to start.

## Getting started

1. **Register your BBS** at [BBSlink.net](https://www.bbslink.net/) and obtain your BBSlink credentials.
2. **Choose a client** from the list below that suits your BBS software and operating system.
3. **Configure the client** with the credentials supplied by BBSlink (commonly `syscode`, `authcode` and `schemecode`). Follow any platform-specific setup instructions.
4. **Add a door or menu entry** to your BBS that launches the client and provides the required user information. Depending on the client, this may be supplied through command-line arguments, environment variables or a BBS drop file.
5. **Test the connection** by launching a game from your BBS.

Exact requirements and setup procedures vary between clients. Check the files and documentation in the relevant client directory before installing.

## Available clients

| Platform / environment | Client directory |
| --- | --- |
| Amiga C-Net | [Amiga C-Net Clients](./Amiga%20C-Net%20Clients/) |
| UNIX-like systems / shell | [BASH Scripts](./BASH%20Scripts/) |
| Python-based integrations | [Python Clients](./Python%20Clients/) |
| Synchronet BBS | [Synchronet JavaScript Clients](./Synchronet%20JavaScript%20Clients/) |
| 32-bit Windows console | [Windows 32-bit Console Client](./Windows%2032-bit%20Console%20Client/) |

Not seeing your BBS platform? You may be able to adapt an existing client or write a new one using the developer guide below. Contributions for additional systems are welcome.

## How BBSlink works

BBSlink acts as a bridge between your local BBS and games hosted on the BBSlink server:

1. A caller chooses a door game on your BBS.
2. Your BBS launches a BBSlink client, which identifies the BBS and the caller and authenticates the request with BBSlink.
3. Once authorized, the client establishes a Telnet game session to the remote server.
4. The caller plays the game and, when the session ends, returns to your BBS.

The player accesses BBSlink through your board rather than signing in directly to the game server. The precise authentication and connection details are described in the developer guide.

## Developing a client

Want to add support for another BBS, operating system or programming language?

Start with the [BBSlink Developer Guide](./bbslinkdevguide.pdf), then look at the existing clients for practical implementations of the authentication and connection process.

A useful client should:

- Obtain the current caller's identifier from the BBS.
- Authenticate with BBSlink using the credentials assigned to the sysop.
- Launch the selected game (or the BBSlink game menu).
- Handle the remote terminal session and return control to the BBS when it ends.
- Report connection and authentication errors clearly.

Keep credentials out of source control, and never commit real authentication codes or tokens to this repository.

## Contributing

Help is welcome, whether you maintain a BBS, write software or just spot something that could be clearer.

You can contribute by:

- Adding a client for a new BBS platform.
- Fixing bugs or improving compatibility in an existing client.
- Adding installation instructions and working configuration examples.
- Testing clients and reporting problems.
- Improving the developer documentation.

For bug reports or suggestions, [open an issue](https://github.com/mercunium/bbslink/issues). For code or documentation changes, fork the repository and submit a pull request.

When submitting a new client, please include its supported BBS software, requirements, installation steps, configuration instructions and any known limitations. Do not include personal credentials or secrets.

## Links and documentation

- [BBSlink official website](https://www.bbslink.net/) – registration and service information
- [BBSlink Developer Guide (PDF)](./bbslinkdevguide.pdf) – technical reference for client developers
- [GitHub Issues](https://github.com/mercunium/bbslink/issues) – questions, bug reports and suggestions

## License

This repository includes a [GNU General Public License v3.0](./LICENSE) license file. Please also respect any copyright and licensing notices included with individual contributed clients.