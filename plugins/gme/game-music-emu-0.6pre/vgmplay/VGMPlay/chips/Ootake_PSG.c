/******************************************************************************
Ootake
・キューの参照処理をシンプルにした。テンポの安定性および音質の向上。
・オーバーサンプリングしないようにした。（筆者の主観もあるが、PSGの場合、響きの
  美しさが損なわれてしまうケースが多いため。速度的にもアップ）
・ノイズの音質・音量を実機並みに調整した。v0.72
・ノイズの周波数に0x1Fが書き込まれたときは、0x1Eと同じ周波数で音量を半分にして
  鳴らすようにした。v0.68
・現状は再生サンプルレートは44.1KHz固定とした。(CD-DA再生時の速度アップのため)
・DDA音の発声が終了したときにいきなり波形を0にせず、フェードアウトさせるように
  し、ノイズを軽減した。v0.57
・DDAモード(サンプリング発声)のときの波形データのノイズが多く含まれている部分
  をカットしして、音質を上げた。音量も調節した。v0.59
・ノイズ音の音質・音量を調整して、実機の雰囲気に近づけた。v0.68
・waveIndexの初期化とDDAモード時の動作を見直して実機の動作に近づけた。v0.63
・waveIndexの初期化時にwaveテーブルも初期化するようにした。ファイヤープロレス
  リング、Ｆ１トリプルバトルなどの音が実機に近づいた。v0.65
・waveの波形の正負を実機同様にした。v0.74
・waveの最小値が-14になるようにし音質を整えた。v0.74
・クリティカルセクションは必要ない(書き込みが同時に行われるわけではない)ような
  ので、省略し高速化した。v1.09
・キュー処理(ApuQueue.c)をここに統合して高速化した。v1.10
・低音領域のボリュームを上げて実機並みの聞こえやすさに近づけた。v1.46
・LFO処理のの実装。"はにいいんざすかい"のOPや、フラッシュハイダースの効果音が
  実機の音に近づいた。v1.59

Copyright(C)2006-2012 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

*******************************************************************************
	[PSG.c]
		ＰＳＧを実装します。

	Implements the PSG.

	Copyright (C) 2004 Ki

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "mamedef.h"
#include "Ootake_PSG.h"
//#include "MainBoard.h" //Kitao追加
//#include "App.h" //Kitao追加
//#include "PRINTF.h" //Kitao追加
//#define PRINTF	printf


#define N_CHANNEL			6

//#define SAMPLE_RATE			44100.0 //Kitao更新。現状は速度優先でサンプルレートを44100固定とした。
#define OVERSAMPLE_RATE		1.0 //Kitao更新。PSGはオーバーサンプリングすると響きの美しさが損なわれてしまうのでオーバーサンプリングしないようにした。速度的にもアップ。
#define PSG_DECLINE			(21.8500*6.0) //21.8500。Kitao追加。PSG音量の減少値。*6.0は各チャンネル足したぶんを割る意味。大きいほど音は減る。CDDAが100%のときにちょうど良いぐらいの音量に合わせよう。v2.19,v2.37,v2.39,v2.62更新
#define VOL_TABLE_DECLINE	-1.05809999010 //-1.05809999010で雀探物語２OK。Kitao追加。音量テーブルの減少値。マイナスが大きいほど小さい音が聞こえづらくなる。マイナスが小さすぎると平面的な音になる。v2.19,v2.37,v2.39,v2.40,v2.62,v2.65更新
									       //  ※PSG_DECLINEの値を変更した場合、減退率のベスト値も変更する必要がある。雀探物語２(マイナスが小さいとPSGが目立ちすぎてADPCMが聴きづらい)，大魔界村(マイナスが大きいと音篭り),ソルジャーブレイドで、PSG_DECLINE=(14.4701*6.0)で減退率-1.0498779900db前後が飛び抜けていい響き(うちの環境で主観)。
										   //																		  モトローダー(マイナスやや大き目がいい),１９４１(マイナス小さめがいい)なども微妙な値変更で大きく変わる。
#define NOISE_TABLE_VALUE	-18 : -1 //キレと聴きやすさで-18:-1をベストとした。最大値が大きい(+に近い)と重い音に。２つの値が離れていると重い音に。フォーメーションサッカー，大魔界村のエンディングのドラムなどで調整。v1.46,v2.40,v2.62更新
									 //  ※VOL_TABLE_DECLINEによってこの値の最適値も変化する。
#define SAMPLE_FADE_DECLINE	 0.305998999951 //0.30599899951。Kitao追加。サンプリング音の消音時の音の減退量。ソルジャーブレイド,将棋初心者無用の音声で調整。基本的にこの値が小さいほうがノイズが減る(逆のケースもある)。v2.40
											//							サンプリングドラムの音色が決まるので大事な値。値が大きすぎるとファイナルソルジャーやソルジャーブレイド,モトローダーなどでドラムがしょぼくなる。

//#define RESMPL_RATE		PSG_FRQ / OVERSAMPLE_RATE / SAMPLE_RATE	// the lack of () is intentional


/*-----------------------------------------------------------------------------
	[DEV NOTE]

	MAL			--- 0 - 15 (15 で -0[dB], １減るごとに -3.0 [dB])
	AL			--- 0 - 31 (31 で -0[dB], １減るごとに -1.5 [dB])
	LAL/RAL		--- 0 - 15 (15 で -0[dB], １減るごとに -3.0 [dB])

	次のように解釈しなおす。

	MAL*2		--- 0 - 30 (30 で -0[dB], １減るごとに -1.5 [dB])
	AL			--- 0 - 31 (31 で -0[dB], １減るごとに -1.5 [dB])
	LAL/RAL*2	--- 0 - 30 (30 で -0[dB], １減るごとに -1.5 [dB])


	dB = 20 * log10(OUT/IN)

	dB / 20 = log10(OUT/IN)

	OUT/IN = 10^(dB/20)

	IN(最大出力) を 1.0 とすると、

	OUT = 10^(dB/20)

					-91 <= -(MAL*2 + AL + LAL(RAL)*2) <= 0

	だから、最も小さい音は、

		-91 * 1.5 [dB] = -136.5 [dB] = 10^(-136.5/20) ~= 1.496236e-7 [倍]

	となる。

	  1e-7 オーダーの値は、固定小数点で表現しようとすると、小数部だけで
	24 ビット以上必要で、なおかつ１６ビットの音声を扱うためには +16ビット
	だから 24+16 = 40ビット以上必要になる。よって、32 ビットの処理系で
	ＰＣＥの音声を固定小数点で表現するのはつらい。そこで、波形の計算は
	float で行なうことにする。

	  float から出力形式に変換するのはＡＰＵの仕事とする。

	[2004.4.28] やっぱり Sint32 で実装することにした(微小な値は無視する)。

	  ＣＰＵとＰＳＧは同じＩＣにパッケージしてあるのだが、
	実際にはＰＳＧはＣＰＵの１／２のクロックで動作すると考えて良いようだ。
	よって、ＰＳＧの動作周波数 Fpsg は、

		Fpsg = 21.47727 [MHz] / 3 / 2 = 3.579545 [MHz]

	となる。

	たとえば３２サンプルを１周期とする波形が再生されるとき、
	この周波数の周期でサンプルを１つずつ拾い出すと、

		M = 3579545 / 32 = 111860.78125 [Hz]

	というマジックナンバーが得られる（ファミコンと同じ）。
	ただし、再生周波数が固定では曲の演奏ができないので、
	FRQ なる周波数パラメータを用いて再生周波数を変化させる。
	FRQ はＰＳＧのレジスタに書き込まれる１２ビット長のパラメータで、
	↑で得られたマジックナンバーの「割る数」になっている。

	上の３２サンプルを１周期とする波形が再生されるとき、
	この波形の周波数 F は、FRQ を用いて、
	
		F = M / FRQ [Hz]  (FRQ != 0)
	
	となる。

	  ＰＣの再生サンプリング周波数が Fpc [Hz] だとすると、
	１周期３２サンプルの波形の再生周波数 F2 は  F2 = Fpc / 32 [Hz]。
	よって、ＰＣの１サンプルに対して、ＰＣＥの１サンプルを拾い出す
	カウンタの進み幅 I は

		I = F / F2 = 32 * F / Fpc = Fpsg / FRQ / Fpc [単位なし]

	となる。

	[NOISE CHANNEL]

	  擬似ノイズの生成にはＭ系列(maximum length sequence)が用いられる。
	Ｍ系列のビット長は未調査につき不明。
	ここでは仮に１５ビットとして実装を行なう。
	出力は１ビットで、D0 がゼロのときは負の値、１のときは正の値とする。

	ＰＣの１サンプルに対して、ＰＣＥの１サンプルを拾い出す
	カウンタの進み幅 I は、

		I = Fpsg / 64 / FRQ / Fpc  (FRQ != 0)

	となる。

	[再生クオリティ向上について] 2004.6.22

	  エミュレータでは、ＰＳＧのレジスタにデータが書き込まれるまで、
	次に発声すべき音がわからない。レジスタにデータが書き込まれたときに、
	サウンドバッファを更新したいのだけど、あいにく現在の実装では、
	サウンドバッファの更新は別スレッドで行なわれていて、
	エミュレーションスレッドから任意の時間に更新することができない。

	  これまでの再生では、サウンドバッファの更新時のレジスタ設定のみが
	有効だったが、これだと例えばサウンドバッファ更新の合間に一瞬だけ
	出力された音などが無視されてしまう。これは特にＤＤＡモードやノイズが
	リズムパートとして使用される上で問題になる。

	  レジスタに書き込まれた値をきちんと音声出力に反映させるには、
	過去に書き込まれたレジスタの値(いつ、どのレジスタに、何が書き込まれたか)
	を保存しておいて、サウンドバッファ更新時にこれを参照する方法が
	考えられる。どのくらい過去までレジスタの値を保存しておくかは、
	サウンドバッファの長さにもよると思われるが、とりあえずは試行錯誤で
	決めることにする。

	  ＰＳＧレジスタへの書き込み動作はエミュレーションスレッドで
	行なわれ、サウンドバッファ更新はその専用スレッドで行なわれる。
	これだと、エミュレーションスレッドがレジスタのキューに書き込みを
	行なっている最中に、サウンドバッファ更新スレッドがキューから
	読み出しを行なってしまい、アクセスが衝突する。この問題を解決するには、

		１．サウンドバッファの更新を別スレッドで行なわない
		２．キューのアクセス部分を排他処理にする

	の２とおりが考えられる。とりあえず２の方法をとることにする。
---------------------------------------------------------------------------*/


