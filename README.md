# fivem-parser

A lightweight tool used to parse chat logs generated during a FiveM session.

Built with [GLFW](https://www.glfw.org/) and [ImGui](https://github.com/ocornut/imgui).

https://github.com/user-attachments/assets/a3eba1fd-cb94-4dc9-af14-4a2675819a05

## Building

Requires [xmake](https://xmake.io/) and a mingw-w64 toolchain.

```bash
xmake
```

## Setup

To be able to parse a session's chat messages, the server must be running the included `parser` resource. This is addon that simply logs chat messages to the client's log file, which the parser then reads.

1. Download or clone the repository with `git clone https://github.com/bd53/fivem-parser`.
2. Copy `resources/parser` folder into the `resources/` directory.
3. Add `ensure parser` to where resources are being loaded (after chat resource).

This removes the need for any modifications to your chat resource.

## License

A complete copy of the license is included in the [fivem-parser/LICENSE](./LICENSE) file.
