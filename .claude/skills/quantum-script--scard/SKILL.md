---
name: quantum-script--scard
description: >-
  How to use the Quantum Script smart card extension (quantum-script--scard),
  loaded with Script.requireExtension("SCard"), a thin layer over the
  Windows PC/SC API (winscard): SCard.establishContext() (context resource,
  undefined when the Smart Card service SCardSvr is not available),
  SCard.releaseContext(context), SCard.listReaders(context) (array of UTF-8
  reader names, [] when none), SCard.getStatusChange(context, reader) (true
  when a card is present, does not wait), SCard.connect(context, reader)
  (card resource, exclusive share mode, T=0 / T=1, undefined when no card /
  in use), SCard.disconnect(card) (powers the card down),
  SCard.getAtrString(context, card) or (card) (ATR as upper case hex),
  SCard.transmit(card, apdu) (APDU as hex text, spaces and lower case
  allowed, returns response data + SW1 SW2 as upper case hex, undefined on
  transmission failure); wrong or closed resources and invalid hex throw
  Error("invalid parameter"); resource lifetime and garbage collection;
  APDU structure, status words (9000, 61XX GET RESPONSE on T=0, 6CXX,
  6A82, 6982), contactless FF CA UID; recipes (wait for a card, T=0 get
  response, cleanup); the C++ side (registerInternalExtension,
  initExecutive, quantumScriptExtension entry point, winscard link,
  resources identified by their delete procedure). Use when writing or
  reviewing Quantum Script code that talks to smart cards or card readers,
  C++ code that includes <XYO/QuantumScript.Extension/SCard.hpp>, a
  fabricare.json depending on "quantum-script--scard", or when working
  inside the quantum-script--scard repository.
---

# quantum-script--scard

Smart card extension of Quantum Script (see the `quantum-script` skill for
the language and its differences from JavaScript; its rules apply — no
hoisting, `typeof(x)` needs parentheses, `substring(start, length)` takes
a **length**, statements and blocks end with `;`). Purpose: **list card
readers, detect cards, read the ATR and exchange APDUs** through the
Windows PC/SC resource manager.

Full documentation: `docs/` in the quantum-script--scard repository
(`X:\Storage\XYO\Gitea\CPP\quantum-script--scard\docs` on this machine):
README (purpose), getting-started (requirements, build, first script,
tests), **script-api** (every function, resources, hex format, APDUs,
status words, errors, limitations, recipes), cpp-api, reference (edge case
tables). When in doubt read
`source/XYO/QuantumScript.Extension/SCard/Library.cpp`.

## Script API

```javascript
Script.requireExtension("SCard");

var ctx = SCard.establishContext();          // Resource; undefined = no smart card service
var readers = SCard.listReaders(ctx);        // ["Reader 0", ...]; [] = no reader
SCard.getStatusChange(ctx, readers[0]);      // true = card present (current state, no wait)
var card = SCard.connect(ctx, readers[0]);   // Resource; undefined = no card / in use / mute
SCard.getAtrString(ctx, card);               // "3B8F80..." (also SCard.getAtrString(card))
var r = SCard.transmit(card, "00 A4 00 00 02 3F 00");  // "....9000"; undefined = failed
var sw = r.substring(r.length - 4);          // status word
SCard.disconnect(card);                      // true; powers the card down
SCard.releaseContext(ctx);                   // true
```

## Hard rules

1. **Windows only**, needs the Smart Card service (`SCardSvr`) and a
   reader. Without the service `establishContext()` returns `undefined`
   (PC/SC `SCARD_E_NO_SERVICE`, also in restricted / sandboxed sessions);
   always test it with `Script.isUndefined`.
2. **Two resource kinds**: context (`establishContext`) and card
   (`connect`). Passing the wrong kind, a non-resource, or a `close()`d /
   `drop()`ped resource throws `Error("invalid parameter")` (catchable,
   `e.message`). Never call `drop()` on them (leaks the PC/SC handle).
3. **Failures are values, not exceptions**: `undefined` (`establishContext`,
   `connect`, `getAtrString`, `transmit`), `false` (`getStatusChange`),
   `[]` (`listReaders`). No PC/SC error code is exposed.
4. **After `releaseContext` / `disconnect`** the resource stays valid:
   repeat calls return `true`, other functions return the failure value.
   A card keeps its context object alive; an explicit `releaseContext`
   still breaks its cards.
5. **`connect` is exclusive** (`SCARD_SHARE_EXCLUSIVE`, T=0 or T=1): a
   second `connect` to the same reader, or a card held by another program,
   gives `undefined`. Disconnect before reconnecting. `disconnect` always
   unpowers the card (session state, PIN verification lost).
