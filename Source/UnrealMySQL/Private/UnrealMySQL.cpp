// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMySQL.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FUnrealMySQLModule"

//THIRD_PARTY_INCLUDES_START
//PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
//#include "jdbc/mysql_driver.h"
//PRAGMA_POP_PLATFORM_DEFAULT_PACKING
//THIRD_PARTY_INCLUDES_END

static void* LibMySQLLibraryHandle = nullptr;


void FUnrealMySQLModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString BaseDir = IPluginManager::Get().FindPlugin("UnrealMySQL")->GetBaseDir();

	

	//// MySQLConnectorCpp
	FString MySQLConnectorCppLibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/LibMySQL/Win64/libmysql.dll"));
	if (!LibMySQLLibraryHandle)
	{
		LibMySQLLibraryHandle = FPlatformProcess::GetDllHandle(*MySQLConnectorCppLibraryPath);
		if (!LibMySQLLibraryHandle)
		{
			UESQL_LOG(Fatal, TEXT("MySQLConnectorCppLibrary MUST BE LOADED"));
		}
	}

}

void FUnrealMySQLModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (LibMySQLLibraryHandle)
	{
		FPlatformProcess::FreeDllHandle(LibMySQLLibraryHandle);
		LibMySQLLibraryHandle = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealMySQLModule, UnrealMySQL)
DEFINE_LOG_CATEGORY(UnrealMySQL);