typedef struct
{
	Uint32		frq;
	BOOL		bOn;
	BOOL		bDDA;
	Uint32		volume;
	Uint32		volumeL;
	Uint32		volumeR;
	Sint32		outVolumeL;
	Sint32		outVolumeR;
	Sint32		wave[32];
	Uint32		waveIndex;
	Sint32		ddaSample;
	Uint32		phase;
	Uint32		deltaPhase;
	BOOL		bNoiseOn;
	Uint32		noiseFrq;
	Uint32		deltaNoisePhase;
} PSG;

typedef struct
{
	double SAMPLE_RATE;
	double PSG_FRQ;
	double RESMPL_RATE;

	PSG			Psg[8];				// 6, 7 is unused
	Sint32		DdaFadeOutL[8]; //Kitao追加
	Sint32		DdaFadeOutR[8]; //Kitao追加
	Uint32		Channel;				// 0 - 5;
	Uint32		MainVolumeL;			// 0 - 15
	Uint32		MainVolumeR;			// 0 - 15
	Uint32		LfoFrq;
	BOOL		bLfoOn; //v1.59から非使用。過去verのステートロードのために残してある。
	Uint32		LfoCtrl;
	Uint32		LfoShift; //v1.59から非使用。過去verのステートロードのために残してある。
	Sint32		PsgVolumeEffect;	// = 0;//Kitao追加
	double		Volume;	// = 0;//Kitao追加
	double		VOL;	// = 0.0;//Kitao追加。v1.08
//	BOOL		_bPsgMute[8] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};//Kitao追加。v1.29
	BOOL		bPsgMute[8];

	Uint8		Port[16];				// for debug purpose 

	BOOL		bWaveCrash; //Kitao追加。DDA再生中にWaveデータが書き換えられたらTRUE
	BOOL		bHoneyInTheSky; //はにいいんざすかいパッチ用。v2.60
} huc6280_state;

