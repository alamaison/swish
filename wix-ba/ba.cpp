/**
    @file

    Installer bootstrapper application.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#include "ba.h"

#include "BalBaseBootstrapperApplication.h" // CBalBaseBootstrapperApplication

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH
#include <winapi/gui/progress.hpp> // progress

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <boost/scoped_ptr.hpp> // scoped_ptr

#include <balutil.h> // BalLogError

#include <exception>
#include <string>

using winapi::gui::progress;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::scoped_ptr;

using std::exception;
using std::wstring;

template<> struct comet::comtype<IBootstrapperApplication>
{
	static const IID& uuid() throw()
	{
		static uuid_t iid = uuid_t("53C31D56-49C0-426B-AB06-099D717C67FE");
		return iid;
	}
	typedef IUnknown base;
};

class swish_bootstrapper_application : public CBalBaseBootstrapperApplication
{
	typedef IBootstrapperApplication interface_is;

public:

	swish_bootstrapper_application(
		IBootstrapperEngine* engine, BOOTSTRAPPER_RESTART restart) :
	CBalBaseBootstrapperApplication(engine, restart), m_engine(engine)
	{
	}

public: // IBootstrapperApplication

	virtual STDMETHODIMP OnStartup()
	{
		try
		{
			m_progress.reset(
				new progress(
					NULL, L"Installing Swish", progress::modality::non_modal,
					progress::time_estimation::automatic_time_estimate,
					progress::bar_type::progress, progress::minimisable::yes,
					progress::cancellability::cancellable));

			HRESULT hr = m_engine->CloseSplashScreen();
			if (FAILED(hr))
				::BalLogError(hr, "Couldn't close splash screen");

			hr = m_engine->Detect();
			if (FAILED(hr))
				::BalLogError(hr, "Couldn't start detection");
		}
		WINAPI_COM_CATCH();

		return S_OK;
	}

    virtual STDMETHODIMP_(int) OnShutdown()
    {
		m_progress.reset();
        return IDNOACTION;
    }

    virtual STDMETHODIMP_(int) OnDetectBegin(DWORD /*cPackages*/)
    {
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detecting");
				m_progress->line(2, L"");
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnDetectPriorBundle(LPCWSTR wzBundleId)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected prior bundle");
				m_progress->line(2, wzBundleId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
	}

	virtual STDMETHODIMP_(int) OnDetectRelatedBundle(
		LPCWSTR wzBundleId, LPCWSTR /*wzBundleTag*/, BOOL /*fPerMachine*/,
		DWORD64 /*dw64Version*/, BOOTSTRAPPER_RELATED_OPERATION /*operation*/)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected related bundle");
				m_progress->line(2, wzBundleId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
	}

    virtual STDMETHODIMP_(int) OnDetectPackageBegin(LPCWSTR wzPackageId)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected package");
				m_progress->line(2, wzPackageId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnDetectRelatedMsiPackage(
        LPCWSTR wzPackageId, LPCWSTR /*wzProductCode*/,
		BOOL /*fPerMachine*/, DWORD64 /*dw64Version*/,
		BOOTSTRAPPER_RELATED_OPERATION /*operation*/) 
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected related MSI package");
				m_progress->line(2, wzPackageId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnDetectTargetMsiPackage(
        LPCWSTR wzPackageId, LPCWSTR /*wzProductCode*/,
        BOOTSTRAPPER_PACKAGE_STATE /*patchState*/)
    {
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected target MSI package");
				m_progress->line(2, wzPackageId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnDetectMsiFeature(
        LPCWSTR wzPackageId, LPCWSTR wzFeatureId,
        BOOTSTRAPPER_FEATURE_STATE /*state*/)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Detected MSI feature");

				wstring message = wzPackageId;
				message += L" (";
				message += wzFeatureId;
				message += L")";
				m_progress->line(2, message);

				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(void) OnDetectComplete(HRESULT hr)
	{
		try
		{
			if (FAILED(hr))
			{
				m_progress->line(2, L"Detection failed");
				::BalLogError(hr, "Detection failed");
			}
			else
			{
				m_progress->line(2, L"Detecting finished");

				hr = m_engine->Plan(BOOTSTRAPPER_ACTION_INSTALL);
				if (FAILED(hr))
					::BalLogError(hr, "Couldn't start planning");
			}
		}
		catch (const exception&) {}
    }

    virtual STDMETHODIMP_(int) OnPlanBegin(DWORD /*cPackages*/)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Planning");
				m_progress->line(2, L"");
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnPlanRelatedBundle(
        LPCWSTR wzBundleId, BOOTSTRAPPER_REQUEST_STATE* pRequestedState)
	{
		try
		{
			m_progress->line(1, L"Planning related bundle");
			m_progress->line(2, wzBundleId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnPlanRelatedBundle(
			wzBundleId, pRequestedState);
    }

    virtual STDMETHODIMP_(int) OnPlanPackageBegin(
        LPCWSTR wzPackageId, BOOTSTRAPPER_REQUEST_STATE* pRequestState)
	{
		try
		{
			m_progress->line(1, L"Planning package");
			m_progress->line(2, wzPackageId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnPlanPackageBegin(
			wzPackageId, pRequestState);
    }

    virtual STDMETHODIMP_(int) OnPlanTargetMsiPackage(
        LPCWSTR wzPackageId, LPCWSTR /*wzProductCode*/,
        BOOTSTRAPPER_REQUEST_STATE* /*pRequestedState*/)
	{
		if (CheckCanceled())
		{
			return IDCANCEL;
		}
		else
		{
			try
			{
				m_progress->line(1, L"Planning target MSI package");
				m_progress->line(2, wzPackageId);
				return IDNOACTION;
			}
			catch (const exception&) { return IDNOACTION; }
		}
    }

    virtual STDMETHODIMP_(int) OnPlanMsiFeature(
        LPCWSTR wzPackageId, LPCWSTR wzFeatureId,
		BOOTSTRAPPER_FEATURE_STATE* pRequestedState)
	{
		try
		{
			m_progress->line(1, L"Planning MSI feature");

			wstring message = wzPackageId;
			message += L" (";
			message += wzFeatureId;
			message += L")";
			m_progress->line(2, message);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnPlanMsiFeature(
			wzPackageId, wzFeatureId, pRequestedState);
    }

    virtual STDMETHODIMP_(void) OnPlanComplete(HRESULT hr)
	{
		try
		{
			if (FAILED(hr))
			{
				m_progress->line(2, L"Planning failed");
				::BalLogError(hr, "Planning failed");
			}
			else
			{
				m_progress->line(2, L"Planning finished");

				hr = m_engine->Apply(NULL);
				if (FAILED(hr))
					::BalLogError(hr, "Couldn't apply plan");
			}
		}
		catch (const exception&) {}
    }

    virtual STDMETHODIMP_(int) OnApplyBegin()
    {
		try
		{
			m_progress->line(1, L"Applying plan");
			m_progress->line(2, L"");
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnApplyBegin();
    }

    virtual STDMETHODIMP_(int) OnApplyComplete(
		HRESULT hr, BOOTSTRAPPER_APPLY_RESTART restart)
	{
		try
		{
			if (FAILED(hr))
			{
				m_progress->line(2, L"Applying failed");
				::BalLogError(hr, "Applying failed");
			}
			else
			{
				m_progress->line(2, L"Applying finished");
			}
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnApplyComplete(hr, restart);
    }

    virtual STDMETHODIMP_(int) OnCacheBegin()
	{
		try
		{
			m_progress->line(1, L"Caching");
			m_progress->line(2, L"");
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnCacheBegin();
    }

    virtual STDMETHODIMP_(int) OnCachePackageBegin(
        LPCWSTR wzPackageId, DWORD cCachePayloads,
        DWORD64 dw64PackageCacheSize)
	{
		try
		{
			m_progress->line(1, L"Caching package");
			m_progress->line(2, wzPackageId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnCachePackageBegin(
			wzPackageId, cCachePayloads, dw64PackageCacheSize);
    }

    virtual STDMETHODIMP_(int) OnCacheAcquireBegin(
        LPCWSTR wzPackageOrContainerId, LPCWSTR wzPayloadId,
		BOOTSTRAPPER_CACHE_OPERATION operation, LPCWSTR wzSource)
	{
		try
		{
			m_progress->line(1, L"Acquiring");
			m_progress->line(2, wzPackageOrContainerId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnCacheAcquireBegin(
			wzPackageOrContainerId, wzPayloadId, operation, wzSource);
    }

    virtual STDMETHODIMP_(int) OnCacheAcquireProgress(
        LPCWSTR wzPackageOrContainerId, LPCWSTR wzPayloadId,
        DWORD64 dw64Progress, DWORD64 dw64Total, DWORD dwOverallPercentage)
	{
		try
		{
			wstring message = wzPackageOrContainerId;
			message += L" (";
			message += wzPayloadId;
			message += L")";
			m_progress->line(2, message);
			m_progress->update(dw64Progress, dw64Total);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnCacheAcquireProgress(
			wzPackageOrContainerId, wzPayloadId, dw64Progress, dw64Total,
			dwOverallPercentage);
    }

    virtual STDMETHODIMP_(int) OnCacheVerifyBegin(
        LPCWSTR wzPackageId, LPCWSTR wzPayloadId)
	{
		try
		{
			m_progress->line(1, L"Verifying");
			wstring message = wzPackageId;
			message += L" (";
			message += wzPayloadId;
			message += L")";
			m_progress->line(2, message);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnCacheVerifyBegin(
			wzPackageId, wzPayloadId);
    }

    virtual STDMETHODIMP_(int) OnExecuteBegin(DWORD cExecutingPackages)
	{
		try
		{
			m_progress->line(1, L"Executing");
			m_progress->line(2, L"");
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnExecuteBegin(
			cExecutingPackages);
    }

    virtual STDMETHODIMP_(int) OnExecutePackageBegin(
        LPCWSTR wzPackageId, BOOL fExecute)
	{
		try
		{
			m_progress->line(1, L"Executing package");
			m_progress->line(2, wzPackageId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnExecutePackageBegin(
			wzPackageId, fExecute);
    }

    virtual STDMETHODIMP_(int) OnExecutePatchTarget(
        LPCWSTR wzPackageId, LPCWSTR wzTargetProductCode)
	{
		try
		{
			m_progress->line(1, L"Executing patch target");
			m_progress->line(2, wzPackageId);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnExecutePatchTarget(
			wzPackageId, wzTargetProductCode);
    }

    virtual STDMETHODIMP_(int) OnProgress(
		DWORD dwProgressPercentage, DWORD dwOverallProgressPercentage)
    {
		try
		{
			m_progress->update(
				dwProgressPercentage, dwOverallProgressPercentage);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnProgress(
			dwProgressPercentage, dwOverallProgressPercentage);
    }

    virtual STDMETHODIMP_(int) OnDownloadPayloadBegin(
        LPCWSTR wzPayloadId, LPCWSTR wzPayloadFileName)
	{
		try
		{
			m_progress->line(1, L"Downloading payload");
			wstring message = wzPayloadId;
			message += L" (";
			message += wzPayloadFileName;
			message += L")";
			m_progress->line(2, message);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnDownloadPayloadBegin(
			wzPayloadId, wzPayloadFileName);
    }

    virtual STDMETHODIMP_(int) OnExecuteProgress(
		LPCWSTR wzPackageId, DWORD dwProgressPercentage,
		DWORD dwOverallProgressPercentage)
	{
		try
		{
			m_progress->line(2, wzPackageId);
			m_progress->update(
				dwProgressPercentage, dwOverallProgressPercentage);
		}
		catch (const exception&) {}

		return CBalBaseBootstrapperApplication::OnExecuteProgress(
			wzPackageId, dwProgressPercentage, dwOverallProgressPercentage);
    }

protected:

    BOOL CheckCanceled()
    {
		try
		{
			if (m_progress->user_cancelled())
			{
				PromptCancel(
					NULL, FALSE, L"Swish Installer",
					L"Are you sure you want to cancel");
			}
		}
		catch (const exception&)
		{
			// Force cancel
			PromptCancel(NULL, TRUE, L"", L"");
		}

		return CBalBaseBootstrapperApplication::CheckCanceled();
    }

private:
	com_ptr<IBootstrapperEngine> m_engine;
	scoped_ptr<progress> m_progress;
};

namespace {

	com_ptr<IBootstrapperApplication> create_bootstrap_app(
		IBootstrapperEngine* engine, const BOOTSTRAPPER_COMMAND* command)
	{
		try
		{
			return new swish_bootstrapper_application(engine, command->restart);
		}
		catch (const com_error& e)
		{
			BalLogError(
				e.hr(), "Failed to create the Swish bootstrap application: %s",
				e.what());
			throw;
		}
		catch (const std::exception& e)
		{
			BalLogError(
				E_FAIL, "Failed to create the Swish bootstrap application: %s",
				e.what());
			throw;
		}
		catch (...)
		{
			BalLogError(
				E_UNEXPECTED,
				"Failed to create the Swish bootstrap application "
				"(unknown source)");
			throw;
		}
	}

}


BOOL WINAPI DllMain(
	HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*reserved*/)
{
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hModule);
        break;
		
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

HRESULT WINAPI BootstrapperApplicationCreate(
	__in IBootstrapperEngine* engine,
	__in const BOOTSTRAPPER_COMMAND* command,
	__out IBootstrapperApplication** application_out)
{
	::BalInitialize(engine);

	try
	{
		*application_out = create_bootstrap_app(engine, command).detach();
	}
	WINAPI_COM_CATCH()

	return S_OK;
}

void WINAPI BootstrapperApplicationDestroy()
{
	::BalUninitialize();
}
