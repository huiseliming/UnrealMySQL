// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SQLThread.h"
#include "UnrealSQLActor.generated.h"

class FSQLThread;

UCLASS()
class UNREALMYSQL_API AUnrealSQLActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUnrealSQLActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	FSimpleMulticastDelegate& OnMySQLConnected() { return OnMySQLConnectedDelegate; };
	FSimpleMulticastDelegate& OnMySQLConnectFailed() { return OnMySQLConnectFailedDelegate; };
	FSimpleMulticastDelegate& OnMySQLDisconnected() { return OnMySQLDisconnectedDelegate; };
	
	void InsertIntoByUnrealStruct(const char* InSQL, void* StructPtr, int32 StructNum, UStruct* InStruct, TFunction<void(bool, void* )> OnSQLExecuteCompleted, FSimpleDelegate OnSQLTaskCompleted);
	
	void InsertIntoByUnrealStruct(const char* InSchema, const char* InSQL, TArray<TArray<FVariant>>&& InBinds);
	
	void QueryByUnrealStruct(const char* InSchema, const char* InSQL, UStruct* InStruct, TFunction<void(void* StructPtr, int32 StructNum)> InCallback);
	
	void CreateTable(const char * Schema, const char * CreateTableSQL);

	void PushSQLQureyTask(FSQLQureyTaskPtr SQLQureyTaskPtr);

private:
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UnrealSQLActor)
	FString Host = TEXT("localhost");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UnrealSQLActor)
	int32 Port = 3306;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UnrealSQLActor)
	FString User = TEXT("root");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UnrealSQLActor)
	FString Password = TEXT("senken");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UnrealSQLActor)
	FString DatabaseName = TEXT("media_collector");
	
private:
	TUniquePtr<FSQLThread> SQLThread;
	
	FSimpleMulticastDelegate OnMySQLConnectedDelegate;
	FSimpleMulticastDelegate OnMySQLConnectFailedDelegate;
	FSimpleMulticastDelegate OnMySQLDisconnectedDelegate;
};
