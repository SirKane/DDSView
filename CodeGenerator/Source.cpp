#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <vector>
#include <string>
#include <map>



class CBlob{
protected:
	void*	m_pData = nullptr;
	size_t	m_Size = 0;
	size_t	m_Capacity = 0;
public:
	inline CBlob(){
	}
	inline ~CBlob(){
		if (m_pData != nullptr){
			free(m_pData);
		}
	}
	CBlob(const CBlob &) = delete;
	CBlob(CBlob&&) = delete;
	CBlob &operator=(const CBlob&) = delete;
	CBlob &operator=(CBlob&&) = delete;
	inline bool Resize(size_t size){
		if (size <= m_Capacity && m_pData != nullptr){
			m_Size = size;
			return true;
		}
		m_pData = realloc(m_pData, size);
		m_Capacity = m_Size = size;

		return m_pData != nullptr;
	}
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



class CCSVParser{
protected:
	CBlob							m_TableStorage;
	CBlob							m_TextStorage;
	char**							m_pTable = nullptr;

	size_t							m_RowCount = 0;
	size_t							m_ColumnCount = 0;

	bool GetBounds(const CBlob &input, size_t &rowCount, size_t &columnCount){
		const char* pStart = (const char*)input.GetPointer();
		const char* pEnd = pStart + input.Size();
		const char* pCurrent = pStart;

		size_t rCount = 0, cCount = 0, currentColumn = 0;
		bool hadChar = false;
		for (; pCurrent <= pEnd; ++pCurrent){
			const char c = pCurrent == pEnd ? '\n' : *pCurrent;
			if (c == ','){
				++currentColumn;
			} else if (c == '\n'){
				if (hadChar){
					++currentColumn;
					hadChar = false;
					++rCount;
					if (currentColumn > cCount){
						cCount = currentColumn;
					}
					currentColumn = 0;
				}
			} else if (c == '\r'){

			} else {
				hadChar = true;
			}
		}
		rowCount = rCount;
		columnCount = cCount;
		return true;
	}
public:
	void Clear(){
		m_pTable = nullptr;
		m_TableStorage.Resize(0);
		m_TextStorage.Resize(0);
	}
	bool Load(const CBlob&input){
		size_t rowCount, columnCount;
		if (!GetBounds(input, rowCount, columnCount)){
			Clear();
			return false;
		}
		if (!m_TextStorage.Resize(input.Size()+1)){
			Clear();
			return false;
		}
		if (!m_TableStorage.Resize(sizeof(char**) * rowCount * columnCount)){
			Clear();
			return false;
		}
		m_pTable = (char**)m_TableStorage.GetPointer();

		size_t row = 0, column = 0;

		const char* pStart = (const char*)input.GetPointer();
		const char* pEnd = pStart + input.Size();
		const char* pCurrent = pStart;
		
		char* pFieldStart = (char*)m_TextStorage.GetPointer();
		char* pDest = pFieldStart;

		bool hadChar = false;
		for (; pCurrent <= pEnd; ++pCurrent){
			const char c = pCurrent == pEnd ? '\n' : *pCurrent;
			if (c == ','){
				*(pDest++) = 0;
				m_pTable[row * columnCount + column] = pFieldStart;
				pFieldStart = pDest;
				++column;
			} else if (c == '\n'){
				if (hadChar){
					*(pDest++) = 0;
					m_pTable[row * columnCount + column] = pFieldStart;
					pFieldStart = pDest;

					hadChar = false;
					column = 0;
					++row;
				}
			} else if (c == '\r'){

			} else {
				*(pDest++) = c;
				hadChar = true;
			}
		}
		m_RowCount = rowCount;
		m_ColumnCount = columnCount;
		return true;
	}
	const char* GetField(size_t row, size_t column) const{
		return m_pTable != nullptr && row < m_RowCount && column < m_ColumnCount ?
			m_pTable[row * m_ColumnCount + column] : nullptr;
	}
	size_t GetRowCount() const{
		return m_RowCount;
	}
	size_t GetColumnCount() const{
		return m_ColumnCount;
	}
};


enum : size_t{
	Col_Name = 0,
	Col_Number,
	Col_TypeLessFormat,
	Col_SRGBFormat,
	Col_LinearBFormat,
	Col_Type,
	Col_SRGB,
	Col_Channel0,
	Col_Channel1,
	Col_Channel2,
	Col_Channel3,
	Col_Channel0Bits,
	Col_Channel1Bits,
	Col_Channel2Bits,
	Col_Channel3Bits,
	Col_ElementType,
	Col_DisplayType,
	Col_Channel0Type,
	Col_Channel1Type,
	Col_Channel2Type,
	Col_Channel3Type,
	Col_Count,
};


bool SkipFormat(size_t i){
	if (i >= 130 && i <= 132){
		return true;
	}
	return false;
}

int main(int argc, const char*const*argv){

	uint16_t packageIndex, resourceIndex;


	CCSVParser parser;
	CBlob blob;
	LoadFileToBlob("DXGIFormat.csv", blob);
	parser.Load(blob);
	FILE* f;
	fopen_s(&f, "../DDSView/FormatInfo.inl", "wt");

	size_t i, j;
	//fprintf_s(f, "#include <dxgi_format.h>\n");

	//Generate GetBitsPerElement
	fprintf_s(f, "size_t Format_GetBitsPerElement(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		fprintf_s(f, "\t\tcase %s: return %s + %s + %s + %s;\n",
			parser.GetField(i, Col_Name), parser.GetField(i, Col_Channel0Bits),
			parser.GetField(i, Col_Channel1Bits), parser.GetField(i, Col_Channel2Bits),
			parser.GetField(i, Col_Channel3Bits));
	}
	fprintf_s(f, "\t\tdefault: return 0;\n\t}\n}\n\n");

