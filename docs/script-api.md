# Script API

```javascript
Script.requireExtension("SCard");
```

Defines the global object `SCard`. All functions are native (C++) and run
synchronously in the calling thread. Quantum Script language rules apply
(no hoisting, `typeof(x)` needs parentheses, `substring(start, length)`,
`;` after blocks); see the `quantum-script` repository's
`docs/language.md`.

- [Resources](#resources)
- [Functions](#functions)
- [Hex strings](#hex-strings)
- [APDUs](#apdus)
- [Errors](#errors)
- [Limitations](#limitations)
- [Recipes](#recipes)

## Resources

The extension hands out two kinds of native handles, both of script type
`Resource` (`Script.isResource(x)` is `true`):

| Resource | Made by | Wraps | Freed by |
|----------|---------|-------|----------|
| **context** | `establishContext()` | a PC/SC resource manager context (`SCARDCONTEXT`) | `releaseContext(context)`, `context.close()`, or the garbage collector |
| **card** | `connect(context, reader)` | a connection to the card in one reader (`SCARDHANDLE`) and its protocol | `disconnect(card)`, `card.close()`, or the garbage collector |

Rules:

- Every function checks the kind of resource it receives. A card where a
  context is expected, a context where a card is expected, any other value
  (`undefined`, number, string, object, ...) or a resource that was
  `close()`d throws `Error("invalid parameter")`.
- `releaseContext` and `disconnect` close the PC/SC handle but keep the
  resource valid: calling them again returns `true`, and the other
  functions return their failure value (`[]`, `false`, `undefined`) instead
  of throwing.
- A card keeps its context object alive, so a card stays usable when the
  script drops its last reference to the context; the context is released
  when both are gone. An explicit `releaseContext` still releases it at
  once, after which the cards made from it stop working.
- When the garbage collector frees a card that was not disconnected, it
  disconnects it (powering the card down); when it frees a context, it
  releases it. Disconnect explicitly when the order matters, e.g. before
  connecting again to the same reader (exclusive access).
- Do not call `drop()` on these resources: it forgets the handle without
  freeing it (the PC/SC handle leaks until the process ends) and the
  resource is then rejected by every function.

## Functions

### `SCard.establishContext()`

Opens a context with the smart card resource manager
(`SCardEstablishContext`, user scope).

Returns a **context** resource, or `undefined` when the service is not
available (`SCardSvr` stopped or no access to it, e.g. no reader connected
on some Windows versions, or a restricted session).

A context is needed by every other function except `transmit` and
`disconnect`. One context per script is enough; several are allowed.

### `SCard.releaseContext(context)`

Releases the context (`SCardReleaseContext`). Returns `true`, also when the
context was already released. Throws when `context` is not a context
resource.

### `SCard.listReaders(context)`

Returns an `Array` with the names of the readers known to the service
(`SCardListReaders`), in the order PC/SC reports them, e.g.
`["Alcor Micro USB Smart Card Reader 0", "ACS ACR122 0"]`.

Returns `[]` when there is no reader, on any PC/SC error and when the
context was released. Names are UTF-8 strings; pass them unchanged to
`getStatusChange` and `connect`. Throws when `context` is not a context
resource.

### `SCard.getStatusChange(context, reader)`

Returns `true` when a card is present in `reader`, `false` otherwise
(`SCardGetStatusChange` with the reader's state unknown, so it reports the
current state at once and does not wait for a change).

Returns `false` for an unknown reader name, on any PC/SC error and when the
context was released. A card can be present but still not usable (mute,
in use by another application): `connect` is the real test. `reader` is
converted to a string. Throws when `context` is not a context resource.

### `SCard.connect(context, reader)`

Connects to the card in `reader` (`SCardConnect`) with:

- share mode **exclusive** (`SCARD_SHARE_EXCLUSIVE`): no other application
  can use the card until it is disconnected;
- preferred protocols **T=0 or T=1** (`SCARD_PROTOCOL_Tx`); the protocol
  chosen by the reader is kept in the card resource and used by `transmit`.

Returns a **card** resource, or `undefined` when there is no card, the card
does not answer, it is already in use (by another application or by
another connection of this script), the reader name is unknown or the
context was released. `reader` is converted to a string. Throws when
`context` is not a context resource.

### `SCard.disconnect(card)`

Disconnects the card (`SCardDisconnect`) and **powers it down**
(`SCARD_UNPOWER_CARD`): the card loses its session state (selected
application, verified PIN, ...). Returns `true`, also when the card was
already disconnected. Throws when `card` is not a card resource.

### `SCard.getAtrString(context, card)`

### `SCard.getAtrString(card)`

Returns the card's ATR (Answer To Reset, `SCardGetAttrib` with
`SCARD_ATTR_ATR_STRING`) as upper case hex text, two digits per byte, e.g.
`"3B8F8001804F0CA000000306030001000000006A"`.

The ATR identifies the card type and its communication parameters; tools
such as the online *Smart card ATR parsing* lists map ATRs to card models.

Returns `undefined` when the card was disconnected, its context released,
or the reader does not report the attribute. With two arguments, the first
must be a context resource (it is checked, but the card's own context is
used); with one argument, it must be a card. Throws on any other value.

### `SCard.transmit(card, apdu)`

Sends the command APDU `apdu` to the card (`SCardTransmit`) and returns the
response APDU (response data followed by the status word `SW1 SW2`) as
upper case hex text.

- `apdu` is converted to a string of hex digits, upper or lower case;
  spaces, tabs, CR and LF anywhere are ignored, so `"00A4000C023F00"`,
  `"00 a4 00 0c 02 3f 00"` and a multi-line string are the same command.
- It must hold a whole number of bytes (an even number of digits), at
  least one byte and at most 65544 bytes (an extended length APDU), and no
  other character; otherwise `Error("invalid parameter")` is thrown.
- The response buffer holds up to 65538 bytes (65536 data bytes plus the
  status word).
- The PC/SC protocol header (T=0, T=1 or raw) is chosen from the protocol
  negotiated by `connect`.

Returns `undefined` when the card was disconnected or the transmission
failed (card removed, reader error, APDU refused by the reader driver).
A command the card rejects is **not** a failure: the response is the error
status word, e.g. `"6A82"` (file not found). Throws when `card` is not a
card resource.

## Hex strings

Both directions use hex text, one byte = two digits:

| Direction | Format |
|-----------|--------|
| into `transmit` | hex digits `0-9 A-F a-f`; whitespace ignored; even number of digits |
| out of `transmit`, `getAtrString` | upper case hex digits, no separators |

Useful string operations (`substring` takes a **length**, not an end
index):

```javascript
var sw = response.substring(response.length - 4);          // "9000"
var data = response.substring(0, response.length - 4);     // response data
var sw1 = response.substring(response.length - 4, 2);      // "61"
var sw2 = response.substring(response.length - 2);         // "1C"
var length = Convert.toNumber("0x" + sw2);                 // 28
```

To build hex from a number, see `hexByte` in the
[T=0 recipe](#t0-get-response).

## APDUs

A command APDU (ISO/IEC 7816-4) is:

| Field | Bytes | Meaning |
|-------|-------|---------|
| CLA | 1 | class: `00` interindustry, `80` / `84` proprietary, `FF` PC/SC reader commands |
| INS | 1 | instruction: `A4` SELECT, `B0` READ BINARY, `B2` READ RECORD, `C0` GET RESPONSE, `CA` GET DATA, `20` VERIFY, ... |
| P1 P2 | 2 | parameters of the instruction |
| Lc | 0, 1 or 3 | length of the data field (omitted when there is no data) |
| data | Lc | command data |
| Le | 0, 1, 2 or 3 | expected response length, `00` = up to 256 |

The response is `[data] SW1 SW2`. Common status words:

| SW | Meaning |
|----|---------|
| `9000` | success |
| `61XX` | success, `XX` more bytes waiting: send `GET RESPONSE` `00C00000XX` (T=0) |
| `6CXX` | wrong Le: repeat the command with Le = `XX` |
| `6700` | wrong length |
| `6982` | security status not satisfied (PIN / authentication needed) |
| `6983` | authentication method blocked |
| `63CX` | verification failed, `X` tries left |
| `6A82` | file or application not found |
| `6A86` | incorrect P1 P2 |
| `6D00` | instruction not supported |
| `6E00` | class not supported |

**T=0 cards** do not return response data directly for commands that send
data (case 4): they answer `61XX` and the data must be fetched with
`GET RESPONSE`. `transmit` does not do this automatically; see the
[recipe](#t0-get-response). T=1 cards return the data at once.

**Contactless readers** that follow PC/SC part 3 accept pseudo APDUs with
class `FF`, handled by the reader, e.g. `FFCA000000` returns the card's UID
(`GET DATA`), `FFCA010000` its historical bytes.

## Errors

| Situation | Result |
|-----------|--------|
| Not the expected SCard resource (wrong kind, any other value, closed resource) | throws `Error("invalid parameter")` |
| Invalid hex APDU (odd digits, other characters, empty, over 65544 bytes) | throws `Error("invalid parameter")` |
| Smart card service not available | `establishContext` returns `undefined` |
| No reader | `listReaders` returns `[]` |
| No card, card in use, card mute, unknown reader | `connect` returns `undefined`, `getStatusChange` `false` |
| Context released | `listReaders` `[]`, `getStatusChange` `false`, `connect` `undefined`, `getAtrString` `undefined` |
| Card disconnected or removed, reader error | `getAtrString` / `transmit` return `undefined` |
| Card rejects the command | `transmit` returns the error status word, e.g. `"6A82"` |

The PC/SC error code is not available to scripts: test the result with
`Script.isUndefined(x)` and check the service, reader and card when it
fails. Catch the exceptions with `try` / `catch`; the error object's
`message` is `"invalid parameter"`.

## Limitations

- **Windows only** (Windows PC/SC API). Linux pcsc-lite is not supported
  by this build.
- **Exclusive connection only**, **T=0 / T=1 only**: there is no shared or
  direct mode and no raw protocol request; `disconnect` always powers the
  card down (no leave / reset / eject choice).
- **No waiting**: `getStatusChange` reports the current state only; poll
  it to wait for a card (see recipes).
- **No transactions** (`SCardBeginTransaction`), **no control codes**
  (`SCardControl`), **no reader attributes** other than the ATR, **no
  reader groups**.
- **No automatic T=0 `GET RESPONSE`** or `6CXX` retry.
- **No PC/SC error codes**: failures are `undefined` / `false` / `[]`.
- A new Quantum Script thread starts with an empty global object: call
  `Script.requireExtension("SCard")` and open a context inside it. Use a
  context and its cards from the thread that created them.

## Recipes

### Wait for a card

```javascript
Script.requireExtension("Thread");   // CurrentThread.sleep

function waitForCard(context, timeoutMs) {
	var readers;
	var k;
	var elapsed = 0;
	while (elapsed < timeoutMs) {
		readers = SCard.listReaders(context);
		for (k = 0; k < readers.length; ++k) {
			if (SCard.getStatusChange(context, readers[k])) {
				return readers[k];
			};
		};
		CurrentThread.sleep(250);
		elapsed += 250;
	};
	return null;
};

var reader = waitForCard(context, 30000);
```

### Status word and data

```javascript
function apdu(card, command) {
	var response = SCard.transmit(card, command);
	if (Script.isUndefined(response)) {
		throw "transmit failed: " + command;
	};
	return {
		data: response.substring(0, response.length - 4),
		sw: response.substring(response.length - 4)
	};
};

var r = apdu(card, "00A4040007A0000000041010");
if (r.sw != "9000") {
	Console.writeLn("SELECT failed: " + r.sw);
};
```

### T=0 GET RESPONSE

```javascript
function hexByte(value) {
	var hex = "0123456789ABCDEF";
	return hex.substring((value >> 4) & 15, 1) + hex.substring(value & 15, 1);
};

function transmitFull(card, command) {
	var response = SCard.transmit(card, command);
	var sw1;
	var sw2;
	var data = "";
	while (!Script.isUndefined(response)) {
		sw1 = response.substring(response.length - 4, 2);
		sw2 = response.substring(response.length - 2);
		if (sw1 == "61") {
			data += response.substring(0, response.length - 4);
			response = SCard.transmit(card, "00C00000" + sw2);
			continue;
		};
		if (sw1 == "6C") {
			// repeat with the right Le
			response = SCard.transmit(card, command.substring(0, command.length - 2) + sw2);
			continue;
		};
		return data + response;
	};
	return undefined;
};
```

(The `6C` retry assumes `command` ends with a one byte Le and is written
without spaces. `hexByte(28)` is `"1C"`: use it to compute lengths, e.g.
`"00B00000" + hexByte(size)`.)

### Contactless card UID

```javascript
var r = SCard.transmit(card, "FFCA000000");
if (!Script.isUndefined(r)) {
	if (r.substring(r.length - 4) == "9000") {
		Console.writeLn("UID " + r.substring(0, r.length - 4));
	};
};
```

### Always clean up

```javascript
var context = SCard.establishContext();
var card;
try {
	card = SCard.connect(context, reader);
	// ...
} finally {
	if (Script.isResource(card)) {
		SCard.disconnect(card);
	};
	SCard.releaseContext(context);
};
```
