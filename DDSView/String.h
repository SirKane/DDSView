#ifndef DDSVIEW_STRING_H
#define DDSVIEW_STRING_H
#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>

class CConstStrA{
protected:
	const char*	m_pString;
	size_t		m_Length;
	CConstStrA() = delete;
	CConstStrA &operator=(const CConstStrA&) = delete;
	CConstStrA &operator=(CConstStrA &&) = delete;
public:
	template<size_t N> constexpr CConstStrA(const char(&str)[N]) :
		m_pString(str), m_Length(N - 1){
	}
	CConstStrA(const void* str, size_t length) : m_pString((const char*)str),
		m_Length(length){
	}
	CConstStrA(const char* str) : m_pString(str){
		m_Length = strlen(str);
	}
	CConstStrA(const CConstStrA&rhs) :
		m_pString(rhs.m_pString), m_Length(rhs.m_Length){
	}
	CConstStrA(CConstStrA &&rhs) :
		m_pString(rhs.m_pString), m_Length(rhs.m_Length){
	}
	constexpr const char* GetString() const{
		return m_pString;
	}
	constexpr size_t GetLength() const{
		return m_Length;
	}
};

class CConstStrW{
protected:
	const wchar_t*	m_pString;
	size_t			m_Length;
	CConstStrW() = delete;
	CConstStrW(const CConstStrW&) = delete;
	CConstStrW(CConstStrW &&) = delete;
	CConstStrW &operator=(const CConstStrW&) = delete;
	CConstStrW &operator=(CConstStrW &&) = delete;
public:
	template<size_t N> constexpr CConstStrW(const wchar_t(&str)[N]) :
		m_pString(str), m_Length(N - 1){
	}
	CConstStrW(const wchar_t* str) : m_pString(str){
		m_Length = wcslen(str);
	}
	constexpr const wchar_t* GetString() const{
		return m_pString;
	}
	constexpr size_t GetLength() const{
		return m_Length;
	}
};

//typedef char T;

