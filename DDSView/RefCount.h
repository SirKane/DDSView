
#ifndef SHARED_REFCOUNT_H
#define SHARED_REFCOUNT_H

#include <Windows.h>

template<typename T> class TRefCountHandle{
protected:
	T*	m_pValue = nullptr;
public:
	inline TRefCountHandle(){
	}
	inline explicit TRefCountHandle(T* pValue) : m_pValue(pValue){
	}
	inline TRefCountHandle(const TRefCountHandle &b) : m_pValue(b.m_pValue){
		if (m_pValue){
			m_pValue->AddRef();
		}
	}
	inline TRefCountHandle(TRefCountHandle&&b) : m_pValue(b.m_pValue){
		b.m_pValue = nullptr;
	}
	inline ~TRefCountHandle(){
		if (m_pValue){
			m_pValue->Release();
			m_pValue = nullptr;
		}
	}
	inline TRefCountHandle &operator=(const TRefCountHandle &b){
		if (this != &b){
			m_pValue = b.m_pValue;
			if (m_pValue){
				m_pValue->AddRef();
			}
		}
		return *this;
	}
	inline TRefCountHandle &operator=(TRefCountHandle &&b){
		if (this != &b){
			T* pTemp = m_pValue;
			m_pValue = b.m_pValue;
			b.m_pValue = pTemp;
		}
		return *this;
	}
	inline operator T*(){
		return m_pValue;
	}
	inline T* Get(){
		return m_pValue;
	}
	inline operator bool(){
		return m_pValue != nullptr;
	}
	inline T* operator->(){
		return m_pValue;
	}
	inline const T* operator->() const{
		return m_pValue;
	}
	inline bool operator==(const TRefCountHandle &b){
		return m_pValue == b.m_pValue;
	}
	inline T** GetAddress(){
		if (m_pValue){
			m_pValue->Release();
			m_pValue = nullptr;
		}
		return &m_pValue;
	}
	inline void Clear(){
		if (m_pValue){
			m_pValue->Release();
			m_pValue = nullptr;
		}
	}
};

class CRefCountable{
protected:
	long	m_RefCount = 1;
public:
	CRefCountable(){
	}
	CRefCountable(const CRefCountable &b){
	}
	CRefCountable(CRefCountable &&b){
	}
	virtual ~CRefCountable(){
	}
	CRefCountable &operator=(const CRefCountable &b){
	}
	CRefCountable &operator=(CRefCountable&&b){
	}
	inline void AddRef(){
		InterlockedIncrement(&m_RefCount);
	}
	inline void Release(){
		if (InterlockedDecrement(&m_RefCount) == 0){
			delete this;
		}
	}
};


#endif //!SHARED_REFCOUNT_H