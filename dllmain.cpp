#include "pch.h"

enum CharSelType : uint8_t
{
    CharSelType_Adventure,
    CharSelType_Trial,
    CharSelType_Mission
};

DataPointer(CharSelType, charSelType, 0x3B2A2FA);
FunctionPointer(void, FreeCharacter, (int id), 0x44B010);

#pragma pack(push, 1)
struct CharSelDataA
{
    task* CharacterObjects[7];
    task* object;
    char gap_20[12];
    char field_2C;
    char CurrentCharAnim[7];
    char CharSelected;
    char gap_35[2];
    char field_37;
};
#pragma pack(pop)

#define BYTE1(x)   (*((uint8_t*)&(x)+1))

static Trampoline* ExtraCharacter_Load_t = nullptr;
static bool HudScale = false;

static NJS_ACTION ExtraCharacterAction{};

static void __cdecl ExtraCharacter_Display(task* obj)
{
    auto data = obj->twp;

    if (!MissedFrames && BYTE1(CharacterSelection) == data->counter.b[0])
    {
        auto data2 = reinterpret_cast<motionwk2*>(obj->mwp);
        auto co2 = (CharObj2*)data2->work.l;
        
        Direct3D_SetZFunc(1u);
        SetMaterialAndSpriteColor_Float(1.0f, 1.0f, 1.0f, 1.0f);

        njPushMatrixEx();
        
        if (charSelType == CharSelType_Adventure)
        {
            // SUPER SONIC

            int backup = SuperSonicFlag;
            njSetTexture(&SUPERSONIC_TEXLIST);
            Direct3D_PerformLighting(2);
            SuperSonicFlag = 1;
            njTranslate(nullptr, 0.0f, 3.0f, HudScale ? -20.0f : -20.0f * HorizontalStretch);
            njRotateY(nullptr, 0x4000);
            njAction(co2->AnimationThing.action, co2->AnimationThing.Frame);
            SuperSonicFlag = backup;
        }
        else
        {
            // METAL SONIC

            int backup = MetalSonicFlag;
            njSetTexture(&METALSONIC_TEXLIST);
            Direct3D_PerformLighting(2);
            MetalSonicFlag = 1;
            njTranslate(nullptr, 0.0f, data->mode == 2 ? 2.0f : 0.0f, HudScale ? -20.0f : -20.0f * HorizontalStretch);
            njRotateY(nullptr, 0x4000);
            njAction(co2->AnimationThing.action, co2->AnimationThing.Frame);
            MetalSonicFlag = backup;
        }

        njPopMatrixEx();
        Direct3D_PerformLighting(0);
        ClampGlobalColorThing_Thing();
        Direct3D_ResetZFunc();
    }
}

static void ExtraCharacterPlayAnim(taskwk* data, CharObj2* co2, CharSelDataA* charseldata, int idleanim, int selectanim)
{
    if (charseldata->CharSelected == 0)
    {
        if (data->mode != 1)
        {
            ExtraCharacterAction.motion = SonicAnimData[idleanim].Animation->motion;
            co2->AnimationThing.Index = idleanim;
            co2->AnimationThing.Frame = 0.0f;
            data->mode = 1;
        }

        co2->AnimationThing.Frame = fmodf(co2->AnimationThing.Frame + 0.5f, co2->AnimationThing.action->motion->nbFrame - 1);
    }
    else
    {
        if (data->mode != 2)
        {
            ExtraCharacterAction.motion = SonicAnimData[selectanim].Animation->motion;
            co2->AnimationThing.Index = selectanim;
            co2->AnimationThing.Frame = 0.0f;
            data->mode = 2;
        }

        if (co2->AnimationThing.Frame < co2->AnimationThing.action->motion->nbFrame - 1)
        {
            co2->AnimationThing.Frame += 0.5f;
        }
        else
        {
            co2->AnimationThing.Frame = co2->AnimationThing.action->motion->nbFrame - 1;
        }
    }
}

static void __cdecl ExtraCharacter_Main(task* obj)
{
    auto data = obj->twp;

    if (BYTE1(CharacterSelection) == data->counter.b[0])
    {
        auto data2 = reinterpret_cast<motionwk2*>(obj->mwp);
        auto co2 = (CharObj2*)data2->work.l;

        auto charselobj = CharSel_ptr;
        auto charseldata = reinterpret_cast<CharSelDataA*>(charselobj->UnknownB_ptr);

        if (charSelType == CharSelType_Adventure)
        {
            // SUPER SONIC

            ExtraCharacterPlayAnim(data, co2, charseldata, 134, 143);
        }
        else
        {
            // METAL SONIC

            ExtraCharacterPlayAnim(data, co2, charseldata, 146, 75);
        }

        if (co2 && co2->AnimationThing.WeldInfo != nullptr)
            PJoinVertexes(data, data2, (playerwk*)co2);
    }

    obj->disp(obj);
}