template<typename T> class TString{
protected:

	template<typename> struct TStringTraits;

	template<> struct TStringTraits<char>{
		typedef CConstStrA	CConstStr;
		typedef char EType;
		static int compare(const EType* a, const EType* b, size_t len){
			return len == 0 ? 0 : memcmp(a, b, len);
		}
		static size_t length(const EType* str){
			return str ? strlen(str) : 0;
		}
		static EType* copy(EType* dst, const EType* src, size_t len){
			return len == 0 ? dst : (EType*)memcpy(dst, src, len);
		}
		static EType* copy_s(EType* dst, size_t dstlen, const EType* src, size_t srclen){
			return srclen == 0 ? dst : (EType*)memcpy_s(dst, dstlen, src, srclen);
		}
		static const EType* find(const EType* s, size_t len, EType c){
			return (const EType*)memchr(s, c, len);
		}
		static EType* move(EType* dst, const EType* src, size_t len){
			return len == 0 ? dst : (EType*)memmove(dst, src, len);
		}
		static EType* move_s(EType* dst, size_t dstlen, const EType* src, size_t srclen){
			return srclen == 0 ? dst : (EType*)memmove_s(dst, dstlen, src, srclen);
		}
		static EType* lower(EType* s, size_t len){
			_strlwr_s(s, len);
			return s;
		}
		static EType* upper(EType* s, size_t len){
			_strupr_s(s, len);
			return s;
		}
		static EType tolower(EType c){
			return (EType)::tolower(c);
		}
		static EType toupper(EType c){
			return (EType)::toupper(c);
		}
		static EType* strchr(EType* s, EType c){
			return ::strchr(s, c);
		}
		static const EType* strchr(const EType* s, EType c){
			return ::strchr(s, c);
		}
		static int scprint(const EType* fmt, va_list va){
			return ::_vscprintf(fmt, va);
		}
		static int vsprintf(EType* buf, size_t len, const EType* fmt, va_list va){
			return ::vsprintf_s(buf, len, fmt, va);
		}
	};


	template<> struct TStringTraits<wchar_t>{
		typedef CConstStrW	CConstStr;
		typedef wchar_t EType;
		static int compare(const EType* a, const EType* b, size_t len){
			return len == 0 ? 0 : wmemcmp(a, b, len);
		}
		static size_t length(const EType* str){
			return str ? wcslen(str) : 0;
		}
		static EType* copy(EType* dst, const EType* src, size_t len){
			return len == 0 ? dst : (EType*)wmemcpy(dst, src, len);
		}
		static EType* copy_s(EType* dst, size_t dstlen, const EType* src, size_t srclen){
			return srclen == 0 ? dst : (EType*)wmemcpy_s(dst, dstlen, src, srclen);
		}
		static const EType* find(const EType* s, size_t len, EType c){
			return (const EType*)wmemchr(s, c, len);
		}
		static EType* move(EType* dst, const EType* src, size_t len){
			return len == 0 ? dst : (EType*)wmemmove(dst, src, len);
		}
		static EType* move_s(EType* dst, size_t dstlen, const EType* src, size_t srclen){
			return srclen == 0 ? dst : (EType*)wmemmove_s(dst, dstlen, src, srclen);
		}
		static EType* lower(EType* s, size_t len){
			_wcslwr_s(s, len);
			return s;
		}
		static EType* upper(EType* s, size_t len){
			_wcsupr_s(s, len);
			return s;
		}
		static EType tolower(EType c){
			return (EType)::towlower(c);
		}
		static EType toupper(EType c){
			return (EType)::towupper(c);
		}
		static EType* strchr(EType* s, EType c){
			return ::wcschr(s, c);
		}
		static const EType* strchr(const EType* s, EType c){
			return ::wcschr(s, c);
		}
		static int scprint(const EType* fmt, va_list va){
			return ::_vscwprintf(fmt, va);
		}
		static int vsprintf(EType* buf, size_t len, const EType* fmt, va_list va){
			return ::vswprintf_s(buf, len, fmt, va);
		}
	};

	typedef TStringTraits<T> TTraits;

	enum {
		SSO_Size = 15,
	};
	union {
		T*			m_pBuffer;
		const T*	m_pConstBuffer;
		T			m_Buffer[SSO_Size];
	};
	bool			m_IsConstant : 1;
	bool			m_IsAllocated : 1;
	size_t			m_Capacity;
	size_t			m_Size;


	//This handles the extra char needed for zero termination
	void InternalResize(size_t len){
		if (m_IsConstant){
			size_t alen = len > m_Size ? len : m_Size;
			if (alen + 1 > SSO_Size){
				T* pBuf = (T*)malloc((alen + 1) * sizeof(T));
				TTraits::copy(pBuf, m_pConstBuffer, m_Size + 1);
				m_pBuffer = pBuf;
				m_IsAllocated = true;
			} else {
				TTraits::copy(m_Buffer, m_pConstBuffer, m_Size + 1);
				m_IsAllocated = false;
			}
			m_Capacity = alen;
			m_IsConstant = false;
			return;
		}
		if (len <= m_Capacity){
			return;
		}
		T* pBuf = (T*)malloc((len + 1) * sizeof(T));
		TTraits::copy(pBuf, GetBuffer(), m_Size + 1);
		if (m_IsAllocated){
			free(m_pBuffer);
		}
		m_pBuffer = pBuf;
		m_IsAllocated = true;
		m_Capacity = len;
	}

	static int InternalCompare(const T* const left, size_t LeftSize,
		const T* const right, size_t RightSize){

		size_t minsize = LeftSize < RightSize ? LeftSize : RightSize;

		int val = TTraits::compare(left, right, minsize);

		if (val != 0){
			return val;
		}

		if (LeftSize < RightSize){
			return -1;
		}

		if (LeftSize > RightSize){
			return 1;
		}
		return 0;
	}
	inline size_t ClampSize(size_t pos, size_t len) const{
		size_t maxsize = Length() - pos;
		return len <= maxsize ? len : maxsize;
	}
	inline void CheckOffset(size_t pos) const{
		assert(pos <= Length());
	}
	inline bool Inside(const T* p) const{
		return m_IsConstant == false && (p >= String() && p <= String() + Length());
	}
	inline void InternalClear(){
		if (m_IsAllocated){
			free(m_pBuffer);
		}
		m_IsAllocated = m_IsConstant = false;
		m_Size = 0;
		m_Capacity = SSO_Size - 1;
		m_Buffer[0] = 0;
	}
	void CopyOnWrite(){
		if (m_IsConstant){
			InternalResize(m_Capacity);
		}
	}
public:
	static const size_t npos = (size_t)-1;
	//Constructors
	inline TString() :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){
		m_Buffer[0] = 0;
	}
	inline TString(const TString &str) :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){
		m_Buffer[0] = 0;
		InternalResize(str.Size());

		TTraits::copy(GetBuffer(), str.String(), str.Length() + 1);
		m_Size = str.Size();
	}
	inline TString(TString &&str) :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){
		m_Buffer[0] = 0;


		if (str.m_IsConstant){
			m_pConstBuffer = str.m_pConstBuffer;
			m_IsConstant = true;
		} else if (str.m_IsAllocated) {
			m_pBuffer = str.m_pBuffer;
			m_IsAllocated = true;
		} else {
			TTraits::copy(m_Buffer, str.m_Buffer, SSO_Size);
		}
		m_Capacity = str.m_Capacity;
		m_Size = str.m_Size;

		str.m_IsAllocated = false;
		str.m_IsConstant = false;
		str.m_Buffer[0] = 0;
		str.m_Capacity = SSO_Size - 1;
		str.m_Size = 0;

	}
	inline TString(const TString &str, size_t pos, size_t len = npos) :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){

		assert(pos <= str.Length());
		m_Buffer[0] = 0;


		if (len == npos || pos + len > str.Length()){
			len = str.Length() - pos;
		}

		if (len == 0){
			return;
		}

		InternalResize(len);

		TTraits::copy(GetBuffer(), str.String() + pos, len);
		GetBuffer()[len] = 0;
		m_Size = len;
	}
	inline explicit TString(const T* str) :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){
		m_Buffer[0] = 0;

		assert(str != nullptr);

		size_t len = TTraits::length(str);

		InternalResize(len);
		TTraits::copy(GetBuffer(), str, len + 1);
		m_Size = len;
	}
	inline TString(const T* str, size_t n) :
		m_Capacity(SSO_Size - 1), m_Size(0), m_IsAllocated(false), m_IsConstant(false){
		m_Buffer[0] = 0;

		assert(str != nullptr);

		size_t len = TTraits::length(str);
		if (n < len){
			len = n;
		}

		InternalResize(len);
		TTraits::copy(GetBuffer(), str, len);
		GetBuffer()[len] = 0;
		m_Size = len;
	}
	inline TString(const typename TTraits::CConstStr &str) :
		m_Capacity(str.GetLength()), m_Size(str.GetLength()),
		m_IsConstant(true), m_IsAllocated(false),
		m_pConstBuffer(str.GetString()){
	}
	inline TString&operator=(const TString&str){
		if (this != &str){
			Assign(str);
		}
		return *this;
	}
	inline TString&operator=(TString&&str){
		if (this != &str){
			Assign((TString&&)str);
		}
		return *this;
	}

	inline TString&operator=(const T*s){
		Assign(s);
		return *this;
	}
	//Destructor
	inline ~TString(){
		InternalClear();
	}
	//Capacity
	inline size_t Size() const{
		return m_Size;
	}
	inline size_t Length() const{
		return m_Size;
	}

	inline size_t Capacity() const{
		return m_Capacity;
	}
	inline void Reserve(size_t n = 0){
		InternalResize(n);
	}
	inline void Resize(size_t n = 0){
		InternalResize(n);
		m_Size = n;
		if (n > 0){
			GetBuffer()[n] = 0;
		}
	}
	inline void Clear(){
		InternalResize(0);
		GetBuffer()[0] = 0;
		m_Size = 0;
	}
	inline bool Empty() const{
		return m_Size == 0;
	}
	//String ops
	inline const T* String() const{
		if (m_IsConstant){
			return m_pConstBuffer;
		}
		return m_IsAllocated ? m_pBuffer : m_Buffer;
	}
	inline const T* operator()() const{
		return String();
	}
	inline T* GetBuffer(){
		CopyOnWrite();
		return m_IsAllocated ? m_pBuffer : m_Buffer;
	}
	inline size_t Copy(T* s, size_t len, size_t pos = 0) const{
		assert(s != nullptr);
		if (pos + len > Length()){
			len = Length() - pos;
		}
		TTraits::copy(s, String() + pos, len);
		return len;
	}
	inline size_t Find(const TString &str, size_t pos = 0) const{
		return Find(str.String(), pos, str.Length());
	}
	inline size_t Find(const T* s, size_t pos = 0) const{
		return Find(s, pos, TTraits::length(s));
	}
	inline size_t Find(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);
		if (n == 0 && pos <= Length()){
			return pos;
		}

		size_t count;
		if (pos < Length() && n <= (count = Length() - pos)){
			const T*current, *next;
			for (count -= n - 1, current = String() + pos;
				(next = TTraits::find(current, count, *s)) != nullptr;
				count -= next - current + 1, current = next + 1){
				if (TTraits::compare(s, next, n) == 0){
					return next - String();
				}
			}
		}
		return npos;
	}
	inline size_t Find(T c, size_t pos = 0) const{
		if (pos < Length()){
			const T* s = String();
			const T* fs = TTraits::find(s + pos, Length(), c);
			if (fs != nullptr){
				return fs - s;
			}
		}
		return npos;
		//return Find(&c, pos, 1);
	}



	inline size_t RFind(const TString &str, size_t pos = npos) const{
		return RFind(str.String(), pos, str.Length());
	}
	inline size_t RFind(const T* s, size_t pos = npos) const{
		return RFind(s, pos, TTraits::length(s));
	}
	inline size_t RFind(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);

		if (n == 0){
			return pos < Length() ? pos : Length();
		}

		if (n <= Length()){
			const T* current = String() +
				(pos < Length() - n ? pos :
					Length() - n);

			for (;; --current){
				if (*s == *current && TTraits::compare(s, current, n) == 0){
					return current - String();
				} else if (current == String()){
					break;
				}
			}
		}
		return npos;
	}
	inline size_t RFind(T c, size_t pos = npos) const{
		return RFind(&c, pos, 1);
	}



	inline size_t FindFirstOf(const TString &str, size_t pos = 0) const{
		return FindFirstOf(str.String(), pos, str.Length());
	}
	inline size_t FindFirstOf(const T* s, size_t pos = 0) const{
		return FindFirstOf(s, pos, TTraits::length(s));
	}
	inline size_t FindFirstOf(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);


		if (n > 0 && pos < Length()){
			const T* current = String() + pos, *end = String() + Length();

			for (; current < end; ++current){
				if (TTraits::find(s, n, *current) != nullptr){
					return current - String();
				}
			}
		}
		return npos;
	}
	inline size_t FindFirstOf(T c, size_t pos = 0) const{
		return FindFirstOf(&c, pos, 1);
	}



	inline size_t FindLastOf(const TString &str, size_t pos = npos) const{
		return FindLastOf(str.String(), pos, str.Length());
	}
	inline size_t FindLastOf(const T* s, size_t pos = npos) const{
		return FindLastOf(s, pos, TTraits::length(s));
	}
	inline size_t FindLastOf(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);


		if (n > 0 && Length() > 0){
			const T* current = String() + (
				pos < Length() ? pos : Length() - 1);

			for (;; --current){
				if (TTraits::find(s, n, *current) != nullptr){
					return current - String();
				} else if (current == String()){
					break;
				}
			}
		}
		return npos;
	}
	inline size_t FindLastOf(T c, size_t pos = npos) const{
		return FindLastOf(&c, pos, 1);
	}




	inline size_t FindFirstNotOf(const TString &str, size_t pos = 0) const{
		return FindFirstNotOf(str.String(), pos, str.Length());
	}
	inline size_t FindFirstNotOf(const T* s, size_t pos = 0) const{
		return FindFirstNotOf(s, pos, TTraits::length(s));
	}
	inline size_t FindFirstNotOf(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);


		if (n > 0 && pos < Length()){
			const T* current = String() + pos, *end = String() + Length();

			for (; current < end; ++current){
				if (TTraits::find(s, n, *current) == nullptr){
					return current - String();
				}
			}
		}
		return npos;
	}
	inline size_t FindFirstNotOf(T c, size_t pos = 0) const{
		return FindFirstNotOf(&c, pos, 1);
	}



	inline size_t FindLastNotOf(const TString &str, size_t pos = npos) const{
		return FindLastNotOf(str.String(), pos, str.Length());
	}
	inline size_t FindLastNotOf(const T* s, size_t pos = npos) const{
		return FindLastNotOf(s, pos, TTraits::length(s));
	}
	inline size_t FindLastNotOf(const T* s, size_t pos, size_t n) const{
		assert(n == 0 || s != nullptr);


		if (n > 0 && Length() > 0){
			const T* current = String() + (
				pos < Length() ? pos : Length() - 1);

			for (;; --current){
				if (TTraits::find(s, n, *current) == nullptr){
					return current - String();
				} else if (current == String()){
					break;
				}
			}
		}
		return npos;
	}
	inline size_t FindLastNotOf(T c, size_t pos = npos) const{
		return FindLastNotOf(&c, pos, 1);
	}


	TString SubStr(size_t pos, size_t len = npos) const{
		return TString(*this, pos, len);
	}

	int Compare(const TString &str) const{
		return InternalCompare(String(), Length(),
			str.String(), str.Length());
	}
	int Compare(size_t pos, size_t len, const TString &str) const{
		CheckOffset(pos);
		return InternalCompare(String() + pos, ClampSize(pos, len),
			str.String(), str.Length());
	}
	int Compare(size_t pos, size_t len, const TString &str,
		size_t subpos, size_t sublen) const{
		CheckOffset(pos);
		str.CheckOffset(subpos);

		return InternalCompare(String() + pos, ClampSize(pos, len),
			str.String() + subpos, str.ClampSize(subpos, sublen));

	}
	int Compare(const T*s) const {
		assert(s != nullptr);
		return InternalCompare(String(), Length(), s, TTraits::length(s));
	}
	int Compare(size_t pos, size_t len, const T*s) const{
		assert(s != nullptr);
		CheckOffset(pos);
		return InternalCompare(String() + pos, ClampSize(pos, len), s, TTraits::length(s));
	}
	int Compare(size_t pos, size_t len, const T*s, size_t n) const{
		assert(n == 0 || s != nullptr);
		CheckOffset(pos);
		return InternalCompare(String() + pos, ClampSize(pos, len), s, n);
	}
	TString<T> operator+(const TString<T> &str) const{
		TString<T> s;
		s.Reserve(Length() + str.Length());
		s = *this;
		s.Append(str);
		return s;
	}
	//Modifiers
	TString &operator+=(const TString&str){
		return Append(str);
	}
	TString &operator+=(const T* s){
		return Append(s);
	}
	TString &operator+=(T c){
		return Append(&c, 1);
	}
	TString& Append(const TString& str){
		return Append(str, 0, npos);
	}
	TString& Append(const TString& str, size_t subpos, size_t sublen = npos){
		str.CheckOffset(subpos);
		sublen = str.ClampSize(subpos, sublen);

		if (sublen > 0){
			InternalResize(Length() + sublen);
			TTraits::copy(GetBuffer() + Length(), str.String() + subpos, sublen);
			m_Size += sublen;
			GetBuffer()[m_Size] = 0;
		}
		return *this;
	}
	TString& Append(const T*s){
		assert(s != nullptr);
		return Append(s, TTraits::length(s));
	}
	TString& Append(const T* s, size_t n){
		assert(s != nullptr || n == 0);
		if (Inside(s)){
			return Append(*this, s - String(), n);
		}
		if (n > 0){
			InternalResize(Length() + n);
			TTraits::copy(GetBuffer() + Length(), s, n);
			m_Size += n;
			GetBuffer()[m_Size] = 0;
		}
		return *this;
	}
	void PushBack(T c){
		Append(&c, 1);
	}

	TString &Assign(const TString &str){
		return Assign(str, 0, npos);
	}
	TString &Assign(const TString &str, size_t subpos, size_t sublen = npos){
		str.CheckOffset(subpos);
		sublen = str.ClampSize(subpos, sublen);

		if (str.m_IsConstant && subpos + sublen == str.Length()){
			InternalClear();
			m_IsConstant = true;
			m_Size = m_Capacity = sublen;
			m_pConstBuffer = str.m_pConstBuffer + subpos;
			return *this;
		}

		if (this == &str){
			Erase(subpos + sublen);
			Erase(0, subpos);
		} else {
			InternalResize(sublen);
			TTraits::copy(GetBuffer(), str.String() + subpos, sublen);
			GetBuffer()[sublen] = 0;
			m_Size = sublen;
		}
		return *this;
	}
	TString &Assign(const T* s){
		assert(s != nullptr);
		return Assign(s, TTraits::length(s));
	}
	TString &Assign(const T* s, size_t n){
		assert(s != nullptr || n == 0);

		if (Inside(s)){
			return Assign(*this, s - String(), n);
		} else {
			InternalResize(n);
			TTraits::copy(GetBuffer(), s, n);
			GetBuffer()[n] = 0;
			m_Size = n;
		}
		return *this;
	}
	TString &Assign(TString &&str){
		if (&str != this){
			InternalClear();

			if (str.m_IsAllocated){
				m_pBuffer = str.m_pBuffer;
				str.m_IsAllocated = false;
				m_IsAllocated = true;
			} else if (str.m_IsConstant){
				m_pConstBuffer = str.m_pConstBuffer;
				str.m_IsConstant = false;
				m_IsConstant = true;
			} else {
				TTraits::copy(m_Buffer, str.m_Buffer, SSO_Size);
			}
			m_Capacity = str.m_Capacity;
			m_Size = str.m_Size;

			str.m_Buffer[0] = 0;
			str.m_Size = 0;
			str.m_Capacity = SSO_Size - 1;

		}
		return *this;
	}
	TString &AssignF(const T* fmt, ...){
		va_list va;
		va_start(va, fmt);
		int size = TTraits::scprint(fmt, va);
		va_end(va);
		Erase();
		assert(size >= 0);
		if (size < 0){
			return *this;
		}
		Resize(size_t(size));
		va_start(va, fmt);
		size = TTraits::vsprintf(GetBuffer(), Capacity()+1, fmt, va);
		va_end(va);

		assert(size >= 0);
		if (size < 0){
			Resize(0);
			return *this;
		}
		return *this;
	}
	TString &Print(const T* fmt, ...){
		va_list va;
		va_start(va, fmt);
		int size = TTraits::scprint(fmt, va);
		va_end(va);
		Erase();
		assert(size >= 0);
		if (size < 0){
			return *this;
		}
		Resize(size_t(size));
		va_start(va, fmt);
		size = TTraits::vsprintf(GetBuffer(), Capacity()+1, fmt, va);
		va_end(va);

		assert(size >= 0);
		if (size < 0){
			Resize(0);
			return *this;
		}
		return *this;		
	}
	TString& Erase(size_t pos = 0, size_t len = npos){
		CheckOffset(pos);
		if (Length() - pos <= len){
			InternalResize(pos);
			GetBuffer()[pos] = 0;
			m_Size = pos;
		} else if (len > 0){
			size_t newsize = Length() - len;
			InternalResize(Length());
			TTraits::move(GetBuffer() + pos, GetBuffer() + pos + len, newsize - pos);
			GetBuffer()[newsize] = 0;
			m_Size = newsize;
		}
		return *this;
	}
	TString &Insert(size_t pos, const TString&str){
		return Insert(pos, str, 0, npos);
	}
	TString &Insert(size_t pos, const TString&str, size_t subpos, size_t sublen = npos){
		CheckOffset(pos);
		str.CheckOffset(subpos);
		sublen = str.ClampSize(subpos, sublen);

		if (sublen > 0){
			InternalResize(Length() + sublen);
			TTraits::move(GetBuffer() + pos + sublen,
				GetBuffer() + pos,
				Length() - pos);

			if (this == &str){
				TTraits::move(GetBuffer() + pos,
					GetBuffer() + (pos < subpos ? subpos + sublen : subpos),
					sublen);
			} else {
				TTraits::copy(GetBuffer() + pos,
					str.String() + subpos, sublen);
			}
			m_Size += sublen;
			GetBuffer()[m_Size] = 0;
		}
		return *this;
	}
	TString &Insert(size_t pos, const T* s){
		assert(s != nullptr);
		return Insert(pos, s, TTraits::length(s));
	}
	TString &Insert(size_t pos, const T* s, size_t n){
		assert(s != nullptr || n == 0);
		if (Inside(s)){
			return Insert(pos, *this, s - String(), n);
		}
		if (n > 0){
			InternalResize(Length() + n);
			TTraits::move(GetBuffer() + pos + n,
				String() + pos,
				Length() - pos);
			TTraits::copy(GetBuffer() + pos, s, n);
			m_Size += n;
			GetBuffer()[m_Size] = 0;
		}
		return *this;
	}


	TString& Replace(size_t pos, size_t len, const TString& str){
		return Replace(pos, len, str, 0, npos);
	}
	TString& Replace(size_t pos, size_t len, const TString& str, size_t subpos, size_t sublen = npos){
		CheckOffset(pos);
		str.CheckOffset(subpos);
		len = ClampSize(pos, len);
		sublen = str.ClampSize(subpos, sublen);

		size_t tailen = Length() - len - pos;
		size_t newsize = Length() + sublen - len;

		InternalResize(newsize);
		if (len == sublen){
			TTraits::move(GetBuffer() + pos, str.String() + subpos, sublen);
		} else if (this != &str){
			TTraits::move(GetBuffer() + pos + sublen,
				String() + pos + len, tailen);
			TTraits::copy(GetBuffer() + pos, str.String() + subpos, sublen);
		} else if (sublen < len){
			TTraits::move(GetBuffer() + pos, String() + subpos, sublen);
			TTraits::move(GetBuffer() + pos + len,
				String() + pos + len, tailen);
		} else if (subpos <= pos){
			TTraits::move(GetBuffer() + pos + sublen,
				String() + pos + len, tailen);
			TTraits::move(GetBuffer() + pos,
				String() + subpos, sublen);
		} else if (pos + len <= subpos){
			TTraits::move(GetBuffer() + pos + sublen,
				String() + pos + len, tailen);
			TTraits::move(GetBuffer() + pos,
				String() + (subpos + sublen - len),
				sublen);
		} else{
			TTraits::move(GetBuffer() + pos,
				String() + subpos, len);
			TTraits::move(GetBuffer() + pos + sublen,
				String() + pos + len, tailen);
			TTraits::move(GetBuffer() + pos + len,
				String() + subpos + len,
				sublen - len);
		}
		GetBuffer()[newsize] = 0;
		m_Size = newsize;
		return *this;
	}
	TString& Replace(size_t pos, size_t len, const T* s){
		assert(s != nullptr);
		return Replace(pos, len, s, TTraits::length(s));
	}
	TString& Replace(size_t pos, size_t len, const T* s, size_t n){
		assert(s != nullptr || n == 0);
		if (Inside(s)){
			return Replace(pos, len, *this, s - String(), n);
		}
		CheckOffset(pos);
		len = ClampSize(pos, len);


		size_t tailsize = Length() - len - pos;
		size_t newsize = Length() + n - len;
		InternalResize(newsize);

		if (n < len){
			TTraits::move(GetBuffer() + pos + n,
				String() + pos + len,
				tailsize);
		}
		if (n > 0 || len > 0){
			if (len < n){
				TTraits::move(GetBuffer() + pos + n,
					String() + pos + len, tailsize);
			}
			TTraits::copy(GetBuffer() + pos, s, n);
			m_Size = newsize;
			GetBuffer()[newsize] = 0;
		}
		return *this;
	}
	inline TString& MakeLower(){
		InternalResize(0);
		TTraits::lower(GetBuffer(), Capacity() + 1);
		return *this;
	}
	inline TString ToLower() const{
		TString str(*this);
		str.MakeLower();
		return str;
	}
	inline bool IsLower() const{
		const T* s = String();
		for (; *s != 0; ++s){
			if (TTraits::tolower(*s) != *s){
				return false;
			}
		}
		return true;
	}

	inline TString& MakeUpper(){
		InternalResize(0);
		TTraits::upper(GetBuffer(), Capacity() + 1);
		return *this;
	}
	inline TString ToUpper() const{
		TString str(*this);
		str.MakeUpper();
		return str;
	}
	inline bool IsUppper() const{
		const T* s = String();
		for (; *s != 0; ++s){
			if (TTraits::toupper(*s) != *s){
				return false;
			}
		}
		return true;
	}



	//Accessor
	inline T &operator[](size_t pos){
		assert(pos <= Length());
		return GetBuffer()[pos];
	}
	inline const T &operator[](size_t pos) const{
		assert(pos <= Length());
		return String()[pos];
	}
	inline bool operator==(const TString<T> &b) const{
		return Compare(b) == 0;
	}
	inline bool operator==(const T* b) const{
		return Compare(b) == 0;
	}
	inline bool operator!=(const TString<T> &b) const{
		return Compare(b) != 0;
	}
	inline bool operator!=(const T* b) const{
		return Compare(b) != 0;
	}
	/*inline explicit operator const T*() const{
	return String();
	}*/
	/*operator const T*() const{
	return String();
	}*/
	/*template<size_t N> constexpr TString(const T(&str)[N]) :
	m_pConstBuffer(str), m_Size(N), m_Capacity(N), m_IsConstant(true), m_IsAllocated(true){
	}
	template<size_t N> static constexpr TString ConstStr(const T(&str)[N]){
	return TString(str);
	}*/
	TString<T> &ReplaceChar(const T oldch, const T newch){
		size_t pos;
		if ((pos = Find(oldch)) != npos){
			CopyOnWrite();
			T* s = GetBuffer() + pos;
			for (; *s != 0; ++s){
				if (*s == oldch){
					*s = newch;
				}
			}
		}
		return *this;
	}
	bool EndsWith(const T ch) const{
		return (Length() > 0) ?
			String()[Length() - 1] == ch :
			false;
	}
	bool IsEmpty() const{
		return Length() == 0;
	}
};

typedef TString<char> CStringA;
typedef TString<wchar_t> CStringW;


inline CConstStrA operator "" _cs(const char* pString, size_t size){
	return CConstStrA(pString, size);
}

#endif //!DDSVIEW_STRING_H