static Sint32		_VolumeTable[92];
static Sint32		_NoiseTable[32768];

//static BOOL		_bPsgInit = FALSE;
static BOOL			_bTblInit = FALSE;

//Kitao更新。v1.10。キュー処理をここに統合して高速化。
/*
	ＡＰＵ専用キューの仕様

	レジスタに書き込みが行なわれるごとに、
	キューにその内容を追加する。

	サウンドバッファ更新時に経過時間をみて、
	過去に書き込まれたレジスタ内容を
	書き込まれた順にキューから取り出し、
	ＰＳＧレジスタを更新する。
	（なおＰＳＧレジスタは全て write only とみなす)
	  ↑要確認

	キューに追加するときには write index を用い、
	取り出すときには read index を用いる。

	// 追加
	queue[write index++] = written data

	// 取り出し
	data = queue[read index++]

	キューから値を取り出したときに read index が
	write index と一致したときは queue underflow。
	→とりあえずなにもしない。

	キューに値を追加したときに write index が
	read index と一致したときは queue overflow。
	→とりあえずリセットすることにする。
*/

/*#define APUQUEUE_SIZE	65536*2				// must be power of 2  v1.61更新。65536だとＡ列車３をオーバークロックしてプレイしたときに足りなかった。

typedef struct		//Kitao更新。clockは非使用とした。v1.61からステートセーブのサイズを減らすために変数上からもカット。
{
	Uint8			reg;					// 0-15
	Uint8			data;					// written data
} ApuQueue;
typedef struct		//v1.60以前のステートロードのため残してある。
{
	Uint32			clock;					// cpu cycles elapsed since previous write  Kitao更新。clockは現在非使用。
	Uint8			reg;					// 0-15
	Uint8			data;					// written data
} OldApuQueue;

static ApuQueue				_Queue[APUQUEUE_SIZE];
static Uint32				_QueueWriteIndex;
static Uint32				_QueueReadIndex;*/


//ボリュームテーブルの作成
//Kitao更新。低音量の音が実機より聞こえづらいので、減退率をVOL_TABLE_DECLINE[db](試行錯誤したベスト値)とし、ノーマライズ処理をするようにした。v1.46
//			 おそらく、実機もアンプを通って出力される際にノーマライズ処理されている。
static void
create_volume_table()
{
	int		i;
	double	v;

	_VolumeTable[0] = 0; //Kitao追加
	for (i = 1; i <= 91; i++)
	{
		v = 91 - i;
		_VolumeTable[i] = (Sint32)(32768.0 * pow(10.0, v * VOL_TABLE_DECLINE / 20.0)); //VOL_TABLE_DECLINE。小さくしすぎると音が平面的な傾向に。ソルジャーブレイドで調整。v1.46。
	}
}


//ノイズテーブルの作成
static void
create_noise_table()
{
	Sint32	i;
	Uint32	bit0;
	Uint32	bit1;
	Uint32	bit14;
	Uint32	reg = 0x100;

	for (i = 0; i < 32768; i++)
	{
		bit0 = reg & 1;
		bit1 = (reg & 2) >> 1;
		bit14 = (bit0 ^ bit1);
		reg >>= 1;
		reg |= (bit14 << 14);
		_NoiseTable[i] = (bit0) ? NOISE_TABLE_VALUE; //Kitao更新。ノイズのボリュームと音質を調整した。
	}
}


