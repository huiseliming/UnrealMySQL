#include "SQLThread.h"

#include "UnrealMySQL.h"
#include "UnrealSQLActor.h"

uint32 FSQLThread::Run()
{
	MYSQL* MySQL = mysql_init(nullptr);
	std::string HostStdString(TCHAR_TO_UTF8(*Host));
	std::string UserStdString(TCHAR_TO_UTF8(*User));
	std::string PasswordStdString(TCHAR_TO_UTF8(*Password));
	std::string DBStdString(TCHAR_TO_UTF8(*Database));
	if(mysql_real_connect(MySQL,
		HostStdString.c_str(),
		UserStdString.c_str(),
		PasswordStdString.c_str(),
		DBStdString.c_str(),
		Port,
		nullptr,
		0
		))
	{
		AsyncTask(ENamedThreads::GameThread, [UnrealSQLActorWeakObjectPtr = UnrealSQLActor]
		{
			if (UnrealSQLActorWeakObjectPtr.IsValid()) UnrealSQLActorWeakObjectPtr->OnMySQLConnected().Broadcast();
		});
		while (!bRequestExit)
		{ 
			FSQLQureyTaskPtr SQLQureyTaskPtr;
			while (SQLQureyTaskQueue.Dequeue(SQLQureyTaskPtr))
			{
				if (0 != mysql_ping(MySQL))
				{
					if(mysql_real_connect(MySQL,
						HostStdString.c_str(),
						UserStdString.c_str(),
						PasswordStdString.c_str(),
						DBStdString.c_str(),
						Port,
						nullptr,
						0
					))
					{
						
					}
				}
				SQLQureyTaskPtr->SQLQueryDelegate.ExecuteIfBound(MySQL);
			}
			FPlatformProcess::Sleep(0.001);
		}
		AsyncTask(ENamedThreads::GameThread, [UnrealSQLActorWeakObjectPtr = UnrealSQLActor]
		{
			if (UnrealSQLActorWeakObjectPtr.IsValid()) UnrealSQLActorWeakObjectPtr->OnMySQLDisconnected().Broadcast();
		});
	}
	else
	{
		UESQL_LOG(Error, "mysql CONNECTED FAILED");
		AsyncTask(ENamedThreads::GameThread, [UnrealSQLActorWeakObjectPtr = UnrealSQLActor]
		{
			if (UnrealSQLActorWeakObjectPtr.IsValid()) UnrealSQLActorWeakObjectPtr->OnMySQLConnectFailed().Broadcast();
		});
	}
	if (MySQL)
	{
		mysql_close(MySQL);
		MySQL = nullptr;
	}
	return EXIT_SUCCESS;
}
