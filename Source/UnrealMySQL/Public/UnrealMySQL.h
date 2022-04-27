// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define UESQL__FUNC__ (FString(__FUNCTION__))
#define UESQL__LINE__ (FString::FromInt(__LINE__))
#define UESQL__FUNC__LINE__ (UESQL__FUNC__ + "(" + UESQL__LINE__ + ")")

#ifdef UE_BUILD_DEBUG
#define UESQL_LOG(Verbosity, Format, ...) UE_LOG(UnrealMySQL, Verbosity, TEXT("[%s] ") Format, *UESQL__FUNC__LINE__, ##__VA_ARGS__ )
#elif
#define UESQL_LOG(Verbosity, Format, ...) UE_LOG(UnrealMySQL, Verbosity, Format, ##__VA_ARGS__ )
#endif
 
class FUnrealMySQLModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//sql::Driver* GetMySQLDriver() { return Driver; }
	//
	//sql::Driver* Driver;
};



DECLARE_LOG_CATEGORY_EXTERN(UnrealMySQL, Log, All);