static int timer = 0;
static void __cdecl ExtraCharacter_Delete(task* obj)
{
    auto data = obj->twp;
    auto data2 = reinterpret_cast<motionwk2*>(obj->mwp);
    auto co2 = (playerwk*)data2->work.l;

    co2->mj.actwkptr = nullptr;

    if (charSelType == CharSelType_Adventure)
    {
        njReleaseTexture(&SUPERSONIC_TEXLIST);
    }
    else
    {
        UnloadCharTextures(Characters_MetalSonic);
    }

    FreeCharacter(data->counter.b[0]);
    timer = 0;
}

PL_JOIN_VERTEX* getCharacterWeld(const uint8_t character)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (playerpwp[i] && playertwp[i])
        {
            if (playertwp[i]->counter.b[1] == character || playertwp[i]->counter.b[1] == Characters_Sonic && character == Characters_MetalSonic)
            {
                playerpwp[i]->mj.pljvptr[0].dstobj = playerpwp[i]->mj.pljvptr[0].dstobj;
                return playerpwp[i]->mj.pljvptr;
            }
        }
    }

    return nullptr;
}


static void __cdecl ExtraCharacter_Load(task* obj)
{
    if (ExtraCharacter_Load_t && charSelType == CharSelType_Adventure && !GetEventFlag(EventFlags_SuperSonicAdventureComplete))
    {
        obj->exec = reinterpret_cast<TaskFuncPtr>(ExtraCharacter_Load_t->Target());
        return;
    }

    if (++timer == 15) //let the game load everyone so we can get the weld data properly (race issue)
    {

        auto data = obj->twp;
        auto data2 = reinterpret_cast<motionwk2*>(obj->mwp);

        // Load everything but CharBubble
        WriteData(reinterpret_cast<int*>(0x442961), 0);
        InitCharacterVars(data->counter.b[0], (ObjectMaster*)obj);
        WriteData(reinterpret_cast<ObjectFuncPtr*>(0x442961), CharBubble_Init);

        auto pwp = (playerwk*)data2->work.l;
        pwp->mj.plactptr = (PL_ACTION*)&SonicAnimData;

        if (charSelType == CharSelType_Adventure)
        {
            // SUPER SONIC
            auto welds = getCharacterWeld(Characters_Sonic);

            LoadPVM("SUPERSONIC", &SUPERSONIC_TEXLIST);
            if (welds)
            {
                welds[0].dstobj = welds[0].dstobj;
                pwp->mj.pljvptr = welds;
            }
            pwp->equipment |= Upgrades_SuperSonic;

            ExtraCharacterAction.object = SONIC_OBJECTS[22];
        }
        else
        {
            // METAL SONIC

            LoadCharTextures(Characters_MetalSonic);

            auto welds = getCharacterWeld(Characters_MetalSonic);
            if (welds)
            {
                welds[0].dstobj = welds[0].dstobj;
                pwp->mj.pljvptr = welds;
            }

            ExtraCharacterAction.object = SONIC_OBJECTS[68];
        }

        if (pwp && pwp->mj.pljvptr != nullptr)
            PJoinVertexes(data, data2, pwp);

        pwp->mj.mtnmode = 2;
        pwp->mj.actwkptr = &ExtraCharacterAction;

        obj->exec = ExtraCharacter_Main;
        obj->disp = ExtraCharacter_Display;
        obj->dest = ExtraCharacter_Delete;
        timer = 0;
    }
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
        const IniFile* config = new IniFile(std::string(path) + "\\config.ini");

        if (config->getBool("", "ShowIfCompleted", false))
        {
            ExtraCharacter_Load_t = new Trampoline(0x5125A0, 0x5125A6, ExtraCharacter_Load, false);
        }
        else
        {
            WriteJump(reinterpret_cast<void*>(0x5125A0), ExtraCharacter_Load);
        }

        delete config;

        const IniFile* loader_config = new IniFile("mods\\SADXModLoader.ini");
        HudScale = loader_config->getBool("", "ScaleHud", false);
        delete loader_config;
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer }; // This is needed for the Mod Loader to recognize the DLL.
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}