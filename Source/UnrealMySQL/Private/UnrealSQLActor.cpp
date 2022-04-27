// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealSQLActor.h"

#include "UnrealMySQL.h"\

#define MYSQL_BIND_BUFFER_SET(BufferName, BufferType, MySQLBufferType, IsUnsigned) \
{																				   \
BufferType* BufferName = (BufferType*)FMemory::Malloc(sizeof(BufferType));         \
*BufferName = BindArray[j].GetValue<BufferType>();;                                \
MySQLBinds[j].buffer_type = enum_field_types::MySQLBufferType;                     \
MySQLBinds[j].buffer = BufferName;                                                 \
MySQLBinds[j].buffer_length = sizeof(BufferType);                                  \
MySQLBinds[j].is_unsigned = IsUnsigned;                                            \
TemporaryBuffers.Add((uint8*)BufferName);                                          \
}																				   \

// Sets default values
AUnrealSQLActor::AUnrealSQLActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void AUnrealSQLActor::BeginPlay()
{
	Super::BeginPlay();
	SQLThread = MakeUnique<FSQLThread>(*Host, Port, *User, *Password, *DatabaseName, this);
}

void AUnrealSQLActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SQLThread.Reset();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AUnrealSQLActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AUnrealSQLActor::InsertIntoByUnrealStruct(
	const char* InSQL,
	void* StructPtr,
	int32 StructNum,
	UStruct* Struct,
	TFunction<void(bool, void* )> OnSQLExecuteCompleted,
	FSimpleDelegate OnSQLTaskCompleted)
{
	FSQLQureyTaskPtr SQLQureyTaskPtr = MakeShared<FSQLQureyTask>();
	SQLQureyTaskPtr->SQLQueryDelegate.BindLambda(
		[SQL = std::string(InSQL), StructPtr, StructNum, Struct, OnSQLTaskCompleted = MoveTemp(OnSQLTaskCompleted), OnSQLExecuteCompleted = MoveTemp(OnSQLExecuteCompleted)]
		(MYSQL* MySQL)
		{
			MYSQL_STMT* MySQLStmt = mysql_stmt_init(MySQL);
			if (0 == mysql_stmt_prepare(MySQLStmt, SQL.c_str(), SQL.size()))
			{
				TArray<FProperty*> InputOrder;
				for (TFieldIterator<FProperty> It(Struct); It; ++It)
				{
					FProperty* Property = *It;
					InputOrder.Add(Property);
				}
				uint8* StructPtrOffset = (uint8*)StructPtr;
				for (size_t i = 0; i < StructNum; i++)
				{
					bool SQLResult = false;
					TArray<MYSQL_BIND> MySQLBinds;
					MySQLBinds.Init({}, InputOrder.Num());
					TArray<uint8*> TemporaryBuffers;
					for (int32 j = 0; j < InputOrder.Num(); ++j)
					{
						auto StructProperty = InputOrder[j];
						if (StructProperty->HasAnyCastFlags(CASTCLASS_FStrProperty))
						{
							//FStrProperty* StrProperty = static_cast<FStrProperty*>(StructProperty);
							FTCHARToUTF8 Converter(*StructProperty->ContainerPtrToValuePtr<FString>(StructPtrOffset));
							char* StringBuffer = (char*)FMemory::Malloc(Converter.Length());
							FMemory::Memcpy(StringBuffer, Converter.Get(), Converter.Length());
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_STRING;
							MySQLBinds[j].buffer = StringBuffer;
							MySQLBinds[j].buffer_length = Converter.Length();
							TemporaryBuffers.Add((uint8*)StringBuffer);
						}
						else if (StructProperty->HasAnyCastFlags(
							CASTCLASS_FInt8Property | CASTCLASS_FInt16Property  | CASTCLASS_FIntProperty    | CASTCLASS_FInt64Property |
							CASTCLASS_FByteProperty | CASTCLASS_FUInt16Property | CASTCLASS_FUInt32Property | CASTCLASS_FUInt64Property
							))
						{
							FNumericProperty* NumericProperty = static_cast<FNumericProperty*>(StructProperty);
							int64* Int64Buffer = (int64*)FMemory::Malloc(sizeof(int64));
							*Int64Buffer = NumericProperty->GetSignedIntPropertyValue(StructPtrOffset);
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_LONGLONG;
							MySQLBinds[j].buffer = Int64Buffer;
							MySQLBinds[j].buffer_length = sizeof(int64);
							MySQLBinds[j].is_unsigned = false;
							TemporaryBuffers.Add((uint8*)Int64Buffer);
						}
						else if (StructProperty->HasAnyCastFlags(CASTCLASS_FDoubleProperty | CASTCLASS_FFloatProperty))
						{
							FNumericProperty* NumericProperty = static_cast<FNumericProperty*>(StructProperty);
							double* DoubleBuffer = (double*)FMemory::Malloc(sizeof(double));
							*DoubleBuffer = NumericProperty->GetFloatingPointPropertyValue(StructPtrOffset);
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_DOUBLE;
							MySQLBinds[j].buffer = DoubleBuffer;
							MySQLBinds[j].buffer_length = sizeof(double);
							MySQLBinds[j].is_unsigned = false;
							TemporaryBuffers.Add((uint8*)DoubleBuffer);
						}
						else if (StructProperty->GetCPPType() == TEXT("FDateTime"))
						{
							MYSQL_TIME* MySQLTime = (MYSQL_TIME*)FMemory::Malloc(sizeof(MYSQL_TIME));
							FDateTime DateTime = *StructProperty->ContainerPtrToValuePtr<FDateTime>(StructPtrOffset);
							MySQLTime->year = DateTime.GetYear();
							MySQLTime->month = DateTime.GetMonth();
							MySQLTime->day = DateTime.GetDay();
							MySQLTime->hour = DateTime.GetHour();
							MySQLTime->minute = DateTime.GetMinute();
							MySQLTime->second = DateTime.GetSecond();
							MySQLTime->time_type = enum_mysql_timestamp_type::MYSQL_TIMESTAMP_DATETIME;
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_DATETIME;
							MySQLBinds[j].buffer = MySQLTime;
							MySQLBinds[j].buffer_length = sizeof(MYSQL_TIME);
							TemporaryBuffers.Add((uint8*)MySQLTime);
						}
						else
						{
							UESQL_LOG(Fatal, TEXT("NOT SUPPORTED TYPE FOR SQL BIND <CPPType:%s>"), *StructProperty->GetCPPType());
						}
					}
					if (0 == mysql_stmt_bind_param(MySQLStmt, MySQLBinds.GetData()))
					{
						if (0 == mysql_stmt_execute(MySQLStmt))
						{
							SQLResult = true;
						}
						else
						{
							UESQL_LOG(Error, TEXT("mysql_stmt_execute FAILED <ErrorCode:%d> <ErrorString:%s>"), mysql_errno(MySQL), UTF8_TO_TCHAR(mysql_error(MySQL)));
						}
					}
					else
					{
						UESQL_LOG(Error, TEXT("mysql_stmt_bind_param FAILED <ErrorCode:%d> <ErrorString:%s>"), mysql_errno(MySQL), UTF8_TO_TCHAR(mysql_error(MySQL)));
					}
					OnSQLExecuteCompleted(SQLResult, StructPtrOffset);
					StructPtrOffset = ((uint8*)StructPtrOffset) + Struct->GetStructureSize();
					for (auto TemporaryBuffer : TemporaryBuffers)
					{
						FMemory::Free(TemporaryBuffer);
					}
					TemporaryBuffers.Empty();
				}
			}
			else
			{
				UESQL_LOG(Error, TEXT("mysql_stmt_prepare FAILED <ErrorCode:%d> <ErrorString:%s>"), mysql_errno(MySQL), UTF8_TO_TCHAR(mysql_error(MySQL)));
			}
			mysql_stmt_close(MySQLStmt);
			OnSQLTaskCompleted.ExecuteIfBound();
		});
	PushSQLQureyTask(SQLQureyTaskPtr);
}

