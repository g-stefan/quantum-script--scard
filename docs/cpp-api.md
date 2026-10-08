# C++ API

For hosts that embed Quantum Script and for maintainers of the extension.
Read the `quantum-script` repository's `docs/embedding.md` and
`docs/writing-extensions.md` first.

## Headers and namespace

```cpp
#include <XYO/QuantumScript.Extension/SCard.hpp>   // Library.hpp

using namespace XYO::QuantumScript;
```

Namespace: `XYO::QuantumScript::Extension::SCard`. Export macro:
`XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT`:

| Define | Effect |
|--------|--------|
| `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_INTERNAL` (or `QUANTUM_SCRIPT__SCARD_INTERNAL`, set by fabricare while building the DLL) | export macro = `XYO_PLATFORM_LIBRARY_EXPORT` |
| none | export macro = `XYO_PLATFORM_LIBRARY_IMPORT` (consumers of the DLL) |
| `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY` | export macro empty, no `quantumScriptExtension` entry point |
| `XYO_PLATFORM_COMPILE_STATIC` (static platforms) | `XYO_PLATFORM_LIBRARY_EXPORT` / `IMPORT` are empty |
| `XYO_QUANTUMSCRIPT_DEBUG_RUNTIME` | every native function prints its name (`- scard-transmit`, ...) when called |

The library links `winscard` (`"library": ["winscard"]` in
`fabricare.json`); a host that compiles the sources itself must link it
too.

## Registering the extension

```cpp
void Extension::SCard::registerInternalExtension(Executive *executive);
void Extension::SCard::initExecutive(Executive *executive, void *extensionId);
```

- `registerInternalExtension` registers `"SCard"` as an internal
  extension; call it from the host's init callback:

  ```cpp
  #include <XYO/QuantumScript.hpp>
  #include <XYO/QuantumScript.Extension/SCard.hpp>

  using namespace XYO::QuantumScript;

  void initExecutive(Executive *executive) {
  	Extension::SCard::registerInternalExtension(executive);
  };
  ```

  Scripts still call `Script.requireExtension("SCard")`.
- `initExecutive` is the extension's init function, run by the engine when a
  script first requires `SCard` in a thread. It sets the extension name,
  info (`"SCard"` plus the short license text), version and public flag,
  runs `var SCard={};` and binds the native functions with
  `setFunction2`:

  | Script name | C++ function | PC/SC calls |
  |-------------|--------------|-------------|
  | `SCard.establishContext()` | `establishContext` | `SCardEstablishContext(SCARD_SCOPE_USER)` |
  | `SCard.releaseContext(context)` | `releaseContext` | `SCardReleaseContext` |
  | `SCard.listReaders(context)` | `listReaders` | `SCardListReadersW`, `SCardFreeMemory` |
  | `SCard.getStatusChange(context,reader)` | `getStatusChange` | `SCardGetStatusChangeW`, 10 ms timeout, state unaware |
  | `SCard.connect(context,reader)` | `connect` | `SCardConnectW(SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_Tx)` |
  | `SCard.disconnect(card)` | `disconnect` | `SCardDisconnect(SCARD_UNPOWER_CARD)` |
  | `SCard.getAtrString(context,card)` | `getAtrString` | `SCardGetAttrib(SCARD_ATTR_ATR_STRING)`, `SCardFreeMemory` |
  | `SCard.transmit(card,string)` | `transmit` | `SCardTransmit` with `SCARD_PCI_T0` / `T1` / `RAW` |

- The DLL build also exports
  `extern "C" void quantumScriptExtension(Executive *, void *)`, which
  forwards to `initExecutive`; it is what `Script.requireExtension` looks up
  in `quantum-script--scard.dll`. It is compiled only when
  `XYO_PLATFORM_COMPILE_DYNAMIC_LIBRARY` is defined and
  `XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY` is not.

## How resources are represented

Both script handles are `VariableResource`s whose `resource` points to a
reference counted `Object` (file-local classes in `Library.cpp`):

```cpp
class SCardContext : public Object {     // establishContext
	SCARDCONTEXT context;
	bool isOk;                           // false after close()
	void close();                        // SCardReleaseContext once
};

class SCardHandle : public Object {      // connect
	TPointer<SCardContext> sCardContext; // context used to connect, kept alive
	SCARDHANDLE handle;
	DWORD dwActiveProtocol;              // SCARD_PROTOCOL_T0 / T1, selects the PCI header
	bool isOk;                           // false after close()
	void close();                        // SCardDisconnect(SCARD_UNPOWER_CARD) once
};
```

- The resource holds one reference (`incReferenceCount` before
  `VariableResource::newVariable`); the delete procedure
  (`sCardContextDelete` / `sCardHandleDelete`) drops it. The destructors
  call `close()`, so the garbage collector, `resource.close()` and the end
  of the script release the PC/SC handles.
- **The delete procedure identifies the kind of resource.**
  `getContext(value)` / `getHandle(value)` accept a value only when it is a
  `VariableResource`, its `resource` is not `nullptr` and its
  `resourceDelete` is `sCardContextDelete` / `sCardHandleDelete`; anything
  else throws `Error("invalid parameter")`. This is what keeps a context
  passed as a card, a resource of another extension or a closed resource
  from being cast to the wrong type.
- `SCardHandle::sCardContext` is a plain `TPointer` (the context cannot
  point back to the card, so no cycle): `getAtrString` frees the
  auto-allocated ATR buffer with the card's own context.
- Reader names cross the API as UTF-16 (`...W` functions) and are converted
  to / from the script's UTF-8 strings with `UTF::utf8FromUTF16` /
  `UTF::utf16FromUTF8`, so there is no fixed size reader buffer.
- `transmit` decodes the hex text into a `std::vector<BYTE>` (at most
  `transmitSendMax` = 65544 bytes) and receives into a
  `transmitReceiveMax` = 65538 byte buffer; invalid hex throws before the
  card is used.

## Notes for maintainers

- All native code is in `SCard/Library.cpp`. Keep the
  `XYO_QUANTUMSCRIPT_DEBUG_RUNTIME` trace line at the start of every
  function.
- Validate every resource argument with `getContext` / `getHandle` (or
  `isResourceOf` for optional forms) before using it; never cast
  `arguments->index(n)` directly.
- After `releaseContext` / `disconnect` the resource stays valid with
  `isOk == false`: functions must check `isOk` and return their failure
  value, not call PC/SC with a closed handle.
- New functions or behavior changes: update `README.md`,
  `docs/script-api.md`, `docs/reference.md`, the skill in
  `.claude/skills/quantum-script--scard/` and the tests in `test/`.
- Tests: `fabricare/test.js` runs `test/test.000k.js` for `k` from 1 to 2;
  raise the bound when adding `test.0003.js`. They load
  `output/bin/quantum-script--scard.dll` when it exists, the installed
  extension otherwise; run `fabricare make` before `fabricare test`. The
  parts that need the smart card service, a reader or a card skip
  themselves when those are missing, so run the tests once on a machine
  with a reader and a card after changing card code.
- Code style: tabs, `.clang-format`, CRLF, `};` after blocks, camelCase.
  SPDX: MIT for `source/` and `docs/`, Unlicense for `test/`, `fabricare/`
  and `.claude/` (see `.reuse/dep5`).