/*-----------------------------------------------------------------------------
	[write_reg]
		ＰＳＧポートの書き込みに対する動作を記述します。
-----------------------------------------------------------------------------*/
//static inline void
INLINE void
write_reg(
	huc6280_state* info,
	Uint8		reg,
	Uint8		data)
{
	Uint32	i;
	Uint32	frq;//Kitao追加
	PSG*	PSGChn;

	info->Port[reg & 15] = data;

	switch (reg & 15)
	{
		case 0:	// register select
			info->Channel = data & 7;
			break;

		case 1:	// main volume
			info->MainVolumeL = (data >> 4) & 0x0F;
			info->MainVolumeR = data & 0x0F;

			/* LMAL, RMAL は全チャネルの音量に影響する */
			for (i = 0; i < N_CHANNEL; i++)
			{
				PSGChn = &info->Psg[i];
				PSGChn->outVolumeL = _VolumeTable[PSGChn->volume + (info->MainVolumeL + PSGChn->volumeL) * 2];
				PSGChn->outVolumeR = _VolumeTable[PSGChn->volume + (info->MainVolumeR + PSGChn->volumeR) * 2];
			}
			break;

		case 2:	// frequency low
			PSGChn = &info->Psg[info->Channel];
			PSGChn->frq &= ~0xFF;
			PSGChn->frq |= data;
			//Kitao更新。update_frequencyは、速度アップのためサブルーチンにせず直接実行するようにした。
			frq = (PSGChn->frq - 1) & 0xFFF;
			if (frq)
				PSGChn->deltaPhase = (Uint32)((double)(65536.0 * 256.0 * 8.0 * info->RESMPL_RATE) / (double)frq +0.5); //Kitao更新。速度アップのためfrq以外は定数計算にした。精度向上のため、先に値の小さいOVERSAMPLE_RATEのほうで割るようにした。+0.5は四捨五入で精度アップ。プチノイズ軽減のため。
			else
				PSGChn->deltaPhase = 0;
			break;

		case 3:	// frequency high
			PSGChn = &info->Psg[info->Channel];
			PSGChn->frq &= ~0xF00;
			PSGChn->frq |= (data & 0x0F) << 8;
			//Kitao更新。update_frequencyは、速度アップのためサブルーチンにせず直接実行するようにした。
			frq = (PSGChn->frq - 1) & 0xFFF;
			if (frq)
				PSGChn->deltaPhase = (Uint32)((double)(65536.0 * 256.0 * 8.0 * info->RESMPL_RATE) / (double)frq +0.5); //Kitao更新。速度アップのためfrq以外は定数計算にした。精度向上のため、先に値の小さいOVERSAMPLE_RATEのほうで割るようにした。+0.5は四捨五入で精度アップ。プチノイズ軽減のため。
			else
				PSGChn->deltaPhase = 0;
			break;

		case 4:	// ON, DDA, AL
			PSGChn = &info->Psg[info->Channel];
			if (info->bHoneyInTheSky) //はにいいんざすかいのポーズ時に、微妙なボリューム調整タイミングの問題でプチノイズが載ってしまうので、現状はパッチ処理で対応。v2.60更新
			{
				if ((PSGChn->bOn)&&(data == 0)) //発声中にdataが0の場合、LRボリュームも0にリセット。はにいいんざすかいのポーズ時のノイズが解消。(data & 0x1F)だけが0のときにリセットすると、サイレントデバッガーズ等でNG。発声してない時にリセットするとアトミックロボでNG。ｖ2.55
				{
					//PRINTF("test %X %X %X %X",info->Channel,PSGChn->bOn,info->MainVolumeL,info->MainVolumeR);
					if ((info->MainVolumeL & 1) == 0) //メインボリュームのbit0が0のときだけ処理(はにいいんざすかいでイレギュラーな0xE。他のゲームは0xF。※ヘビーユニットも0xEだった)。これがないとミズバク大冒険で音が出ない。実機の仕組みと同じかどうかは未確認。v2.53追加
						PSGChn->volumeL = 0;
					if ((info->MainVolumeR & 1) == 0) //右チャンネルも同様とする
						PSGChn->volumeR = 0;
				}
			}

			PSGChn->bOn = ((data & 0x80) != 0);
			if ((PSGChn->bDDA)&&((data & 0x40)==0)) //DDAからWAVEへ切り替わるとき or DDAから消音するとき
			{
				//Kitao追加。DDAはいきなり消音すると目立つノイズが載るのでフェードアウトする。
				info->DdaFadeOutL[info->Channel] = (Sint32)((double)(PSGChn->ddaSample * PSGChn->outVolumeL) *
															((1 + (1 >> 3) + (1 >> 4) + (1 >> 5) + (1 >> 7) + (1 >> 12) + (1 >> 14) + (1 >> 15)) * SAMPLE_FADE_DECLINE)); //元の音量。v2.65更新
				info->DdaFadeOutR[info->Channel] = (Sint32)((double)(PSGChn->ddaSample * PSGChn->outVolumeR) *
															((1 + (1 >> 3) + (1 >> 4) + (1 >> 5) + (1 >> 7) + (1 >> 12) + (1 >> 14) + (1 >> 15)) * SAMPLE_FADE_DECLINE));

			}
			PSGChn->bDDA = ((data & 0x40) != 0);
			
			//Kitao追加。dataのbit7,6が01のときにWaveインデックスをリセットする。
			//DDAモード時にWaveデータを書き込んでいた場合はここでWaveデータを修復（初期化）する。ファイヤープロレスリング。
			if ((data & 0xC0) == 0x40)
			{
				PSGChn->waveIndex = 0;
				if (info->bWaveCrash)
				{
					for (i=0; i<32; i++)
						PSGChn->wave[i] = -14; //Waveデータを最小値で初期化
					info->bWaveCrash = FALSE;
				}
			}

			PSGChn->volume = data & 0x1F;
			PSGChn->outVolumeL = _VolumeTable[PSGChn->volume + (info->MainVolumeL + PSGChn->volumeL) * 2];
			PSGChn->outVolumeR = _VolumeTable[PSGChn->volume + (info->MainVolumeR + PSGChn->volumeR) * 2];
			break;

		case 5:	// LAL, RAL
			PSGChn = &info->Psg[info->Channel];
			PSGChn->volumeL = (data >> 4) & 0xF;
			PSGChn->volumeR = data & 0xF;
			PSGChn->outVolumeL = _VolumeTable[PSGChn->volume + (info->MainVolumeL + PSGChn->volumeL) * 2];
			PSGChn->outVolumeR = _VolumeTable[PSGChn->volume + (info->MainVolumeR + PSGChn->volumeR) * 2];
			break;

		case 6:	// wave data  //Kitao更新。DDAモードのときもWaveデータを更新するようにした。v0.63。ファイヤープロレスリング
			PSGChn = &info->Psg[info->Channel];
			data &= 0x1F;
			info->bWaveCrash = FALSE; //Kitao追加
			if (!PSGChn->bOn) //Kitao追加。音を鳴らしていないときだけWaveデータを更新する。v0.65。F1トリプルバトルのエンジン音。
			{
				PSGChn->wave[PSGChn->waveIndex++] = 17 - data; //17。Kitao更新。一番心地よく響く値に。ミズバク大冒険，モトローダー，ドラゴンスピリット等で調整。
				PSGChn->waveIndex &= 0x1F;
			}
			if (PSGChn->bDDA)
			{
				//Kitao更新。ノイズ軽減のため6より下側の値はカットするようにした。v0.59
				if (data < 6) //サイバーナイトで6に決定
					data = 6; //ノイズが多いので小さな値はカット
				PSGChn->ddaSample = 11 - data; //サイバーナイトで11に決定。ドラムの音色が最適。v0.74
			
				if (!PSGChn->bOn) //DDAモード時にWaveデータを書き換えた場合
					info->bWaveCrash = TRUE;
			}
			break;

		case 7:	// noise on, noise frq
			if (info->Channel >= 4)
			{
				PSGChn = &info->Psg[info->Channel];
				PSGChn->bNoiseOn = ((data & 0x80) != 0);
				PSGChn->noiseFrq = 0x1F - (data & 0x1F);
				if (PSGChn->noiseFrq == 0)
					PSGChn->deltaNoisePhase = (Uint32)((double)(2048.0 * info->RESMPL_RATE) +0.5); //Kitao更新
				else
					PSGChn->deltaNoisePhase = (Uint32)((double)(2048.0 * info->RESMPL_RATE) / (double)PSGChn->noiseFrq +0.5); //Kitao更新
			}
			break;

		case 8:	// LFO frequency
			info->LfoFrq = data;
			//Kitaoテスト用
			//PRINTF("LFO Frq = %X",info->LfoFrq);
			break;

		case 9:	// LFO control  Kitao更新。シンプルに実装してみた。実機で同じ動作かは未確認。はにいいんざすかいの音が似るように実装。v1.59
			if (data & 0x80) //bit7を立てて呼ぶと恐らくリセット
			{
				info->Psg[1].phase = 0; //LfoFrqは初期化しない。はにいいんざすかい。
				//Kitaoテスト用
				//PRINTF("LFO control = %X",data);
			}
			info->LfoCtrl = data & 7; //ドロップロックほらホラで5が使われる。v1.61更新
			if (info->LfoCtrl & 4)
				info->LfoCtrl = 0; //ドロップロックほらホラ。実機で聴いた感じはLFOオフと同じ音のようなのでbit2が立っていた(負の数扱い？)ら0と同じこととする。
			//Kitaoテスト用
			//PRINTF("LFO control = %X,  Frq =%X",data,info->LfoFrq);
			break;

		default:	// invalid write
			break;
	}

	return;
}


