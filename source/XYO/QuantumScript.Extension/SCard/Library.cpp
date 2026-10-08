// Quantum Script Extension SCard
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <windows.h>
#include <winscard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <vector>

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

	// Extended APDU: header (4) + Lc (3) + data (65535) + Le (2)
	static const size_t transmitSendMax = 65544;
	// Extended APDU response: data (65536) + SW1 SW2 (2)
	static const size_t transmitReceiveMax = 65538;

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
			// Context used to connect, kept alive while the card handle exists
			TPointer<SCardContext> sCardContext;
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

	// The resource delete procedure identifies the kind of resource,
	// any other value (or a closed resource) is rejected

	static bool isResourceOf(Variable *value, VariableResource::ResourceDelete resourceDelete) {
		if (!TIsType<VariableResource>(value)) {
			return false;
		};
		if (((VariableResource *)value)->resource == nullptr) {
			return false;
		};
		return (((VariableResource *)value)->resourceDelete == resourceDelete);
	};

	static SCardContext *getContext(Variable *value) {
		if (!isResourceOf(value, sCardContextDelete)) {
			throw(Error("invalid parameter"));
		};
		return (SCardContext *)(((VariableResource *)value)->resource);
	};

	static SCardHandle *getHandle(Variable *value) {
		if (!isResourceOf(value, sCardHandleDelete)) {
			throw(Error("invalid parameter"));
		};
		return (SCardHandle *)(((VariableResource *)value)->resource);
	};

	static int hexDigit(char value) {
		if ((value >= '0') && (value <= '9')) {
			return value - '0';
		};
		if ((value >= 'A') && (value <= 'F')) {
			return value - 'A' + 10;
		};
		if ((value >= 'a') && (value <= 'f')) {
			return value - 'a' + 10;
		};
		return -1;
	};

	// Hex digits, whitespace between them is ignored
	static bool hexDecode(const String &in, std::vector<BYTE> &out) {
		const char *scan = in.value();
		int high = -1;
		int digit;
		for (; *scan != 0; ++scan) {
			if ((*scan == ' ') || (*scan == '\t') || (*scan == '\r') || (*scan == '\n')) {
				continue;
			};
			digit = hexDigit(*scan);
			if (digit < 0) {
				return false;
			};
			if (high < 0) {
				high = digit;
				continue;
			};
			out.push_back((BYTE)((high << 4) | digit));
			high = -1;
		};
		return (high < 0);
	};

	static String hexEncode(const BYTE *in, size_t length) {
		static const char *hex = "0123456789ABCDEF";
		String retV;
		char buf[3];
		size_t k;
		buf[2] = 0;
		for (k = 0; k < length; ++k) {
			buf[0] = hex[(in[k] >> 4) & 0x0F];
			buf[1] = hex[in[k] & 0x0F];
			retV << buf;
		};
		return retV;
	};

	static TPointer<Variable> establishContext(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
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
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-release-context\n");
#endif
		getContext(arguments->index(0))->close();
		return VariableBoolean::newVariable(true);
	};

	static TPointer<Variable> listReaders(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-list-readers\n");
#endif
		LONG retV;
		TPointer<Variable> strList;
		LPWSTR mszReaders;
		LPWSTR mszReader;
		DWORD pcchReaders;
		SCardContext *sCardContext;

		sCardContext = getContext(arguments->index(0));
		strList = VariableArray::newVariable();
		if (!sCardContext->isOk) {
			return strList;
		};

		mszReaders = NULL;
		pcchReaders = SCARD_AUTOALLOCATE;
		retV = SCardListReadersW(
		    sCardContext->context,
		    NULL,
		    (LPWSTR)&mszReaders,
		    &pcchReaders);
		if (retV == SCARD_S_SUCCESS) {
			if (mszReaders != NULL) {
				int count_;
				count_ = 0;
				mszReader = mszReaders;
				while (*mszReader != 0) {
					strList->setPropertyByIndex(count_, VariableString::newVariable(UTF::utf8FromUTF16((const utf16 *)mszReader)));
					++count_;
					mszReader += wcslen(mszReader) + 1;
				};
				SCardFreeMemory(sCardContext->context, mszReaders);
			};
		};
		return strList;
	};

	static TPointer<Variable> getStatusChange(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-get-status-change\n");
#endif

		LONG retV;
		SCARD_READERSTATEW rgReaderStates;
		SCardContext *sCardContext;
		StringUTF16 reader;

		sCardContext = getContext(arguments->index(0));
		if (!sCardContext->isOk) {
			return VariableBoolean::newVariable(false);
		};

		reader = UTF::utf16FromUTF8((arguments->index(1))->toString().value());

		memset(&rgReaderStates, 0, sizeof(rgReaderStates));
		rgReaderStates.szReader = (LPCWSTR)reader.value();
		rgReaderStates.dwCurrentState = SCARD_STATE_UNAWARE;
		rgReaderStates.dwEventState = SCARD_STATE_UNAWARE;

		retV = SCardGetStatusChangeW(
		    sCardContext->context,
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
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-connect\n");
#endif

		LONG retV;
		TPointer<SCardHandle> handle;
		SCardContext *sCardContext;
		StringUTF16 reader;

		sCardContext = getContext(arguments->index(0));
		if (!sCardContext->isOk) {
			return VariableUndefined::newVariable();
		};

		reader = UTF::utf16FromUTF8((arguments->index(1))->toString().value());

		handle.newMemory();
		handle->sCardContext = sCardContext;

		retV = SCardConnectW(
		    sCardContext->context,
		    (LPCWSTR)reader.value(),
		    SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_Tx,
		    &handle->handle,
		    &handle->dwActiveProtocol);
		if (retV == SCARD_S_SUCCESS) {
			handle->isOk = true;
			handle->incReferenceCount();
			return VariableResource::newVariable(handle.value(), sCardHandleDelete);
		};

		return VariableUndefined::newVariable();
	};

	static TPointer<Variable> disconnect(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-disconnect\n");
#endif

		getHandle(arguments->index(0))->close();
		return VariableBoolean::newVariable(true);
	};

	static TPointer<Variable> getAtrString(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-get-atr-string\n");
#endif

		LONG retV;
		LPBYTE pbAttr;
		DWORD pcbAttrLen;
		SCardHandle *handle;

		// getAtrString(context, card) or getAtrString(card)
		if (isResourceOf(arguments->index(0), sCardHandleDelete)) {
			handle = getHandle(arguments->index(0));
		} else {
			getContext(arguments->index(0));
			handle = getHandle(arguments->index(1));
		};

		if (!handle->isOk) {
			return VariableUndefined::newVariable();
		};
		if (!handle->sCardContext->isOk) {
			return VariableUndefined::newVariable();
		};

		pbAttr = NULL;
		pcbAttrLen = SCARD_AUTOALLOCATE;
		retV = SCardGetAttrib(
		    handle->handle,
		    SCARD_ATTR_ATR_STRING,
		    (LPBYTE)&pbAttr,
		    &pcbAttrLen);
		if (retV == SCARD_S_SUCCESS) {
			String atr;
			if (pbAttr != NULL) {
				atr = hexEncode(pbAttr, pcbAttrLen);
				// Allocated by the resource manager of the card context
				SCardFreeMemory(handle->sCardContext->context, pbAttr);
			};
			return VariableString::newVariable(atr);
		};
		return VariableUndefined::newVariable();
	};

	static TPointer<Variable> transmit(VariableFunction *function, Variable *this_, VariableArray *arguments) {
#ifdef XYO_QUANTUMSCRIPT_DEBUG_RUNTIME
		printf("- scard-transmit\n");
#endif

		LONG retV;
		LPCSCARD_IO_REQUEST pioSendPci;
		std::vector<BYTE> sendBuffer;
		std::vector<BYTE> recvBuffer;
		DWORD recvLength;
		SCardHandle *handle;

		handle = getHandle(arguments->index(0));

		if (!hexDecode((arguments->index(1))->toString(), sendBuffer)) {
			throw(Error("invalid parameter"));
		};
		if (sendBuffer.size() == 0) {
			throw(Error("invalid parameter"));
		};
		if (sendBuffer.size() > transmitSendMax) {
			throw(Error("invalid parameter"));
		};

		if (!handle->isOk) {
			return VariableUndefined::newVariable();
		};

		switch (handle->dwActiveProtocol) {
		case SCARD_PROTOCOL_T0:
			pioSendPci = SCARD_PCI_T0;
			break;
		case SCARD_PROTOCOL_T1:
			pioSendPci = SCARD_PCI_T1;
			break;
		default:
			pioSendPci = SCARD_PCI_RAW;
			break;
		};

		recvBuffer.resize(transmitReceiveMax);
		recvLength = (DWORD)recvBuffer.size();

		retV = SCardTransmit(
		    handle->handle,
		    pioSendPci,
		    sendBuffer.data(),
		    (DWORD)sendBuffer.size(),
		    NULL,
		    recvBuffer.data(),
		    &recvLength);
		if (retV == SCARD_S_SUCCESS) {
			return VariableString::newVariable(hexEncode(recvBuffer.data(), recvLength));
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
		executive->setFunction2("SCard.disconnect(card)", disconnect);
		executive->setFunction2("SCard.getAtrString(context,card)", getAtrString);
		executive->setFunction2("SCard.transmit(card,string)", transmit);
	};

};

#ifndef XYO_QUANTUMSCRIPT_EXTENSION_SCARD_LIBRARY
#	ifdef XYO_PLATFORM_COMPILE_DYNAMIC_LIBRARY
extern "C" XYO_QUANTUMSCRIPT_EXTENSION_SCARD_EXPORT void quantumScriptExtension(XYO::QuantumScript::Executive *executive, void *extensionId) {
	XYO::QuantumScript::Extension::SCard::initExecutive(executive, extensionId);
};
#	endif
#endif
