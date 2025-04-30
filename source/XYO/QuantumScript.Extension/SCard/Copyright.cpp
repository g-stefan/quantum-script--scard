// Quantum Script Extension SCard
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/QuantumScript.Extension/SCard/Copyright.hpp>
#include <XYO/QuantumScript.Extension/SCard/Copyright.rh>

namespace XYO::QuantumScript::Extension::SCard::Copyright {

	static const char *copyright_ = XYO_QUANTUMSCRIPT_EXTENSION_SCARD_COPYRIGHT;
	static const char *publisher_ = XYO_QUANTUMSCRIPT_EXTENSION_SCARD_PUBLISHER;
	static const char *company_ = XYO_QUANTUMSCRIPT_EXTENSION_SCARD_COMPANY;
	static const char *contact_ = XYO_QUANTUMSCRIPT_EXTENSION_SCARD_CONTACT;

	const char *copyright() {
		return copyright_;
	};

	const char *publisher() {
		return publisher_;
	};

	const char *company() {
		return company_;
	};

	const char *contact() {
		return contact_;
	};

};
