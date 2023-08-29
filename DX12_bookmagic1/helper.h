//	インクルードガード
#pragma once
#include <d3d12.h>
#include <vector>
#include <string>

namespace Helper
{
	//	コマンドプロンプトにデバッグ情報を出力する
	void DebugOutputFormatString(const char* format, ...);
	//	シェーダーエラー検出処理
	void DebugShaderError(HRESULT result, ID3DBlob* errorBlob);

	//	ガウシアンウェイト計算処理
	std::vector<float> GetGaussianWeights(
		size_t count,	//	要素数
		float s			//	分散値
	);

	//	ファイル名から拡張子を取得する
	//	@param path 対象のパス文字列
	//	@return 拡張子
	std::string GetExtension(const std::string& path);

	//	std::string（マルチバイト文字列）からstd::wstring（ワイド文字列）を得る処理
	//	@param str マルチバイト文字列
	//	@return 変換されたワイド文字列
	std::wstring GetWideStringFromString(const std::string& str);

	//	アライメントにそろえたサイズを返す
	//	@param size			元の大きさ
	//	@param alignment	アライメントサイズ
	//	@return				アライメントをそろえたサイズ
	size_t AlignmentedSize(size_t size, size_t alignment);

	//	モデルのパスとテクスチャのパスから合成パスを得る処理
	//	@param modelPath アプリケーションから見たテクスチャのパス
	//	@param texPath PMDモデルから見たテクスチャのパス
	//	@return アプリケーションから見たテクスチャのパス
	std::string GetTexturePathFromModelAndTexPath(
		const std::string& modelPath,	//	モデルパス
		const char* texPath				//	テクスチャーパス
	);
	//	テクスチャのパスをセパレーター文字で分離する
	//	@param path 対象のパス文字列
	//	@param splitter 区切り文字
	//	@return 分離前後の文字列ペア
	std::pair<std::string, std::string> SplitFileName(
		const std::string& path, const char splitter = '*');

}