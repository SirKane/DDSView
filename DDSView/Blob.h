#ifndef BLOB_H
#define BLOB_H
#include <stdint.h>

class CBlob{
protected:
	void*	m_pData = nullptr;
	size_t	m_Size = 0;
	size_t	m_Capacity = 0;
public:
	inline CBlob(){
	}
	~CBlob();
	CBlob(const CBlob &) = delete;
	CBlob(CBlob&&rhs) : m_pData(rhs.m_pData), m_Size(rhs.m_Size), 
		m_Capacity(rhs.m_Capacity){
	}
	CBlob &operator=(const CBlob&) = delete;
	CBlob &operator=(CBlob&&rhs);
	bool Resize(size_t size);
	inline void* GetPointer(){
		return m_pData;
	}
	inline const void* GetPointer() const{
		return m_pData;
	}
	inline size_t Size() const{
		return m_Size;
	}
};

bool LoadFileToBlob(const char* pFileName, CBlob &blob);
bool LoadFileToBlob(const wchar_t* pFileName, CBlob &blob);
bool WriteBlobToFile(const char* pFileName, const CBlob &blob);
bool WriteBlobToFile(const wchar_t* pFileName, const CBlob &blob);

#endif //!BLOB_H
