// Quantum Script Extension SCard
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY_HPP
#define XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY_HPP

#ifndef XYO_QUANTUMSCRIPT_EXTENSION_SCARD_DEPENDENCY_HPP
#	include <XYO/QuantumScript.Extension/SCard/Dependency.hpp>
#endif

namespace XYO::QuantumScript::Extension::SCard {

	XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT void initExecutive(Executive *executive, void *extensionId);
	XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT void registerInternalExtension(Executive *executive);

};

#endif
