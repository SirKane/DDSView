#include <d3dcompiler.h>
#include <stdint.h>
#include <stdio.h>



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


void OutputShaderBlob(const char* pName, ID3DBlob* pBlob, FILE* fOutput){
	size_t i, codeSize;


	ID3D10Blob* pStrippedBlob = nullptr;

	if (SUCCEEDED(D3DStripShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(),
		D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO |
		D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA, 
		&pStrippedBlob)) && pStrippedBlob != nullptr){
		pBlob = pStrippedBlob;
	}

	codeSize = pBlob->GetBufferSize();

	const unsigned char* pData = (const unsigned char*)pBlob->GetBufferPointer();

	fprintf(fOutput, "const uint8_t %s[%zu] = {", pName, codeSize);
	for (i = 0; i < codeSize; ++i){
		if ((i & 0xF) == 0){
			fprintf(fOutput, "\n\t");
		}
		fprintf(fOutput, "0x%.2X, ", pData[i]);

	}
	fprintf(fOutput, "\n};\n\n");
	if (pStrippedBlob){
		pStrippedBlob->Release();
	}
}


enum : size_t{
	Component_RGBA = 0,
	Component_RGB,
	Component_R,
	Component_G,
	Component_B,
	Component_A,
	Component_Count
};

enum : size_t {
	Swap_None = 0,
	Swap_RB,
	Swap_Count,
};

enum : size_t {
	Type_UNorm = 0,
	Type_SNorm,
	Type_Float,
	Type_SInt,
	Type_UInt,
	Type_Count,
};

const char* ComponentNames[] = {
	"RGBA",
	"RGB",
	"R",
	"G",
	"B",
	"A",
};

const char* SwapNames[] = {
	"None",
	"RB",
};

const char* TypeNames[] = {
	"UNorm",
	"SNorm",
	"Float",
	"SInt",
	"UInt",
};



int main(int argc, const char*const*argv){

	CBlob blob, blob2;
	if (!LoadFileToBlob("Shader.hlsl", blob)) {
		return 0;
	}
	if (!LoadFileToBlob("CheckerBoard.hlsl", blob2)) {
		return 0;
	}
	HRESULT hr;
	FILE* fOutput;
	if (fopen_s(&fOutput, "../DDSView/Shaders.inl", "wt") != 0){
		return 0;
	}
	

	D3D_SHADER_MACRO macros[] = {
		{ nullptr, nullptr },
		{nullptr, nullptr},
		{nullptr, nullptr},
		{nullptr, nullptr},
		{nullptr, nullptr},
	};

	ID3DBlob* pCode, *pMessage;


	fprintf(fOutput, "#include <stdint.h>\n");
	fprintf(fOutput, "struct SShader {\n");
	fprintf(fOutput, "\tconst uint8_t* Code;\n");
	fprintf(fOutput, "\tconst size_t Size;\n");
	fprintf(fOutput, "};\n");



	hr = D3DCompile(blob2.GetPointer(), blob2.Size(), "CheckerBoard.hlsl", macros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Main", "vs_4_0",
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pMessage);

	if (FAILED(hr)) {
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	} else  {
		OutputShaderBlob("VS_Main_CheckerBoard", pCode, fOutput);
		pCode->Release();
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	}
	hr = D3DCompile(blob2.GetPointer(), blob2.Size(), "CheckerBoard.hlsl", macros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Main", "ps_4_0",
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pMessage);

	if (FAILED(hr)) {
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	} else  {
		OutputShaderBlob("PS_Main_CheckerBoard", pCode, fOutput);
		pCode->Release();
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	}


	hr = D3DCompile(blob.GetPointer(), blob.Size(), "Shader.hlsl", macros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Main", "vs_4_0",
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pMessage);

	if (FAILED(hr)) {
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	} else  {
		OutputShaderBlob("VS_Main", pCode, fOutput);
		pCode->Release();
		if (pMessage){
			OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
			pMessage->Release();
		}
	}

	size_t i, j, k;

	char buf0[32], buf1[32], buf2[32], name[128];


	macros[0].Name = "PIXEL_SHADER";
	macros[0].Definition = "";
	macros[1].Name = "TYPE";
	macros[1].Definition = buf0;
	macros[2].Name = "SWAP";
	macros[2].Definition = buf1;
	macros[3].Name = "COMPONENT";
	macros[3].Definition = buf2;

	for (i = 0; i < Type_Count; ++i){
		for (j = 0; j < Swap_Count; ++j){
			for (k = 0; k < Component_Count; ++k){
				sprintf_s(buf0, "%zu", i);
				sprintf_s(buf1, "%zu", j);
				sprintf_s(buf2, "%zu", k);
				pCode = pMessage = nullptr;

				hr = D3DCompile(blob.GetPointer(), blob.Size(), "Shader.hlsl", macros,
					D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Main", "ps_4_0",
					D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pMessage);
				sprintf_s(name, "PS_Main_%s_%s_%s", TypeNames[i], SwapNames[j], ComponentNames[k]);
				if (FAILED(hr)) {
					if (pMessage){
						OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
						pMessage->Release();
					}
				} else  {
					OutputShaderBlob(name, pCode, fOutput);
					pCode->Release();
					if (pMessage){
						OutputDebugStringA((LPCSTR)pMessage->GetBufferPointer());
						pMessage->Release();
					}
				}
			}
		}
	}


	fprintf(fOutput, "const SShader VertexShader = { VS_Main, sizeof(VS_Main) };\n");
	fprintf(fOutput, "const SShader PixelShaders[%zu][%zu][%zu] = {\n", Type_Count,
		Swap_Count, Component_Count);
	for (i = 0; i < Type_Count; ++i){
		fprintf(fOutput, "\t{\n");
		for (j = 0; j < Swap_Count; ++j){
			fprintf(fOutput, "\t\t{\n");
			for (k = 0; k < Component_Count; ++k){
				fprintf(fOutput, "\t\t\t{");
				sprintf_s(name, "PS_Main_%s_%s_%s", TypeNames[i], SwapNames[j], ComponentNames[k]);
				fprintf(fOutput, "%s, sizeof(%s)", name, name);

				fprintf(fOutput, "},\n");
			}
			fprintf(fOutput, "\t\t},\n");
		}
		fprintf(fOutput, "\t},\n");
	}
	fprintf(fOutput, "};\n");

	fflush(fOutput);
	fclose(fOutput);

	return 0;
}
