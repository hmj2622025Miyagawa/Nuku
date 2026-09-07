#include "DxLib.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

enum GameScene {
    SCENE_TITLE,
    SCENE_MAIN,
    SCENE_CLEAR,
    SCENE_GAMEOVER,
	SCENE_GACHA // ガチャシーンの定義
};

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    ChangeWindowMode(TRUE);
    SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    // ---- [ 変数設定 ] ----
    GameScene currentScene = SCENE_TITLE;

    int kabuBaseX = 270;
    int kabuX = kabuBaseX;
    int kabuY = 350;
    int targetY = 150;

    int startTime = 0;
    int limitTime = 10;
    int remainingTime = 10;
    int score = 0;

    bool lastMouseLeft = false;
    bool isPressingMouse = false;
    int basePullPower = 8;

    int comboCount = 0;
    int lastClickTime = 0;
    const int COMBO_LIMIT_TIME = 500;

    int kabuGraph = -1;
    int clearKabuX = 120;

	int imgNormal = -1;
	int imgGold = -1;
	int imgDaikon = -1;
	int imgMandragora = -1;

	int imgMogura = -1;
	int imgMogura_Stun = -1;

	imgNormal = LoadGraph("Kabu.png");
    imgGold = LoadGraph("Kabu_Gold.png");
    imgDaikon = LoadGraph("Daikon.png");
	imgMandragora = LoadGraph("Mandragora.png");
	imgMogura = LoadGraph("Mogura.png");
	imgMogura_Stun = LoadGraph("Mogura_Stun.png");

    if (
        imgNormal == -1 ||
        imgGold == -1 ||
        imgDaikon == -1 ||
        imgMandragora == -1 ||
		imgMogura == -1 ||
		imgMogura_Stun == -1
        )

    {

        DxLib_End();
        return -1;
    }


	const char* gachaNameList[] = { "通常のカブ", "金のカブ", "大根", "マンドラ" };

    unsigned int bodyC = 0;
	unsigned int botC = 0;

    int totalWalletPoints = 0;          // 所持ポイント（スコアがそのまま入ります）
    bool hasUnlocked[4] = { true, false, false, false }; // 野菜の解放状況（最初は通常のみ）
    int currentEquippedYasai = 0;       // 今セットされている野菜 (0:通常, 1:金, 2:大根, 3:マンドラ)

    int gachaResultYasai = 0;           // ガチャで当たった野菜
    const int GACHA_COST = 3000;         // ガチャ1回に必要なポイント
	bool isGachaAnimation = false;         // ガチャアニメーション中かどうか
	int gachaAnimationStartTime = 0;      // ガチャアニメーション開始時間
	int gachaAnimationYasai = 0;          // ガチャアニメーションで表示する野菜
	int gachaFinalYasai = 0;          // ガチャアニメーションで最終的に表示する野菜
	bool showGachaResult = false;          // ガチャ結果を表示するかどうか
	int gachaResultStartTime = 0;          // ガチャ結果表示開始時間

    // ---- おじゃまモグラ用の変数 ----
    bool isSign = false;
    bool isMogura = false;
	bool isMoguraStunned = false;

    int signStartTime = 0;
    int moguraStartTime = 0;
    int stunEndTime = 0;

    // ---- [ メインループ ] ----
    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
        ClearDrawScreen();

        int nowTime = GetNowCount();
        bool isStunned = (nowTime < stunEndTime);

        // 1. シーンごとの計算・入力処理
        switch (currentScene) {
        case SCENE_TITLE:
            if (CheckHitKey(KEY_INPUT_1) == 1) {
                // 金のカブ(1)を装備中ならパワーを1.5倍にする処理
                basePullPower = (currentEquippedYasai == 1) ? 15 : 10;
                limitTime = 10;
				kabuY = 350; 
				comboCount = 0;
                isSign = false;
				isMogura = false;
				stunEndTime = 0;
				isMoguraStunned = false;
				startTime = nowTime;
                currentScene = SCENE_MAIN;
            }
            if (CheckHitKey(KEY_INPUT_2) == 1) {
                basePullPower = (currentEquippedYasai == 1) ? 6 : 4;
                limitTime = 15; 
				kabuY = 350;
				comboCount = 0;
                isSign = false; 
				isMogura = false;
				isMoguraStunned = false;
				stunEndTime = 0; 
				startTime = nowTime;
                currentScene = SCENE_MAIN;
            }
            // Gキーでガチャ画面へ行く
            if (CheckHitKey(KEY_INPUT_G) == 1) {
                currentScene = SCENE_GACHA;
                WaitTimer(200);
            }
            break;

        case SCENE_MAIN:
        {
            int elapsedTime;
            elapsedTime = (nowTime - startTime) / 1000;
            remainingTime = limitTime - elapsedTime;

            if (remainingTime <= 0)
            {
                remainingTime = 0;
                currentScene = SCENE_GAMEOVER;
            }

            kabuX = kabuBaseX;
            isPressingMouse = false;

            if (comboCount > 0 && (nowTime - lastClickTime) > COMBO_LIMIT_TIME)
            {
                comboCount = 0;
            }

            if (isSign && (nowTime - signStartTime) > 400)
            {
                isSign = false;
                isMogura = true;
				isMoguraStunned = false;
                moguraStartTime = nowTime;
            }

			if (isMogura)
			{
				if (isMoguraStunned)
				{
					// スタンしてから700ms経過したら消える
					if (nowTime >= stunEndTime)
					{
						isMogura = false;
						isMoguraStunned = false;
					}
				}
				else
				{
					// 叩かれなかったら出現から1秒で消える
					if ((nowTime - moguraStartTime) > 1000)
					{
						isMogura = false;
					}
				}
			}

            // マウスの左クリック判定
            if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0)
            {
                if (!isStunned)
                {
                    isPressingMouse = true;
                }

                if (!lastMouseLeft && !isStunned)
                {

                    if (isMogura)
                    {
                        stunEndTime = nowTime + 700;
						isMoguraStunned = true;
                        comboCount = 0;
                    }
                    else
                    {
                        if (comboCount == 0 || (nowTime - lastClickTime) <= COMBO_LIMIT_TIME)
                        {
                            comboCount++;
                        }

                        int currentPullPower = basePullPower;
                        if (comboCount >= 20)
                        {
                            currentPullPower = basePullPower * 3;
                        }
                        else if (comboCount >= 5)
                        {
                            currentPullPower = basePullPower * 2;
                        }

                        kabuY -= currentPullPower;
                        kabuX = kabuBaseX + (GetRand(10) - 5);

                        if (!isSign && GetRand(100) < 15)
                        {
                            isSign = true;
                            signStartTime = nowTime;
                        }
                    }

                    lastClickTime = nowTime;
                }
                lastMouseLeft = true;
            }
            else
            {
                lastMouseLeft = false;
            }

            if (kabuY <= targetY)
            {
                score = remainingTime * 1000 + (comboCount * 50);

                // スコアを所持ポイントに加算
                totalWalletPoints += score;

                currentScene = SCENE_CLEAR;
            }
            break;
        }
        case SCENE_CLEAR:
        case SCENE_GAMEOVER:
            if (CheckHitKey(KEY_INPUT_RETURN) == 1)
            {
                currentScene = SCENE_TITLE;
                WaitTimer(300);
            }
            break;

        case SCENE_GACHA:
           
			// ガチャを回す
			if (CheckHitKey(KEY_INPUT_SPACE) == 1 && !isGachaAnimation)
			{
				if (totalWalletPoints >= GACHA_COST)
				{
					// ポイントを消費
					totalWalletPoints -= GACHA_COST;

					// 最終結果を先に決めておく
					int r = GetRand(99);

					if (r < 40)
						gachaFinalYasai = 0;
					else if (r < 55)
						gachaFinalYasai = 1;
					else if (r < 80)
						gachaFinalYasai = 2;
					else
						gachaFinalYasai = 3;

					// 演出開始
					isGachaAnimation = true;
					gachaAnimationStartTime = nowTime;
					gachaAnimationYasai = 0;

					WaitTimer(200);
				}

				// ---- ガチャ演出 ----
				if (isGachaAnimation)
				{
					int elapsed = nowTime - gachaAnimationStartTime;

					// 3秒間演出
					if (elapsed < 2500)
					{
						// 時間によって切り替え速度を変える
						int changeTime;

						if (elapsed < 1000)
							changeTime = 80;
						else if (elapsed < 1800)
							changeTime = 140;
						else
							changeTime = 250;

						gachaAnimationYasai = (elapsed / changeTime) % 4;
					}
					else
					{
						// 演出終了 → 本当の結果
						gachaAnimationYasai = gachaFinalYasai;

						gachaResultYasai = gachaFinalYasai;

						hasUnlocked[gachaResultYasai] = true;
						currentEquippedYasai = gachaResultYasai;
						
						// 結果表示
						showGachaResult = true;
						gachaResultStartTime = GetNowCount();

						WaitTimer(500);


						isGachaAnimation = false;
					}
				}
			}

            // 【1〜4】キーが押されたら、解放済みのスキンを切り替える
            if (CheckHitKey(KEY_INPUT_1) == 1 && hasUnlocked[0]) currentEquippedYasai = 0;
            if (CheckHitKey(KEY_INPUT_2) == 1 && hasUnlocked[1]) currentEquippedYasai = 1;
            if (CheckHitKey(KEY_INPUT_3) == 1 && hasUnlocked[2]) currentEquippedYasai = 2;
            if (CheckHitKey(KEY_INPUT_4) == 1 && hasUnlocked[3]) currentEquippedYasai = 3;

            // 【Enter】キーでタイトルに戻る
            if (CheckHitKey(KEY_INPUT_RETURN) == 1) {
                currentScene = SCENE_TITLE;
                WaitTimer(300);
            }
            break;
        }
        // 2. シーンごとの描画処理
        switch (currentScene)
        {
        case SCENE_TITLE:
            DrawBox(0, 380, SCREEN_WIDTH, SCREEN_HEIGHT,
                GetColor(139, 69, 19), TRUE);

            DrawString(180, 150,
                "ーー 大きなかぶを引き抜け！ ーー",
                GetColor(255, 255, 255));

            DrawString(160, 190,
                "【 1 】キー：普通のカブを抜く（簡単）",
                GetColor(150, 255, 150));

            DrawString(160, 230,
                "【 2 】キー：巨大なカブを抜く（難しい）",
                GetColor(255, 150, 150));

            DrawString(160, 270,
                "【 G 】キー：ガチャ・図鑑ショップ画面へ",
                GetColor(255, 215, 0));

            DrawFormatString(
                160, 320,
                GetColor(255, 255, 255),
                "現在の装備: [ %s ]",
                gachaNameList[currentEquippedYasai]);

            DrawFormatString(
                160, 340,
                GetColor(255, 255, 255),
                "所持ポイント: %d pt",
                totalWalletPoints);

            break;


        case SCENE_MAIN:

            // 土
            DrawBox(
                0, 380,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                GetColor(139, 69, 19),
                TRUE);

            // モグラ
            if (isMogura)
            {
                // モグラの表示位置
                int moguraX = kabuX - 10;
                int moguraY = kabuY - 20;

                // 気絶中なら気絶画像、それ以外は通常画像
                if (isMoguraStunned)
                {
                    DrawExtendGraph(
                        moguraX,
                        moguraY,
                        moguraX + 120,
                        moguraY + 120,
                        imgMogura_Stun,
                        TRUE);
                }
                else
                {
                    DrawExtendGraph(
                        moguraX,
                        moguraY,
                        moguraX + 120,
                        moguraY + 120,
                        imgMogura,
                        TRUE);
                }
            }
            else
            {
                int currentgraph = imgNormal;

                int width = 70;
                int height = 90;

                if (currentEquippedYasai == 1)
                {
                    currentgraph = imgGold;
                    width = 70;
                    height = 90;
                }
                else if (currentEquippedYasai == 2)
                {
                    currentgraph = imgDaikon;
                    width = 60;
                    height = 110;
                }
                else if (currentEquippedYasai == 3)
                {
                    currentgraph = imgMandragora;
                    width = 70;
                    height = 100;
                }

                DrawExtendGraph(
                    kabuX,
                    kabuY,
                    kabuX + width,
                    kabuY + height,
                    currentgraph,
                    TRUE);
            }

            // 手を表示
            if (isPressingMouse && !isStunned)
            {
                DrawBox(
                    kabuX + 35,
                    0,
                    kabuX + 65,
                    kabuY - 10,
                    GetColor(100, 149, 237),
                    TRUE);

                DrawCircle(
                    kabuX + 50,
                    kabuY - 10,
                    18,
                    GetColor(255, 218, 185),
                    TRUE);
            }

            DrawFormatString(
                20, 20,
                GetColor(255, 255, 255),
                "残り時間: %d 秒",
                remainingTime);

            DrawString(
                20, 50,
                "【左クリック】を連打して引っ張れ！！！",
                GetColor(255, 255, 0));

            if (isStunned)
            {
                DrawString(
                    180, 220,
                    "痛っ！モグラを叩いて気絶してしまった！",
                    GetColor(255, 50, 50));
            }

			// WARNING
			if (isSign)
			{
				// 150msごとに点滅
				if ((nowTime / 150) % 2 == 0)
				{
					// 画面全体に赤い警告枠
					DrawBox(
						10, 10,
						SCREEN_WIDTH - 10,
						SCREEN_HEIGHT - 10,
						GetColor(255, 0, 0),
						FALSE
					);

					// 「！！！」を表示
					DrawString(
						250, 70,
						"！！！",
						GetColor(255, 0, 0)
					);

					// モグラ出現警告
					DrawString(
						190, 120,
						"モグラ出現注意！",
						GetColor(255, 255, 0)
					);

					// 下に説明
					DrawString(
						250, 170,
						"準備して！",
						GetColor(255, 255, 255)
					);
				}
			}

            // コンボ
            if (comboCount > 0)
            {
                unsigned int comboColor =
                    GetColor(255, 255, 255);

                double multiplier = 1.0;

                if (comboCount >= 20)
                {
                    comboColor =
                        GetColor(255, 50, 50);

                    multiplier = 3.0;
                }
                else if (comboCount >= 5)
                {
                    comboColor =
                        GetColor(255, 165, 0);

                    multiplier = 2.0;
                }

                DrawFormatString(
                    20, 90,
                    comboColor,
                    "%d COMBO !",
                    comboCount);

                DrawFormatString(
                    20, 110,
                    comboColor,
                    "引き抜きパワー: %.1f倍 !",
                    multiplier);
            }

            break;


        case SCENE_CLEAR:
        {
            DrawBox(
                0, 380,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                GetColor(139, 69, 19),
                TRUE);

            int clearGraph = imgNormal;

			int clearWidth = 70;
			int clearHeight = 90;

            if (currentEquippedYasai == 1)
            {
                clearGraph = imgGold;
				clearWidth = 70;
				clearHeight = 90;
            }
            else if (currentEquippedYasai == 2)
            {
                clearGraph = imgDaikon;
				clearWidth = 60;
				clearHeight = 110;
            }
            else if (currentEquippedYasai == 3)
            {
                clearGraph = imgMandragora;
				clearWidth = 70;
				clearHeight = 100;
            }

            DrawExtendGraph(
                clearKabuX,
                100,
                clearKabuX + clearWidth,
                100 + clearHeight,
                clearGraph,
                TRUE);

            DrawString(
                340, 130,
                "すぽーーん！！！",
                GetColor(255, 215, 0));

            DrawString(
                340, 160,
                "★★ CLEAR !! ★★",
                GetColor(255, 215, 0));

            DrawFormatString(
                340, 220,
                GetColor(255, 255, 255),
                "獲得スコア: %d 点",
                score);

            DrawFormatString(
                340, 250,
                GetColor(255, 255, 255),
                "最高コンボ: %d COMBO",
                comboCount);

            DrawString(
                340, 320,
                "[Enter] キーでタイトルへ",
                GetColor(200, 200, 200));

            break;
        }


        case SCENE_GAMEOVER:

            DrawBox(
                0, 380,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                GetColor(139, 69, 19),
                TRUE);

            DrawString(
                220, 150,
                "時間切れ... GAME OVER",
                GetColor(255, 50, 50));

            DrawString(
                200, 300,
                "[Enter] キーでタイトルへ",
                GetColor(200, 200, 200));

            break;


        case SCENE_GACHA:

			// ガチャ結果表示
			if (showGachaResult)
			{
				// 背景を真っ暗にして見やすくする
				DrawBox(
					0, 0,
					SCREEN_WIDTH, SCREEN_HEIGHT,
					GetColor(20, 20, 30),
					TRUE);

				// 結果パネル
				DrawBox(
					70, 50,
					570, 430,
					GetColor(60, 40, 90),
					TRUE);

				DrawBox(
					70, 50,
					570, 430,
					GetColor(255, 215, 0),
					FALSE);

				// 大きな「結果」
				DrawString(
					245, 80,
					"★★ ガチャ結果 ★★",
					GetColor(255, 215, 0));

				// 出た野菜の名前
				DrawFormatString(
					190, 140,
					GetColor(255, 255, 255),
					"「%s」が出た！",
					gachaNameList[gachaResultYasai]);

				// 野菜画像
				int resultGraph = imgNormal;
				int resultWidth = 120;
				int resultHeight = 150;

				if (gachaResultYasai == 1)
				{
					resultGraph = imgGold;
				}
				else if (gachaResultYasai == 2)
				{
					resultGraph = imgDaikon;
					resultWidth = 80;
					resultHeight = 150;
				}
				else if (gachaResultYasai == 3)
				{
					resultGraph = imgMandragora;
				}

				DrawExtendGraph(
					320 - resultWidth / 2,
					190,
					320 + resultWidth / 2,
					190 + resultHeight,
					resultGraph,
					TRUE);

				// レア度
				if (gachaResultYasai == 1)
				{
					DrawString(
						250, 355,
						"★★★ 超レア！！ ★★★",
						GetColor(255, 215, 0));
				}
				else if (gachaResultYasai == 3)
				{
					DrawString(
						245, 355,
						"★★ レア！！ ★★",
						GetColor(200, 100, 255));
				}
				else
				{
					DrawString(
						270, 355,
						"GET！！",
						GetColor(100, 255, 100));
				}

				DrawString(
					220, 400,
					"[Enter] または [Space] で戻る",
					GetColor(200, 200, 200));

				// Enter または Space で結果画面を閉じる
				if (CheckHitKey(KEY_INPUT_RETURN) == 1 ||
					CheckHitKey(KEY_INPUT_SPACE) == 1)
				{
					showGachaResult = false;
					WaitTimer(200);
				}

				break;
			}

            DrawString(
                200, 50,
                "=== ガチャ＆ショップ（図鑑） ===",
                GetColor(255, 215, 0));

            DrawFormatString(
                200, 90,
                GetColor(255, 255, 255),
                "あなたの所持ポイント: %d pt",
                totalWalletPoints);

            DrawFormatString(
                200, 120,
                GetColor(255, 165, 0),
                "【Space】キーでガチャを回す (1回 %d pt)",
                GACHA_COST);

			if (isGachaAnimation)
			{
				// 画面全体を暗くする
				DrawBox(
					0, 0,
					SCREEN_WIDTH, SCREEN_HEIGHT,
					GetColor(20, 20, 30),
					TRUE
				);

				// ガチャ演出の枠
				DrawBox(
					80, 60,
					560, 440,
					GetColor(80, 50, 120),
					TRUE
				);

				// 枠線
				DrawBox(
					80, 60,
					560, 440,
					GetColor(255, 215, 0),
					FALSE
				);

				DrawString(
					220, 90,
					"★★ ガチャ中！！ ★★",
					GetColor(255, 215, 0)
				);

				// 野菜の名前
				DrawFormatString(
					220, 150,
					GetColor(255, 255, 255),
					"？？？ %s ？？？",
					gachaNameList[gachaAnimationYasai]
				);

				// 画像
				int graph = imgNormal;
				int width = 100;
				int height = 130;

				if (gachaAnimationYasai == 1)
				{
					graph = imgGold;
				}
				else if (gachaAnimationYasai == 2)
				{
					graph = imgDaikon;
					width = 80;
					height = 150;
				}
				else if (gachaAnimationYasai == 3)
				{
					graph = imgMandragora;
				}

				DrawExtendGraph(
					320 - width / 2,
					190,
					320 + width / 2,
					190 + height,
					graph,
					TRUE
				);

				DrawString(
					220, 370,
					"何が出るかな・・・！？",
					GetColor(255, 255, 255)
				);

				// 演出中はここで終了
				break;
			}

            DrawString(
                120, 180,
                "--- 所有スキン（数字キーを押して装備変更） ---",
                GetColor(200, 200, 200));

            DrawFormatString(
                120, 210,
                hasUnlocked[0]
                ? GetColor(255, 255, 255)
                : GetColor(80, 80, 80),
                "【1】 ふつうのカブ : %s",
                hasUnlocked[0]
                ? "解放済み"
                : "未解放");

            DrawFormatString(
                120, 240,
                hasUnlocked[1]
                ? GetColor(255, 215, 0)
                : GetColor(80, 80, 80),
                "【2】 金のカブ     : %s",
                hasUnlocked[1]
                ? "解放済み (パワー1.5倍!)"
                : "未解放 [激レア]");

            DrawFormatString(
                120, 270,
                hasUnlocked[2]
                ? GetColor(150, 255, 150)
                : GetColor(80, 80, 80),
                "【3】 大根         : %s",
                hasUnlocked[2]
                ? "解放済み"
                : "未解放");

            DrawFormatString(
                120, 300,
                hasUnlocked[3]
                ? GetColor(147, 112, 219)
                : GetColor(80, 80, 80),
                "【4】 マンドラゴラ : %s",
                hasUnlocked[3]
                ? "解放済み"
                : "未解放");

            DrawString(
                80,
                210 + (currentEquippedYasai * 30),
                "->",
                GetColor(255, 255, 0));

            DrawString(
                200, 380,
                "[Enter] キーでタイトルに戻る",
                GetColor(200, 200, 200));

            break;
        }

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}