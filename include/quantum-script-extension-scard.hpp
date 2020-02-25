//
// Quantum Script Extension SCard
//
// Copyright (c) 2020 Grigore Stefan <g_stefan@yahoo.com>
// Created by Grigore Stefan <g_stefan@yahoo.com>
//
// MIT License (MIT) <http://opensource.org/licenses/MIT>
//

#ifndef QUANTUM_SCRIPT_EXTENSION_SCARD_HPP
#define QUANTUM_SCRIPT_EXTENSION_SCARD_HPP

#ifndef QUANTUM_SCRIPT_HPP
#include "quantum-script.hpp"
#endif

#ifndef QUANTUM_SCRIPT_EXTENSION_SCARD__EXPORT_HPP
#include "quantum-script-extension-scard--export.hpp"
#endif

#ifndef QUANTUM_SCRIPT_EXTENSION_SCARD_COPYRIGHT_HPP
#include "quantum-script-extension-scard-copyright.hpp"
#endif

#ifndef QUANTUM_SCRIPT_EXTENSION_SCARD_LICENSE_HPP
#include "quantum-script-extension-scard-license.hpp"
#endif

#ifndef QUANTUM_SCRIPT_EXTENSION_SCARD_VERSION_HPP
#include "quantum-script-extension-scard-version.hpp"
#endif

namespace Quantum {
	namespace Script {
		namespace Extension {
			namespace SCard {

				using namespace Quantum::Script;

				QUANTUM_SCRIPT_EXTENSION_SCARD_EXPORT void initExecutive(Executive *executive, void *extensionId);

			};
		};
	};
};

#endif
