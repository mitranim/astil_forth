## Overview

Astil Forth support for the [Sublime Text](https://sublimetext.com) editor.

## Usage

Symlink this `./sublime` directory to your Sublime packages directory. Example for MacOS:

```sh
git clone https://github.com/mitranim/astil_forth.git
cd astil_forth
ln -sfn "$(pwd)/sublime" "$HOME/Library/Application Support/Sublime Text/Packages/astil_forth"
```

To find the packages directory on your system, use Sublime Text menu → Preferences → Browse Packages.

## Run file

This package provides an ST "build system": a shortcut for running the current file using the Astil Forth interpreter. Works out of the box in `*.af` files. Hit ST's built-in hotkey ⌘B to run in default mode, or ⌘⇧B to select between variants.

## Eval selection

This package also provides the command ⌘⇧P → "Astil Forth: eval selection", which runs selected code using `astil` as subprocess. Example keybind configuration; choose your own keys:

```json
{
  "keys": ["alt+ctrl+b"],
  "command": "astil_forth_eval_selection",
  "context": [{"key": "selector", "operand": "source.astil_forth"}],
}
```

The subprocess can be killed via Ctrl+C or simply by closing the output panel.

The command is opt-in. Our Python plugin doesn't do anything else.
