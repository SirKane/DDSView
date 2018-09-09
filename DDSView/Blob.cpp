#include <stdio.h>
#include <stdlib.h>

#include "Blob.h"

CBlob::~CBlob(){
	if (m_pData != nullptr){
		free(m_pData);
	}
}


CBlob &CBlob::operator=(CBlob&&rhs){
	if (this != &rhs){
		void* pTempData = m_pData;
		size_t tempSize = m_Size;
		size_t tempCapacity = m_Capacity;
		m_pData = rhs.m_pData;
		m_Size = rhs.m_Size;
		m_Capacity = rhs.m_Capacity;

		rhs.m_pData = pTempData;
		rhs.m_Size = tempSize;
		rhs.m_Capacity = tempCapacity;
	}
	return *this;
}

bool CBlob::Resize(size_t size){
	if (size <= m_Capacity && m_pData != nullptr){
		m_Size = size;
		return true;
	}
	m_pData = realloc(m_pData, size);
	m_Capacity = m_Size = size;

	return m_pData != nullptr;
}

bool LoadFileToBlob(const char* pFileName, CBlob &blob){
	FILE* pFile;
	if (fopen_s(&pFile, pFileName, "rb") != 0){
		return false;
	}
	if (fseek(pFile, 0, SEEK_END) != 0){
		fclose(pFile);
		return false;
	}
	long length = ftell(pFile);
	if (length == -1){
		fclose(pFile);
		return false;
	}
	if (!blob.Resize((size_t)length)){
		fclose(pFile);
		return false;
	}
	if (fseek(pFile, 0, SEEK_SET) != 0){
		fclose(pFile);
		return false;
	}
	if (fread(blob.GetPointer(), 1, blob.Size(), pFile) == blob.Size()){
		fclose(pFile);
		return true;
	}
	fclose(pFile);
	return false;
}


bool LoadFileToBlob(const wchar_t* pFileName, CBlob &blob){
	FILE* pFile;
	if (_wfopen_s(&pFile, pFileName, L"rb") != 0){
		return false;
	}
	if (fseek(pFile, 0, SEEK_END) != 0){
		fclose(pFile);
		return false;
	}
	long length = ftell(pFile);
	if (length == -1){
		fclose(pFile);
		return false;
	}
	if (!blob.Resize((size_t)length)){
		fclose(pFile);
		return false;
	}
	if (fseek(pFile, 0, SEEK_SET) != 0){
		fclose(pFile);
		return false;
	}
	if (fread(blob.GetPointer(), 1, blob.Size(), pFile) == blob.Size()){
		fclose(pFile);
		return true;
	}
	fclose(pFile);
	return false;
}


bool WriteBlobToFile(const char* pFileName, const CBlob &blob){
	FILE* pFile;
	if (fopen_s(&pFile, pFileName, "wb") != 0){
		return false;
	}
	if (blob.Size() != 0 &&
		fwrite(blob.GetPointer(), blob.Size(), 1, pFile) != 1){
		fclose(pFile);
		return false;
	}
	fclose(pFile);
	return true;
}

bool WriteBlobToFile(const wchar_t* pFileName, const CBlob &blob){
	FILE* pFile;
	if (_wfopen_s(&pFile, pFileName, L"wb") != 0){
		return false;
	}
	if (blob.Size() != 0 &&
		fwrite(blob.GetPointer(), blob.Size(), 1, pFile) != 1){
		fclose(pFile);
		return false;
	}
	fclose(pFile);
	return true;
}
