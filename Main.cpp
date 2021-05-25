#include "Main.h"

/* __index Function */
int MetaIndex(uintptr_t rL) {
	rlua_State State; State >> rL;

	void* self = State.touserdata(1);
	const char* Index = State.tostring(2);

	/* self == game */
	{
		State.getglobal("game");
		void* game = State.touserdata(-1);
		if (self != game) goto OldIndex;
	}

	/* Not called from Arch */
	if (!(State.GetIdentity() >= 6)) {
		goto OldIndex;
	}

	if (!std::strcmp(Index, "OpenVideosFolder")) {
		State.pushcclosure(reinterpret_cast<DWORD>(Env::Functions::protected_func), 0);
		return 1;
	}
	if (!std::strcmp(Index, "OpenScreenshotsFolder")) {
		State.pushcclosure(reinterpret_cast<DWORD>(Env::Functions::protected_func), 0);
		return 1;
	}
	if (!std::strcmp(Index, "HttpGet")) {
		State.pushcclosure(reinterpret_cast<DWORD>(Env::Functions::httpget), 0);
		return 1;
	}
	else if (!std::strcmp(Index, "HttpGetAsync")) {
		State.pushcclosure(reinterpret_cast<DWORD>(Env::Functions::httpget), 0);
		return 1;
	}
	else if (!std::strcmp(Index, "GetObjects")) {
		State.pushcclosure(reinterpret_cast<DWORD>(Env::Functions::getobjects), 0);
		return 1;
	}

OldIndex: {
	State.pushvalue(lua_upvalueindex(1));
	State.pushvalue(1);
	State.pushvalue(2);
	State.call(2, 1);
	return 1;
	}
return 0;
};

void __cdecl Input(VOID) {
	/* Create Pipe */
	HANDLE PipeHandle = CreateNamedPipe(TEXT("\\\\.\\pipe\\Arch"),
		PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		PIPE_WAIT,
		1,
		999999,
		999999,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	/* Check if pipe exists, run loop */
	while (PipeHandle != INVALID_HANDLE_VALUE)
	{
		/* If a connection is established, read from pipe */
		if (ConnectNamedPipe(PipeHandle, NULL) != FALSE)
		{
			/* Declare Variables */
			std::string WholeScript;
			DWORD ReadBytes;
			char Buffer[1024] = {};

			/* Read Input */
			while (ReadFile(PipeHandle, Buffer, sizeof(Buffer) - 1, &ReadBytes, NULL) != FALSE)
				WholeScript.append(Buffer, ReadBytes);

			/* Execute and Deallocate Buffer */
			AddScriptToQueue(new Env::Script(WholeScript));

			/* Clear */
			WholeScript = "";
		}
		DisconnectNamedPipe(PipeHandle);
	}
};

bool Teleporting = false;
int CurrentTeleportState = 0;
std::vector<const char*> TeleportStates = { "RequestedFromServer", "Started", "WaitingForServer", "Failed", "InProgress" }; /* lowest index = 0 */
int __cdecl TeleportHandler(uintptr_t rL) {
	rlua_State State; State >> rL;

	/* Save TeleportState */
	void* TeleportState = State.touserdata(1);

	/* Clear */
	State.settop(0);

	/* UnHook Renderer */
	Renderer::UnHook();

	/* Show Console and Write */
	Console::Show();
	Console::Write({ "TeleportHandler", "Teleport" }, LogType::Info, "Arch->Teleport %s.", Teleporting ? "Started" : "Detected");

	/* Check TeleportState */
	State.getglobal("Enum");
	State.getfield(-1, "TeleportState");
	for (int i = 0; i < TeleportStates.size(); i++) {
		State.getfield(-1, TeleportStates[i]);
		if (TeleportState == State.touserdata(-1)) {
			CurrentTeleportState = i;
		}
		State.settop(2);
	}

	/* Initialize TeleportHandler */
	if (!Teleporting) {
		std::thread([]() {
			/* Wait */
			while (CurrentTeleportState != 4) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); };
			while (Env::GetDataModel() == Env::DataModel || Env::GetDataModel() == NULL) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); };
			std::this_thread::sleep_for(std::chrono::seconds(10));

			/* Re-Initialize */
			std::thread(Start).detach();
			return 0;
			}).detach();
	}

	/* Prevent double calls */
	Teleporting = true;

	return 0;
};