//Kitao追加
static void
set_VOL(huc6280_state* info)
{
	//Sint32	v;

	if (info->PsgVolumeEffect == 0)
		//info->VOL = 0.0; //ミュート
		info->VOL = 1.0 / 128.0;
	else if (info->PsgVolumeEffect == 3)
		info->VOL = info->Volume / (double)(OVERSAMPLE_RATE * 4.0/3.0); // 3/4。v1.29追加
	else
		info->VOL = info->Volume / (double)(OVERSAMPLE_RATE * info->PsgVolumeEffect); //Kitao追加。_PsgVolumeEffect=ボリューム調節効果。

	/*if (!APP_GetCDGame()) //Huカードゲームのときだけ、ボリューム101～120を有効化。v2.62
	{
		v = APP_GetWindowsVolume();
		if (v > 100)
			_VOL *= ((double)(v-100) * 3.0 + 100.0) / 100.0; //101～120は通常の3.0倍の音量変化。3.0倍のVol120でソルジャーブレイド最適。ビックリマンワールドOK。3.1倍以上だと音が薄くなる＆音割れの心配もあり。
	}*/
}

/*-----------------------------------------------------------------------------
	[Mix]
		ＰＳＧの出力をミックスします。
-----------------------------------------------------------------------------*/
void
PSG_Mix(
//	Sint16*		pDst,		// 出力先バッファ //Kitao更新。PSG専用バッファにしたためSint16に。
	void*		chip,
	Sint32**	pDst,
	Sint32		nSample)	// 書き出すサンプル数 
{
	huc6280_state* info = (huc6280_state*)chip;
	PSG*		PSGChn;
	Sint32		i;
	Sint32		j;
	Sint32		sample; //Kitao追加
	Sint32		lfo;
	Sint32		sampleAllL; //Kitao追加。6chぶんのサンプルを足していくためのバッファ。精度を維持するために必要。6chぶん合計が終わった後に、これをSint16に変換して書き込むようにした。
	Sint32		sampleAllR; //Kitao追加。上のＲチャンネル用
	Sint32		smp; //Kitao追加。DDA音量,ノイズ音量計算用
	Sint32*		bufL = pDst[0];
	Sint32*		bufR = pDst[1];

//	if (!_bPsgInit)
//		return;

	for (j=0; j<nSample; j++)
	{
		sampleAllL = 0;
		sampleAllR = 0;
		for (i=0; i<N_CHANNEL; i++)
		{
			PSGChn = &info->Psg[i];
			
			if ((PSGChn->bOn)&&((i != 1)||(info->LfoCtrl == 0))&&(!info->bPsgMute[i])) //Kitao更新
			{
				if (PSGChn->bDDA)
				{
					smp = PSGChn->ddaSample * PSGChn->outVolumeL;
					sampleAllL += smp + (smp >> 3) + (smp >> 4) + (smp >> 5) + (smp >> 7) + (smp >> 12) + (smp >> 14) + (smp >> 15); //Kitao更新。サンプリング音の音量を実機並みに調整。v2.39,v2.40,v2.62,v2.65再調整した。
					smp = PSGChn->ddaSample * PSGChn->outVolumeR;
					sampleAllR += smp + (smp >> 3) + (smp >> 4) + (smp >> 5) + (smp >> 7) + (smp >> 12) + (smp >> 14) + (smp >> 15); //Kitao更新。サンプリング音の音量を実機並みに調整。v2.39,v2.40,v2.62,v2.65再調整した。
				}
				else if (PSGChn->bNoiseOn)
				{
					sample = _NoiseTable[PSGChn->phase >> 17];
					
					if (PSGChn->noiseFrq == 0) //Kitao追加。noiseFrq=0(dataに0x1Fが書き込まれた)のときは音量が通常の半分とした。（ファイヤープロレスリング３、パックランド、桃太郎活劇、がんばれゴルフボーイズなど）
					{
						smp = sample * PSGChn->outVolumeL;
						sampleAllL += (smp >> 1) + (smp >> 12) + (smp >> 14); //(1/2 + 1/4096 + (1/32768 + 1/32768))
						smp = sample * PSGChn->outVolumeR;
						sampleAllR += (smp >> 1) + (smp >> 12) + (smp >> 14);
					}
					else //通常
					{
						smp = sample * PSGChn->outVolumeL;
						sampleAllL += smp + (smp >> 11) + (smp >> 14) + (smp >> 15); //Kitao更新。ノイズの音量を実機並みに調整(1 + 1/2048 + 1/16384 + 1/32768)。この"+1/32768"で絶妙(主観。大魔界村,ソルジャーブレイドなど)になる。v2.62更新
						smp = sample * PSGChn->outVolumeR;
						sampleAllR += smp + (smp >> 11) + (smp >> 14) + (smp >> 15); //Kitao更新。ノイズの音量を実機並みに調整
					}
					
					PSGChn->phase += PSGChn->deltaNoisePhase; //Kitao更新
				}
				else if (PSGChn->deltaPhase)
				{
					//Kitao更新。オーバーサンプリングしないようにした。
					sample = PSGChn->wave[PSGChn->phase >> 27];
					if (PSGChn->frq < 128)
						sample -= sample >> 2; //低周波域の音量を制限。ブラッドギアのスタート時などで実機と同様の音に。ソルジャーブレイドなども実機に近くなった。v2.03

					sampleAllL += sample * PSGChn->outVolumeL; //Kitao更新
					sampleAllR += sample * PSGChn->outVolumeR; //Kitao更新
					
					//Kitao更新。Lfoオンが有効になるようにし、Lfoの掛かり具合を実機に近づけた。v1.59
					if ((i==0)&&(info->LfoCtrl>0))
					{
						//_LfoCtrlが1のときに0回シフト(そのまま)で、はにいいんざすかいが実機の音に近い。
						//_LfoCtrlが3のときに4回シフトで、フラッシュハイダースが実機の音に近い。
						lfo = info->Psg[1].wave[info->Psg[1].phase >> 27] << ((info->LfoCtrl-1) << 1); //v1.60更新
						info->Psg[0].phase += (Uint32)((double)(65536.0 * 256.0 * 8.0 * info->RESMPL_RATE) / (double)(info->Psg[0].frq + lfo) +0.5);
						info->Psg[1].phase += (Uint32)((double)(65536.0 * 256.0 * 8.0 *info-> RESMPL_RATE) / (double)(info->Psg[1].frq * info->LfoFrq) +0.5); //v1.60更新
					}
					else
						PSGChn->phase += PSGChn->deltaPhase;
				}
			}
			//Kitao追加。DDA消音時はノイズ軽減のためフェードアウトで消音する。
			//			 ベラボーマン(「わしがばくだはかせじゃ」から数秒後)やパワーテニス(タイトル曲終了から数秒後。点数コール)，将棋初心者無用(音声)等で効果あり。
			if (info->DdaFadeOutL[i] > 0)
				--info->DdaFadeOutL[i];
			else if (info->DdaFadeOutL[i] < 0)
				++info->DdaFadeOutL[i];
			if (info->DdaFadeOutR[i] > 0)
				--info->DdaFadeOutR[i];
			else if (info->DdaFadeOutR[i] < 0)
				++info->DdaFadeOutR[i];
			sampleAllL += info->DdaFadeOutL[i];
			sampleAllR += info->DdaFadeOutR[i];
		}
		//Kitao更新。6ch合わさったところで、ボリューム調整してバッファに書き込む。
		sampleAllL = (Sint32)((double)sampleAllL * info->VOL);
		//if ((sampleAllL>32767)||(sampleAllL<-32768)) PRINTF("PSG Sachitta!");//test用
//		if (sampleAllL> 32767) sampleAllL= 32767; //Volをアップしたのでサチレーションチェックが必要。v2.39
//		if (sampleAllL<-32768) sampleAllL=-32768; //  パックランドでUFO等にやられたときぐらいで、通常のゲームでは起こらない。音量の大きなビックリマンワールドもOK。パックランドも通常はOKでサチレーションしたときでもわずかなので音質的に大丈夫。
												  //  なので音質的には、PSGを２つのDirectXチャンネルに分けて鳴らすべき(処理は重くなる)だが、現状はパックランドでもサチレーション処理だけで音質的に問題なし(速度優先)とする。
		sampleAllR = (Sint32)((double)sampleAllR * info->VOL);
		//if ((sampleAllR>32767)||(sampleAllR<-32768)) PRINTF("PSG Satitta!");//test用
//		if (sampleAllR> 32767) sampleAllR= 32767; //Volをアップしたのでサチレーションチェックが必要。v2.39
//		if (sampleAllR<-32768) sampleAllR=-32768; //
		*bufL++ = sampleAllL;
		*bufR++ = sampleAllR;
		
		//キューを参照しPSGレジスタを更新する。Kitao更新。高速化のためサブルーチンにせずここで処理。キューの参照はシンプルにした(テンポの安定性向上)。
		/*while (_QueueReadIndex != _QueueWriteIndex) //v1.10更新。キュー処理をここへ統合し高速化。
		{
			write_reg(_Queue[_QueueReadIndex].reg, _Queue[_QueueReadIndex].data);
			_QueueReadIndex++; //Kitao更新
			_QueueReadIndex &= APUQUEUE_SIZE-1; //Kitao更新
		}*/
	}
}


