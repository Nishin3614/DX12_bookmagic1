#include "helper.h"
#include <cstdarg>
#include <assert.h>
#include <string>


#ifdef _DEBUG
#include <iostream>
#endif

//	コマンドプロンプトにデバッグ情報を出力する
void Helper::DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//	シェーダーエラー検出処理
void Helper::DebugShaderError(HRESULT result, ID3DBlob* errorBlob)
{
	//	シェーダーエラー確認処理
	if (FAILED(result))
	{
		//	ファイルが見当たらないとき
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			//	出力ウィンドウのデバッグに表示される
			::OutputDebugStringA("ファイルが見当たりません");
			return;
		}
		//	それ以外
		else
		{
			std::string errstr;	//	エラー文受取用
			errstr.resize(errorBlob->GetBufferSize());	//	必要サイズ確保
			//	データコピー
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin()
			);
			errstr += "\n";
			//	エラー文のデータ表示
			::OutputDebugStringA(errstr.c_str());
		}
	}
}

//	ガウシアンウェイト計算処理
std::vector<float> Helper::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);	//	ウェイト配列返却用
	float x = 0.0f;
	float total = 0.0f;

	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));	//	ネイピア数のx乗
		total += wgt;
		x += 1.0f;
	}

	//	実際は左右対称なので、合計値は2倍
	//	また、中心0のピクセルが重複するため、1(e^0)を引いてつじつまを合わせる
	total = total * 2.0f - 1;
	//float one = 0.0f;
	//	-7～7のウェイト値をすべて足して1になるようにする
	for (auto& wgt : weights)
	{
		wgt /= total;
		//one += wgt;
	}
	/*	ウェイト値が足して1になっているか確かめよう
	one *= 2;
	one -= weights[0];
	Debugoutput::DebugOutputFormatString("%f\n",one);
	*/

	return weights;
}

std::string Helper::GetExtension(const std::string& path)
{
	size_t idx = path.rfind('.');
	if (idx == std::string::npos)
	{
		return "";
	}
	else
	{
		return path.substr(
			idx + 1, path.length() - idx - 1);
	}
}

std::wstring Helper::GetWideStringFromString(const std::string& str)
{
	//	呼び出し1回目（文字列数を得る）
	auto num1 = MultiByteToWideChar(
		CP_ACP,									//	Windows ANSI（日本版だとShift JIS）
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,	//	構成済み文字（例）「ぱ」を一つの文字列としてみる） | 無効な文字を検知
		str.c_str(),							//	マルチバイト文字列のアドレス
		-1,										//	マルチバイト文字列のサイズ（-1の場合自動で長さを計算してくれる）
		nullptr,								//	ワイド文字列のアドレス
		0										//	バッファのサイズ（0を指定すると、ワイド文字列に変換するための必要なバッファのサイズを返す）
	);

	std::wstring wstr;	//	stringのwchar_t版
	wstr.resize(num1);	//	1回目で得た文字列数を確保

	//	呼び出し2回目（確保済みのwstrに変換文字列をコピー）
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1
	);

	//	取得した変換文字列のバッファサイズは正しいか確認
	assert(num1 == num2);
	return wstr;
}

//	アライメントにそろえたサイズを返す
//	@param size			元の大きさ
//	@param alignment	アライメントサイズ
//	@return				アライメントをそろえたサイズ
size_t Helper::AlignmentedSize(size_t size, size_t alignment)
{
	//	アライメント通りのサイズなら、そのままサイズを返す
	if (size % alignment == 0)
	{
		return size;
	}
	return size + alignment - size % alignment;
}

//	モデルのパスとテクスチャのパスから合成パスを得る処理
//	@param modelPath アプリケーションから見たテクスチャのパス
//	@param texPath PMDモデルから見たテクスチャのパス
//	@return アプリケーションから見たテクスチャのパス
std::string Helper::GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath)
{
	//	/,\\のどちらで設定されているかわからないため、判別する処理
	//	rfindで見つからない場合、epos(-1→0xffffffff）を返す
	size_t pathIndex1 = modelPath.rfind('/');
	size_t pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = min(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);	//	フォルダ名だけ取得
	return folderPath + texPath;
}

//	テクスチャのパスをセパレーター文字で分離する
//	@param path 対象のパス文字列
//	@param splitter 区切り文字
//	@return 分離前後の文字列ペア
std::pair<std::string, std::string> Helper::SplitFileName(const std::string& path, const char splitter)
{
	size_t idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(
		idx + 1, path.length() - idx - 1);
	return ret;
}
