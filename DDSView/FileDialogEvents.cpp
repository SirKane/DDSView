#include "FileDialogEvents.h"
#include "RefCount.h"

/*
CFileDialogEvents

*/

CFileDialogEvents::CFileDialogEvents(){
}
CFileDialogEvents::~CFileDialogEvents(){
}
HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::QueryInterface(REFIID riid, void **ppvObject){

	if (riid == IID_IUnknown){
		AddRef();
		*ppvObject = (IUnknown*)this;
		return S_OK;
	}
	if (riid == IID_IFileDialogEvents){
		AddRef();
		*ppvObject = (IFileDialogEvents*)this;
		return S_OK;
	}
	return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE 
CFileDialogEvents::AddRef(void){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE 
CFileDialogEvents::Release(void){
	if (--m_RefCount == 0){
		delete this;
		return 0;
	}
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnFileOk(IFileDialog *pfd){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnFolderChanging(IFileDialog *pfd,
	IShellItem *psiFolder){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnFolderChange(IFileDialog *pfd){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnSelectionChange(IFileDialog *pfd){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnShareViolation(IFileDialog *pfd, IShellItem *psi, 
	FDE_SHAREVIOLATION_RESPONSE *pResponse){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnTypeChange(IFileDialog *pfd){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CFileDialogEvents::OnOverwrite(IFileDialog *pfd, IShellItem *psi,
	FDE_OVERWRITE_RESPONSE *pResponse){
	return E_NOTIMPL;
}

/*
CFileDialogEventsGetLastFolder

*/


HRESULT STDMETHODCALLTYPE
CFileDialogEventsGetLastFolder::OnFolderChange(IFileDialog *pfd){

	TRefCountHandle<IShellItem> folderItem;
	HRESULT hr;
	if (pfd){
		hr = pfd->GetFolder(folderItem.GetAddress());
		if (SUCCEEDED(hr)){
			LPWSTR pPath;
			hr = folderItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
			if (SUCCEEDED(hr)){
				m_LastFolder = pPath;
				CoTaskMemFree(pPath);
			}
		}
	}
	return S_OK;
}
const CStringW &CFileDialogEventsGetLastFolder::GetLastFolder() const{
	return m_LastFolder;
}