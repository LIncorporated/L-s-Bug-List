#include <YYToolkit/Shared.hpp>
#include <string>
#include <map>

using namespace Aurie;
using namespace YYTK;

std::vector<std::string> bugNames;
int current_location = -1;
static YYTKInterface* g_ModuleInterface = nullptr;
static const char* const version = "1.0.0";

void PopupCallback(FWFrame& Context)
{
	if (GetAsyncKeyState('B') & 1)
	{
		g_ModuleInterface->Print(CM_LIGHTGREEN, "BugList v%s displaying bug options", version);

		std::string bugText = "Here are the potential bug spawns!:\n";

		for (const std::string& bug : bugNames) {
			bugText += bug + "\n";
		}

		g_ModuleInterface->CallBuiltin(
			"show_message",       
			{ bugText.c_str() }
		);
	}
}

RValue& RetieveSpawnedBugs(
	CInstance* Self,
	CInstance* Other,
	OUT RValue& Result,
	int ArgumentCount,
	RValue** Arguments
)
{
	const PFUNC_YYGMLScript original = reinterpret_cast<PFUNC_YYGMLScript>(
		MmGetHookTrampoline(g_ArSelfModule, "gml_Script_get_candidate@anon@3481@__SpawnableCandidateVotes@SpawnDistribution@SpawnDistributions")
		);
	original(Self, Other, Result, ArgumentCount, Arguments);

	CInstance* global_instance = nullptr;
	g_ModuleInterface->GetGlobalInstance(&global_instance);

	RValue itemDataArray = global_instance->at("__item_id__");
	int targetRecipeKey = Result.m_i64; 

	int arrayLength = g_ModuleInterface->CallBuiltin("array_length", { itemDataArray }).m_Real;

	//right now, acorn is at 0 so skip it since it's not a bug. 
	for (int i = 1; i < arrayLength; i++) {
		RValue recipeKey = g_ModuleInterface->CallBuiltin("array_get", { itemDataArray, i });

		if (recipeKey.m_Kind != VALUE_STRING) {
			continue;
		}

		if (i == targetRecipeKey) {
			std::string recipeKeyStr = std::string(recipeKey.AsString());
			bugNames.emplace_back(recipeKeyStr.c_str());
			break;
		}
	}
	return Result;
}

void CreateHookRetieveSpawnedBugs(AurieStatus& status)
{
	CScript* script_retrieve_bugs = nullptr;
	status = g_ModuleInterface->GetNamedRoutinePointer(
		"gml_Script_get_candidate@anon@3481@__SpawnableCandidateVotes@SpawnDistribution@SpawnDistributions",
		(PVOID*)&script_retrieve_bugs
	);

	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- failed in CreateHookRetieveSpawnedBugs", version);
	}

	status = MmCreateHook(
		g_ArSelfModule,
		"gml_Script_get_candidate@anon@3481@__SpawnableCandidateVotes@SpawnDistribution@SpawnDistributions",
		script_retrieve_bugs->m_Functions->m_ScriptFunction,
		RetieveSpawnedBugs,
		nullptr
	);

	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- failed in CreateHookRetieveSpawnedBugs", version);
	}
}

RValue& RetieveLocation(
	CInstance* Self,
	CInstance* Other,
	OUT RValue& Result,
	int ArgumentCount,
	RValue** Arguments
)
{
	const PFUNC_YYGMLScript original = reinterpret_cast<PFUNC_YYGMLScript>(
		MmGetHookTrampoline(g_ArSelfModule, "gml_Script_on_room_start@Anchor@Anchor")
		);
	original(Self, Other, Result, ArgumentCount, Arguments);

	CInstance* global_instance = nullptr;
	g_ModuleInterface->GetGlobalInstance(&global_instance);

	RValue taxiDataArray = global_instance->at("__taxi");
	RValue location = g_ModuleInterface->CallBuiltin("struct_get", { taxiDataArray, "location_current" });
	if (current_location != (int)location.m_Real) {
		bugNames.clear();
	}
	current_location = (int)location.m_Real;

	return Result;
}

void CreateHookLoadLocation(AurieStatus& status)
{
	CScript* script_location = nullptr;
	status = g_ModuleInterface->GetNamedRoutinePointer(
		"gml_Script_on_room_start@Anchor@Anchor",
		(PVOID*)&script_location
	);

	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- failed in CreateHookLoadLocation", version);
	}

	status = MmCreateHook(
		g_ArSelfModule,
		"gml_Script_on_room_start@Anchor@Anchor",
		script_location->m_Functions->m_ScriptFunction,
		RetieveLocation,
		nullptr
	);

	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- failed in CreateHookLoadLocation", version);
	}
}

EXPORTED AurieStatus ModuleInitialize(IN AurieModule* Module, IN const fs::path& ModulePath) {
	UNREFERENCED_PARAMETER(ModulePath);

	AurieStatus status = AURIE_SUCCESS;

	status = ObGetInterface(
		"YYTK_Main",
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	if (!AurieSuccess(status))
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

	g_ModuleInterface->Print(CM_LIGHTPURPLE, "[BugList v%s] is loading up", version);

	g_ModuleInterface->CreateCallback(
		Module,
		EVENT_FRAME,
		PopupCallback,
		0
	);
	CreateHookLoadLocation(status);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- unable to start", version);
		return status;
	}
	CreateHookRetieveSpawnedBugs(status);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_LIGHTRED, "[BugList v%s] -- unable to start", version);
		return status;
	}

	g_ModuleInterface->Print(CM_LIGHTPURPLE, "[BugList v%s] has been loaded", version);
	return AURIE_SUCCESS;
}