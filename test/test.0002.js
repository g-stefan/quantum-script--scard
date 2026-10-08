// Public domain
// http://unlicense.org/
Script.requireExtension("Console");
Script.requireExtension("Shell");

// Prefer the extension just built in output/bin over the installed one
if (Shell.fileExists("output/bin/quantum-script--scard.dll")) {
	Script.requireExtension("output/bin/SCard");
} else {
	Script.requireExtension("SCard");
};

// Card operations, needs a reader with a card inserted, skipped without one

var failed = 0;

function check(name, value, expected) {
	if (value === expected) {
		return;
	};
	Console.writeLn("-> " + name + ": got [" + value + "] expected [" + expected + "]");
	++failed;
};

function throws(name, fn) {
	try {
		fn();
	} catch (e) {
		return;
	};
	Console.writeLn("-> " + name + ": no exception");
	++failed;
};

function isHex(value) {
	if (!Script.isString(value)) {
		return false;
	};
	if ((value.length % 2) != 0) {
		return false;
	};
	for (var k = 0; k < value.length; ++k) {
		if ("0123456789ABCDEF".indexOf(value.substring(k, 1)) < 0) {
			return false;
		};
	};
	return true;
};

function run() {
	var context = SCard.establishContext();
	if (Script.isUndefined(context)) {
		Console.writeLn("-> test 0002: no smart card service, skipped");
		return;
	};

	var readers = SCard.listReaders(context);
	var reader = null;
	for (var k = 0; k < readers.length; ++k) {
		if (SCard.getStatusChange(context, readers[k])) {
			reader = readers[k];
			break;
		};
	};
	if (reader == null) {
		Console.writeLn("-> test 0002: no card present, skipped");
		SCard.releaseContext(context);
		return;
	};

	Console.writeLn("-> reader: " + reader);

	var card = SCard.connect(context, reader);
	if (Script.isUndefined(card)) {
		// Exclusive access, another application may hold the card
		Console.writeLn("-> test 0002: card in use, skipped");
		SCard.releaseContext(context);
		return;
	};
	check("card type", Script.isResource(card), true);

	// A card is not a context
	throws("listReaders card", function() {
		SCard.listReaders(card);
	});

	// ATR, both call forms
	var atr = SCard.getAtrString(context, card);
	Console.writeLn("-> ATR: " + atr);
	check("atr hex", isHex(atr), true);
	check("atr length", (atr.length >= 4), true);
	check("atr one argument", SCard.getAtrString(card), atr);

	// SELECT the master file, any card answers with at least SW1 SW2
	var response = SCard.transmit(card, "00A40000023F00");
	Console.writeLn("-> SELECT 3F00: " + response);
	check("transmit hex", isHex(response), true);
	check("transmit sw", (response.length >= 4), true);

	// Whitespace and lower case are accepted
	check("transmit spaced", isHex(SCard.transmit(card, "00 a4 00 00 02 3f 00")), true);

	// Invalid hex
	throws("transmit odd", function() {
		SCard.transmit(card, "00A400000");
	});
	throws("transmit not hex", function() {
		SCard.transmit(card, "00A4ZZ00");
	});
	throws("transmit empty", function() {
		SCard.transmit(card, "");
	});

	check("disconnect", SCard.disconnect(card), true);
	check("disconnect twice", SCard.disconnect(card), true);
	check("atr disconnected", SCard.getAtrString(card), undefined);
	check("transmit disconnected", SCard.transmit(card, "00A40000023F00"), undefined);

	// Card released by the garbage collector, context still usable
	card = SCard.connect(context, reader);
	check("connect again", Script.isResource(card), true);
	card = null;
	check("readers after", SCard.listReaders(context).length, readers.length);

	SCard.releaseContext(context);
};

run();

if (failed > 0) {
	throw "test 0002 failed";
};
Console.writeLn("-> test 0002 ok");
