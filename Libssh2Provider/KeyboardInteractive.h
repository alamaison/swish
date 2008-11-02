/*  Handles keyboard-interactive authentication via a callback.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    In addition, as a special exception, the the copyright holders give you
	permission to combine this program with free software programs or the 
	OpenSSL project's "OpenSSL" library (or with modified versions of it, 
	with unchanged license). You may copy and distribute such a system 
	following the terms of the GNU GPL for this program and the licenses 
	of the other code concerned. The GNU General Public License gives 
	permission to release a modified version without this exception; this 
	exception also makes it possible to release a modified version which 
	carries forward this exception.
*/

#pragma once

#include <atlsafe.h>         // CComSafeArray

class CKeyboardInteractive
{
public:
	typedef CComSafeArray<BSTR> PromptArray;
	typedef CComSafeArray<VARIANT_BOOL> EchoArray;
	typedef CComSafeArray<BSTR> ResponseArray;

	CKeyboardInteractive(__in ISftpConsumer *pConsumer) throw();

	void SetErrorState(HRESULT hr) throw();
	HRESULT GetErrorState() throw();
	
	__callback static void OnKeyboardInteractive(
		__in_bcount(name_len) const char *name, int name_len,
		__in_bcount(instruction_len) const char *instruction,
		int instruction_len, int num_prompts,
		__in_ecount(num_prompts) const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts, 
		__out_ecount(num_prompts) LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
		__deref_inout void **abstract) throw();

private:
	inline ResponseArray _SendRequest(
		__in const BSTR bstrName, __in const BSTR bstrInstruction,
		__in PromptArray& saPrompts, __in EchoArray& saShowPrompts) throw(...);

	static inline PromptArray _PackPromptSafearray(
		int cPrompts,
		__in_ecount(cPrompts) const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts)
		throw();

	static inline CComSafeArray<VARIANT_BOOL> _PackEchoSafearray(
		int cPrompts,
		__in_ecount(cPrompts) const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts)
		throw();
	
	static inline void _ProcessResponses(
		__in ResponseArray& saResponses,
		int cPrompts,
		__out_ecount(cPrompts) LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses)
		throw();

	CComPtr<ISftpConsumer> m_spConsumer;

	/** @name Delayed-error holder */
	// As we cannot safely throw an exception throw the C code which calls
	// OnKeyboardInteractive, we must cache any error here and the
	// code which invokes the Libssh2 C library call must check for an error
	// afterwards.
	// @{
	HRESULT m_hr;
	// @}
};