//Kitao更新
static void
psg_reset(huc6280_state* info)
{
	int		i,j;

	memset(info->Psg, 0, sizeof(info->Psg));
	memset(info->DdaFadeOutL, 0, sizeof(info->DdaFadeOutL)); //Kitao追加
	memset(info->DdaFadeOutR, 0, sizeof(info->DdaFadeOutR)); //Kitao追加
	info->MainVolumeL = 0;
	info->MainVolumeR = 0;
	info->LfoFrq = 0;
	info->LfoCtrl = 0;
	info->Channel = 0; //Kitao追加。v2.65
	info->bWaveCrash = FALSE; //Kitao追加

	//Kitao更新。v0.65．waveデータを初期化。
	for (i=0; i<N_CHANNEL; i++)
		for (j=0; j<32; j++)
			info->Psg[i].wave[j] = -14; //最小値で初期化。ファイプロ，フォーメーションサッカー'90，F1トリプルバトルで必要。
	for (j=0; j<32; j++)
		info->Psg[3].wave[j] = 17; //ch3は最大値で初期化。F1トリプルバトル。v2.65

	//Kitao更新。v1.10。キュー処理をここに統合
//	_QueueWriteIndex = 0;
//	_QueueReadIndex  = 0;
}


static void PSG_SetVolume(huc6280_state* info);
/*-----------------------------------------------------------------------------
	[Init]
		ＰＳＧを初期化します。
-----------------------------------------------------------------------------*/
//Sint32
void*
PSG_Init(
	Sint32		clock,
	Sint32		sampleRate)
{
	huc6280_state* info;
	
	info = (huc6280_state*)malloc(sizeof(huc6280_state));
	if (info == NULL)
		return NULL;
	
	info->PSG_FRQ = clock & 0x7FFFFFFF;
	PSG_SetHoneyInTheSky(info, (clock >> 31) & 0x01);
//	PSG_SetHoneyInTheSky(0x01);
	
	info->PsgVolumeEffect = 0;
	info->Volume = 0;
	info->VOL = 0.0;
	
	//PSG_SetVolume(APP_GetPsgVolume());//Kitao追加
	PSG_SetVolume(info);

	psg_reset(info);

	if (! _bTblInit)
	{
		create_volume_table();
		create_noise_table();
		_bTblInit = TRUE;
	}

	//PSG_SetSampleRate(sampleRate);
	info->SAMPLE_RATE = sampleRate;
	info->RESMPL_RATE = info->PSG_FRQ / OVERSAMPLE_RATE / info->SAMPLE_RATE;

//	_bPsgInit = TRUE;

	return info;
}


