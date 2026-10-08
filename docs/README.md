# Quantum Script Extension SCard — Documentation

`quantum-script--scard` is the **smart card extension of Quantum Script**.
Loaded with `Script.requireExtension("SCard")`, it adds an `SCard` object
that talks to smart cards through the Windows **PC/SC** API (`winscard`):
list the card readers, see whether a card is inserted, connect to it, read
its ATR and exchange APDU commands with it.

```javascript
SCard.establishContext();                  // -> context resource, undefined on failure
SCard.releaseContext(context);             // -> true
SCard.listReaders(context);                // -> array of reader names
SCard.getStatusChange(context, reader);    // -> true when a card is present in reader
SCard.connect(context, reader);            // -> card resource, undefined on failure
SCard.disconnect(card);                    // -> true
SCard.getAtrString(context, card);         // -> ATR as hex text, undefined on failure
SCard.transmit(card, apdu);                // -> response as hex text, undefined on failure
```

A smart card is a chip card (bank card, ID card, SIM, access badge, crypto
token, contactless tag, ...) read through a card reader. Programs talk to
the card by sending it **APDUs** (Application Protocol Data Units, ISO/IEC
7816-4): a command of a few bytes (`CLA INS P1 P2 [Lc data] [Le]`) for
which the card returns response data followed by a two byte status word
(`SW1 SW2`, `9000` = success). The operating system's smart card service
(PC/SC resource manager, `SCardSvr` on Windows) arbitrates the readers;
this extension is a thin layer over it, with APDUs written as hex strings.

- **Readers.** `listReaders` returns the names of the connected readers,
  `getStatusChange` tells whether a card is inserted in one of them.
- **Connect.** `connect` opens the card in **exclusive** mode with protocol
  T=0 or T=1 (whichever the card and reader agree on) and returns a card
  resource.
- **ATR.** `getAtrString` returns the card's Answer To Reset, the bytes the
  card sends when powered on; it identifies the card type.
- **APDUs.** `transmit(card, "00A4040007A0000000041010")` sends the command
  bytes and returns the response, e.g. `"6F...9000"`; the last four hex
  digits are the status word.
- **Errors.** Operations that fail at the PC/SC level (no service, no
  reader, card removed, ...) return `undefined` (`false` for
  `getStatusChange`). Passing something that is not the expected SCard
  resource throws `Error("invalid parameter")`.

```
scripts: quantum-script .js, tools that embed Quantum Script
quantum-script--scard     <-- this extension: SCard.*
quantum-script            (Executive, Variable, VariableResource)
quantum-script--console   (link dependency)
winscard                  Windows PC/SC API (SCardEstablishContext, SCardConnect, SCardTransmit, ...)
xyo-system, xyo-encoding, xyo-multithreading, xyo-data-structures, xyo-managed-memory, xyo-platform
```

## Why it exists

| Need | What `SCard` gives |
|------|--------------------|
| Script smart card tasks without writing C++ | the PC/SC calls as script functions |
| Detect readers and inserted cards | `listReaders`, `getStatusChange` |
| Identify a card | `getAtrString` |
| Talk to a card application (select, read records, get data, ...) | `transmit` with APDUs as hex text |
| Test and debug card applications from a script | send an APDU, print the response and status word |

## Concepts at a glance

| Need | Use | Notes |
|------|-----|-------|
| Load the extension | `Script.requireExtension("SCard");` | Windows only, needs `quantum-script--scard.dll` |
| Open the smart card service | `var ctx = SCard.establishContext();` | `undefined` when the service is not available |
| Readers | `SCard.listReaders(ctx)` | `[]` when there is no reader |
| Card inserted? | `SCard.getStatusChange(ctx, reader)` | `true` / `false`, does not wait |
| Connect | `var card = SCard.connect(ctx, reader);` | exclusive access, `undefined` if no card or card in use |
| ATR | `SCard.getAtrString(ctx, card)` or `SCard.getAtrString(card)` | upper case hex |
| Command | `SCard.transmit(card, "00A4000C023F00")` | hex in, hex out; spaces allowed in the input |
| Status word | `r.substring(r.length - 4)` | `9000` success, `61XX` more data (T=0), `6CXX` wrong Le |
| Close | `SCard.disconnect(card); SCard.releaseContext(ctx);` | resources are also freed by the garbage collector |

## Contents

| Document | What it covers |
|----------|----------------|
| [Getting started](getting-started.md) | Requirements (Windows, smart card service, reader), build and install, a first script, running the tests, C++ hosts |
| [Script API](script-api.md) | Every function, resources and their lifetime, hex format, APDU basics, status words, T=0 `GET RESPONSE`, errors, limitations, recipes |
| [C++ API](cpp-api.md) | `registerInternalExtension`, `initExecutive`, the DLL entry point, build defines, how resources are checked, notes for maintainers |
| [API reference](reference.md) | Every script and C++ symbol, return values and edge cases on one page |

Quantum Script itself (the language, `Script.requireExtension`, embedding,
writing extensions) is documented in the `quantum-script` repository,
`docs/`.

## Source map

```
source/XYO/QuantumScript.Extension/SCard.hpp            umbrella header, include this from C++
source/XYO/QuantumScript.Extension/SCard.Amalgam.cpp    the whole extension in one translation unit
source/XYO/QuantumScript.Extension/SCard/
    Dependency.hpp                                      <XYO/QuantumScript.hpp>, export macro
    Library[.hpp/.cpp]                                  SCard.* native functions, initExecutive,
                                                        registerInternalExtension, DLL entry point
    Copyright / License / Version                       library metadata
    Library.rc, *.rh                                    Windows version resource
fabricare.json                                          dll-or-lib, links winscard
fabricare/test.js                                       runs test/test.0001.js and test/test.0002.js
test/test.0001.js                                       API surface, argument checks, context life cycle
test/test.0002.js                                       card operations, needs a reader with a card
```

## AI assistant skill

A Claude Code skill describing how to use this extension lives in
[`.claude/skills/quantum-script--scard/`](../.claude/skills/quantum-script--scard/SKILL.md).
It is picked up automatically inside this repository; copy the folder to
`~/.claude/skills/` to have it available in the projects that use `SCard`.
