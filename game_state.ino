#include "sub-altar.h"

/**
 * @brief DB gamestate가 setting 일 때 한번동작하는 코드
 */
void SettingFunc()
{
    activate_bool = false;
    sendCommand("page pgSetting");
    NeoFunc = NeoNo;
    NeopixelSet(white);
}

/**
 * @brief DB gamestate가 ready 일 때 한번동작하는 코드
 */

void ReadyFunc()
{
    activate_bool = false;
    sendCommand("page pgInfo");
    NeoFunc = NeoBeforeTagger;
}

/**
 * @brief DB gamestate가 activate 일 때 반복동작하는 코드
 */
void ActivateFunc()
{
    RfidLoop();
}

/**
 * @brief DB gamestate가 activate 일 때 한번동작하는 코드
 */
void ActivateRunOnce()
{
    activate_bool = true;
    altar_used_local = false;   // 라운드(activate) 진입 시 로컬 잠금 해제 → 다음 봉헌 허용
}

void DataChange()
{
    if (!(const char *)my["device_name"])
    {
        Serial.println("[DataChange] 서버 데이터 없음, 스킵");
        return;
    }

    static StaticJsonDocument<1000> cur;

    String cmd;

    bool brightness_changed = ((int)my["brightness"] != (int)cur["brightness"]);

    if ((String)(const char *)my["game_state"] != (String)(const char *)cur["game_state"])
    {
        if ((String)(const char *)my["game_state"] == "setting")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            SettingFunc();
        }
        else if ((String)(const char *)my["game_state"] == "ready")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            ReadyFunc();
        }
        else if ((String)(const char *)my["game_state"] == "activate")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            ActivateRunOnce();
        }
    }
    else if (brightness_changed)
    {
        SetBrightness((int)my["brightness"]);
    }

    if ((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"])
    {
        if ((String)(const char *)my["device_state"] == "activate")
        {
            // 게임 중 blink(술래 유인) → activate 복귀로는 잠금을 풀지 않는다.
            // 이 복귀로 잠금이 풀리면 봉헌 후 재태그 시 생명이 다시 바쳐지는 버그 발생.
            // 라운드 시작 시 잠금 해제는 game_state→activate(ActivateRunOnce)가 담당한다.
            if ((String)(const char *)cur["device_state"] != "blink")
                altar_used_local = false;

            // 이미 봉헌된(used) 제단은 pgUsed/빨강 고정 화면을 유지한다.
            if (!altar_used_local)
            {
                sendCommand("page pgChipCount");
                NeoFunc = NeoGaming;
            }
        }
        else if ((String)(const char *)my["device_state"] == "player_win")
        {
            sendCommand("page pgTaggerLose");
            NeoFunc = NeoLose;
        }
        else if ((String)(const char *)my["device_state"] == "player_lose")
        {
            sendCommand("page pgTaggerWin");
            NeoFunc = NeoWin;
        }
        else if ((String)(const char *)my["device_state"] == "blink")
        {
            // used(봉헌 완료) 제단은 blink 요청을 무시하고 pgUsed를 유지한다.
            if (rfid_tag || altar_used_local) return;
            sendCommand("page pgAltarTag");
            NeoFunc = NeoTagger;
        }
        else if ((String)(const char *)my["device_state"] == "github")
        {
            ota.check();
        }
    }

    if((int)my["taken_chip"] != (int)cur["taken_chip"])
    {
        cmd = "pgChipCount.vSacrificeChip.val=" + (String)(int)my["taken_chip"];
        sendCommand(cmd.c_str());
        // 타이머가 이미 만료된 경우(서버 응답이 2초 이후) 페이지 전환.
        // 단, 봉헌 후 잠금(used) 상태면 pgUsed 화면을 유지해야 하므로 전환하지 않는다.
        if (!altar_used_local && !keep_tag_timer.isEnabled(keep_tag_timer_id))
            sendCommand("page pgChipCount");
    }
    if((int)my["max_taken_chip"] != (int)cur["max_taken_chip"])
    {
        cmd = "pgChipCount.vMaxChip.val=" + (String)(int)my["max_taken_chip"];
        sendCommand(cmd.c_str());
    }
    SyncLanguage();

    Serial.println("Data Change");
    cur = my;
}