/*-----------------------------------------------------------------------------
	[SetSampleRate]
-----------------------------------------------------------------------------*/
/*void
PSG_SetSampleRate(
	Uint32		sampleRate)
{
	//_SampleRate = sampleRate;
}*/


/*-----------------------------------------------------------------------------
	[Deinit]
		ＰＳＧを破棄します。
-----------------------------------------------------------------------------*/
void
PSG_Deinit(void* chip)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	/*memset(info->Psg, 0, sizeof(_Psg));
	memset(info->DdaFadeOutL, 0, sizeof(_DdaFadeOutL)); //Kitao追加
	memset(info->DdaFadeOutR, 0, sizeof(_DdaFadeOutR)); //Kitao追加
	info->MainVolumeL = 0;
	info->MainVolumeR = 0;
	info->LfoFrq = 0;
	info->LfoCtrl = 0;
	info->bWaveCrash = FALSE; //Kitao追加
//	_bPsgInit = FALSE;*/
	
	free(info);
}


/*-----------------------------------------------------------------------------
	[Read]
		ＰＳＧポートの読み出しに対する動作を記述します。
-----------------------------------------------------------------------------*/
Uint8
PSG_Read(
	void*	chip,
	Uint32	regNum)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	if (regNum == 0)
		return (Uint8)info->Channel;

	return info->Port[regNum & 15];
}


/*-----------------------------------------------------------------------------
	[Write]
		ＰＳＧポートの書き込みに対する動作を記述します。
-----------------------------------------------------------------------------*/
void
PSG_Write(
	void*		chip,
	Uint32		regNum,
	Uint8		data)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	//v1.10更新。キュー処理をここに統合して高速化。
	//Kitao更新。clockは考慮せずにシンプルにして高速化した。
/*	if (((_QueueWriteIndex + 1) & (APUQUEUE_SIZE-1)) == _QueueReadIndex)
	{
		PRINTF("PSG Queue Over!"); // キューが満タン
		return;
	}
	_Queue[_QueueWriteIndex].reg   = (Uint8)(regNum & 15);
	_Queue[_QueueWriteIndex].data  = data;
	_QueueWriteIndex++; //Kitao更新
	_QueueWriteIndex &= APUQUEUE_SIZE-1; //Kitao更新
*/
	write_reg(chip, regNum, data);
}


/*Sint32
PSG_AdvanceClock(
	Sint32		clock)
{
	return 0;
}*/


//Kitao追加。PSGのボリュームも個別に設定可能にした。
/*static void
PSG_SetVolume(
	Uint32	volume)		// 0 - 65535*/
static void PSG_SetVolume(huc6280_state* info)
{
	/*if (volume < 0)	volume = 0;
	if (volume > 65535)	volume = 65535;*/

	//_Volume = (double)volume / 65535.0 / PSG_DECLINE;
	info->Volume = 1.0 / PSG_DECLINE;
	set_VOL(info);
}

//Kitao追加。ボリュームミュート、ハーフなどをできるようにした。
/*static void
PSG_SetVolumeEffect(
	Uint32	volumeEffect)
{
	_PsgVolumeEffect = (Sint32)volumeEffect; //※数値が大きいほどボリュームは小さくなる
	set_VOL();
}*/


//Kitao追加
void
PSG_ResetVolumeReg(void* chip)
{
	huc6280_state* info = (huc6280_state*)chip;
	int	i;

	info->MainVolumeL = 0;
	info->MainVolumeR = 0;
	for (i = 0; i < N_CHANNEL; i++)
	{
		info->Psg[i].volume = 0;
		info->Psg[i].outVolumeL = 0;
		info->Psg[i].outVolumeR = 0;
		info->DdaFadeOutL[i] = 0;
		info->DdaFadeOutR[i] = 0;
	}
}


//Kitao追加
void
PSG_SetMutePsgChannel(
	void*	chip,
	Sint32	num,
	BOOL	bMute)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	info->bPsgMute[num] = bMute;
	if (bMute)
	{
		info->DdaFadeOutL[num] = 0;
		info->DdaFadeOutR[num] = 0;
	}
}

