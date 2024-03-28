// Quantum Script Extension SCard
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <windows.h>
#include <winscard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XYO_OS_TYPE_WIN
#	ifdef XYO_MEMORY_LEAK_DETECTOR
#		include "vld.h"
#	endif
#endif

#include <XYO/QuantumScript.Extension/SCard/Library.hpp>
#include <XYO/QuantumScript.Extension/SCard/Copyright.hpp>
#include <XYO/QuantumScript.Extension/SCard/License.hpp>
#include <XYO/QuantumScript.Extension/SCard/Version.hpp>

namespace XYO::QuantumScript::Extension::SCard {

	class SCardContext : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(SCardContext);

		public:
			SCARDCONTEXT context;
			bool isOk;

			inline SCardContext() {
				isOk = false;
			};

			inline ~SCardContext() {
				close();
			};

			inline void close() {
				if (isOk) {
					isOk = false;
					SCardReleaseContext(context);
				};
			};
	};

	class SCardHandle : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(SCardHandle);

		public:
			SCARDHANDLE handle;
			DWORD dwActiveProtocol;
			bool isOk;

			inline SCardHandle() {
				isOk = false;
			};

			inline ~SCardHandle() {
				close();
			};

			inline void close() {
				if (isOk) {
					isOk = false;
					SCardDisconnect(handle, SCARD_UNPOWER_CARD);
				};
			};
	};

	static void sCardContextDelete(void *value) {
		((SCardContext *)value)->decReferenceCount();
	};

	static void sCardHandleDelete(void *value) {
		((SCardHandle *)value)->decReferenceCount();
	};

	static TPointer<Variable> establishContext(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-establish-context\n");
#endif
		LONG retV;
		TPointer<SCardContext> sCardContext;
		sCardContext.newMemory();

		retV = SCardEstablishContext(
		    SCARD_SCOPE_USER,
		    NULL,
		    NULL,
		    &sCardContext->context);

		if (retV == SCARD_S_SUCCESS) {
			sCardContext->isOk = true;
			sCardContext->incReferenceCount();
			return VariableResource::newVariable(sCardContext.value(), sCardContextDelete);
		};

		return VariableUndefined::newVariable();
	};

	static TPointer<Variable> releaseContext(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-release-context\n");
#endif
		((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->close();
		return VariableBoolean::newVariable(true);
	};

	static TPointer<Variable> listReaders(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-list-readers\n");
#endif
		LONG retV;
		TPointer<Variable> strList;
		LPSTR mszReaders;
		LPSTR mszReader;
		DWORD pcchReaders;

		strList = VariableArray::newVariable();
		mszReaders = NULL;
		pcchReaders = SCARD_AUTOALLOCATE;
		retV = SCardListReaders(
		    ((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->context,
		    NULL,
		    (LPSTR)&mszReaders,
		    &pcchReaders);
		if (retV == SCARD_S_SUCCESS) {
			if (mszReaders != NULL) {
				int count_;
				count_ = 0;
				mszReader = mszReaders;
				while (*mszReader != 0) {
					strList->setPropertyByIndex(count_, VariableString::newVariable(mszReader));
					++count_;
					mszReader += strlen(mszReader) + 1;
				};
				SCardFreeMemory(
				    ((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->context,
				    mszReaders);
			};
		};
		return strList;
	};

	static TPointer<Variable> getStatusChange(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-get-status-change\n");
#endif

		LONG retV;
		CHAR szReader[256];
		SCARD_READERSTATE rgReaderStates;

		strcpy(szReader, (char *)(((arguments->index(1))->toString()).value()));

		rgReaderStates.szReader = szReader;
		rgReaderStates.dwCurrentState = SCARD_STATE_UNAWARE;
		rgReaderStates.dwEventState = SCARD_STATE_UNAWARE;

		retV = SCardGetStatusChange(
		    ((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->context,
		    10,
		    &rgReaderStates,
		    1);

		if (retV == SCARD_S_SUCCESS) {
			if ((rgReaderStates.dwEventState & SCARD_STATE_PRESENT) == SCARD_STATE_PRESENT) {
				return VariableBoolean::newVariable(true);
			};
		};

		return VariableBoolean::newVariable(false);
	};

	static TPointer<Variable> connect(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-connect\n");
#endif

		LONG retV;
		TPointer<SCardHandle> handle;

		handle.newMemory();

		retV = SCardConnect(
		    ((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->context,
		    (char *)(((arguments->index(1))->toString()).value()),
		    SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_Tx,
		    &handle->handle,
		    &handle->dwActiveProtocol);
		if (retV == SCARD_S_SUCCESS) {
			handle->isOk = true;
			handle->incReferenceCount();
			return VariableResource::newVariable(handle.value(), NULL);
		};

		return VariableUndefined::newVariable();
	};

	static TPointer<Variable> disconnect(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-disconnect\n");
#endif

		((SCardHandle *)(((VariableResource *)(arguments->index(0)).value())->resource))->close();
		return VariableBoolean::newVariable(true);
	};

	static TPointer<Variable> getAtrString(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-get-atr-string\n");
#endif

		LONG retV;
		LPBYTE pbAttr;
		DWORD pcbAttrLen;
		unsigned int byteScan;

		pbAttr = NULL;
		pcbAttrLen = SCARD_AUTOALLOCATE;
		retV = SCardGetAttrib(
		    ((SCardHandle *)(((VariableResource *)(arguments->index(1)).value())->resource))->handle,
		    SCARD_ATTR_ATR_STRING,
		    (LPBYTE)&pbAttr,
		    &pcbAttrLen);
		if (retV == SCARD_S_SUCCESS) {
			char buf[4];
			int k;
			Variable *strValue = VariableString::newVariable("");
			for (k = 0; k < pcbAttrLen; ++k) {
				byteScan = pbAttr[k];
				sprintf(buf, "%02X", byteScan);
				((VariableString *)strValue)->value << (&buf[0]);
			};
			SCardFreeMemory(((SCardContext *)(((VariableResource *)(arguments->index(0)).value())->resource))->context, pbAttr);
			return strValue;
		};
		return VariableUndefined::newVariable();
	};

	static TPointer<Variable> transmit(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef QUANTUM_SCRIPT_VM_DEBUG_RUNTIME
		printf("- scard-transmit\n");
#endif

		LONG retV;
		LPCSCARD_IO_REQUEST pioSendPci;
		DWORD dwActiveProtocol;
		BYTE sendBuffer[4096];
		DWORD sendLength;
		BYTE recvBuffer[4096];
		DWORD recvLength;
		String inScan;
		unsigned int byteScan;
		SCARD_IO_REQUEST pioSendPciIO;

		dwActiveProtocol = ((SCardHandle *)(((VariableResource *)(arguments->index(0)).value())->resource))->dwActiveProtocol;

		switch (dwActiveProtocol) {
		case SCARD_PROTOCOL_T0:
			pioSendPci = SCARD_PCI_T0;
			pioSendPciIO = *SCARD_PCI_T0;
			break;
		case SCARD_PROTOCOL_T1:
			pioSendPci = SCARD_PCI_T1;
			pioSendPciIO = *SCARD_PCI_T1;
			break;
		default:
			pioSendPci = SCARD_PCI_RAW;
			pioSendPciIO = *SCARD_PCI_RAW;
			break;
		};

		inScan = (arguments->index(1))->toString();
		for (sendLength = 0; sendLength < (inScan.length() / 2); ++sendLength) {
			if (sscanf((char *)(inScan.index(sendLength * 2)), "%02X", &byteScan) != 1) {
				byteScan = 0;
			};
			sendBuffer[sendLength] = byteScan;
		};

		recvLength = 4096;

		retV = SCardTransmit(
		    ((SCardHandle *)(((VariableResource *)(arguments->index(0)).value())->resource))->handle,
		    pioSendPci,
		    sendBuffer,
		    sendLength,
		    &pioSendPciIO,
		    recvBuffer,
		    &recvLength);
		if (retV == SCARD_S_SUCCESS) {
			char buf[4];
			int k;
			Variable *strValue = VariableString::newVariable("");
			for (k = 0; k < recvLength; ++k) {
				byteScan = recvBuffer[k];
				sprintf((char *)buf, "%02X", byteScan);
				((VariableString *)strValue)->value << buf;
			};
			return strValue;
		};
		return VariableUndefined::newVariable();
	};

	void registerInternalExtension(Executive *executive) {
		executive->registerInternalExtension("SCard", initExecutive);
	};

	void initExecutive(Executive *executive, void *extensionId) {
		String info = "SCard\r\n";
		info << License::shortLicense().c_str();

		executive->setExtensionName(extensionId, "SCard");
		executive->setExtensionInfo(extensionId, info);
		executive->setExtensionVersion(extensionId, Extension::SCard::Version::versionWithBuild());
		executive->setExtensionPublic(extensionId, true);

		executive->compileStringX("var SCard={};");
		executive->setFunction2("SCard.establishContext()", establishContext);
		executive->setFunction2("SCard.releaseContext(context)", releaseContext);
		executive->setFunction2("SCard.listReaders(context)", listReaders);
		executive->setFunction2("SCard.getStatusChange(context,reader)", getStatusChange);
		executive->setFunction2("SCard.connect(context,reader)", connect);
		executive->setFunction2("SCard.disconect(card)", disconnect);
		executive->setFunction2("SCard.getAtrString(context,card)", getAtrString);
		executive->setFunction2("SCard.transmit(card,string)", transmit);
	};

};

#ifdef XYO_PLATFORM_COMPILE_DYNAMIC_LIBRARY
extern "C" XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT void quantumScriptExtension(XYO::QuantumScript::Executive *executive, void *extensionId) {
	XYO::QuantumScript::Extension::SCard::initExecutive(executive, extensionId);
};
#endif
