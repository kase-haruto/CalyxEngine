#include "../Copy/CopyImage.hlsli"

// 通常描画されたシーンカラー
Texture2D<float4> gSceneColor : register(t0);

// ブルーム対象のみを書き込んだマスクテクスチャ
// 基本的には Emissive 成分だけが入っている想定
Texture2D<float4> gBloomMask : register(t1);

SamplerState gSampler : register(s0);

// ブルーム調整用パラメータ
cbuffer BloomParameter : register(b0) {
	// ブルームの最終的な強さ
	float intensity;

	// 旧方式の輝度抽出用しきい値
	// 現在は BloomMask を使うため基本的には未使用
	float threshold;

	// 旧方式のソフトしきい値用
	// 現在は BloomMask を使うため基本的には未使用
	float softKnee;

	// ブルームの広がり具合
	float radius;

	// ブルームに乗算する色
	float3 tint;

	float padding;
}

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

// RGB から輝度を計算する
// 旧方式のしきい値抽出で使用していた関数
float Luminance(float3 color) {
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// ブルームとして扱う色を抽出する
float3 ExtractBloom(float3 color) {
	// BloomMask にはすでに Emissive 成分のみが入っている想定のため、
	// ここでは輝度による追加の threshold 判定は行わない
	return color;
}

// BloomMask からブルーム用の色をサンプリングする
float3 SampleMask(float2 uv) {
	return ExtractBloom(gBloomMask.Sample(gSampler, uv).rgb);
}

// 上下左右方向からサンプリングする
// 十字方向にぼかし成分を広げるために使用する
float3 SampleCross(float2 uv, float2 stepSize) {
	return SampleMask(uv + float2( stepSize.x, 0.0f)) +
		   SampleMask(uv + float2(-stepSize.x, 0.0f)) +
		   SampleMask(uv + float2(0.0f,  stepSize.y)) +
		   SampleMask(uv + float2(0.0f, -stepSize.y));
}

// 斜め4方向からサンプリングする
// 十字方向だけでは出にくい丸い広がりを補う
float3 SampleDiagonal(float2 uv, float2 stepSize) {
	return SampleMask(uv + float2( stepSize.x,  stepSize.y)) +
		   SampleMask(uv + float2(-stepSize.x,  stepSize.y)) +
		   SampleMask(uv + float2( stepSize.x, -stepSize.y)) +
		   SampleMask(uv + float2(-stepSize.x, -stepSize.y));
}

// BloomMask を複数点サンプリングして疑似的にぼかす
float3 SampleBloom(float2 uv, float2 texelSize) {
	// radius が負にならないように補正する
	float bloomRadius = max(radius, 0.0f);

	// radius がほぼ 0 の場合はぼかさず、そのままマスクを返す
	if(bloomRadius <= 0.001f) {
		return SampleMask(uv);
	}

	// サンプリング間隔を3段階に分ける
	// near : 近距離の強いにじみ
	// mid  : 中距離の広がり
	// far  : 遠距離の薄い光の拡散
	float2 nearStep = texelSize * bloomRadius * 1.75f;
	float2 midStep  = texelSize * bloomRadius * 4.25f;
	float2 farStep  = texelSize * bloomRadius * 8.50f;

	// 中心の色を少し強めに残す
	float3 bloom = SampleMask(uv) * 0.14f;

	// 近距離サンプル
	// 発光部分の周辺を比較的強くにじませる
	bloom += SampleCross(uv, nearStep) * 0.065f;
	bloom += SampleDiagonal(uv, nearStep * 0.75f) * 0.045f;

	// 中距離サンプル
	// 光の広がりを追加する
	bloom += SampleCross(uv, midStep) * 0.045f;
	bloom += SampleDiagonal(uv, midStep * 0.75f) * 0.025f;

	// 遠距離サンプル
	// 外側に薄く広がる光を追加する
	bloom += SampleCross(uv, farStep) * 0.025f;
	bloom += SampleDiagonal(uv, farStep * 0.75f) * 0.010f;

	return bloom;
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	// BloomMask のサイズを取得する
	// texelSize を計算し、1ピクセル分のUV距離として使用する
	uint width;
	uint height;
	gBloomMask.GetDimensions(width, height);
	float2 texelSize = 1.0f / float2(max(width, 1), max(height, 1));

	// 元のシーンカラーを取得する
	float4 baseColor = gSceneColor.Sample(gSampler, input.texcoord);

	// intensity がほぼ 0 の場合はブルームを適用せず、そのまま返す
	if(intensity <= 0.001f) {
		output.color = baseColor;
		return output;
	}

	// BloomMask をぼかして、色と強度を反映する
	float3 bloom = SampleBloom(input.texcoord, texelSize) * tint * intensity;

	// 元のシーンカラーにブルームを加算する
	// saturate により 0.0 ～ 1.0 の範囲に収める
	output.color = float4(saturate(baseColor.rgb + bloom), baseColor.a);

	return output;
}