void PSG_SetMuteMask(void* chip, Uint32 MuteMask)
{
	Uint8 CurChn;
	
	for (CurChn = 0x00; CurChn < N_CHANNEL; CurChn ++)
		PSG_SetMutePsgChannel(chip, CurChn, (MuteMask >> CurChn) & 0x01);
	
	return;
}

//Kitao追加
BOOL
PSG_GetMutePsgChannel(
	void*	chip,
	Sint32	num)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	return info->bPsgMute[num];
}

//Kitao追加。v2.60
void
PSG_SetHoneyInTheSky(
	void*	chip,
	BOOL	bHoneyInTheSky)
{
	huc6280_state* info = (huc6280_state*)chip;
	
	info->bHoneyInTheSky = bHoneyInTheSky;
}


/*// save variable
#define SAVE_V(V)	if (fwrite(&V, sizeof(V), 1, p) != 1)	return FALSE
#define LOAD_V(V)	if (fread(&V, sizeof(V), 1, p) != 1)	return FALSE
// save array
#define SAVE_A(A)	if (fwrite(A, sizeof(A), 1, p) != 1)	return FALSE
#define LOAD_A(A)	if (fread(A, sizeof(A), 1, p) != 1)		return FALSE*/
/*-----------------------------------------------------------------------------
	[SaveState]
		状態をファイルに保存します。 
-----------------------------------------------------------------------------*/
/*BOOL
PSG_SaveState(
	FILE*		p)
{
	BOOL	bFlashHiders = FALSE; //Kitao更新。現在非使用。旧バージョンのステートセーブとの互換のため

	if (p == NULL)
		return FALSE;

	SAVE_A(_Psg);

	SAVE_V(_Channel);
	SAVE_V(_MainVolumeL);
	SAVE_V(_MainVolumeR);
	SAVE_V(_LfoFrq);
	SAVE_V(_bLfoOn); //v1.59から非使用に。
	SAVE_V(_LfoCtrl);
	SAVE_V(_LfoShift); //v1.59から非使用に。
	SAVE_V(_bWaveCrash); //Kitao追加。v0.65

	SAVE_V(bFlashHiders); //Kitao追加。v0.62

	//v1.10追加。キュー処理をここへ統合。
	SAVE_A(_Queue); //v1.61からサイズが２倍になった。
	SAVE_V(_QueueWriteIndex);
	SAVE_V(_QueueReadIndex);

	return TRUE;
}*/


/*-----------------------------------------------------------------------------
	[LoadState]
		状態をファイルから読み込みます。 
-----------------------------------------------------------------------------*/
/*BOOL
PSG_LoadState(
	FILE*		p)
{
	Uint32			i;
	double			clockCounter; //Kitao更新。現在非使用。旧バージョンのステートセーブとの互換のため
	BOOL			bInit; //Kitao更新。現在非使用。旧バージョンのステートセーブとの互換のため
	Sint32			totalClockAdvanced; //Kitao更新。現在非使用。旧バージョンのステートセーブとの互換のため
	BOOL			bFlashHiders; //Kitao更新。現在非使用。旧バージョンのステートセーブとの互換のため
	OldApuQueue		oldQueue[65536]; //v1.60以前のステートを読み込み用。
	

	if (p == NULL)
		return FALSE;

	LOAD_A(_Psg);

	LOAD_V(_Channel);
	LOAD_V(_MainVolumeL);
	LOAD_V(_MainVolumeR);
	LOAD_V(_LfoFrq);
	LOAD_V(_bLfoOn); //v1.59から非使用に。
	LOAD_V(_LfoCtrl);
	if (MAINBOARD_GetStateVersion() >= 3) //Kitao追加。v0.57以降のセーブファイルなら
		LOAD_V(_LfoShift); //v1.59から非使用に。
	if (MAINBOARD_GetStateVersion() >= 9) //Kitao追加。v0.65以降のセーブファイルなら
	{
		LOAD_V(_bWaveCrash);
	}
	else
		_bWaveCrash = FALSE;

	if (MAINBOARD_GetStateVersion() >= 7) //Kitao追加。v0.62以降のセーブファイルなら
		LOAD_V(bFlashHiders);

	//v1.10追加。キュー処理をここへ統合。v1.61更新
	if (MAINBOARD_GetStateVersion() >= 34) //v1.61beta以降のセーブファイルなら
	{
		LOAD_A(_Queue); //v1.61からサイズが２倍＆clock部分を削除した。
		LOAD_V(_QueueWriteIndex);
		LOAD_V(_QueueReadIndex);
	}
	else //v1.60以前のキュー(旧)バージョンのステートの場合、新バージョンの方に合うように変換。
	{
		LOAD_A(oldQueue);
		LOAD_V(_QueueWriteIndex);
		LOAD_V(_QueueReadIndex);
		if (_QueueWriteIndex >= _QueueReadIndex)
		{
			for (i=_QueueReadIndex; i<=_QueueWriteIndex; i++)
			{
				_Queue[i].reg  = oldQueue[i].reg;
				_Queue[i].data = oldQueue[i].data;
			}
		}
		else //Writeの位置がReadの位置よりも前（65536地点をまたいでデータが存在しているとき）の場合
		{
			for (i=_QueueReadIndex; i<=65535; i++)
			{
				_Queue[i].reg  = oldQueue[i].reg;
				_Queue[i].data = oldQueue[i].data;
			}
			for (i=0; i<=_QueueWriteIndex; i++)
			{
				_Queue[65536+i].reg  = oldQueue[i].reg;
				_Queue[65536+i].data = oldQueue[i].data;
			}
			_QueueWriteIndex += 65536;
		}
	}
	if (MAINBOARD_GetStateVersion() < 26) //Kitao追加。v1.11より前のセーブファイルなら
	{
		LOAD_V(clockCounter); //現在非使用。v0.95
		LOAD_V(bInit); //現在非使用。v1.10
		LOAD_V(totalClockAdvanced); //現在非使用。v0.95
	}

	return TRUE;
}

#undef SAVE_V
#undef SAVE_A
#undef LOAD_V
#undef LOAD_A*/

