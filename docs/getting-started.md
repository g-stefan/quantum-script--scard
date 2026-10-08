# Getting started

## 1. Requirements

- **Windows.** The extension calls the Windows PC/SC API (`winscard.dll`,
  `<winscard.h>`) and is built for Windows only.
- **Smart Card service.** The PC/SC resource manager, service `SCardSvr`
  ("Smart Card"). Windows starts it when a reader is plugged in and may stop
  it again when no reader is connected; check with:

  ```bash
  sc query SCardSvr
  ```

  Without a working service `SCard.establishContext()` returns `undefined`
  (PC/SC error `SCARD_E_NO_SERVICE`). This also happens in sessions that
  have no access to the smart card service, e.g. some sandboxed or service
  processes.
- **A card reader** (USB contact reader, contactless / NFC reader, a token
  with a built-in reader, ...) with its driver installed. It shows in
  Device Manager under *Smart card readers*.
- **A card** for anything beyond listing readers.

## 2. Build and install

The extension is built with [fabricare](https://github.com/g-stefan/fabricare),
the build tool of all XYO C++ projects. `quantum-script` and
`quantum-script--console` must be installed to the SDK first. From the
repository root:

```bash
fabricare make       # build into output/
fabricare test       # run test/test.0001.js and test/test.0002.js
fabricare install    # copy output/{bin,include,lib} to ~/.fabricare/<platform>
fabricare clean      # remove output/ and temp/
```

After `fabricare install`, `quantum-script--scard.dll` sits in the SDK
`bin` folder next to `quantum-script.exe`, which is where
`Script.requireExtension("SCard")` finds it. The library links `winscard`
(declared in `fabricare.json`).

## 3. First script

List the readers and print the ATR of every inserted card:

```javascript
Script.requireExtension("Console");
Script.requireExtension("SCard");

var context = SCard.establishContext();
if (Script.isUndefined(context)) {
	Console.writeLn("smart card service not available");
	Script.exit(1);
};

var readers = SCard.listReaders(context);
if (readers.length == 0) {
	Console.writeLn("no reader");
};

for (var k = 0; k < readers.length; ++k) {
	var reader = readers[k];
	if (!SCard.getStatusChange(context, reader)) {
		Console.writeLn(reader + ": no card");
		continue;
	};
	var card = SCard.connect(context, reader);
	if (Script.isUndefined(card)) {
		Console.writeLn(reader + ": card in use or not responding");
		continue;
	};
	Console.writeLn(reader + ": ATR " + SCard.getAtrString(context, card));
	SCard.disconnect(card);
};

SCard.releaseContext(context);
```

Run it with:

```bash
quantum-script cards.js
```

Example output:

```
Alcor Micro USB Smart Card Reader 0: ATR 3B8F8001804F0CA000000306030001000000006A
```

## 4. Sending a command

`transmit` takes the APDU as hex text and returns the response as hex
text, the last four digits being the status word:

```javascript
// SELECT the master file 3F00
var response = SCard.transmit(card, "00 A4 00 00 02 3F 00");
if (Script.isUndefined(response)) {
	Console.writeLn("transmit failed (card removed?)");
} else {
	var sw = response.substring(response.length - 4);
	var data = response.substring(0, response.length - 4);
	Console.writeLn("SW " + sw + " data " + data);
};
```

What the bytes mean depends on the card: read the card's or the
application's specification (ISO/IEC 7816-4 for the common commands, EMV
for bank cards, PC/SC part 3 for the `FF ...` contactless reader
commands). See [Script API](script-api.md#apdus) for the basics.

## 5. Running the tests

```bash
fabricare make
fabricare test
```

`fabricare test` (`fabricare/test.js`) runs
`quantum-script --execution-time test/test.000k.js` for `k` = 1..2 and stops
at the first failure:

- `test/test.0001.js` checks that every function exists and that values
  which are not SCard resources (`undefined`, numbers, strings, objects, a
  context where a card is expected, a closed resource, ...) throw instead
  of crashing. With the smart card service available it also checks
  `listReaders`, `getStatusChange` and `connect` on unknown and very long
  reader names, and the released context behavior.
- `test/test.0002.js` connects to the first reader with a card and checks
  `getAtrString`, `transmit` (a `SELECT 3F00`, hex input with spaces and
  lower case, invalid hex), `disconnect` and the disconnected card
  behavior. It needs a reader with a card that is not in use by another
  application.

Parts that need the service, a reader or a card print a *skipped* line and
pass when those are missing. Both tests load the DLL from `output/bin` when
it exists (so the build you just made is tested), and the installed one
otherwise.

## 6. C++ hosts

A C++ host that embeds Quantum Script can register the extension as an
internal extension with
`XYO::QuantumScript::Extension::SCard::registerInternalExtension(executive)`
(see [C++ API](cpp-api.md)); scripts still call
`Script.requireExtension("SCard")`.