6. **`transmit` hex input**: hex digits, upper or lower case, whitespace
   ignored, even digit count, 1..65544 bytes; anything else throws. Output
   is upper case hex, no spaces; last 4 digits = `SW1 SW2`.
7. **A card error is a normal response** (`"6A82"`, `"6982"`), not
   `undefined`. `undefined` means the transmission failed (card removed,
   disconnected, reader error).
8. **T=0 `61XX`** is not handled for you: send `GET RESPONSE`
   `"00C00000" + XX`. **`6CXX`**: resend with Le = `XX`.
9. **`getStatusChange` does not wait**: poll with
   `CurrentThread.sleep(ms)` (`Script.requireExtension("Thread")`) to wait
   for a card.
10. New threads start empty: `requireExtension("SCard")` and open a context
    inside the thread; use resources only in the thread that created them.

## Recipes

```javascript
// wait for a card (30 s)
Script.requireExtension("Thread");
function waitForCard(ctx, timeoutMs) {
	var k;
	var readers;
	var elapsed = 0;
	while (elapsed < timeoutMs) {
		readers = SCard.listReaders(ctx);
		for (k = 0; k < readers.length; ++k) {
			if (SCard.getStatusChange(ctx, readers[k])) {
				return readers[k];
			};
		};
		CurrentThread.sleep(250);
		elapsed += 250;
	};
	return null;
};

// command with status check
function apdu(card, command) {
	var r = SCard.transmit(card, command);
	if (Script.isUndefined(r)) {
		throw "transmit failed: " + command;
	};
	return {data: r.substring(0, r.length - 4), sw: r.substring(r.length - 4)};
};

// T=0: fetch the data announced by 61XX
var r = apdu(card, "00A4040007A0000000041010");
if (r.sw.substring(0, 2) == "61") {
	r = apdu(card, "00C00000" + r.sw.substring(2));
};

// contactless UID (PC/SC part 3 reader command)
var uid = apdu(card, "FFCA000000");
if (uid.sw == "9000") {
	Console.writeLn("UID " + uid.data);
};

// cleanup
try {
	// ...
} finally {
	if (Script.isResource(card)) {
		SCard.disconnect(card);
	};
	SCard.releaseContext(ctx);
};

// hex byte from a number
function hexByte(value) {
	var hex = "0123456789ABCDEF";
	return hex.substring((value >> 4) & 15, 1) + hex.substring(value & 15, 1);
};
```

Common status words: `9000` ok, `61XX` more data (T=0), `6CXX` wrong Le,
`6700` wrong length, `6982` security status not satisfied, `63CX` PIN
wrong (`X` tries left), `6A82` not found, `6A86` bad P1 P2, `6D00` INS not
supported, `6E00` CLA not supported.

## C++

```cpp
#include <XYO/QuantumScript.Extension/SCard.hpp>
using namespace XYO::QuantumScript;

void initExecutive(Executive *executive) {                   // host init callback
	Extension::SCard::registerInternalExtension(executive);  // scripts still requireExtension("SCard")
};
```

Native functions in `Library.cpp`, bound with `setFunction2`. Link
`winscard`. fabricare.json dependency: `"quantum-script--scard"`
(`dll-or-lib`). Resources are `VariableResource`s over reference counted
`SCardContext` / `SCardHandle` objects; the delete procedure
(`sCardContextDelete` / `sCardHandleDelete`) identifies the kind, checked
by `getContext` / `getHandle`.

## Working in this repository

- Build on Windows after clearing `NoDefaultCurrentDirectoryInExePath`
  (see the `fabricare` skill): `fabricare make`, `fabricare test`,
  `fabricare clean`.
- Tests: `test/test.0001.js` (functions exist, invalid arguments throw,
  context life cycle when the service is available) and
  `test/test.0002.js` (ATR, transmit, disconnect; needs a reader with a
  card). Missing service / reader / card → the part prints *skipped* and
  passes. They load `output/bin/quantum-script--scard.dll` when present.
  Raise the loop bound in `fabricare/test.js` when adding `test.0003.js`.
- Validate every resource argument with `getContext` / `getHandle`, check
  `isOk` before calling PC/SC.
- New functions or behavior changes: update `README.md`,
  `docs/script-api.md`, `docs/reference.md`, the tests and this skill.
- Code style: tabs, CRLF, `};` after blocks, camelCase. SPDX: MIT for
  `source/` and `docs/`, Unlicense for `test/`, `fabricare/` and
  `.claude/` (see `.reuse/dep5`).
