#pragma once
#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Windows/AllowWindowsPlatformTypes.h"
#include "mysql.h"
#include "Windows/HideWindowsPlatformTypes.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING
THIRD_PARTY_INCLUDES_END


DECLARE_DELEGATE_OneParam(FSQLQueryDelegate, MYSQL*);

struct FSQLQureyTask
{
	FSQLQueryDelegate SQLQueryDelegate;
};

using FSQLQureyTaskPtr = TSharedPtr<FSQLQureyTask>;

class AUnrealSQLActor;

class FSQLThread : public FRunnable
{
public:
	FSQLThread(
		const FString& InHost,
		uint32_t InPort,
		const FString& InUser,
		const FString& InPassword,
		const FString& InDatabase,
		AUnrealSQLActor* InUnrealSQLActor
		)
		: Host(*InHost)
		, Port(InPort)
		, User(InUser)
		, Password(InPassword)
		, Database(InDatabase)
		, UnrealSQLActor(InUnrealSQLActor)
	{
		static int32 Counter = 0;
		Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("SQLThread_%d"), Counter++));
	}
	~FSQLThread()
	{
		bRequestExit = true;
		if (Thread)
		{
			Thread->Kill();
			delete Thread;
		}
	}

	void PushSQLQureyTask(FSQLQureyTaskPtr SQLQureyTaskPtr)
	{
		SQLQureyTaskQueue.Enqueue(SQLQureyTaskPtr);
	}
	
	bool Init() override { return true; }
	uint32 Run() override;
	void Stop() override{}

protected:
	TQueue<FSQLQureyTaskPtr, EQueueMode::Mpsc> SQLQureyTaskQueue;
	
private:
	FRunnableThread* Thread;
	FThreadSafeBool bRequestExit{ false };
	FString Host{ "localhost" };
    uint32_t Port = 3306;
	FString User;
    FString Password;
	FString Database;
	TWeakObjectPtr<AUnrealSQLActor> UnrealSQLActor;
};
