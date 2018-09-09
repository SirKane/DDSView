#ifndef FILEDIALOGEVENTS_H
#define FILEDIALOGEVENTS_H


#include <Shobjidl.h>
#include "String.h"

class CFileDialogEvents : public IFileDialogEvents{
protected:
	ULONG	m_RefCount = 1;
public:
	CFileDialogEvents();
	virtual ~CFileDialogEvents();
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, 
		void **ppvObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
	virtual ULONG STDMETHODCALLTYPE Release(void) override;
	virtual HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialog *pfd) override;
	virtual HRESULT STDMETHODCALLTYPE OnFolderChanging(IFileDialog *pfd,
		IShellItem *psiFolder) override;
	virtual HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnSelectionChange(IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnShareViolation(IFileDialog *pfd,
		IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pResponse) override;

	virtual HRESULT STDMETHODCALLTYPE OnTypeChange(IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnOverwrite(IFileDialog *pfd, IShellItem *psi,
		FDE_OVERWRITE_RESPONSE *pResponse) override;
};

class CFileDialogEventsGetLastFolder : public CFileDialogEvents{
protected:
	CStringW	m_LastFolder;
public:

	virtual HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialog *pfd) override;
	const CStringW &GetLastFolder() const;
};

#endif //!FILEDIALOGEVENTS_H
