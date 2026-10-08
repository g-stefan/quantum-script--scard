# Quantum Script Extension SCard

Smart card access for Quantum Script through the Windows PC/SC API
(`winscard`): list card readers, detect inserted cards, read the ATR and
exchange APDU commands, with APDUs written as hex text.

```javascript
Script.requireExtension("SCard");

SCard.establishContext();
SCard.releaseContext(context);
SCard.listReaders(context);
SCard.getStatusChange(context,reader);
SCard.connect(context,reader);
SCard.disconnect(card);
SCard.getAtrString(context,card);
SCard.getAtrString(card);
SCard.transmit(card,string);
```

## Documentation

See [docs](docs/README.md): [getting started](docs/getting-started.md),
[script API](docs/script-api.md), [C++ API](docs/cpp-api.md),
[reference](docs/reference.md).

## License

Copyright (c) 2016-2026 Grigore Stefan
Licensed under the [MIT](LICENSE) license.
