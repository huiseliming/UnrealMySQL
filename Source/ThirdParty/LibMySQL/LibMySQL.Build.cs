// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using EpicGames.Core;
using UnrealBuildTool;

public class LibMySQL : ModuleRules
{
	private void CopyToBinaries(string Filepath, ReadOnlyTargetRules Target)
	{
		string BinariesDir = Path.Combine(
			PluginDirectory,
			"Binaries", "ThirdParty", "LibMySQL",
			Target.Platform.ToString());
		string Filename = Path.GetFileName(Filepath);
		if (!Directory.Exists(BinariesDir))
			Directory.CreateDirectory(BinariesDir);
		//Console.WriteLine(Filepath);
		//Console.WriteLine(Path.Combine(binariesDir, filename));
		if (!File.Exists(Path.Combine(BinariesDir, Filename)))
		{
			File.Copy(Filepath, Path.Combine(BinariesDir, Filename), true);
		}
	} 
	
	public LibMySQL(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicIncludePaths.AddRange(new string[]{ Path.Combine(ModuleDirectory, "include") });
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "libmysql.lib"));
			PublicDelayLoadDLLs.Add("libmysql.dll");
			CopyToBinaries(
				Path.Combine(ModuleDirectory, "lib", "libmysql.dll"),
				Target);
			
			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/LibMySQL/Win64/libmysql.dll");
			// Copy Dll for run editor
		}
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
            // RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/ArcFaceLibrary/Mac/Release/libExampleLibrary.dylib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			// string ExampleSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "ArcFaceLibrary", "Linux", "x86_64-unknown-linux-gnu", "libExampleLibrary.so");
			// PublicAdditionalLibraries.Add(ExampleSoPath);
			// PublicDelayLoadDLLs.Add(ExampleSoPath);
			// RuntimeDependencies.Add(ExampleSoPath);
		}
	}
}