int __cdecl Start(VOID) {
	/* Start Console */
	OutputOptions Settings;
	Settings.Name = "Beeb - vbeta | https://discord.gg/kSPFEKj5E7";
	Settings.Debugging = DEBUG;
	Settings.Timestamp = true;
	Console::Start(Settings);

	/* Fetch and Load JSON */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Fetching JSON...");
	JSONParser::Initialize();

	/* Get LuaState */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Grabbing LuaState...");
	MainState >> Env::GetRobloxState();
	MainState >> luaL_newstate();
	Console::Write({ "Main", "Start" }, LogType::Debug, "LuaState: %X", MainState.rL);

	/* Wait For Load */
	uintptr_t DM = Env::GetDataModel();
	bool IsLoaded = false;
	do {
		MainState.getglobal("game");
		MainState.getfield(-1, "IsLoaded");
		MainState.pushvalue(-2);
		MainState.pcall(2, 0, 0);
		IsLoaded = MainState.toboolean(-1);
		MainState.settop(0);
		Sleep(10);
	} while (!IsLoaded);

	/* Bypasses */
	OverrideIsTainted();
	/* Tools.HookFunction(&reinterpret_cast<LPVOID&>(addr_robloxcrashed), &OnRobloxCrash); TODO: NOP Integrity Check */

	/* Newthread */
	MainState >> Env::newthread(MainState.rL);

	/* Teleport shit */
	Env::DataModel = DM;
	Teleporting = false;

	/* Pause Roblox */
	Tools.PauseRoblox();

	/* Set Identity */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Setting Level...");
	MainState.SetIdentity(8);
	Console::Write({ "Main", "Start" }, LogType::Debug, "Level: %c", std::to_string(MainState.GetIdentity()));

	/* Initialize Environment */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Initializing Environment...");
	Env::Initialize(MainState);


	//Console::Write({ "Main", "Start" }, LogType::Debug, "Applying __index Hook...");
	///* Index Hook */
	//{
	//	/* Clear */
	//	MainState.settop(0);
	//
	//	/* Get metadata */
	//	MainState.getglobal("game");
	//	MainState.getmetatable(-1);
	//	MainState.setreadonly(-1, false);
	//
	//	/* Get __index */
	//	MainState.getfield(-1, "__index");
	//
	//	/* Use it as an upvalue */
	//	MainState.pushcclosure(reinterpret_cast<DWORD>(MetaIndex), 1);
	//
	//	/* Override __index */
	//	MainState.setfield(-2, "__index");
	//
	//	/* Set metadata to read-only */
	//	MainState.getglobal("game");
	//	MainState.getmetatable(-1);
	//	MainState.setreadonly(-1, true);
	//
	//	/* Clear */
	//	MainState.settop(0);
	//}


	/* Pipe Input */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Creating Input...");
	std::thread(Input).detach();

	/* Init Script */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Starting InitScript...");
	StartInitScript();

	/* Task Scheduler and Teleport Handler */
	Console::Write({ "Main", "Start" }, LogType::Debug, "Initializing Thread Scheduler and Teleport Handler...");
	Memory Memory;
	Memory.TaskScheduler.Initialize(MainState);
	Memory.TeleportHandler.Initialize(MainState, reinterpret_cast<DWORD>(TeleportHandler));

	/* autoexec */
	{
		std::string AutoExecDir = Env::Path + std::string("\\autoexec\\");
		if (Tools.DirectoryExists(AutoExecDir)) {
			Console::Write({ "Main", "Start" }, LogType::Debug, "Running AutoExec...");
			for (auto File : std::filesystem::directory_iterator(AutoExecDir)) {
				if (!std::filesystem::is_directory(File.path())) {
					std::string All;
					std::string Line;
					std::ifstream Read(File.path());

					/* Read */
					while (std::getline(Read, Line)) {
						All += Line;
					}

					/* Execute */
					AddScriptToQueue(new Env::Script(All));
				}
			}
		}
	}

	/* AfterTeleport Scripts */
	{
		if (!ScriptAfterTeleportQueue.empty()) {
			Console::Write({ "Main", "Start" }, LogType::Debug, "Running AfterTeleport...");
			for (int i = 0; unsigned(i) < ScriptAfterTeleportQueue.size(); i++) {
				/* Execute */
				AddScriptToQueue(ScriptAfterTeleportQueue.front());

				/* Pop */
				ScriptAfterTeleportQueue.pop();
			}
		}
	}

	/* Resume Roblox */
	Tools.ResumeRoblox();

	Console::Write({ "Main", "Start" }, LogType::Debug, "Ready!");
#if DEBUG
	//std::string ConsoleExec;
	//while (std::getline(std::cin, ConsoleExec)) {
	//	Env::ExecuteProto(Env::newthread(MainState.rL), ConsoleExec.c_str());
	//}
#else
	Console::Hide();
#endif
	return 1337;
};