	//Generate get SRGB format
	fprintf_s(f, "DXGI_FORMAT Format_GetSRGBFormat(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		if (_stricmp(parser.GetField(i, Col_SRGBFormat), "DXGI_FORMAT_UNKNOWN") != 0){
			fprintf_s(f, "\t\tcase %s: return %s;\n",
				parser.GetField(i, Col_Name), parser.GetField(i, Col_SRGBFormat));
		}
	}
	fprintf_s(f, "\t\tdefault: return DXGI_FORMAT_UNKNOWN;\n\t}\n}\n\n");


	//Generate get linear format
	fprintf_s(f, "DXGI_FORMAT Format_GetLinearFormat(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		if (_stricmp(parser.GetField(i, Col_LinearBFormat), "DXGI_FORMAT_UNKNOWN") != 0){
			fprintf_s(f, "\t\tcase %s: return %s;\n",
				parser.GetField(i, Col_Name), parser.GetField(i, Col_LinearBFormat));
		}
	}
	fprintf_s(f, "\t\tdefault: return DXGI_FORMAT_UNKNOWN;\n\t}\n}\n\n");

	//Generate element type

	fprintf_s(f, "eTextureElementType Format_GetElementType(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		fprintf_s(f, "\t\tcase %s: return eTextureElementType::%s;\n",
			parser.GetField(i, Col_Name), parser.GetField(i, Col_ElementType));
	}
	fprintf_s(f, "\t\tdefault: return eTextureElementType::Invalid;\n\t}\n}\n\n");



	fprintf_s(f, "eFormatType Format_GetFormatType(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		fprintf_s(f, "\t\tcase %s: return eFormatType::%s;\n",
			parser.GetField(i, Col_Name), parser.GetField(i, Col_Type));
	}
	fprintf_s(f, "\t\tdefault: return eFormatType::Void;\n\t}\n}\n\n");

	fprintf_s(f, "uint32_t Format_GetSupportedChannel(DXGI_FORMAT format){\n");
	fprintf_s(f, "\tswitch(format){\n");

	for (i = 1; i < parser.GetRowCount(); ++i){
		if (SkipFormat((size_t)atoi(parser.GetField(i, Col_Number)))){
			continue;
		}
		fprintf_s(f, "\t\tcase %s: return ",
			parser.GetField(i, Col_Name));
		bool hadChannel = false;
		for (j = 0; j < 4; ++j){
			const char* pChannel = parser.GetField(i, Col_Channel0 + j);
			if (_stricmp(pChannel, "R") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R");
				hadChannel = true;
			} else if (_stricmp(pChannel, "D") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R");
				hadChannel = true;
			} else if (_stricmp(pChannel, "G") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_G");
				hadChannel = true;
			} else if (_stricmp(pChannel, "B") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_B");
				hadChannel = true;
			} else if (_stricmp(pChannel, "A") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_A");
				hadChannel = true;
			} else if (_stricmp(pChannel, "C1") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R");
				hadChannel = true;
			} else if (_stricmp(pChannel, "C2") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R | CCF_G");
				hadChannel = true;
			} else if (_stricmp(pChannel, "C3") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R | CCF_G | CCF_B");
				hadChannel = true;
			} else if (_stricmp(pChannel, "C4") == 0){
				if (hadChannel){
					fprintf_s(f, " | ");
				}
				fprintf_s(f, "CCF_R | CCF_G | CCF_B | CCF_A");
				hadChannel = true;
			}
		}
		if (!hadChannel){
			fprintf_s(f, "0");
		}
		fprintf_s(f, ";\n");

	}
	fprintf_s(f, "\t\tdefault: return 0;\n\t}\n}\n\n");

	fflush(f);
	fclose(f);

	return 0;
}