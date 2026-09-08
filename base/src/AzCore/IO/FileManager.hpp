/*
	File: FileManager.hpp
	Author: Philip Haynes
	Generic system for pipelining and parallelizing file input
*/

#ifndef AZCORE_FILE_MANAGER_HPP
#define AZCORE_FILE_MANAGER_HPP

#include "../Memory/HashMap.hpp"
#include "../Memory/String.hpp"
#include "../Memory/Any.hpp"
#include "../Thread.hpp"

namespace AzCore::io {

struct File {
	String filepath;
	Array<char> data;
	Any userdata;
	// return value is whether or not to keep the file data after running the decoder. If you std::move the data out of File, return false.
	typedef bool (*fp_Decoder)(File *file, Any &userdata);
	fp_Decoder decoder;
	i32 priority;
	enum class Stage {
		REQUESTED,
		LOADING,
		READY_FOR_DECODE,
		DECODING,
		READY,
		DISCARDED,
		FILE_NOT_FOUND,
	} stage;
};

struct FileManager {
	HashMap<Str, File> files;
	// Highest priority is at the back, otherwise order is undefined
	Array<File*> filesRequested;
	// Highest priority is at the back, otherwise order is undefined
	Array<File*> filesToDecode;
	Mutex mutexFiles;
	Mutex mutexRequested;
	CondVar condRequested;
	Mutex mutexToDecode;
	CondVar condToDecode;
	Thread threadDisk;
	Array<Thread> threadsDecode;
	i32 availableDecoders;
	Mutex mutexPipeline;
	CondVar condPipeline;
	i32 filesInPipeline;
	bool initted = false;
	bool close;
	bool warnFileNotFound = false;
	Array<String> searchDirectories = {""};
	
	// Spawn threads. With numDecodeWorkers < 1, defaults to max(Thread::HardwareConcurrency()-2, 2)
	void Init(i32 numDecodeWorkers=0);
	// Cleanup files and threads
	void Deinit();
	
	// Blocks until there are no more files requested or left to decode.
	void WaitUntilDone();
	
	FileManager() = default;
	FileManager(const FileManager&) = delete;
	FileManager(FileManager&&) = delete;
	
	// userdata will be passed into decoder when it comes time to decode the file
	// decoder can be nullptr, indicating that the raw data is all that's needed
	File* RequestFile(String filepath, i32 priority, File::fp_Decoder decoder, Any userdata=Any());
	File* RequestDecode(Array<char> &&buffer, String filepath, i32 priority, File::fp_Decoder decoder, Any userdata=Any());
	
	static void DiskProc(FileManager *manager);
	static void DecodeProc(FileManager *manager);
};

} // namespace AzCore::io

#endif // AZCORE_FILE_MANAGER_HPP