void AUnrealSQLActor::InsertIntoByUnrealStruct(const char* InSchema, const char* InSQL, TArray<TArray<FVariant>>&& InBinds)
{
	FSQLQureyTaskPtr SQLQureyTaskPtr = MakeShared<FSQLQureyTask>();
	SQLQureyTaskPtr->SQLQueryDelegate.BindLambda(
		[SQL = std::string(InSQL), Binds = MoveTemp(InBinds)]
		(MYSQL* MySQL)
		{
			MYSQL_STMT* MySQLStmt = mysql_stmt_init(MySQL);
			mysql_stmt_prepare(MySQLStmt, SQL.c_str(), SQL.size());
			for (size_t i = 0; i < Binds.Num(); i++)
			{
				const TArray<FVariant>& BindArray = Binds[i];
				TArray<MYSQL_BIND> MySQLBinds;
				MySQLBinds.Init({}, BindArray.Num());
				TArray<uint8*> TemporaryBuffers;
				for (int32 j = 0; j < BindArray.Num(); ++j)
				{
					switch (BindArray[j].GetType())
					{
					case EVariantTypes::DateTime:
						{
							MYSQL_TIME* MySQLTime = (MYSQL_TIME*)new uint8[sizeof(MYSQL_TIME)];
							FDateTime DateTime = BindArray[j].GetValue<FDateTime>();
							MySQLTime->year = DateTime.GetYear();
							MySQLTime->month = DateTime.GetMonth();
							MySQLTime->day = DateTime.GetDay();
							MySQLTime->hour = DateTime.GetHour();
							MySQLTime->minute = DateTime.GetMinute();
							MySQLTime->second = DateTime.GetSecond();
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_TIMESTAMP;
							MySQLBinds[j].buffer = MySQLTime;
							MySQLBinds[j].buffer_length = sizeof(MYSQL_TIME);
							TemporaryBuffers.Add((uint8*)MySQLTime);
							break;
						}
					case EVariantTypes::String:
						{
							FTCHARToUTF8 Converter(*BindArray[j].GetValue<FString>());
							char* StringBuffer = (char*)new uint8[sizeof(Converter.Length())];
							FMemory::Memcpy(StringBuffer, Converter.Get(), Converter.Length());
							MySQLBinds[j].buffer_type = enum_field_types::MYSQL_TYPE_STRING;
							MySQLBinds[j].buffer = StringBuffer;
							MySQLBinds[j].buffer_length = Converter.Length();
							TemporaryBuffers.Add((uint8*)StringBuffer);
							break;
						}
					case EVariantTypes::Double:
                        MYSQL_BIND_BUFFER_SET(DoubleBuffer, double, MYSQL_TYPE_DOUBLE, false);
                        break;
					case EVariantTypes::Float:
						MYSQL_BIND_BUFFER_SET(FloatBuffer, float, MYSQL_TYPE_FLOAT, false);
						break;
					case EVariantTypes::Int8:
						MYSQL_BIND_BUFFER_SET(Int8Buffer, int8, MYSQL_TYPE_TINY, false);
						break;
					case EVariantTypes::Int16:
						MYSQL_BIND_BUFFER_SET(Int16Buffer, int16, MYSQL_TYPE_SHORT, false);
						break;
					case EVariantTypes::Int32:
						MYSQL_BIND_BUFFER_SET(Int32Buffer, int32, MYSQL_TYPE_LONG, false);
						break;
					case EVariantTypes::Int64:
						MYSQL_BIND_BUFFER_SET(Int64Buffer, int64, MYSQL_TYPE_LONGLONG, false);
						break;
					case EVariantTypes::UInt8:
						MYSQL_BIND_BUFFER_SET(UInt8Buffer, uint8, MYSQL_TYPE_TINY, true);
						break;
					case EVariantTypes::UInt16:
						MYSQL_BIND_BUFFER_SET(UInt16Buffer, uint16, MYSQL_TYPE_SHORT, true);
						break;
					case EVariantTypes::UInt32:
						MYSQL_BIND_BUFFER_SET(UInt32Buffer, uint32, MYSQL_TYPE_LONG, true);
						break;
					case EVariantTypes::UInt64:
						MYSQL_BIND_BUFFER_SET(UInt64Buffer, uint64, MYSQL_TYPE_LONGLONG, true);
						break;
					default:
						UESQL_LOG(Fatal, TEXT("CreateMediaCollectorBackendTable FAILED <ErrorCode:%d> <SQLState:%s>"));
					}
				}
				mysql_stmt_bind_param(MySQLStmt, MySQLBinds.GetData());
				mysql_stmt_execute(MySQLStmt);
				for (auto TemporaryBuffer : TemporaryBuffers)
				{
					delete [] TemporaryBuffer;
				}
			}
			mysql_stmt_close(MySQLStmt);
		});
	PushSQLQureyTask(SQLQureyTaskPtr);
}

