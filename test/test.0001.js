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

// API surface and argument checks, no reader or card needed

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

// Functions
var names = ["establishContext", "releaseContext", "listReaders", "getStatusChange", "connect", "disconnect", "getAtrString", "transmit"];
for (var k = 0; k < names.length; ++k) {
	check("function " + names[k], Script.isFunction(SCard[names[k]]), true);
};
check("no misspelled disconect", Script.isUndefined(SCard.disconect), true);

// Values that are not an SCard resource are rejected
var invalid = [undefined, null, 0, 1, "context", true, {}, []];
for (var k = 0; k < invalid.length; ++k) {
	var value = invalid[k];
	throws("releaseContext invalid " + k, function() {
		SCard.releaseContext(value);
	});
	throws("listReaders invalid " + k, function() {
		SCard.listReaders(value);
	});
	throws("getStatusChange invalid " + k, function() {
		SCard.getStatusChange(value, "reader");
	});
	throws("connect invalid " + k, function() {
		SCard.connect(value, "reader");
	});
	throws("disconnect invalid " + k, function() {
		SCard.disconnect(value);
	});
	throws("getAtrString invalid " + k, function() {
		SCard.getAtrString(value, value);
	});
	throws("getAtrString one invalid " + k, function() {
		SCard.getAtrString(value);
	});
	throws("transmit invalid " + k, function() {
		SCard.transmit(value, "00A4040000");
	});
};

// Context, needs the Smart Card service (SCardSvr), skipped without it
var context = SCard.establishContext();
if (Script.isUndefined(context)) {
	Console.writeLn("-> test 0001: no smart card service, context checks skipped");
} else {
	check("context type", Script.isResource(context), true);

	var readers = SCard.listReaders(context);
	check("readers type", Script.isArray(readers), true);
	for (var k = 0; k < readers.length; ++k) {
		check("reader name " + k, Script.isString(readers[k]), true);
		check("reader status " + k, Script.isBoolean(SCard.getStatusChange(context, readers[k])), true);
	};

	check("status unknown reader", SCard.getStatusChange(context, "quantum-script--scard unknown reader"), false);
	check("connect unknown reader", SCard.connect(context, "quantum-script--scard unknown reader"), undefined);

	// Long reader name, overflowed a 256 byte buffer before
	var longName = "";
	for (var k = 0; k < 64; ++k) {
		longName += "quantum-script--scard ";
	};
	check("status long reader", SCard.getStatusChange(context, longName), false);
	check("connect long reader", SCard.connect(context, longName), undefined);

	// A context is not a card
	throws("disconnect context", function() {
		SCard.disconnect(context);
	});
	throws("transmit context", function() {
		SCard.transmit(context, "00A4040000");
	});
	throws("getAtrString context", function() {
		SCard.getAtrString(context);
	});
	throws("getAtrString context context", function() {
		SCard.getAtrString(context, context);
	});

	// Released context
	check("release", SCard.releaseContext(context), true);
	check("release twice", SCard.releaseContext(context), true);
	check("readers released", SCard.listReaders(context).length, 0);
	check("status released", SCard.getStatusChange(context, "reader"), false);
	check("connect released", SCard.connect(context, "reader"), undefined);

	// Closed resource
	var context2 = SCard.establishContext();
	check("context2 type", Script.isResource(context2), true);
	context2.close();
	throws("listReaders closed", function() {
		SCard.listReaders(context2);
	});
	throws("release closed", function() {
		SCard.releaseContext(context2);
	});
};

if (failed > 0) {
	throw "test 0001 failed";
};
Console.writeLn("-> test 0001 ok");
