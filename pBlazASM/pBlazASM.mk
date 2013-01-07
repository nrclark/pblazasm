##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=pBlazASM
ConfigurationName      :=Debug
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
WorkspacePath          := "C:\Program Files\CodeLite\Main"
ProjectPath            := "C:\Users\henk\Google Drive\SVN\pBlazASM"
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=henk
Date                   :=16-8-2012
CodeLitePath           :="C:\Program Files\CodeLite"
LinkerName             :=gcc
ArchiveTool            :=ar rcus
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
CompilerName           :=gcc
C_CompilerName         :=gcc
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="C:\Users\henk\Google Drive\SVN\pBlazASM\pBlazASM.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
CmpOptions             := -g $(Preprocessors)
C_CmpOptions           := -g $(Preprocessors)
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
LibPath                := $(LibraryPathSwitch). 


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Srcs=pbParser.c pbLibgen.c pbLexer.c pBlazASM.c getopt.c pbSymbols.c 

Objects=$(IntermediateDirectory)/pbParser$(ObjectSuffix) $(IntermediateDirectory)/pbLibgen$(ObjectSuffix) $(IntermediateDirectory)/pbLexer$(ObjectSuffix) $(IntermediateDirectory)/pBlazASM$(ObjectSuffix) $(IntermediateDirectory)/getopt$(ObjectSuffix) $(IntermediateDirectory)/pbSymbols$(ObjectSuffix) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects) > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "./Debug"

PreBuild:
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/pbParser$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/pbParser$(DependSuffix)
	$(RM) $(IntermediateDirectory)/pbParser$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/pbLibgen$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/pbLibgen$(DependSuffix)
	$(RM) $(IntermediateDirectory)/pbLibgen$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/pbLexer$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/pbLexer$(DependSuffix)
	$(RM) $(IntermediateDirectory)/pbLexer$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/pBlazASM$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/pBlazASM$(DependSuffix)
	$(RM) $(IntermediateDirectory)/pBlazASM$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/getopt$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/getopt$(DependSuffix)
	$(RM) $(IntermediateDirectory)/getopt$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/pbSymbols$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/pbSymbols$(DependSuffix)
	$(RM) $(IntermediateDirectory)/pbSymbols$(PreprocessSuffix)
	$(RM) $(OutputFile)
	$(RM) $(OutputFile).exe
	$(RM) "C:\Program Files\CodeLite\Main\.build-debug\pBlazASM"