void AUnrealSQLActor::QueryByUnrealStruct(const char* InSchema, const char* InSQL, UStruct* InStruct, TFunction<void (void * StructPtr, int32 StructNum)> InCallback)
{
	FSQLQureyTaskPtr SQLQureyTaskPtr = MakeShared<FSQLQureyTask>();
	SQLQureyTaskPtr->SQLQueryDelegate.BindLambda(
	[SQL = std::string(InSQL), Struct = InStruct, Callback = MoveTemp(InCallback)]
	(MYSQL* MySQL)
	{
		if(!mysql_real_query(MySQL, SQL.data(), SQL.size()))
		{
			if (MYSQL_RES* MySQLRes = mysql_store_result(MySQL))
			{
				int32 NumFields = mysql_field_count(MySQL);
				if (MYSQL_FIELD *MySQLFields = mysql_fetch_fields(MySQLRes))
				{
					TArray<FProperty*> StructProperties;
					StructProperties.Init(nullptr, NumFields);
					for (int32 i = 0; i < NumFields; i++)
					{
						StructProperties[i] = Struct->FindPropertyByName(UTF8_TO_TCHAR(MySQLFields[i].name));
					}
					uint64 NumRows = mysql_num_rows(MySQLRes);
					TArray<uint8> StructBuffer;
					StructBuffer.AddUninitialized(Struct->GetStructureSize() * NumRows);
					Struct->InitializeStruct(StructBuffer.GetData(), NumRows);
					void* StructPtr = StructBuffer.GetData();
					for (uint64 i = 0; i < NumRows; i++)
					{
						MYSQL_ROW MySQLRow = mysql_fetch_row(MySQLRes);
						unsigned long* RowLength = mysql_fetch_lengths(MySQLRes);
						for (int32 j = 0; j < NumFields; ++j)
						{
							auto StructProperty = StructProperties[j];
							if (StructProperty)
							{
								enum_field_types FieldType = MySQLFields[j].type;
								switch (FieldType)
								{
								case MYSQL_TYPE_BIT:
								case MYSQL_TYPE_TINY:
								case MYSQL_TYPE_SHORT:
								case MYSQL_TYPE_INT24:
								case MYSQL_TYPE_LONG:
								case MYSQL_TYPE_LONGLONG:
								case MYSQL_TYPE_FLOAT:
								case MYSQL_TYPE_DOUBLE:
									if (StructProperty->HasAnyCastFlags(CASTCLASS_FNumericProperty))
									{
										FNumericProperty* NumericProperty = static_cast<FNumericProperty*>(StructProperty);
										NumericProperty->SetNumericPropertyValueFromString(StructPtr, UTF8_TO_TCHAR(MySQLRow[j]));
									}
									break;
								case MYSQL_TYPE_STRING:
								case MYSQL_TYPE_VARCHAR:
								case MYSQL_TYPE_VAR_STRING:
									if (StructProperty->HasAnyCastFlags(CASTCLASS_FStrProperty))
									{
										//FStrProperty* StrProperty = static_cast<FStrProperty*>(StructProperty);
										FString dsadas = UTF8_TO_TCHAR(MySQLRow[j]);
										*StructProperty->ContainerPtrToValuePtr<FString>(StructPtr) = UTF8_TO_TCHAR(MySQLRow[j]);
									}
									break;
								case MYSQL_TYPE_BLOB:
									if (StructProperty->HasAnyCastFlags(CASTCLASS_FArrayProperty))
									{
										FArrayProperty* ArrayProperty = static_cast<FArrayProperty*>(StructProperty);
										if (ArrayProperty->Inner->HasAnyPropertyFlags(CASTCLASS_FByteProperty))
										{
											TArray<uint8>* ByteArrayPtr = StructProperty->ContainerPtrToValuePtr<TArray<uint8>>(StructPtr);
											ByteArrayPtr->SetNumUninitialized(RowLength[j]);
											FMemory::Memcpy(ByteArrayPtr->GetData(),MySQLRow[j], RowLength[j]);
										}
									}
									break;
								case MYSQL_TYPE_TIMESTAMP:
								case MYSQL_TYPE_DATE:
								case MYSQL_TYPE_TIME:
								case MYSQL_TYPE_YEAR:
								{
									FString NameCPP = StructProperty->GetCPPType();
									if (NameCPP == TEXT("FDateTime"))
									{
										FDateTime::Parse(FString(UTF8_TO_TCHAR(MySQLRow[j])), *StructProperty->ContainerPtrToValuePtr<FDateTime>(StructPtr));
									}
									break;
								}
								default:
									UESQL_LOG(Fatal, TEXT("[tan90]"));
									break;
								}
							}
						}
						StructPtr = ((uint8*)StructPtr) + Struct->GetStructureSize();
					}
					Callback(StructBuffer.GetData(), NumRows);
					Struct->DestroyStruct(StructBuffer.GetData(), NumRows);
				}
			}
		}
		else
		{
			UESQL_LOG(Error, TEXT("<SQL:%s> FAILED <Errno:%d> <Error:%s>"), UTF8_TO_TCHAR(SQL.c_str()), mysql_errno(MySQL), UTF8_TO_TCHAR(mysql_error(MySQL)));
		}
	});
	PushSQLQureyTask(SQLQureyTaskPtr);
}

void AUnrealSQLActor::CreateTable(const char * Schema, const char * CreateTableSQL)
{
	FSQLQureyTaskPtr SQLQureyTaskPtr = MakeShared<FSQLQureyTask>();
	SQLQureyTaskPtr->SQLQueryDelegate.BindLambda([CreateTableSQL](MYSQL* MySQL)
	{
		if (mysql_query(MySQL, CreateTableSQL))
		{
			UESQL_LOG(Fatal, TEXT("<SQL:%s> FAILED <Errno:%d> <Error:%s>"), UTF8_TO_TCHAR(CreateTableSQL), mysql_errno(MySQL), UTF8_TO_TCHAR(mysql_error(MySQL)));
		}
	});
	PushSQLQureyTask(SQLQureyTaskPtr);
}

void AUnrealSQLActor::PushSQLQureyTask(FSQLQureyTaskPtr SQLQureyTaskPtr)
{
	SQLThread->PushSQLQureyTask(SQLQureyTaskPtr);
}
