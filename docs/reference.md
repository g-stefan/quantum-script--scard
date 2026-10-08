# API reference

## Script

```javascript
Script.requireExtension("SCard");
```

| Function | Returns | Failure value | Throws `Error("invalid parameter")` when |
|----------|---------|---------------|------------------------------------------|
| `SCard.establishContext()` | context resource | `undefined` (no smart card service) | never |
| `SCard.releaseContext(context)` | `true` (also when already released) | — | `context` is not a context |
| `SCard.listReaders(context)` | `Array` of reader names (UTF-8) | `[]` (no reader, error, context released) | `context` is not a context |
| `SCard.getStatusChange(context, reader)` | `true` card present | `false` (no card, unknown reader, error, context released) | `context` is not a context |
| `SCard.connect(context, reader)` | card resource (exclusive, T=0 / T=1) | `undefined` (no card, in use, mute, unknown reader, context released) | `context` is not a context |
| `SCard.disconnect(card)` | `true` (also when already disconnected); powers the card down | — | `card` is not a card |
| `SCard.getAtrString(context, card)` | ATR, upper case hex | `undefined` (disconnected, context released, attribute not available) | `context` is not a context or `card` is not a card |
| `SCard.getAtrString(card)` | ATR, upper case hex | `undefined` | `card` is not a card |
| `SCard.transmit(card, apdu)` | response data + `SW1 SW2`, upper case hex | `undefined` (disconnected, card removed, reader error) | `card` is not a card, or `apdu` is not valid hex (odd digits, other characters, empty, > 65544 bytes) |

"Is not a context / card" covers: any value that is not a resource, a
resource of the other SCard kind, a resource of another extension and a
resource after `close()` or `drop()`.

### Edge cases

| Call | Result |
|------|--------|
| `SCard.releaseContext(ctx)` twice | `true`, `true` |
| `SCard.listReaders(ctx)` after `releaseContext(ctx)` | `[]` |
| `SCard.connect(ctx, r)` after `releaseContext(ctx)` | `undefined` |
| `ctx.close()` then `SCard.listReaders(ctx)` | throws |
| `SCard.getStatusChange(ctx, "no such reader")` | `false` |
| `SCard.getStatusChange(ctx, <1000 character name>)` | `false` |
| `SCard.connect(ctx, r)` twice on the same reader | second call `undefined` (exclusive) until the first card is disconnected |
| `SCard.disconnect(card)` twice | `true`, `true` |
| `SCard.transmit(card, x)` after `disconnect(card)` | `undefined` (after the hex check) |
| `SCard.transmit(card, "00 a4 00 00 02 3f 00")` | same as `"00A40000023F00"` |
| `SCard.transmit(card, "00A400000")` | throws (odd number of digits) |
| `SCard.transmit(card, "")`, `SCard.transmit(card, "  ")` | throws (no byte) |
| `SCard.transmit(card, "0x00")` | throws (`x` is not a hex digit) |
| `SCard.transmit(card, apdu)` the card rejects | the error status word, e.g. `"6A82"`, not `undefined` |
| `SCard.transmit(ctx, apdu)` | throws (a context is not a card) |
| `SCard.getAtrString(ctx, ctx)` | throws |
| card dropped without `disconnect` | disconnected (powered down) when the garbage collector frees it |
| context dropped while a card made from it is alive | context kept until the card is freed |

### Status words

| SW | Meaning |
|----|---------|
| `9000` | success |
| `61XX` | `XX` bytes available: `GET RESPONSE` `00C00000XX` |
| `6CXX` | wrong Le, use `XX` |
| `6700` | wrong length |
| `6982` | security status not satisfied |
| `63CX` | verification failed, `X` tries left |
| `6A82` | file / application not found |
| `6A86` | incorrect P1 P2 |
| `6D00` | INS not supported |
| `6E00` | CLA not supported |

## C++

| Symbol | Description |
|--------|-------------|
| `#include <XYO/QuantumScript.Extension/SCard.hpp>` | umbrella header |
| `namespace XYO::QuantumScript::Extension::SCard` | |
| `void registerInternalExtension(Executive *executive)` | register `"SCard"` as an internal extension |
| `void initExecutive(Executive *executive, void *extensionId)` | extension init: metadata, `var SCard={};`, native functions |
| `extern "C" void quantumScriptExtension(Executive *, void *)` | DLL entry point, forwards to `initExecutive` (not with `..._SCARD_LIBRARY`) |
| `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT` | export / import macro |
| `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_INTERNAL`, `QUANTUM_SCRIPT__SCARD_INTERNAL` | building the DLL: export |
| `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY` | compiled into a host: no export, no entry point |
| `XYO_QUANTUMSCRIPT_DEBUG_RUNTIME` | trace native calls with `printf` |
| `Version::version()`, `build()`, `versionWithBuild()`, `datetime()` | library version (`Version.hpp`) |
| `License::license()`, `License::shortLicense()` | MIT license text (`License.hpp`) |
| `Copyright::copyright()`, `publisher()`, `company()`, `contact()` | copyright text (`Copyright.hpp`) |

| Build | Value |
|-------|-------|
| fabricare project | `quantum-script--scard`, `make: dll-or-lib` |
| dependencies | `quantum-script`, `quantum-script--console` |
| system library | `winscard` |
| output | `quantum-script--scard.dll` / `.lib` |
| platform | Windows |
